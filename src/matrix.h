#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    double **data;
    int size;
} Matrix;

Matrix* matrix_create(int size);

void matrix_free(Matrix* matrix);

void matrix_fill_random(Matrix* matrix, int min_val, int max_val);

void matrix_print(const Matrix* matrix);

Matrix* matrix_create_submatrix(const Matrix* matrix, int exclude_row, int exclude_col);

Matrix* matrix_copy(const Matrix* matrix);

double** copy_matrix_data(const Matrix* matrix);
void free_matrix_data(double** data, int size);

int matrix_is_valid(const Matrix* matrix);

#endif 
