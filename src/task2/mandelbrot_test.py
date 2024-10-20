import subprocess
import csv
import os
import re

def start_benchmark(nthreads, n_runs=10, avg_time_thread_1_dict=None):
    program_name = './mandelbrot.o'
    folder_name = 'benchmarks'
    npoints_list = [500, 1000, 2000]

    os.makedirs(folder_name, exist_ok=True)
    csv_file = f'{folder_name}/execution_times_{nthreads}_threads.csv'

    with open(csv_file, 'w', newline='') as csvfile:
        csvwriter = csv.writer(csvfile)
        csvwriter.writerow(['npoints', 'nthreads', 'Среднее время (с)', 'Ускорение (S)', 'Эффективность (E)'])

        for npoints in npoints_list:
            execution_times = []
            for i in range(n_runs):
                result = subprocess.run(
                    [program_name, str(nthreads), str(npoints)],
                    capture_output=True,
                    text=True
                )
                output = result.stdout
                time_match = re.search(r'Execution Time = ([\d\.]+) seconds', output)

                if time_match:
                    exec_time = float(time_match.group(1))
                else:
                    continue
                execution_times.append(exec_time)

                if (i+1) % 100 == 0:
                    print(f'Выполнено {i+1} запусков из {n_runs}')

            if execution_times:
                average_time = sum(execution_times) / len(execution_times)
            else:
                average_time = None

            if nthreads == 1:
                avg_time_thread_1_dict[npoints] = average_time
                S = '-'
                E = '-'
            else:
                T1 = avg_time_thread_1_dict.get(npoints)
                if T1 and average_time:
                    S = T1 / average_time
                    E = S / nthreads
                else:
                    S = '-'
                    E = '-'

            csvwriter.writerow([npoints, nthreads, average_time, S, E])

            print(f'npoints={npoints}, nthreads={nthreads}, среднее время={average_time:.6f} секунд, S={S}, E={E}')

    print(f'Результаты сохранены в файл {csv_file}')

if __name__ == '__main__':
    avg_time_thread_1_dict = {}

    for n_thread in (1, 2, 8, 20, 64):
        start_benchmark(nthreads=n_thread, avg_time_thread_1_dict=avg_time_thread_1_dict)
        print('-' * 50)
