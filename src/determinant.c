#include "determinant.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#define a 1

// pivot = опорный элемент

double algorithm_sequential(const Matrix* matrix) {
    if (!matrix_is_valid(matrix)) {
        return 0.0;
    }
    
    int n = matrix->size;
    double** temp = copy_matrix_data(matrix);
    if (!temp) return 0.0;
    
    double det = 1.0;
    int swap_count = 0;
    const double EPS = 1e-12;
    
    for (int col = 0; col < n; col++) {
        int max_row = col;
        double max_val = fabs(temp[col][col]);
        
        for (int row = col + 1; row < n; row++) {
            double val = fabs(temp[row][col]);
            if (val > max_val) {
                max_val = val;
                max_row = row;
            }
        }
        
        if (max_val < EPS) {
            free_matrix_data(temp, n);
            return 0.0;
        }
        
        if (max_row != col) {
            for (int k = 0; k < n; k++) {
                double tmp = temp[col][k];
                temp[col][k] = temp[max_row][k];
                temp[max_row][k] = tmp;
            }
            swap_count++;
        }
        
        double pivot = temp[col][col];
        
        for (int row = col + 1; row < n; row++) {
            double factor = temp[row][col] / pivot;
            
            for (int k = col + 1; k < n; k++) {
                temp[row][k] -= factor * temp[col][k];
            }
            temp[row][col] = 0.0;
        }
    }
    
    for (int i = 0; i < n; i++) {
        det *= temp[i][i];
    }
    
    if (swap_count % 2 == 1) {
        det = -det;
    }
    
    free_matrix_data(temp, n);
    
    return det;
}

void* eliminate_rows_thread(void* arg) {
    RowEliminationData* data = (RowEliminationData*)arg;
    
    double** matrix = data->matrix;
    int size = data->size;
    int pivot_row = data->pivot_row;
    double pivot = matrix[pivot_row][pivot_row];
    
    for (int row = data->start_row; row < data->end_row; row++) {
        if (row <= pivot_row) continue;
        
        double factor = matrix[row][pivot_row] / pivot;
        
        for (int col = pivot_row + 1; col < size; col++) {
            matrix[row][col] -= factor * matrix[pivot_row][col];
        }
        matrix[row][pivot_row] = 0.0;
    }
    
    return NULL;
}

double algorithm_parallel(const Matrix* matrix, int max_threads) {
    if (!matrix_is_valid(matrix)) {
        return 0.0;
    }
    
    int n = matrix->size;
    
    if (max_threads == 1) {
        return algorithm_sequential(matrix);
    }
    
    double** temp = copy_matrix_data(matrix);
    if (!temp) return 0.0;
    
    double det = 1.0;
    int swap_count = 0;
    const double EPS = 1e-12;
    
    pthread_t* threads = (pthread_t*)malloc(max_threads * sizeof(pthread_t));
    RowEliminationData* thread_data = (RowEliminationData*)malloc(max_threads * sizeof(RowEliminationData));
    
    if (!threads || !thread_data) {
        free(threads);
        free(thread_data);
        free_matrix_data(temp, n);
        return algorithm_sequential(matrix);
    }
    
    for (int col = 0; col < n; col++) {
        int max_row = col;
        double max_val = fabs(temp[col][col]);
        
        for (int row = col + 1; row < n; row++) {
            double val = fabs(temp[row][col]);
            if (val > max_val) {
                max_val = val;
                max_row = row;
            }
        }
        
        if (max_val < EPS) {
            free(threads);
            free(thread_data);
            free_matrix_data(temp, n);
            return 0.0;
        }
        
        if (max_row != col) {
            double* tmp_row = temp[col];
            temp[col] = temp[max_row];
            temp[max_row] = tmp_row;
            swap_count++;
        }
        
        int rows_to_process = n - col - 1;
        
        if (rows_to_process < max_threads * 2) {
            double pivot = temp[col][col];
            for (int row = col + 1; row < n; row++) {
                double factor = temp[row][col] / pivot;
                for (int k = col + 1; k < n; k++) {
                    temp[row][k] -= factor * temp[col][k];
                }
                temp[row][col] = 0.0;
            }
        } else {
            int actual_threads = (rows_to_process < max_threads) ? rows_to_process : max_threads;
            int rows_per_thread = rows_to_process / actual_threads;
            int extra_rows = rows_to_process % actual_threads;
            
            int current_row = col + 1;
            
            for (int t = 0; t < actual_threads; t++) {
                thread_data[t].matrix = temp;
                thread_data[t].size = n;
                thread_data[t].pivot_row = col;
                thread_data[t].start_row = current_row;
                
                int rows_for_this_thread = rows_per_thread + (t < extra_rows ? 1 : 0);
                current_row += rows_for_this_thread;
                thread_data[t].end_row = current_row;
                
                pthread_create(&threads[t], NULL, eliminate_rows_thread, &thread_data[t]);
            }
            
            for (int t = 0; t < actual_threads; t++) {
                pthread_join(threads[t], NULL);
            }
        }
    }
    
    for (int i = 0; i < n; i++) {
        det *= temp[i][i];
    }
    
    // Опытным путем (костыль скорее)
    if (swap_count % 2 == 1) {
        det = -det;
    }
    
    free(threads);
    free(thread_data);
    free_matrix_data(temp, n);
    
    return det;
}

double get_time_difference_precise(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

DeterminantResult determinant_benchmark(const Matrix* matrix, int max_threads) {
    DeterminantResult result = {0};
    
    if (!matrix_is_valid(matrix)) {
        return result;
    }
    
    struct timespec start, end;

    // Sequential
    clock_gettime(CLOCK_MONOTONIC, &start);
    double seq_det = algorithm_sequential(matrix);
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.sequential_time = get_time_difference_precise(start, end);

    // Parallel
    clock_gettime(CLOCK_MONOTONIC, &start);
    double par_det = algorithm_parallel(matrix, max_threads);
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.parallel_time = get_time_difference_precise(start, end);
    
    result.determinant = par_det;
    result.threads_used = max_threads;
    
    if (result.parallel_time > 1e-9) {
        result.speedup = result.sequential_time / result.parallel_time;
        result.efficiency = result.speedup / max_threads;
    } else {
        result.speedup = 0.0;
        result.efficiency = 0.0;
    }
    
    if (fabs(seq_det - par_det) > 1e-6 * fabs(seq_det) && fabs(seq_det) > 1e-12) {
        printf("Warning: Results differ! Sequential: %g, Parallel: %g\n", seq_det, par_det);
    }
    
    return result;
}


void print_benchmark_results(const DeterminantResult* result) {
    if (!result) {
        return;
    }
    
    printf("Детерминант: %.6f\n", result->determinant);
    printf("Время последовательно: %.9f сек (%.3f мс)\n", result->sequential_time, result->sequential_time * 1000);
    printf("Время параллельно: %.9f сек (%.3f мс)\n", result->parallel_time, result->parallel_time * 1000);
    printf("Ускорение: %.3fx\n", result->speedup);
    printf("Эффективность: %.2f%% (%.4f)\n", result->efficiency * 100, result->efficiency);
    printf("Потоков использовано: %d\n", result->threads_used);
}
