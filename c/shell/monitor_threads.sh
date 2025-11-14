#!/bin/bash


echo "=== Демонстрация мониторинга потоков ==="
echo ""

if [ $# -eq 1 ]; then
    PID=$1
    echo "Мониторинг процесса с PID: $PID"
else
    echo "Запуск программы в фоновом режиме для демонстрации..."
    ./determinant -s 6 -t 8 &
    PID=$!
    echo "PID процесса: $PID"
    sleep 1
fi

echo ""
echo "=== Информация о процессе ==="

if ! kill -0 $PID 2>/dev/null; then
    echo "Процесс с PID $PID не найден или уже завершен."
    exit 1
fi

OS=$(uname -s)

case $OS in
    "Darwin")
        echo "Операционная система: macOS"
        echo ""
        echo "1. Информация о процессе:"
        ps -p $PID -o pid,ppid,user,pcpu,pmem,time,command
        echo ""
        echo "2. Потоки процесса (ps -M):"
        ps -M $PID 2>/dev/null || echo "Команда ps -M недоступна на этой версии macOS"
        echo ""
        echo "3. Детальная информация о потоках:"
        ps -M $PID -o pid,tid,user,pcpu,pmem,time 2>/dev/null || echo "Детальная информация недоступна"
        ;;
    "Linux")
        echo "Операционная система: Linux"
        echo ""
        echo "1. Информация о процессе:"
        ps -p $PID -o pid,ppid,user,pcpu,pmem,time,cmd
        echo ""
        echo "2. Количество потоков из /proc:"
        if [ -f /proc/$PID/status ]; then
            cat /proc/$PID/status | grep Threads
            echo "Список потоков: $(ls /proc/$PID/task 2>/dev/null | wc -l) потоков"
        else
            echo "/proc/$PID/status не найден"
        fi
        echo ""
        echo "3. Потоки процесса (ps -T):"
        ps -T -p $PID -o pid,spid,user,pcpu,pmem,time,cmd 2>/dev/null
        echo ""
        echo "4. Потоки с помощью top:"
        echo "Используйте: top -H -p $PID"
        ;;
    *)
        echo "Операционная система: $OS"
        echo ""
        echo "Базовая информация о процессе:"
        ps -p $PID 2>/dev/null || echo "Процесс не найден"
        ;;
esac

echo ""
echo "=== Дополнительные команды для мониторинга ==="
echo "htop (если установлен):"
echo "  htop -p $PID"
echo "  Нажмите 'H' в htop для отображения потоков"
echo ""
echo "Мониторинг в реальном времени:"
case $OS in
    "Darwin")
        echo "  Activity Monitor (GUI)"
        echo "  ps -M $PID (повторить команду)"
        ;;
    "Linux")
        echo "  top -H -p $PID"
        echo "  htop -p $PID"
        echo "  watch 'ps -T -p $PID'"
        ;;
esac

echo ""
echo "Для завершения процесса используйте: kill $PID"

if [ $# -eq 0 ]; then
    echo ""
    echo "Ожидание завершения программы..."
    wait $PID
    echo "Программа завершена."
fi
