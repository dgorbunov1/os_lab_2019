#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <getopt.h>
#include <pthread.h>

#include "sum.h"
#include "utils.h"

struct ThreadData {
  struct SumArgs args;
  long long result;
};

void *ThreadSum(void *data) {
  struct ThreadData *td = (struct ThreadData *)data;
  td->result = Sum(&td->args);
  return NULL;
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  static struct option options[] = {{"threads_num", required_argument, 0, 0},
                                    {"seed", required_argument, 0, 0},
                                    {"array_size", required_argument, 0, 0},
                                    {0, 0, 0, 0}};
  while (1) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;
    if (c != 0) {
      return 1;
    }
    long value = atol(optarg);
    if (value <= 0) {
      printf("%s is a positive number\n", options[option_index].name);
      return 1;
    }
    switch (option_index) {
      case 0:
        threads_num = (uint32_t)value;
        break;
      case 1:
        seed = (uint32_t)value;
        break;
      case 2:
        array_size = (uint32_t)value;
        break;
    }
  }

  if (optind < argc || threads_num == 0 || seed == 0 || array_size == 0) {
    printf(
        "Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n",
        argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
  struct ThreadData *data = malloc(sizeof(struct ThreadData) * threads_num);
  if (array == NULL || threads == NULL || data == NULL) {
    printf("Error: out of memory\n");
    return 1;
  }
  GenerateArray(array, array_size, seed);

  uint32_t chunk_size = array_size / threads_num;
  for (uint32_t i = 0; i < threads_num; i++) {
    data[i].args.array = array;
    data[i].args.begin = i * chunk_size;
    data[i].args.end = (i == threads_num - 1) ? array_size : (i + 1) * chunk_size;
    data[i].result = 0;
  }

  struct timespec start_time, finish_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);

  for (uint32_t i = 0; i < threads_num; i++) {
    if (pthread_create(&threads[i], NULL, ThreadSum, &data[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  long long total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    pthread_join(threads[i], NULL);
    total_sum += data[i].result;
  }

  clock_gettime(CLOCK_MONOTONIC, &finish_time);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

  free(data);
  free(threads);
  free(array);

  printf("Total: %lld\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}
