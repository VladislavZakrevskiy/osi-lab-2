#include "determinant.h"
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_available = PTHREAD_COND_INITIALIZER;
int global_active_threads = 0;
int global_max_threads = 1;

double determinant_sequential_fast(const Matrix* matrix) {
    if (!matrix_is_valid(matrix)) {
        return 0.0;
    }
    
    if (matrix->size == 1) {
        return matrix->data[0][0];
    }
    
    if (matrix->size == 2) {
        return matrix->data[0][0] * matrix->data[1][1] - matrix->data[0][1] * matrix->data[1][0];
    }
    
    int n = matrix->size;
    double** A = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        A[i] = (double*)malloc(n * sizeof(double));
        for (int j = 0; j < n; j++) {
            A[i][j] = matrix->data[i][j];
        }
    }
    
    double det = 1.0;
    int sign = 1;
    
    for (int k = 0; k < n - 1; k++) {
        int max_row = k;
        for (int i = k + 1; i < n; i++) {
            if (fabs(A[i][k]) > fabs(A[max_row][k])) {
                max_row = i;
            }
        }
        
        if (max_row != k) {
            double* temp = A[k];
            A[k] = A[max_row];
            A[max_row] = temp;
            sign = -sign;
        }
        
        if (fabs(A[k][k]) < 1e-10) {
            det = 0.0;
            break;
        }
        
        det *= A[k][k];
        
        for (int i = k + 1; i < n; i++) {
            double factor = A[i][k] / A[k][k];
            for (int j = k + 1; j < n; j++) {
                A[i][j] -= factor * A[k][j];
            }
        }
    }
    
    if (det != 0.0) {
        det *= A[n-1][n-1] * sign;
    }
    
    for (int i = 0; i < n; i++) {
        free(A[i]);
    }
    free(A);
    
    return det;
}

double determinant_sequential(const Matrix* matrix) {
    return determinant_sequential_fast(matrix);
}

typedef struct {
    double** A;
    int start_row;
    int end_row;
    int start_col;
    int end_col;
    int pivot_row;
    int pivot_col;
    int n;
} BlockThreadData;

void* block_elimination_thread(void* arg) {
    BlockThreadData* data = (BlockThreadData*)arg;
    
    double pivot = data->A[data->pivot_row][data->pivot_col];
    
    for (int i = data->start_row; i < data->end_row; i++) {
        if (i <= data->pivot_row) continue;
        
        double factor = data->A[i][data->pivot_col] / pivot;
        
        for (int j = data->start_col; j < data->end_col; j++) {
            if (j <= data->pivot_col) continue;
            data->A[i][j] -= factor * data->A[data->pivot_row][j];
        }
    }
    
    pthread_mutex_lock(&global_mutex);
    global_active_threads--;
    pthread_cond_signal(&thread_available);
    pthread_mutex_unlock(&global_mutex);
    
    return NULL;
}

double determinant_parallel_block(const Matrix* matrix, int max_threads) {
    if (!matrix_is_valid(matrix)) {
        return 0.0;
    }
    
    if (matrix->size == 1) {
        return matrix->data[0][0];
    }
    
    if (matrix->size == 2) {
        return matrix->data[0][0] * matrix->data[1][1] - matrix->data[0][1] * matrix->data[1][0];
    }
    
    if (matrix->size == 3 || max_threads == 1) {
        return determinant_sequential_fast(matrix);
    }
    
    int n = matrix->size;
    
    double** A = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        A[i] = (double*)malloc(n * sizeof(double));
        for (int j = 0; j < n; j++) {
            A[i][j] = matrix->data[i][j];
        }
    }
    
    double det = 1.0;
    int sign = 1;
    
    int block_size = n / max_threads;
    if (block_size < 2) block_size = 2;
    
    for (int k = 0; k < n - 1; k++) {
        int max_row = k;
        for (int i = k + 1; i < n; i++) {
            if (fabs(A[i][k]) > fabs(A[max_row][k])) {
                max_row = i;
            }
        }
        
        if (max_row != k) {
            double* temp = A[k];
            A[k] = A[max_row];
            A[max_row] = temp;
            sign = -sign;
        }
        
        if (fabs(A[k][k]) < 1e-10) {
            det = 0.0;
            break;
        }
        
        det *= A[k][k];
        
        int remaining_size = n - k - 1;
        if (remaining_size > block_size && max_threads > 1) {
            int num_blocks = (remaining_size + block_size - 1) / block_size;
            if (num_blocks > max_threads) num_blocks = max_threads;
            
            pthread_t* threads = (pthread_t*)malloc(num_blocks * sizeof(pthread_t));
            BlockThreadData* thread_data = (BlockThreadData*)malloc(num_blocks * sizeof(BlockThreadData));
            
            int threads_created = 0;
            
            for (int b = 0; b < num_blocks; b++) {
                pthread_mutex_lock(&global_mutex);
                while (global_active_threads >= max_threads) {
                    pthread_cond_wait(&thread_available, &global_mutex);
                }
                global_active_threads++;
                pthread_mutex_unlock(&global_mutex);
                
                int start_row = k + 1 + b * block_size;
                int end_row = start_row + block_size;
                if (end_row > n) end_row = n;
                
                thread_data[b].A = A;
                thread_data[b].start_row = start_row;
                thread_data[b].end_row = end_row;
                thread_data[b].start_col = k + 1;
                thread_data[b].end_col = n;
                thread_data[b].pivot_row = k;
                thread_data[b].pivot_col = k;
                thread_data[b].n = n;
                
                pthread_create(&threads[b], NULL, block_elimination_thread, &thread_data[b]);
                threads_created++;
            }
            
            for (int b = 0; b < threads_created; b++) {
                pthread_join(threads[b], NULL);
            }
            
            free(threads);
            free(thread_data);
        } else {
            for (int i = k + 1; i < n; i++) {
                double factor = A[i][k] / A[k][k];
                for (int j = k + 1; j < n; j++) {
                    A[i][j] -= factor * A[k][j];
                }
            }
        }
    }
    
    if (det != 0.0) {
        det *= A[n-1][n-1] * sign;
    }
    
    for (int i = 0; i < n; i++) {
        free(A[i]);
    }
    free(A);
    
    return det;
}

double determinant_parallel(const Matrix* matrix, int max_threads) {
    return determinant_parallel_block(matrix, max_threads);
}

void* calculate_cofactor_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    *(data->result) = 0.0;
    
    pthread_mutex_lock(&global_mutex);
    global_active_threads--;
    pthread_cond_signal(&thread_available);
    pthread_mutex_unlock(&global_mutex);
    
    return NULL;
}

double get_time_difference_precise(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

double get_time_difference(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}

DeterminantResult determinant_benchmark(const Matrix* matrix, int max_threads) {
    DeterminantResult result = {0};
    
    if (!matrix_is_valid(matrix)) {
        return result;
    }
    
    struct timespec start, end;
    
    int iterations = 10;
    // if (matrix->size > 25) iterations = 2;
    // if (matrix->size > 30) iterations = 1;
    
    // Лол я нашел что вычисляется быстрее если запустить 5 раз сначала
    // Тотал прикол - кэш наверное - для бенча норм решение
    for (int i = 0; i < 5; i++) {
        determinant_sequential_fast(matrix);
        determinant_parallel_block(matrix, max_threads);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    double det_seq = 0;
    for (int i = 0; i < iterations; i++) {
        det_seq = determinant_sequential_fast(matrix);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.sequential_time = get_time_difference_precise(start, end) / iterations;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    double det_par = 0;
    for (int i = 0; i < iterations; i++) {
        det_par = determinant_parallel_block(matrix, max_threads);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.parallel_time = get_time_difference_precise(start, end) / iterations;
    
    result.determinant = det_par;
    result.threads_used = max_threads;
    
    if (result.parallel_time > 1e-9) {
        result.speedup = result.sequential_time / result.parallel_time;
        result.efficiency = result.speedup / max_threads;
    } else {
        result.speedup = 0.0;
        result.efficiency = 0.0;
    }
    
    double error = fabs(det_seq - det_par);
    if (fabs(det_seq) > 1e-10) {
        error = error / fabs(det_seq) * 100.0;
    }
    
    return result;
}

void print_benchmark_results(const DeterminantResult* result, int matrix_size) {
    if (!result) {
        return;
    }
    
    printf("Детерминант: %.6f\n", result->determinant);
    printf("Время последовательно: %.9f сек\n", result->sequential_time);
    printf("Время параллельно: %.9f сек\n", result->parallel_time);
    printf("Ускорение: %.3fx\n", result->speedup);
    printf("Эффективность: %.2f%% (%.4f)\n", result->efficiency * 100, result->efficiency);
}

void determinant_init_threading(int max_threads) {
    global_max_threads = max_threads;
    global_active_threads = 0;
}

void determinant_cleanup_threading(void) {
    pthread_mutex_lock(&global_mutex);
    while (global_active_threads > 0) {
        pthread_cond_wait(&thread_available, &global_mutex);
    }
    pthread_mutex_unlock(&global_mutex);
}
