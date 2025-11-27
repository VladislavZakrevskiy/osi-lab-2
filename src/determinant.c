#include "determinant.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int waiting;
    int generation;
} pthread_barrier_custom_t;

void pthread_barrier_custom_init(pthread_barrier_custom_t* barrier, int count) {
    pthread_mutex_init(&barrier->mutex, NULL);
    pthread_cond_init(&barrier->cond, NULL);
    barrier->count = count;
    barrier->waiting = 0;
    barrier->generation = 0;
}

void pthread_barrier_custom_destroy(pthread_barrier_custom_t* barrier) {
    pthread_mutex_destroy(&barrier->mutex);
    pthread_cond_destroy(&barrier->cond);
}

void pthread_barrier_custom_wait(pthread_barrier_custom_t* barrier) {
    pthread_mutex_lock(&barrier->mutex);
    
    int my_generation = barrier->generation;
    barrier->waiting++;
    
    if (barrier->waiting >= barrier->count) {
        barrier->waiting = 0;
        barrier->generation++;
        pthread_cond_broadcast(&barrier->cond);
    } else {
        while (my_generation == barrier->generation) {
            pthread_cond_wait(&barrier->cond, &barrier->mutex);
        }
    }
    
    pthread_mutex_unlock(&barrier->mutex);
}

// ========== ПОСЛЕДОВАТЕЛЬНЫЙ АЛГОРИТМ ==========
double determinant_sequential_fast(const Matrix* matrix) {
    if (!matrix_is_valid(matrix)) {
        return 0.0;
    }
    
    if (matrix->size == 1) {
        return matrix->data[0][0];
    }
    
    if (matrix->size == 2) {
        return matrix->data[0][0] * matrix->data[1][1] - 
               matrix->data[0][1] * matrix->data[1][0];
    }
    
    int n = matrix->size;
    double** A = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        A[i] = (double*)malloc(n * sizeof(double));
        memcpy(A[i], matrix->data[i], n * sizeof(double));
    }
    
    double det = 1.0;
    int sign = 1;
    
    // Гауссово исключение с частичным выбором главного элемента
    for (int k = 0; k < n; k++) {
        // Поиск максимального элемента
        int max_row = k;
        for (int i = k + 1; i < n; i++) {
            if (fabs(A[i][k]) > fabs(A[max_row][k])) {
                max_row = i;
            }
        }
        
        // Перестановка строк
        if (max_row != k) {
            double* temp = A[k];
            A[k] = A[max_row];
            A[max_row] = temp;
            sign = -sign;
        }
        
        // Проверка на вырожденность
        if (fabs(A[k][k]) < 1e-10) {
            det = 0.0;
            break;
        }
        
        det *= A[k][k];
        
        // Исключение
        for (int i = k + 1; i < n; i++) {
            double factor = A[i][k] / A[k][k];
            for (int j = k + 1; j < n; j++) {
                A[i][j] -= factor * A[k][j];
            }
        }
    }
    
    if (det != 0.0) {
        det *= sign;
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

// ========== ПАРАЛЛЕЛЬНЫЙ АЛГОРИТМ ==========
typedef struct {
    double** A;
    int n;
    int thread_id;
    int num_threads;
    pthread_barrier_custom_t* barrier;
    double* det;
    int* sign;
    volatile int* should_stop;
} StaticThreadData;

void* gaussian_elimination_static_thread(void* arg) {
    StaticThreadData* data = (StaticThreadData*)arg;
    int thread_id = data->thread_id;
    int num_threads = data->num_threads;
    int n = data->n;
    double** A = data->A;
    
    // Каждый поток работает над своей частью строк
    for (int k = 0; k < n - 1; k++) {
        // Только первый поток выполняет поиск главного элемента и перестановку
        if (thread_id == 0) {
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
                *(data->sign) = -(*(data->sign));
            }
            
            if (fabs(A[k][k]) < 1e-10) {
                *(data->should_stop) = 1;
                *(data->det) = 0.0;
            } else {
                *(data->det) *= A[k][k];
            }
        }
        
        // Барьер - все ждут пока главный поток закончит перестановку
        pthread_barrier_custom_wait(data->barrier);
        
        // Если матрица вырождена, выходим
        if (*(data->should_stop)) {
            break;
        }
        
        // Параллельное Гауссово исключение
        int remaining_rows = n - k - 1;
        int rows_per_thread = remaining_rows / num_threads;
        int extra_rows = remaining_rows % num_threads;
        
        int start_row = k + 1 + thread_id * rows_per_thread + (thread_id < extra_rows ? thread_id : extra_rows);
        int end_row = start_row + rows_per_thread + (thread_id < extra_rows ? 1 : 0);
        
        double pivot = A[k][k];
        double* pivot_row = A[k];
        
        // Каждый поток обрабатывает свои строки
        for (int i = start_row; i < end_row; i++) {
            double factor = A[i][k] / pivot;
            double* current_row = A[i];
            
            // Векторизованная обработка
            int j = k + 1;
            int remaining = n - j;
            int end_aligned = j + (remaining / 4) * 4;
            
            for (; j < end_aligned; j += 4) {
                current_row[j] -= factor * pivot_row[j];
                current_row[j+1] -= factor * pivot_row[j+1];
                current_row[j+2] -= factor * pivot_row[j+2];
                current_row[j+3] -= factor * pivot_row[j+3];
            }
            
            for (; j < n; j++) {
                current_row[j] -= factor * pivot_row[j];
            }
        }
        
        // Барьер - все ждут пока все закончат текущую итерацию
        pthread_barrier_custom_wait(data->barrier);
    }
    
    return NULL;
}

double determinant_parallel_block_lu(const Matrix* matrix, int max_threads) {
    if (!matrix_is_valid(matrix)) {
        return 0.0;
    }
    
    int n = matrix->size;
    
    if (n == 1) {
        return matrix->data[0][0];
    }
    
    if (n == 2) {
        return matrix->data[0][0] * matrix->data[1][1] - 
               matrix->data[0][1] * matrix->data[1][0];
    }
    
    if (n < 800 || max_threads == 1) {
        return determinant_sequential_fast(matrix);
    }
    
    double** A = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        A[i] = (double*)malloc(n * sizeof(double));
        memcpy(A[i], matrix->data[i], n * sizeof(double));
    }
    
    double det = 1.0;
    int sign = 1;
    volatile int should_stop = 0;
    
    int num_threads = max_threads > 16 ? 16 : max_threads;
    
    pthread_barrier_custom_t barrier;
    pthread_barrier_custom_init(&barrier, num_threads);
    
    pthread_t threads[16];
    StaticThreadData thread_data[16];
    
    for (int t = 0; t < num_threads; t++) {
        thread_data[t].A = A;
        thread_data[t].n = n;
        thread_data[t].thread_id = t;
        thread_data[t].num_threads = num_threads;
        thread_data[t].barrier = &barrier;
        thread_data[t].det = &det;
        thread_data[t].sign = &sign;
        thread_data[t].should_stop = &should_stop;
        
        pthread_create(&threads[t], NULL, gaussian_elimination_static_thread, &thread_data[t]);
    }
    
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
    
    pthread_barrier_custom_destroy(&barrier);
    
    det *= sign;
    
    for (int i = 0; i < n; i++) {
        free(A[i]);
    }
    free(A);
    
    return det;
}

double determinant_parallel(const Matrix* matrix, int max_threads) {
    return determinant_parallel_block_lu(matrix, max_threads);
}

double get_time_difference_precise(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

DeterminantResult determinant_benchmark(const Matrix* matrix, int max_threads) {
    DeterminantResult result = {0};
    
    if (!matrix_is_valid(matrix)) {
        return result;
    }
    
    struct timespec start, end;

    // Sequential
    clock_gettime(CLOCK_MONOTONIC, &start);

    determinant_sequential_fast(matrix);

    clock_gettime(CLOCK_MONOTONIC, &end);

    result.sequential_time = get_time_difference_precise(start, end);

    // Parallel
    clock_gettime(CLOCK_MONOTONIC, &start);

    determinant_parallel(matrix, max_threads);

    clock_gettime(CLOCK_MONOTONIC, &end);

    result.parallel_time = get_time_difference_precise(start, end);
    
    result.determinant = determinant_parallel(matrix, max_threads);
    result.threads_used = max_threads;
    
    if (result.parallel_time > 1e-9) {
        result.speedup = result.sequential_time / result.parallel_time;
        result.efficiency = result.speedup / max_threads;
    } else {
        result.speedup = 0.0;
        result.efficiency = 0.0;
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
