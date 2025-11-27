CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -pthread
LDFLAGS = -lm -lpthread

TARGET = determinant
MAIN_SOURCE = ./src/main.c
MAIN_OBJECT = ./objects/main.o

MATRIX_SOURCE = ./src/matrix.c
MATRIX_OBJECT = ./objects/matrix.o
DETERMINANT_SOURCE = ./src/determinant.c
DETERMINANT_OBJECT = ./objects/determinant.o
FILE_IO_SOURCE = ./src/file_io.c
FILE_IO_OBJECT = ./objects/file_io.o

OBJECTS = $(MAIN_OBJECT) $(MATRIX_OBJECT) $(DETERMINANT_OBJECT) $(FILE_IO_OBJECT)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

$(MAIN_OBJECT): $(MAIN_SOURCE) ./src/matrix.h ./src/determinant.h ./src/file_io.h
	mkdir -p objects
	$(CC) $(CFLAGS) -c $(MAIN_SOURCE) -o $(MAIN_OBJECT)

$(MATRIX_OBJECT): $(MATRIX_SOURCE) ./src/matrix.h
	mkdir -p objects
	$(CC) $(CFLAGS) -c $(MATRIX_SOURCE) -o $(MATRIX_OBJECT)

$(DETERMINANT_OBJECT): $(DETERMINANT_SOURCE) ./src/determinant.h ./src/matrix.h
	mkdir -p objects
	$(CC) $(CFLAGS) -c $(DETERMINANT_SOURCE) -o $(DETERMINANT_OBJECT)

$(FILE_IO_OBJECT): $(FILE_IO_SOURCE) ./src/file_io.h ./src/matrix.h
	mkdir -p objects
	$(CC) $(CFLAGS) -c $(FILE_IO_SOURCE) -o $(FILE_IO_OBJECT)

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f ../files/*.txt benchmark_results_*.txt

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

run: $(TARGET)
	./$(TARGET)

run-file: $(TARGET) sample_3x3.txt
	./$(TARGET) -f ../files/sample_3x3.txt -t 4

samples: $(TARGET)
	./$(TARGET) --create-sample ./files/sample_2000x2000.txt 2000
	./$(TARGET) --create-sample ./files/sample_3000x3000.txt 3000
	./$(TARGET) --create-sample ./files/sample_4000x4000.txt 4000

demo-files: $(TARGET) samples
	@echo "=== Демонстрация работы с файлами ==="
	@echo "1. Матрица 3x3 из файла:"
	./$(TARGET) -f ../files/sample_3x3.txt -t 2
	@echo ""
	@echo "2. Матрица 4x4 из файла с сохранением результата:"
	./$(TARGET) -f ../files/sample_4x4.txt -t 4 --save result_4x4.txt
	@echo ""
	@echo "3. Случайная матрица 5x5 с сохранением:"
	./$(TARGET) -s 5 -t 4 --save ../files/random_5x5.txt

demo: $(TARGET)
	@echo "=== Демонстрация работы программы ==="
	@echo "1. Случайная матрица 3x3 с 1 потоком:"
	./$(TARGET) -s 3 -t 1
	@echo ""
	@echo "2. Случайная матрица 4x4 с 4 потоками:"
	./$(TARGET) -s 4 -t 4
	@echo ""
	@echo "3. Случайная матрица 5x5 с диапазоном -5..5:"
	./$(TARGET) -s 5 -t 2 -r -5 5

test: $(TARGET)
	./$(TARGET) --test

memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) -s 4 -t 2

benchmark-files: $(TARGET) samples
	@echo "=== Бенчмарк с файлами ==="
	@for file in ../files/sample_*.txt; do \
		echo "Тестирование файла: $$file"; \
		./$(TARGET) -f $$file -t 1; \
		./$(TARGET) -f $$file -t 4; \
		echo ""; \
	done

benchmark: $(TARGET)
	@echo "=== Автоматический бенчмарк ==="
	@for size in 3 4 5 6; do \
		for threads in 1 2 4 8; do \
			echo "Размер: $$size, Потоки: $$threads"; \
			./$(TARGET) -s $$size -t $$threads; \
			echo ""; \
		done; \
	done

sysinfo:
	@echo "=== Информация о системе ==="
	@echo "Операционная система: $$(uname -s)"
	@echo "Архитектура: $$(uname -m)"
	@echo "Количество CPU: $$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 'неизвестно')"
	@echo "Компилятор: $$($(CC) --version | head -1)"
	@echo ""

format-help: $(TARGET)
	./$(TARGET) --format-help

help:
	@echo "Доступные команды:"
	@echo "  all          - Компилировать все программы (по умолчанию)"
	@echo "  clean        - Удалить скомпилированные файлы"
	@echo "  run          - Запустить программу с параметрами по умолчанию"
	@echo "  run-file     - Запустить с примером файла"
	@echo "  samples      - Создать примеры файлов матриц"
	@echo "  demo         - Демонстрация работы с разными параметрами"
	@echo "  demo-files   - Демонстрация работы с файлами"
	@echo "  threads_demo - Демонстрация мониторинга потоков"
	@echo "  test         - Комплексное тестирование"
	@echo "  benchmark    - Автоматический бенчмарк"
	@echo "  benchmark-files - Бенчмарк с файлами"
	@echo "  memcheck     - Проверка на утечки памяти (требует valgrind)"
	@echo "  install      - Установить в систему"
	@echo "  uninstall    - Удалить из системы"
	@echo "  sysinfo      - Показать информацию о системе"
	@echo "  format-help  - Показать формат файла матрицы"
	@echo "  help         - Показать эту справку"
	@echo ""
	@echo "Параметры программы:"
	@echo "  -f FILE      - Загрузить матрицу из файла"
	@echo "  -t N         - Максимальное количество потоков"
	@echo "  -s N         - Размер случайной матрицы NxN"
	@echo "  -r MIN MAX   - Диапазон значений для случайной матрицы"
	@echo "  --save FILE  - Сохранить матрицу в файл"
	@echo "  --test       - Режим тестирования"
	@echo "  -h, --help   - Справка программы"

.PHONY: all clean run run-file samples demo demo-files threads_demo test benchmark benchmark-files memcheck install uninstall sysinfo format-help help