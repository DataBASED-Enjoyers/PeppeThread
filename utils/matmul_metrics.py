import subprocess
import matplotlib.pyplot as plt

# Функция для запуска программы и сбора результатов
def run_mpi_program(num_processes):
    result = {}
    try:
        output = subprocess.run(
            ["mpirun", "-np", str(num_processes), "./matmul"],
            capture_output=True, text=True
        )
        lines = output.stdout.splitlines()
        for line in lines:
            if "Row-split time" in line:
                result["row_split"] = float(line.split(": ")[1])
            elif "Column-split time" in line:
                result["column_split"] = float(line.split(": ")[1])
            elif "Block-split time" in line:
                result["block_split"] = float(line.split(": ")[1])
        # Проверяем, все ли ключи присутствуют в результатах
        if "row_split" not in result:
            print(f"Warning: 'row_split' time missing for {num_processes} processes.")
        if "column_split" not in result:
            print(f"Warning: 'column_split' time missing for {num_processes} processes.")
        if "block_split" not in result:
            print(f"Warning: 'block_split' time missing for {num_processes} processes.")
    except Exception as e:
        print(f"Error running MPI program with {num_processes} processes: {e}")
    return result

# Параметры для экспериментов
matrix_sizes = [256, 512, 1024]  # Размеры матрицы
num_processes = [1, 2, 4, 8]     # Количество процессов

# Сбор данных
data = {}
for size in matrix_sizes:
    data[size] = {}
    for processes in num_processes:
        print(f"Running for matrix size {size} with {processes} processes...")
        result = run_mpi_program(processes)
        data[size][processes] = result

# Построение графиков
for size in matrix_sizes:
    plt.figure()
    plt.title(f"Execution Time for Matrix Size {size}")
    for method in ["row_split", "column_split", "block_split"]:
        times = [data[size][p].get(method, None) for p in num_processes]
        # Убираем значения None из графика
        filtered_processes = [p for p, time in zip(num_processes, times) if time is not None]
        filtered_times = [time for time in times if time is not None]
        if filtered_times:
            plt.plot(filtered_processes, filtered_times, label=method)
    plt.xlabel("Number of Processes")
    plt.ylabel("Execution Time (s)")
    plt.legend()
    plt.show()

# Опционально: Добавьте графики ускорения и эффективности
