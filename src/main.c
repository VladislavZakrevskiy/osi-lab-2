#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "matrix.h"
#include "determinant.h"
#include "file_io.h"

void print_usage(const char* program_name) {
    printf("=== Программа вычисления детерминанта матрицы (многопоточная версия) ===\n\n");
    printf("Использование: %s [опции]\n\n", program_name);
    printf("Опции:\n");
    printf("  -f, --file FILE    Загрузить матрицу из файла\n");
    printf("  -t, --threads N    Максимальное количество потоков (по умолчанию: 4)\n");
    printf("  -s, --size N       Размер случайной матрицы NxN (по умолчанию: 5)\n");
    printf("  -r, --range MIN MAX Диапазон значений для случайной матрицы (по умолчанию: -10 10)\n");
    printf("  --save FILE        Сохранить матрицу в файл\n");
    printf("  --create-sample FILE SIZE Создать пример файла матрицы\n");
    printf("  --test             Режим тестирования производительности\n");
    printf("  --format-help      Показать формат файла матрицы\n");
    printf("  -h, --help         Показать эту справку\n\n");
    printf("Примеры:\n");
    printf("  %s -f matrix.txt -t 8          # Загрузить из файла, 8 потоков\n", program_name);
    printf("  %s -s 6 -t 4 --save result.txt # Случайная 6x6, сохранить в файл\n", program_name);
    printf("  %s --create-sample test.txt 4  # Создать пример файла 4x4\n", program_name);
    printf("  %s --test                      # Режим тестирования\n", program_name);
}

void performance_test_with_matrix(const Matrix* matrix, int max_threads) {
    if (!matrix_is_valid(matrix)) {
        printf("Ошибка: некорректная матрица\n");
        return;
    }

    DeterminantResult result = determinant_benchmark(matrix, max_threads);
    print_benchmark_results(&result);
}

void run_comprehensive_test() {
    printf("\n=== Комплексное тестирование производительности ===\n");
    
    int sizes[] = {3, 4, 5, 6, 7};
    int thread_counts[] = {1, 2, 4, 8};
    
    printf("\nРазмер | Потоки | Детерминант | Послед.(с) | Паралл.(с) | Ускорение | Эффект.(%%)\n");
    printf("-------|--------|-------------|------------|-------------|-----------|----------\n");
    
    for (int s = 0; s < 5; s++) {
        for (int t = 0; t < 4; t++) {
            Matrix* matrix = matrix_create(sizes[s]);
            if (!matrix) continue;
            
            matrix_fill_random(matrix, -10, 10);
            DeterminantResult result = determinant_benchmark(matrix, thread_counts[t]);
            
            printf("  %d    |   %d    | %10.2f | %9.6f | %9.6f |   %5.2fx   |  %6.1f%%\n",
                    sizes[s], thread_counts[t], result.determinant,
                    result.sequential_time, result.parallel_time,
                    result.speedup, result.efficiency * 100);
            
            matrix_free(matrix);
            
            if (s < 4 || t < 3) {
                printf("Нажмите Enter для продолжения...");
                getchar();
            }
        }
        printf("-------|--------|-------------|------------|-------------|-----------|----------\n");
    }
}

int main(int argc, char* argv[]) {
    char* input_file = NULL;
    char* output_file = NULL;
    char* sample_file = NULL;
    int max_threads = 4;
    int matrix_size = 5;
    int min_val = -10;
    int max_val = 10;
    int sample_size = 4;
    int test_mode = 0;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) && i + 1 < argc) {
            input_file = argv[i + 1];
            i++;
        } else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0) && i + 1 < argc) {
            max_threads = atoi(argv[i + 1]);
            i++;
        } else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--size") == 0) && i + 1 < argc) {
            matrix_size = atoi(argv[i + 1]);
            i++;
        } else if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--range") == 0) && i + 2 < argc) {
            min_val = atoi(argv[i + 1]);
            max_val = atoi(argv[i + 2]);
            i += 2;
        } else if (strcmp(argv[i], "--save") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--create-sample") == 0 && i + 2 < argc) {
            sample_file = argv[i + 1];
            sample_size = atoi(argv[i + 2]);
            i += 2;
        } else if (strcmp(argv[i], "--test") == 0) {
            test_mode = 1;
        } else if (strcmp(argv[i], "--format-help") == 0) {
            print_matrix_file_format_help();
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("Неизвестный параметр: %s\n", argv[i]);
            return 1;
        }
    }

    if (max_threads < 1 ) {
        printf("Ошибка: количество потоков должно быть от 1\n");
        return 1;
    }

    if (matrix_size < 1) {
        printf("Ошибка: размер матрицы должен быть от 1\n");
        return 1;
    }

    if (min_val >= max_val) {
        printf("Ошибка: минимальное значение должно быть меньше максимального\n");
        return 1;
    }

    if (sample_file) {
        if (!create_sample_matrix_file(sample_file, sample_size, min_val, max_val)) {
            printf("Ошибка создания файла\n");
            return 1;
        }
        return 0;
    }

    Matrix* matrix = NULL;

    if (input_file) {
        matrix = matrix_read_from_file(input_file);
        if (!matrix) {
            printf("Не удалось загрузить матрицу из файла\n");
            return 1;
        }
    } else {
        matrix = matrix_create(matrix_size);
        if (!matrix) {
            printf("Ошибка создания матрицы\n");
            return 1;
        }

        matrix_fill_random(matrix, min_val, max_val);
    }

    if (output_file) {
        if (!matrix_save_to_file(matrix, output_file)) {
            printf("Ошибка сохранения матрицы в файл\n");
        }
    }

    if (test_mode) {
        run_comprehensive_test();
    } else {
        performance_test_with_matrix(matrix, max_threads);
    }

    matrix_free(matrix);

    return 0;
}
