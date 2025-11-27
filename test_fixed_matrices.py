#!/usr/bin/env python3

import subprocess
import re
import json
import matplotlib.pyplot as plt
import numpy as np
import os

def run_test_with_file(matrix_file, threads):
    try:
        cmd = ["./determinant", "-f", matrix_file, "-t", str(threads)]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        
        if result.returncode != 0:
            print(f"    ❌ Ошибка: {result.stderr}")
            return None
            
        output = result.stdout
        
        data = {}
        
        # Парсим детерминант (включая inf и -inf)
        det_match = re.search(r'Детерминант:\s*([-+]?(?:\d+\.?\d*|inf))', output, re.IGNORECASE)
        if det_match:
            det_str = det_match.group(1)
            if 'inf' in det_str.lower():
                data['determinant'] = str(float('inf') if det_str[0] != '-' else float('-inf'))
            else:
                data['determinant'] = float(det_str)
            
        speedup_match = re.search(r'Ускорение:\s*([\d.]+)x', output)
        if speedup_match:
            data['speedup'] = float(speedup_match.group(1))
            
        eff_match = re.search(r'Эффективность:\s*([\d.]+)%', output)
        if eff_match:
            data['efficiency'] = float(eff_match.group(1))
            
        seq_time_match = re.search(r'Время последовательно:\s*([\d.]+)\s*сек', output)
        if seq_time_match:
            data['sequential_time'] = float(seq_time_match.group(1))
            
        par_time_match = re.search(r'Время параллельно:\s*([\d.]+)\s*сек', output)
        if par_time_match:
            data['parallel_time'] = float(par_time_match.group(1))
        
        return data
        
    except Exception as e:
        print(f"    ❌ Ошибка: {e}")
        return None

def main():
    print("🔬 Тестирование на ФИКСИРОВАННЫХ матрицах")
    print("=" * 60)
    
    test_matrices = {
        "2000x2000": "./files/sample_2000x2000.txt",
        "3000x3000": "./files/sample_3000x3000.txt",
        "4000x4000": "./files/sample_4000x4000.txt",
    }
    
    thread_counts = list(range(1, 11))
    
    for name, path in test_matrices.items():
        if not os.path.exists(path):
            print(f"❌ Файл {path} не найден")
            return 1
    
    results = {}
    
    for matrix_name, matrix_file in test_matrices.items():
        print(f"\n📊 Тестирование матрицы {matrix_name} (файл: {matrix_file}):")
        
        size_data = {
            'threads': [],
            'speedup': [],
            'efficiency': [],
            'sequential_times': [],
            'parallel_times': [],
            'determinant': None
        }
        
        for threads in thread_counts:
            print(f"  Потоки: {threads:2d}...", end=" ")
            
            data = run_test_with_file(matrix_file, threads)
            
            if data:
                size_data['threads'].append(threads)
                size_data['speedup'].append(data.get('speedup', 0))
                size_data['efficiency'].append(data.get('efficiency', 0))
                size_data['sequential_times'].append(data.get('sequential_time', 0))
                size_data['parallel_times'].append(data.get('parallel_time', 0))
                
                if size_data['determinant'] is None and 'determinant' in data:
                    size_data['determinant'] = data['determinant']
                
                print(f"Ускорение: {data.get('speedup', 0):5.2f}x, Эффективность: {data.get('efficiency', 0):5.1f}%")
                print(f"Последовательное время: {data.get('sequential_time', 0):5.3f} сек ({(data.get('sequential_time', 0) * 1000):5.0f} мс), Параллельное время: {data.get('parallel_time', 0):5.3f} сек ({(data.get('parallel_time', 0) * 1000):5.0f} мс)")
            else:
                print("❌")
        
        results[matrix_name] = size_data
    
    create_plots(results)
    
    with open('fixed_matrix_results.json', 'w', encoding='utf-8') as f:
        json.dump(results, f, ensure_ascii=False, indent=2)
    
    print(f"\n💾 Результаты сохранены: fixed_matrix_results.json")
    
    return 0

def create_plots(results):
    plt.style.use('seaborn-v0_8' if 'seaborn-v0_8' in plt.style.available else 'default')
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    fig.suptitle('Производительность', fontsize=16, fontweight='bold')
    
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c']
    markers = ['o', 's', '^']
    
    ax1.set_title('Ускорение от количества потоков', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Количество потоков', fontsize=12)
    ax1.set_ylabel('Ускорение (раз)', fontsize=12)
    ax1.grid(True, alpha=0.3)
    
    ax2.set_title('Эффективность от количества потоков', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Количество потоков', fontsize=12)
    ax2.set_ylabel('Эффективность (%)', fontsize=12)
    ax2.grid(True, alpha=0.3)
    
    max_threads = 0
    
    for i, (matrix_name, data) in enumerate(results.items()):
        if not data['threads']:
            continue
            
        color = colors[i % len(colors)]
        marker = markers[i % len(markers)]
        label = f'Матрица {matrix_name}'
        
        threads = data['threads']
        speedup = data['speedup']
        efficiency = data['efficiency']
        
        max_threads = max(max_threads, max(threads))
        
        ax1.plot(threads, speedup, 
                marker=marker, color=color, linewidth=2, markersize=8, 
                label=label, markerfacecolor='white', markeredgewidth=2)
        
        ax2.plot(threads, efficiency, 
                marker=marker, color=color, linewidth=2, markersize=8, 
                label=label, markerfacecolor='white', markeredgewidth=2)
    
    if max_threads > 0:
        ideal_threads = list(range(1, max_threads + 1))
        ax1.plot(ideal_threads, ideal_threads, '--', color='gray', alpha=0.7, 
                linewidth=2, label='Идеальное ускорение')
    
    ax2.axhline(y=100, color='gray', linestyle='--', alpha=0.7, linewidth=2, label='Идеальная эффективность')
    
    for ax in [ax1, ax2]:
        ax.legend(loc='best', frameon=True, fancybox=True, shadow=True)
        if max_threads > 0:
            ax.set_xlim(0.5, max_threads + 0.5)
            ax.set_xticks(range(1, max_threads + 1))
        
    ax1.set_ylim(0, max_threads)
    ax2.set_ylim(0, 150)
    
    plt.tight_layout()
    plt.savefig('fixed_matrices_performance.png', dpi=300, bbox_inches='tight', facecolor='white')
    
    print(f"\nГрафики сохранены: fixed_matrices_performance.png")
    
    try:
        plt.show()
    except:
        print("Графики сохранены в файл (GUI недоступен)")

if __name__ == "__main__":
    main()
