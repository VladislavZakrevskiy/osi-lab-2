#ifndef FILE_IO_H
#define FILE_IO_H

#include "matrix.h"

Matrix* matrix_read_from_file(const char* filename);
int matrix_save_to_file(const Matrix* matrix, const char* filename);
int file_exists(const char* filename);
int create_sample_matrix_file(const char* filename, int size, int min_val, int max_val);
void print_matrix_file_format_help(void);

#endif 
