#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

static volatile sig_atomic_t timed_out = 0;

static void HandleAlarm(int sig) {
  (void)sig;
  timed_out = 1;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0; 
  bool with_files = false;

  while (true) {
    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"timeout", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed is a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size is a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum is a positive number\n");
              return 1;
            }
            break;
          case 3:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout is a positive number\n");
              return 1;
            }
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf(
        "Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" "
        "[--timeout \"sec\"] [--by_files]\n",
        argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int pipefd[2];
  if (!with_files) {
    if (pipe(pipefd) == -1) {
      printf("Pipe failed!\n");
      return 1;
    }
  } else {
    FILE *f = fopen("shared_data.txt", "w");
    if (f) fclose(f);
  }

  int chunk_size = array_size / pnum;

  pid_t *children = calloc(pnum, sizeof(pid_t));

  if (timeout > 0) {
    signal(SIGALRM, HandleAlarm);
    alarm(timeout);
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        int begin = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;

        struct MinMax local_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          FILE *fp = fopen("shared_data.txt", "a");
          fprintf(fp, "%d %d\n", local_min_max.min, local_min_max.max);
          fclose(fp);
        } else {
          if (write(pipefd[1], &local_min_max, sizeof(struct MinMax)) < 0) {
            perror("write");
          }
        }
        free(array);
        free(children);
        return 0;
      }
      children[i] = child_pid;

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  if (!with_files) close(pipefd[1]);

  int killed = 0;
  bool kill_sent = false;
  while (active_child_processes > 0) {
    int status;
    pid_t done = waitpid(-1, &status, WNOHANG);

    if (done > 0) {
      active_child_processes -= 1;
      for (int i = 0; i < pnum; i++) {
        if (children[i] == done) children[i] = 0;
      }
      if (WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL) killed++;
    } else if (done == 0) {
      if (timed_out && !kill_sent) {
        for (int i = 0; i < pnum; i++) {
          if (children[i] > 0) kill(children[i], SIGKILL);
        }
        kill_sent = true;
      }
      usleep(1000); 
    } else if (errno != EINTR) {
      perror("waitpid");
      break;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  int received = 0;
  FILE *fp = NULL;
  if (with_files) {
    fp = fopen("shared_data.txt", "r");
  }

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      if (fp == NULL || fscanf(fp, "%d %d", &min, &max) != 2) break;
    } else {
      struct MinMax temp;
      if (read(pipefd[0], &temp, sizeof(struct MinMax)) !=
          (ssize_t)sizeof(struct MinMax))
        break;
      min = temp.min;
      max = temp.max;
    }
    received++;

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  if (with_files) {
    if (fp) fclose(fp);
    remove("shared_data.txt");
  } else {
    close(pipefd[0]);
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(children);

  if (killed > 0) {
    printf("Timeout (%d s): killed %d of %d child processes with SIGKILL\n",
           timeout, killed, pnum);
  }
  if (received > 0) {
    if (received < pnum) {
      printf("Result is partial: %d of %d parts of the array\n", received,
             pnum);
    }
    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
  } else {
    printf("No results: all children were killed\n");
  }
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
