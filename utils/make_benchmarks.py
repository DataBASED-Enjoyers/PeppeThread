import subprocess
import time
import numpy as np
import os
import matplotlib.pyplot as plt
from generate_n_points import generate_bodies
from tqdm import tqdm

def run_program(num_threads, exec_path="src/task1/n_body_cuda"):
    result = subprocess.run([exec_path, str(num_threads)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True, text=True)
    output = result.stdout.strip().split('\n')
    cpu_time = None
    gpu_time = None
    for line in output:
        line = line.strip()
        if line.startswith("Total computation time:"):
            parts = line.split(":")[1].strip().split()
            cpu_time = float(parts[0])
        elif line.startswith("Total GPU time accumulated:"):
            parts = line.split(":")[1].strip().split()
            gpu_time = float(parts[0])

    if cpu_time is None or gpu_time is None:
        raise RuntimeError("Не удалось прочитать общее время/время GPU из вывода программы")

    return cpu_time, gpu_time

if __name__ == "__main__":
    ns = [100, 200, 500, 1000, 2500]
    thread_list = [1, 4, 16, 64, 256, 1024]
    repetitions = 5

    benchmark_dir = "src/task1/benchmarks"
    if not os.path.exists(benchmark_dir):
        os.makedirs(benchmark_dir)

    task1_dir = os.getcwd() + "/src/task1"
    input_path = f'{task1_dir}/input.txt'
    results = {}

    for n in ns:
        generate_bodies(n, input_path)
        for tcount in thread_list:
            cpu_times = []
            gpu_times = []
            for _ in tqdm(range(repetitions), desc=f"N={n}, threads={tcount}", leave=True):
                c_t, g_t = run_program(tcount)
                cpu_times.append(c_t)
                gpu_times.append(g_t)
            results[(n, tcount)] = {
                "cpu_avg": np.mean(cpu_times),
                "cpu_std": np.std(cpu_times),
                "gpu_avg": np.mean(gpu_times),
                "gpu_std": np.std(gpu_times)
            }
    md_filename = os.path.join(benchmark_dir, "benchmark_results.md")
    with open(md_filename, "w") as f:
        f.write("# Benchmark Results\n\n")
        f.write("## Summary\n")
        f.write("Ниже приводится таблица результатов с разными N и количеством потоков:\n\n")

        f.write("| N | threads | avg_time (s) | avg_time_CUDA (s) | Ускорение, S |\n")
        f.write("|---|----------|-----------------|-------------------|-----------|\n")
        for n in ns:
            T_serial = results[(n, 1)]["cpu_avg"]  # Время при 1 потоке
            for tcount in thread_list:
                cpu_avg = results[(n, tcount)]["cpu_avg"]
                gpu_avg = results[(n, tcount)]["gpu_avg"]
                S = T_serial / cpu_avg
                f.write(f"| {n} | {tcount} | {cpu_avg:.6f} | {gpu_avg:.6f} | {S:.2f} |\n")

    plt.figure(figsize=(8, 6))
    for tcount in thread_list:
        avg_times_gpu = [results[(n, tcount)]["gpu_avg"] for n in ns]
        plt.plot(ns, avg_times_gpu, marker='o', label=f"{tcount} threads")
    plt.xlabel("Number of bodies (N)")
    plt.ylabel("Average CUDA Time (s)")
    plt.title("GPU Performance scaling with number of bodies")
    plt.legend()
    plt.grid(True)
    plt.xscale("log")
    plt.xticks(ns, labels=ns)
    time_vs_n_gpu_png = os.path.join(benchmark_dir, "time_vs_n_gpu.png")
    plt.savefig(time_vs_n_gpu_png, dpi=150)
    plt.close()

    for n in ns:
        T_serial = results[(n, 1)]["gpu_avg"]
        speedups = []
        for tcount in thread_list:
            T_parallel = results[(n, tcount)]["gpu_avg"]
            S = T_serial / T_parallel
            speedups.append(S)

        plt.figure(figsize=(8, 6))
        plt.plot(thread_list, speedups, marker='o')
        plt.xlabel("Number of threads")
        plt.ylabel("Speedup (S)")
        plt.title(f"Speedup for N={n}")
        plt.grid(True)
        plt.xscale("log")
        plt.xticks(thread_list, labels=thread_list)
        fig_name = os.path.join(benchmark_dir, f"speedup_n_{n}.png")
        plt.savefig(fig_name, dpi=150)
        plt.close()

    print(f"Benchmark complete. Results and graphs saved in {benchmark_dir}. See {md_filename}.")
