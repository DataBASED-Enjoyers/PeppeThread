import subprocess
import matplotlib.pyplot as plt
import os
import tqdm
from markdown_table import MarkdownTable

ROOT_DIR = os.path.abspath('..')

def run_mpi_program(num_processes):
    result = {}
    output = subprocess.run(
        ["mpirun", "-np", str(num_processes), ROOT_DIR + "/src/task2/cannon_matmul.o"],
        capture_output=True, text=True
    )
    try:
        res = float(output.stdout.splitlines()[0].split()[-1])
        return res
    except ValueError:
        print(*output.stdout.splitlines())
        exit(0)

def get_S_E(t_1, t_n, n):
     S = round(t_1 / t_n, 4)
     E = round(S / n, 4)
     return S, E


def plot_matrix_performance(data, save_path):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    p_list = []
    s_list = []
    e_list = []
    n = data[0][0]

    for sublist in data:
        p_list.append(sublist[1])
        
        S, E = sublist[2], sublist[3]
        s_list.append(S)
        e_list.append(E)
    
    ax1.plot(p_list, s_list, marker='o', label='S')
    ax1.set_xticks(p_list)
    ax1.set_title(f'Скорость обработки матрицы {n}x{n}')
    ax1.set_xlabel('Количество процессов')
    ax1.set_ylabel('Скорость')
    ax1.legend()
    ax1.grid()
    
    ax2.plot(p_list, e_list, marker='o', label='E')
    ax2.set_xticks(p_list)
    ax2.set_title(f'Эффективность обработки матрицы {n}x{n}')
    ax2.set_xlabel('Количество процессов')
    ax2.set_ylabel('Эффективность')
    ax2.legend()
    ax2.grid()

    plt.tight_layout()
    plt.savefig(save_path)
    plt.close()


def main() -> None:
    matrix_sizes = [x for x in range(500, 3_000) if x % 4 == 0 and x % 9 == 0][::10]
    num_processes = [1, 4, 9]

    os.makedirs(f'{ROOT_DIR}/src/task2/task_2_benchmark', exist_ok=True)
    for size in matrix_sizes:
        data = []
        os.environ["MAT_SIZE"] = str(size)
        avg_time = None
        os.makedirs(f'{ROOT_DIR}/src/task2/task_2_benchmark/size={size}', exist_ok=True)
        table = MarkdownTable(
            alignment='center',
            headers=[
                'Размер матрицы **A**',
                'Количество процессов **P**',
                '**S**',
                '**E**']
        )
        for processes in tqdm.tqdm(num_processes, desc=f'[Size={os.environ.get("MAT_SIZE")}]'):
            result = run_mpi_program(processes)
            if processes == 1:
                S = '-'
                E = '-'
                avg_time = result
            else:
                 S, E = get_S_E(avg_time, result, processes)
                 data.append([size, processes, S, E])

            table.add_row([
                f"{size}x{size}", processes, S, E])
        
        table.save_to_file(f'{ROOT_DIR}/src/task2/task_2_benchmark/size={size}/table.md')
        plot_matrix_performance(data, f'{ROOT_DIR}/src/task2/task_2_benchmark/size={size}/plot.png')
        


if __name__ == '__main__':
    main()
