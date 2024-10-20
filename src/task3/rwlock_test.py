import subprocess
import csv
import re
import os


def start_benchmark(nthreads, test, program_name, n_runs = 1000):
    bench_folder = program_name.split('.')[1][1:]
    folder_name = f'{bench_folder}/benchmarks_{test[0]}_{test[1]}_{test[2]}_{test[3]}'

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

    file = open('tables.txt', mode='w')
    file.close()

    S, E = 0, 0

    # test = [num_of_keys, num_of_ops, search_rate, insert_rate]
    test_1 = [1024, 1024, 0.3, 0.2]
    test_2 = [2024, 555, 0.3, 0.2]
    test_3 = [555, 2024, 0.3, 0.2]
    test_4 = [1024, 1024, 0.5, 0.4]
    test_5 = [1024, 1024, 0.8, 0.1]
    test_6 = [1024, 1024, 0.1, 0.1]

    program_original_name = './pth_ll_rwl.o'
    program_peppe_name = './ppt_ll_rwl.o'

    for num_test, test in enumerate((test_1, test_2, test_3, test_4, test_5, test_6), start=1):

        for p in (program_original_name, program_peppe_name):
            print(f"{p:.^50}")
            S_list = []
            E_list = []
            time_list = []
            for n_thread in (1, 2, 8, 16, 20, 64):
                if n_thread == 1:
                    avg_time_thread_1 = start_benchmark(nthreads=n_thread, test=test, program_name=p)
                    S = '-'
                    E = '-'
                    time_list.append(avg_time_thread_1)
                else:
                    avg_time_thread_n = start_benchmark(nthreads=n_thread, test=test, program_name=p)
                    S = round(avg_time_thread_1 / avg_time_thread_n, 4)
                    E = round(S / n_thread, 4)
                    time_list.append(avg_time_thread_n)

                E_list.append(E)
                S_list.append(S)

            print(E_list)
            print(S_list)
            print(time_list)
            print('-' * 50)

            with open('tables.txt', 'a') as file:
                    table = (
                        f"| Количество потоков | Среднее время выполнения (`T_avg`), сек | Ускорение (`S`) | Эффективность (`E`) |\n",
                        f"|--------------------|-----------------------------------------|-----------------|---------------------|\n",
                        f"| 1 (последовательный алгоритм) | {time_list[0]:.6f} | {S_list[0]} | {E_list[0]} |\n",
                        f"| 2  | {time_list[1]:.6f} | {S_list[1]} | {E_list[1]} |\n",
                        f"| 8  | {time_list[2]:.6f} | {S_list[2]} | {E_list[2]} |\n",
                        f"| 16 | {time_list[3]:.6f} | {S_list[3]} | {E_list[3]} |\n",
                        f"| 20 | {time_list[4]:.6f} | {S_list[4]} | {E_list[4]} |\n",
                        f"| 64 | {time_list[5]:.6f} | {S_list[5]} | {E_list[5]} |\n"
                    )
                    file.write("".join(table))
                    file.write('\n\n')

        print(f"Done {num_test}/6")
