#!/bin/bash

echo "=== Автоматический бенчмарк производительности ==="
echo "Дата: $(date)"
echo "Система: $(uname -s) $(uname -m)"
echo "CPU ядер: $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 'неизвестно')"
echo ""

if [ ! -f "./determinant" ]; then
    echo "Программа не найдена. Компилируем..."
    make clean && make
    if [ $? -ne 0 ]; then
        echo "Ошибка компиляции!"
        exit 1
    fi
fi

RESULTS_FILE="benchmark_results_$(date +%Y%m%d_%H%M%S).txt"
echo "Результаты сохраняются в: $RESULTS_FILE"
echo ""

cat > $RESULTS_FILE << EOF
Бенчмарк производительности многопоточного вычисления детерминанта
Дата: $(date)
Система: $(uname -s) $(uname -m)
CPU ядер: $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 'неизвестно')
Компилятор: $(gcc --version | head -1)

Формат: Размер_матрицы Потоки Время_последовательно Время_параллельно Ускорение Эффективность Погрешность
=====================================================================================================

EOF

SIZES=(3 4 5 6 7)
THREADS=(1 2 4 8)

echo "Размер | Потоки | Послед.(с) | Паралл.(с) | Ускорение | Эффект.(%) | Погрешн.(%)"
echo "-------|--------|------------|-------------|-----------|------------|------------"

for size in "${SIZES[@]}"; do
    for threads in "${THREADS[@]}"; do
        echo -n "  $size    |   $threads    |"
        
        OUTPUT=$(./determinant -s $size -t $threads 2>/dev/null)
        
        TIME_SEQ=$(echo "$OUTPUT" | grep "Время выполнения:" | head -1 | awk '{print $3}')
        TIME_PAR=$(echo "$OUTPUT" | grep "Время выполнения:" | tail -1 | awk '{print $3}')
        SPEEDUP=$(echo "$OUTPUT" | grep "Ускорение:" | awk '{print $2}' | sed 's/x//')
        EFFICIENCY=$(echo "$OUTPUT" | grep "Эффективность:" | awk '{print $2}' | sed 's/%//')
        ERROR=$(echo "$OUTPUT" | grep "Погрешность:" | awk '{print $2}' | sed 's/%//')
        
        printf " %9s | %9s | %8s | %9s | %9s\n" "$TIME_SEQ" "$TIME_PAR" "$SPEEDUP" "$EFFICIENCY" "$ERROR"
        
        echo "$size $threads $TIME_SEQ $TIME_PAR $SPEEDUP $EFFICIENCY $ERROR" >> $RESULTS_FILE
        
        sleep 0.1
    done
    echo "-------|--------|------------|-------------|-----------|------------|------------"
done

echo ""
echo "=== Анализ результатов ==="

echo ""
echo "1. Лучшее ускорение по размерам матриц:"
for size in "${SIZES[@]}"; do
    BEST_SPEEDUP=$(grep "^$size " $RESULTS_FILE | awk '{print $5}' | sort -nr | head -1)
    BEST_THREADS=$(grep "^$size " $RESULTS_FILE | awk -v max="$BEST_SPEEDUP" '$5 == max {print $2}' | head -1)
    echo "  Размер $size: ${BEST_SPEEDUP}x с $BEST_THREADS потоками"
done

echo ""
echo "2. Эффективность по количеству потоков:"
for threads in "${THREADS[@]}"; do
    AVG_EFF=$(grep " $threads " $RESULTS_FILE | awk '{sum+=$6; count++} END {if(count>0) print sum/count; else print 0}')
    echo "  $threads потоков: $(printf "%.1f" $AVG_EFF)% средняя эффективность"
done

echo ""
echo "3. Рекомендации:"

BEST_OVERALL=$(awk '{print $5, $2}' $RESULTS_FILE | sort -nr | head -1)
BEST_SPEEDUP_VAL=$(echo $BEST_OVERALL | awk '{print $1}')
BEST_THREADS_VAL=$(echo $BEST_OVERALL | awk '{print $2}')

echo "  - Лучшее общее ускорение: ${BEST_SPEEDUP_VAL}x с $BEST_THREADS_VAL потоками"

HIGH_EFF_COUNT=$(awk '$6 > 50 {count++} END {print count+0}' $RESULTS_FILE)
TOTAL_TESTS=$(wc -l < $RESULTS_FILE | awk '{print $1-6}') 

if [ $HIGH_EFF_COUNT -gt 0 ]; then
    echo "  - Высокая эффективность (>50%) в $HIGH_EFF_COUNT из $TOTAL_TESTS тестов"
else
    echo "  - Низкая эффективность во всех тестах - алгоритм плохо масштабируется"
fi

OVERHEAD_COUNT=$(awk '$5 < 1 {count++} END {print count+0}' $RESULTS_FILE)
if [ $OVERHEAD_COUNT -gt 0 ]; then
    echo "  - Замедление из-за накладных расходов в $OVERHEAD_COUNT тестах"
    echo "  - Рекомендуется использовать многопоточность только для больших матриц"
fi

echo ""
echo "4. Объяснение результатов:"
echo "  - Малые матрицы (3x3, 4x4): накладные расходы превышают выигрыш от параллелизации"
echo "  - Большие матрицы (6x6+): заметное ускорение, но эффективность снижается"
echo "  - Алгоритм имеет ограниченный параллелизм из-за рекурсивной природы"
echo "  - Оптимальное количество потоков обычно равно количеству CPU ядер"

echo ""
echo "Подробные результаты сохранены в файле: $RESULTS_FILE"

echo ""
echo "=== Текстовый график ускорения ==="
echo ""
for size in "${SIZES[@]}"; do
    echo "Размер ${size}x${size}:"
    for threads in "${THREADS[@]}"; do
        SPEEDUP=$(grep "^$size $threads " $RESULTS_FILE | awk '{print $5}')
        if [ ! -z "$SPEEDUP" ]; then
            BARS=$(echo "$SPEEDUP" | awk '{printf "%.0f", $1*10}')
            BAR_STR=""
            for ((i=1; i<=BARS && i<=50; i++)); do
                BAR_STR="${BAR_STR}█"
            done
            printf "  %d потоков: %-20s %.2fx\n" "$threads" "$BAR_STR" "$SPEEDUP"
        fi
    done
    echo ""
done

echo "Легенда: каждый символ █ = 0.1x ускорения"
