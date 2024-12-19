import subprocess
import matplotlib.pyplot as plt
import numpy as np
import os
from tqdm import tqdm

ROOT_DIR = os.getcwd()


def run_solver(points, processes):
    command = f"{ROOT_DIR}/src/task2/openMP_task.o {points} {processes}"
    try:
        result = subprocess.run(command.split(), capture_output=True, text=True, check=True)
        output = result.stdout
        for line in output.splitlines():
            if "Elapsed time:" in line:
                return float(line.split()[-2])
    except subprocess.CalledProcessError as e:
        print(f"Error running the solver: {e}")
    return None

def get_statistics(points, processes, repeats=100):
    times = []
    for _ in range(repeats):
        time = run_solver(points, processes)
        if time is not None:
            times.append(time)
    mean_time = np.mean(times)
    std_dev_time = np.std(times)
    return mean_time, std_dev_time

def save_markdown(results, filepath):
    with open(filepath, 'w') as md_file:
        md_file.write("# Benchmark Results\n\n")
        for points, data in results.items():
            md_file.write(f"## Results for {points} Points\n\n")
            md_file.write("| Processes | Mean Time (s) | Speedup | Efficiency |\n")
            md_file.write("|-----------|---------------|---------|------------|\n")
            base_time = data[0][1]  # Time for 1 process
            for processes, mean_time, _ in data:
                speedup = base_time / mean_time if mean_time else 0
                efficiency = speedup / processes if processes else 0
                md_file.write(f"| {processes} | {mean_time:.6f} | {speedup:.2f} | {efficiency:.2f} |\n")
            md_file.write("\n")

def main():
    os.makedirs(f"{ROOT_DIR}/src/task2/benchmarks", exist_ok=True)
    point_counts = [50, 100, 200, 300]
    process_counts = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    repeats = 10
    results = {points: [] for points in point_counts}
    
    # Run benchmarks
    for points in point_counts:
        for processes in tqdm(iterable=process_counts, 
                              total=len(process_counts),
                              desc=f'[Point cnt: {points}]'):
            mean_time, std_dev_time = get_statistics(points, processes, repeats)
            results[points].append((processes, mean_time, std_dev_time))

    # Save graphs
    for points, data in results.items():
        processes = [x[0] for x in data]
        mean_times = [x[1] for x in data]
        base_time = mean_times[0]
        speedup = [base_time / t if t else 0 for t in mean_times]
        efficiency = [s / p for s, p in zip(speedup, processes)]

        plt.figure(figsize=(12, 12))
        plt.subplot(3, 1, 1)
        plt.plot(processes, mean_times, label=f"{points} points", marker='o')
        plt.title(f"Execution Time for {points} Points")
        plt.xlabel("Number of Processes")
        plt.grid()
        plt.xticks(processes)
        plt.ylabel("Time (s)")
        plt.legend()

        plt.subplot(3, 1, 2)
        plt.plot(processes, speedup, label="Speedup", marker='o')
        plt.title(f"Speedup for {points} Points")
        plt.xticks(processes)
        plt.grid()
        plt.xlabel("Number of Processes")
        plt.ylabel("Speedup")
        plt.legend()

        plt.subplot(3, 1, 3)
        plt.plot(processes, efficiency, label="Efficiency", marker='o')
        plt.title(f"Efficiency for {points} Points")
        plt.xticks(processes)
        plt.grid()
        plt.xlabel("Number of Processes")
        plt.ylabel("Efficiency")
        plt.legend()

        plt.tight_layout()
        plt.savefig(f"{ROOT_DIR}/src/task2/benchmarks/performance_metrics_{points}_points.png", dpi=300)
    
    # Save Markdown report
    save_markdown(results, f"{ROOT_DIR}/src/task2/benchmarks/benchmark_results.md")

if __name__ == '__main__':
    main()
