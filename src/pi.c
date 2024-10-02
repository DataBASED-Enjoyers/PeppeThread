#include "timer.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  long trials;
  long count;
  unsigned int seed;
} ThreadData;

void *compute_pi(void *arg) {
  ThreadData *data = (ThreadData *)arg;
  long local_count = 0;
  for (long i = 0; i < data->trials; i++) {
    double x = (double)rand_r(&data->seed) / RAND_MAX * 2.0 - 1.0;
    double y = (double)rand_r(&data->seed) / RAND_MAX * 2.0 - 1.0;
    if (x * x + y * y <= 1.0)
      local_count++;
  }
  data->count = local_count;
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: ./program nthreads ntrials\n");
    exit(EXIT_FAILURE);
  }

  int nthreads = atoi(argv[1]);
  long ntrials = atol(argv[2]);

  pthread_t threads[nthreads];
  ThreadData thread_data[nthreads];
  double start, end;
  GET_TIME(start);

  long trials_per_thread = ntrials / nthreads;
  long total_count = 0;

  for (int i = 0; i < nthreads; i++) {
    thread_data[i].trials = trials_per_thread;
    thread_data[i].count = 0;
    thread_data[i].seed = rand();
    pthread_create(&threads[i], NULL, compute_pi, &thread_data[i]);
  }

  for (int i = 0; i < nthreads; i++) {
    pthread_join(threads[i], NULL);
    total_count += thread_data[i].count;
  }

  GET_TIME(end);

  double pi_estimate = 4.0 * (double)total_count / (double)ntrials;
  printf("Estimated π = %.8f\n", pi_estimate);
  printf("Execution Time = %f seconds\n", end - start);

  return 0;
}
