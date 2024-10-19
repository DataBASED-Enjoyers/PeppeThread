import subprocess
import csv
import re
import os


def start_benchmark(nthreads, n_runs = 1000, ntrials=10_000_000):
    program_name = './pi.o'
    folder_name = 'benchmarks'

    os.makedirs(folder_name, exist_ok=True)
    csv_file = f'{folder_name}/execution_times_{nthreads}_threads.csv'

    execution_times = []
    pi_estimates = []

    for i in range(n_runs):
        result = subprocess.run([program_name, str(nthreads), str(ntrials)], 
                                capture_output=True, text=True)
        output = result.stdout
        time_match = re.search(r'Execution Time = ([\d\.]+) seconds', output)
        pi_match = re.search(r'Estimated π = ([\d\.]+)', output)

        if time_match and pi_match:
            exec_time = float(time_match.group(1))
            pi_estimate = float(pi_match.group(1))
        else:
            continue
        execution_times.append(exec_time)
        pi_estimates.append(pi_estimate)
        
        if (i+1) % 100 == 0:
            print(f'Выполнено {i+1} запусков из {n_runs}')

    with open(csv_file, 'w', newline='') as csvfile:
        csvwriter = csv.writer(csvfile)
        csvwriter.writerow(['Запуск', 'Время выполнения (с)', 'Оценка π'])
        for idx, (exec_time, pi_estimate) in enumerate(zip(execution_times, pi_estimates), 1):
            csvwriter.writerow([idx, exec_time, pi_estimate])

    print(f'Результаты сохранены в файл {csv_file}')

    average_time = sum(execution_times) / len(execution_times)
    print(f'Среднее время выполнения: {average_time:.6f} секунд')
    return average_time


if __name__ == '__main__':
    avg_time_thread_1 = 0
    avg_time_thread_n = 0

    S, E = 0, 0

    for n_thread in (1, 2, 8, 16, 64):
        if n_thread == 1:
            avg_time_thread_1 = start_benchmark(nthreads=n_thread)
        else:
            avg_time_thread_n = start_benchmark(nthreads=n_thread)
            S = avg_time_thread_1 / avg_time_thread_n
            E = S / n_thread

            print(f"{E=} | {S=}")

        print('-' * 50)