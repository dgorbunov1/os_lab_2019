#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

static void Sleep(int seconds) {
  struct timespec ts = {seconds, 0};
  while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
  }
}

static void ShowChildren(const char *title) {
  char cmd[128];
  printf("\n--- %s ---\n", title);
  fflush(stdout);
  snprintf(cmd, sizeof(cmd), "ps -o pid,ppid,stat,cmd --ppid %d", (int)getpid());
  if (system(cmd) == -1) perror("system");
  fflush(stdout);
}

static void ReapChildren(int sig) {
  (void)sig;
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0) {
  }
  errno = saved_errno;
}

int main(int argc, char **argv) {
  const char *mode = argc > 1 ? argv[1] : "zombie";
  int count = argc > 2 ? atoi(argv[2]) : 3;
  if (count <= 0) count = 3;

  if (strcmp(mode, "zombie") != 0 && strcmp(mode, "wait") != 0 &&
      strcmp(mode, "sigchld") != 0 && strcmp(mode, "ignore") != 0) {
    printf("Usage: %s [zombie|wait|sigchld|ignore] [children]\n", argv[0]);
    return 1;
  }

  if (strcmp(mode, "sigchld") == 0) signal(SIGCHLD, ReapChildren);
  if (strcmp(mode, "ignore") == 0) signal(SIGCHLD, SIG_IGN);

  printf("Parent pid = %d, mode = %s, children = %d\n", (int)getpid(), mode,
         count);

  for (int i = 0; i < count; i++) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      return 1;
    }
    if (pid == 0) {
      _exit(0);
    }
    printf("Parent: created child %d\n", (int)pid);
  }

  Sleep(1);
  ShowChildren("1 second later: children have exited, parent did not wait()");

  if (strcmp(mode, "wait") == 0) {
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
      printf("Parent: reaped child %d\n", (int)pid);
    }
    ShowChildren("after waitpid(): zombies are gone");
  }

  if (strcmp(mode, "zombie") == 0) {
    printf(
        "\nParent sleeps 20 s and does not call wait(): zombies stay.\n"
        "Check in another terminal:  ps -o pid,ppid,stat,cmd --ppid %d\n",
        (int)getpid());
    fflush(stdout);
    Sleep(20);
    printf("Parent exits: init/systemd adopts the zombies and reaps them.\n");
  } else {
    Sleep(1);
  }
  return 0;
}
