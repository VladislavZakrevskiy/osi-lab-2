#ifndef DETERMINANT_H
#define DETERMINANT_H

#include "matrix.h"
#include <pthread.h>
#include <sys/time.h>

typedef struct {
    const Matrix* matrix;
    int row;
    int col;
    double* result;
    int thread_id;
    int max_threads;
} ThreadData;

typedef struct {
    double determinant;
    double sequential_time;
    double parallel_time;
    double speedup;
    double efficiency;
    int threads_used;
} DeterminantResult;

double determinant_sequential(const Matrix* matrix);

double determinant_parallel(const Matrix* matrix, int max_threads);

DeterminantResult determinant_benchmark(const Matrix* matrix, int max_threads);

void print_benchmark_results(const DeterminantResult* result);

void* calculate_cofactor_thread(void* arg);

double get_time_difference(struct timeval start, struct timeval end);

void determinant_init_threading(int max_threads);

void determinant_cleanup_threading(void);

extern pthread_mutex_t global_mutex;
extern pthread_cond_t thread_available;
extern int global_active_threads;
extern int global_max_threads;

#endif 
