#ifndef DETERMINANT_H
#define DETERMINANT_H

#include "matrix.h"
#include <pthread.h>
#include <sys/time.h>

typedef struct {
    double** matrix;
    int size;
    int pivot_row;
    double pivot_value;
    int start_row;
    int end_row;
    int* swap_count;
    pthread_mutex_t* mutex;
} ThreadData;

typedef struct {
    double** matrix;
    int size;
    int pivot_row;
    int start_row;
    int end_row;
} RowEliminationData;


typedef struct {
    double determinant;
    double sequential_time;
    double parallel_time;
    double speedup;
    double efficiency;
    int threads_used;
} DeterminantResult;

// Основные функции
double determinant_sequential(const Matrix* matrix);
double determinant_parallel(const Matrix* matrix, int max_threads);
double determinant_parallel_optimized(const Matrix* matrix, int max_threads);
double determinant_parallel_best(const Matrix* matrix, int max_threads);
double determinant_parallel_simple(const Matrix* matrix, int max_threads);
double determinant_parallel_ultimate(const Matrix* matrix, int max_threads);
double determinant_parallel_demo(const Matrix* matrix, int max_threads);
double determinant_parallel_block(const Matrix* matrix, int max_threads);

// Бенчмарк
DeterminantResult determinant_benchmark(const Matrix* matrix, int max_threads);
void print_benchmark_results(const DeterminantResult* result);

#endif
