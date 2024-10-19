import subprocess
import csv
import time
import re

# Параметры
n_runs = 1000        # Количество запусков программы
nthreads = 2         # Число потоков (укажите нужное значение)
ntrials = 100000    # Число испытаний на запуск (укажите нужное значение)

# Имя скомпилированной C-программы
program_name = './pi'  # Убедитесь, что путь к программе указан правильно

# CSV-файл для сохранения результатов
csv_file = 'execution_times_2_threads.csv'

# Списки для хранения результатов
execution_times = []
pi_estimates = []

for i in range(n_runs):
    # Запуск программы и захват вывода
    result = subprocess.run([program_name, str(nthreads), str(ntrials)], capture_output=True, text=True)
    
    # Извлечение времени выполнения и оценки π из вывода программы
    output = result.stdout

    # Используем регулярные выражения для поиска нужных значений
    time_match = re.search(r'Execution Time = ([\d\.]+) seconds', output)
    pi_match = re.search(r'Estimated π = ([\d\.]+)', output)

    if time_match and pi_match:
        exec_time = float(time_match.group(1))
        pi_estimate = float(pi_match.group(1))
    else:
        # Если парсинг не удался, можно пропустить этот запуск или обработать ошибку
        continue

    # Сохранение результатов
    execution_times.append(exec_time)
    pi_estimates.append(pi_estimate)
    
    # Вывод прогресса
    if (i+1) % 100 == 0:
        print(f'Выполнено {i+1} запусков из {n_runs}')

# Сохранение результатов в CSV-файл
with open(csv_file, 'w', newline='') as csvfile:
    csvwriter = csv.writer(csvfile)
    csvwriter.writerow(['Запуск', 'Время выполнения (с)', 'Оценка π'])
    for idx, (exec_time, pi_estimate) in enumerate(zip(execution_times, pi_estimates), 1):
        csvwriter.writerow([idx, exec_time, pi_estimate])

print(f'Результаты сохранены в файл {csv_file}')

average_time = sum(execution_times) / len(execution_times)
print(f'Среднее время выполнения: {average_time:.6f} секунд')
