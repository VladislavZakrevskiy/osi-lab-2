#!/bin/bash

for size in 3 4 5 6 7; do
  for threads in 1 2 4 8; do
    echo "Размер: $size, Потоки: $threads"
    ./determinant -s $size -t $threads
    echo "---"
  done
done