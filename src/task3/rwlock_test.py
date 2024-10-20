import subprocess
import csv
import re
import os


def start_benchmark(nthreads, test, n_runs = 1000):
    program_original_name = './pth_ll_rwl.o'
    program_peppe_name = './ppt_ll_rwl.o'
    program_name = program_peppe_name
    folder_name = f'benchmarks_{test[0]}_{test[1]}_{test[2]}_{test[3]}'

    os.makedirs(folder_name, exist_ok=True)
    csv_file = f'{folder_name}/execution_times_{nthreads}_threads.csv'

    execution_times = []

    input_data = '\n'.join(map(str, test)) + '\n'

    for i in range(n_runs):
        result = subprocess.run([program_name, str(nthreads)], 
                                capture_output=True, text=True,
                                input=input_data,
                                )
        output = result.stdout
        time_match = re.search(r'elapsed time = ([-+]?\d*\.?\d+([eE][-+]?\d+)?) seconds', output)

        if time_match:
            exec_time = float(time_match.group(1))
        else:
            print("No time!")
            print(output)
            break
        execution_times.append(exec_time)
        
        # if (i+1) % 100 == 0:
        #     print(f'Выполнено {i+1} запусков из {n_runs}')

    with open(csv_file, 'w', newline='') as csvfile:
        csvwriter = csv.writer(csvfile)
        csvwriter.writerow(['Запуск', 'Время выполнения (с)'])
        for idx, exec_time in enumerate(execution_times):
            csvwriter.writerow([idx, exec_time])

    print(f'Результаты сохранены в файл {csv_file}')

    average_time = sum(execution_times) / len(execution_times)
    print(f'Среднее время выполнения: {average_time:.6f} секунд')
    return average_time


if __name__ == '__main__':
    avg_time_thread_1 = 0
    avg_time_thread_n = 0

    S, E = 0, 0

    # test = [num_of_keys, num_of_ops, search_rate, insert_rate]
    test_1 = [1024, 1024, 0.3, 0.2]
    test_2 = [2024, 555, 0.3, 0.2]
    test_3 = [555, 2024, 0.3, 0.2]
    test_4 = [1024, 1024, 0.5, 0.4]
    test_5 = [1024, 1024, 0.8, 0.1]
    test_6 = [1024, 1024, 0.1, 0.1]

    for test in (test_1, test_2, test_3, test_4, test_5, test_6):
        for n_thread in (1, 2, 8, 16, 20, 64):
            if n_thread == 1:
                avg_time_thread_1 = start_benchmark(nthreads=n_thread, test=test)
            else:
                avg_time_thread_n = start_benchmark(nthreads=n_thread, test=test)
                S = avg_time_thread_1 / avg_time_thread_n
                E = S / n_thread

                print(f"{E=} | {S=}")

            print('-' * 50)