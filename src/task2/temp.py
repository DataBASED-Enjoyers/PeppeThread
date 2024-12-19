import subprocess
import matplotlib.pyplot as plt
import numpy as np

# Функция для замера времени выполнения
def run_solver(points, processes):
    command = f"./src/task2/openMP_task.o {points} {processes}"
    try:
        result = subprocess.run(command.split(), capture_output=True, text=True, check=True)
        # Извлекаем время выполнения из вывода программы
        output = result.stdout
        for line in output.splitlines():
            if "Elapsed time:" in line:
                return float(line.split()[-2])  # Считываем время
    except subprocess.CalledProcessError as e:
        print(f"Error running the solver: {e}")
    return None

# Функция для многократного запуска и вычисления статистики
def get_statistics(points, processes, repeats=100):
    times = []
    for _ in range(repeats):
        time = run_solver(points, processes)
        if time is not None:
            times.append(time)
    mean_time = np.mean(times)
    std_dev_time = np.std(times)
    return mean_time, std_dev_time

# Параметры тестирования
point_counts = [10, 30, 100, 300]
process_counts = [1, 2, 3, 4]
repeats = 10  # Количество повторений

# Хранилище для результатов
results = {points: [] for points in point_counts}

# Запуск тестов
for points in point_counts:
    for processes in process_counts:
        print(f"Running {repeats} repetitions with {points} points and {processes} processes...")
        mean_time, std_dev_time = get_statistics(points, processes, repeats)
        results[points].append((processes, mean_time, std_dev_time))

# Построение графиков
for points, data in results.items():
    processes = [x[0] for x in data]
    mean_times = [x[1] for x in data]
    std_devs = [x[2] for x in data]
    base_time = mean_times[0]
    speedup = [base_time / t if t else 0 for t in mean_times]
    efficiency = [s / p for s, p in zip(speedup, processes)]

    # Создаём фигуру для текущего числа точек
    plt.figure(figsize=(12, 12))

    # График времени выполнения (логарифмическая шкала)
    plt.subplot(3, 1, 1)
    plt.plot(processes, mean_times, label=f"{points} points")
    plt.title(f"Execution Time (Log Scale) for {points} Points")
    plt.xlabel("Number of Processes")
    plt.ylabel("Time (s)")
    plt.legend()

    # График ускорения
    plt.subplot(3, 1, 2)
    plt.plot(processes, speedup, label="Speedup", marker='o')
    plt.title(f"Speedup for {points} Points")
    plt.xlabel("Number of Processes")
    plt.ylabel("Speedup")
    plt.legend()

    # График эффективности
    plt.subplot(3, 1, 3)
    plt.plot(processes, efficiency, label="Efficiency", marker='o')
    plt.title(f"Efficiency for {points} Points")
    plt.xlabel("Number of Processes")
    plt.ylabel("Efficiency")
    plt.legend()

    plt.tight_layout()
    # Сохраняем график
    plt.savefig(f"performance_metrics_{points}_points.png", dpi=300)
    plt.show()
