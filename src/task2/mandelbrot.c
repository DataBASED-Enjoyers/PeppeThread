#include <complex.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "timer.h"

typedef struct {
    int thread_id;
    int nthreads;
    int npoints;
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    FILE *file;
    pthread_mutex_t *file_mutex;
} thread_data_t;

void *mandelbrot_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int npoints = data->npoints;
    double x_min = data->x_min;
    double x_max = data->x_max;
    double y_min = data->y_min;
    double y_max = data->y_max;
    int max_iter = 1000;

    int start = (npoints * data->thread_id) / data->nthreads;
    int end = (npoints * (data->thread_id + 1)) / data->nthreads;

    double x_step = (x_max - x_min) / npoints;
    double y_step = (y_max - y_min) / npoints;

    for (int i = start; i < end; ++i) {
        for (int j = 0; j < npoints; ++j) {
            double x = x_min + i * x_step;
            double y = y_min + j * y_step;
            double complex c = x + y * I;
            double complex z = 0 + 0 * I;
            int iter = 0;
            while (cabs(z) < 2 && iter < max_iter) {
                z = z * z + c;
                iter++;
            }

            if (iter == max_iter) {
                pthread_mutex_lock(data->file_mutex);
                fprintf(data->file, "%lf,%lf\n", x, y);
                pthread_mutex_unlock(data->file_mutex);
            }
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s nthreads npoints\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int nthreads = atoi(argv[1]);
    int npoints = atoi(argv[2]);

    double x_min = -2.0, x_max = 1.0;
    double y_min = -1.5, y_max = 1.5;

    FILE *file = fopen("mandelbrot.csv", "w");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    double start, end;
    GET_TIME(start);

    pthread_t threads[nthreads];
    thread_data_t thread_data[nthreads];
    pthread_mutex_t file_mutex;
    pthread_mutex_init(&file_mutex, NULL);

    for (int i = 0; i < nthreads; ++i) {
        thread_data[i].thread_id = i;
        thread_data[i].nthreads = nthreads;
        thread_data[i].npoints = npoints;
        thread_data[i].x_min = x_min;
        thread_data[i].x_max = x_max;
        thread_data[i].y_min = y_min;
        thread_data[i].y_max = y_max;
        thread_data[i].file = file;
        thread_data[i].file_mutex = &file_mutex;

        int rc = pthread_create(&threads[i], NULL, mandelbrot_thread,
                                (void *)&thread_data[i]);
        if (rc) {
            fprintf(stderr, "Error creating thread: %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < nthreads; ++i) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&file_mutex);

    GET_TIME(end);
    printf("Execution Time = %f seconds\n", end - start);

    fclose(file);

    return 0;
}
