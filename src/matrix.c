#include "matrix.h"
#include <time.h>
#include <string.h>


void matrix_fill_random(Matrix* matrix, int min_val, int max_val) {
    if (!matrix_is_valid(matrix) || min_val >= max_val) {
        return;
    }

    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }

    int range = max_val - min_val;
    for (int i = 0; i < matrix->size; i++) {
        for (int j = 0; j < matrix->size; j++) {
            matrix->data[i][j] = (double)(rand() % range + min_val);
        }
    }
}

void matrix_print(const Matrix* matrix) {
    if (!matrix_is_valid(matrix)) {
        printf("Некорректная матрица\n");
        return;
    }

    printf("Матрица %dx%d:\n", matrix->size, matrix->size);
    for (int i = 0; i < matrix->size; i++) {
        for (int j = 0; j < matrix->size; j++) {
            printf("%8.2f ", matrix->data[i][j]);
        }
        printf("\n");
    }

    printf("\n");
}

Matrix* matrix_create_submatrix(const Matrix* matrix, int exclude_row, int exclude_col) {
    if (!matrix_is_valid(matrix) || 
        exclude_row < 0 || exclude_row >= matrix->size ||
        exclude_col < 0 || exclude_col >= matrix->size ||
        matrix->size <= 1) {
        return NULL;
    }

    Matrix* submatrix = matrix_create(matrix->size - 1);
    if (!submatrix) {
        return NULL;
    }

    int sub_i = 0;
    for (int i = 0; i < matrix->size; i++) {
        if (i == exclude_row) continue;

        int sub_j = 0;
        for (int j = 0; j < matrix->size; j++) {
            if (j == exclude_col) continue;
            
            submatrix->data[sub_i][sub_j] = matrix->data[i][j];
            sub_j++;
        }
        sub_i++;
    }

    return submatrix;
}

Matrix* matrix_copy(const Matrix* matrix) {
    if (!matrix_is_valid(matrix)) {
        return NULL;
    }

    Matrix* copy = matrix_create(matrix->size);
    if (!copy) {
        return NULL;
    }

    for (int i = 0; i < matrix->size; i++) {
        for (int j = 0; j < matrix->size; j++) {
            copy->data[i][j] = matrix->data[i][j];
        }
    }

    return copy;
}

int matrix_is_valid(const Matrix* matrix) {
    return matrix != NULL && matrix->data != NULL && matrix->size > 0;
}

Matrix* matrix_create(int size) {
    Matrix* matrix = (Matrix*)malloc(sizeof(Matrix));
    if (!matrix) return NULL;
    
    matrix->size = size;
    matrix->data = (double**)malloc(size * sizeof(double*));
    if (!matrix->data) {
        free(matrix);
        return NULL;
    }
    
    for (int i = 0; i < size; i++) {
        matrix->data[i] = (double*)calloc(size, sizeof(double));
        if (!matrix->data[i]) {
            for (int j = 0; j < i; j++) {
                free(matrix->data[j]);
            }
            free(matrix->data);
            free(matrix);
            return NULL;
        }
    }
    
    return matrix;
}

void matrix_free(Matrix* matrix) {
    if (!matrix) return;
    if (matrix->data) {
        for (int i = 0; i < matrix->size; i++) {
            free(matrix->data[i]);
        }
        free(matrix->data);
    }
    free(matrix);
}

double** copy_matrix_data(const Matrix* matrix) {
    int n = matrix->size;
    double** copy = (double**)malloc(n * sizeof(double*));
    if (!copy) return NULL;
    
    for (int i = 0; i < n; i++) {
        copy[i] = (double*)malloc(n * sizeof(double));
        if (!copy[i]) {
            for (int j = 0; j < i; j++) {
                free(copy[j]);
            }
            free(copy);
            return NULL;
        }
        memcpy(copy[i], matrix->data[i], n * sizeof(double));
    }
    
    return copy;
}

void free_matrix_data(double** data, int size) {
    if (!data) return;
    for (int i = 0; i < size; i++) {
        free(data[i]);
    }
    free(data);
}