import subprocess
import time
import numpy as np
import os
import matplotlib.pyplot as plt
from generate_n_points import generate_bodies

def run_program(num_threads, exec_path="src/task1/n_body_cuda"):
    """
    Запускает программу с заданным числом потоков и измеряет время её выполнения.
    Возвращает время в секундах.
    Предполагается, что программа принимает число потоков в качестве аргумента.
    Если это не так, нужно адаптировать вызов.
    """
    start = time.time()
    # Передаём число потоков как аргумент командной строки
    subprocess.run([exec_path, str(num_threads)], check=True)
    end = time.time()
    return end - start

if __name__ == "__main__":
    # Параметры бенчмарка
    ns = [100, 200, 500, 1000, 2500]
    thread_list = [1, 4, 16, 64, 256, 1024]  # пример набора потоков
    repetitions = 5

    if not os.path.exists("src/task1"):
        os.makedirs("src/task1")

    root_dir = os.getcwd()
    input_path = f'{root_dir}/src/task1/input.txt'

    # Результаты будем хранить в формате:
    # results[(n, num_threads)] = (avg_time, std_time)
    results = {}

    # Запускаем тесты
    for n in ns:
        # Генерируем входные данные для данной задачи
        generate_bodies(n, input_path)
        for tcount in thread_list:
            times = []
            for _ in range(repetitions):
                elapsed = run_program(tcount)
                times.append(elapsed)
            avg_time = np.mean(times)
            std_time = np.std(times)
            results[(n, tcount)] = (avg_time, std_time)
            print(f"N={n}, threads={tcount}: avg_time={avg_time:.6f}s ±{std_time:.6f}s")

    # Записываем результаты в Markdown
    md_filename = "benchmark_results.md"
    with open(md_filename, "w") as f:
        f.write("# Benchmark Results\n\n")
        f.write("## Summary\n")
        f.write("Below is a table of benchmark results measured by varying N and number of threads.\n\n")

        f.write("| N | Threads | avg_time_s | std_time_s |\n")
        f.write("|---|----------|------------|------------|\n")
        for n in ns:
            for tcount in thread_list:
                avg_t, std_t = results[(n, tcount)]
                f.write(f"| {n} | {tcount} | {avg_t:.6f} | {std_t:.6f} |\n")

    # Строим графики производительности

    # 1. График времени от N при разном числе потоков
    # Для каждого tcount построим зависимость avg_time от N
    plt.figure(figsize=(8,6))
    for tcount in thread_list:
        avg_times = [results[(n, tcount)][0] for n in ns]
        plt.plot(ns, avg_times, marker='o', label=f"{tcount} threads")
    plt.xlabel("Number of bodies (N)")
    plt.ylabel("Average Time (s)")
    plt.title("Performance scaling with number of bodies")
    plt.legend()
    plt.grid(True)
    plt.savefig("time_vs_n.png", dpi=150)
    plt.close()

    # Добавим ссылку в md
    with open(md_filename, "a") as f:
        f.write("\n## Time vs N\n")
        f.write("![Time vs N](time_vs_n.png)\n")

    # 2. Графики ускорения для каждого N
    # Ускорение S = T_serial / T_parallel, где T_serial - время при 1 потоке
    # Для каждого N построим график speedup vs threads
    for n in ns:
        T_serial = results[(n, 1)][0]
        speedups = []
        for tcount in thread_list:
            T_parallel = results[(n, tcount)][0]
            S = T_serial / T_parallel
            speedups.append(S)
        
        plt.figure(figsize=(8,6))
        plt.plot(thread_list, speedups, marker='o')
        plt.xlabel("Number of threads")
        plt.ylabel("Speedup (S)")
        plt.title(f"Speedup for N={n}")
        plt.grid(True)
        fig_name = f"speedup_n_{n}.png"
        plt.savefig(fig_name, dpi=150)
        plt.close()

        # Добавим ссылку на график в md
        with open(md_filename, "a") as f:
            f.write(f"\n## Speedup for N={n}\n")
            f.write(f"![Speedup for N={n}]({fig_name})\n")

    print(f"Benchmark complete. Results and graphs saved. See {md_filename}.")
