#include "file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

Matrix* matrix_read_from_file(const char* filename) {
    if (!filename) {
        printf("Ошибка: не указано имя файла\n");
        return NULL;
    }

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Ошибка: не удалось открыть файл '%s': %s\n", filename, strerror(errno));
        return NULL;
    }

    int size;
    if (fscanf(file, "%d", &size) != 1) {
        printf("Ошибка: не удалось прочитать размер матрицы из файла '%s'\n", filename);
        fclose(file);
        return NULL;
    }

    if (size <= 0 || size > 50) {
        printf("Ошибка: некорректный размер матрицы %d (должен быть от 1 до 50)\n", size);
        fclose(file);
        return NULL;
    }

    Matrix* matrix = matrix_create(size);
    if (!matrix) {
        printf("Ошибка: не удалось создать матрицу размера %d\n", size);
        fclose(file);
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (fscanf(file, "%lf", &matrix->data[i][j]) != 1) {
                printf("Ошибка: не удалось прочитать элемент [%d][%d] из файла '%s'\n", i, j, filename);
                matrix_free(matrix);
                fclose(file);
                return NULL;
            }
        }
    }

    fclose(file);
    return matrix;
}

int matrix_save_to_file(const Matrix* matrix, const char* filename) {
    if (!matrix_is_valid(matrix) || !filename) {
        return 0;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Ошибка: не удалось создать файл '%s': %s\n", filename, strerror(errno));
        return 0;
    }
    
    fprintf(file, "%d\n", matrix->size);
    
    for (int i = 0; i < matrix->size; i++) {
        for (int j = 0; j < matrix->size; j++) {
            fprintf(file, "%.6f", matrix->data[i][j]);
            if (j < matrix->size - 1) {
                fprintf(file, " ");
            }
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    return 1;
}

int file_exists(const char* filename) {
    if (!filename) {
        return 0;
    }
    
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

int create_sample_matrix_file(const char* filename, int size, int min_val, int max_val) {
    if (!filename || size <= 0 || size > 50 || min_val >= max_val) {
        return 0;
    }
    
    Matrix* matrix = matrix_create(size);
    if (!matrix) {
        return 0;
    }
    
    matrix_fill_random(matrix, min_val, max_val);
    
    int result = matrix_save_to_file(matrix, filename);
    matrix_free(matrix);
    
    return result;
}

void print_matrix_file_format_help(void) {
    printf("=== Формат файла матрицы ===\n\n");
    printf("Файл должен содержать:\n");
    printf("1. Первая строка: размер матрицы (целое число от 1 до 20)\n");
    printf("2. Следующие строки: элементы матрицы (вещественные числа)\n\n");
    printf("Пример файла для матрицы 3x3:\n");
    printf("3\n");
    printf("1.0 2.0 3.0\n");
    printf("4.0 5.0 6.0\n");
    printf("7.0 8.0 9.0\n\n");
    printf("Примечания:\n");
    printf("- Элементы в строке разделяются пробелами\n");
    printf("- Можно использовать целые числа (они будут преобразованы в вещественные)\n");
    printf("- Пустые строки и лишние пробелы игнорируются\n");
}
