import subprocess
import matplotlib.pyplot as plt
import os
import tqdm
from markdown_table import MarkdownTable

ROOT_DIR = os.path.abspath('..')

def run_mpi_program(num_processes, matrix_size):
    result = {}
    try:
        lines = []
        for prog_name in ["matmul_row.o", "matmul_col.o", "matmul_chess.o"]:
            output = subprocess.run(
                ["mpirun", "-np", str(num_processes), ROOT_DIR + "/src/task1/" + prog_name, str(matrix_size)],
                capture_output=True, text=True
            )
            lines += output.stdout.splitlines()

        split_func = lambda x: x.split("time: ")[1].replace(' seconds', '')

        for line in lines:
            if "Row-split time" in line:
                result["row_split"] = float(split_func(line))
            elif "Column-split time" in line:
                result["col_split"] = float(split_func(line))
            elif "Block-split time" in line:
                result["block_split"] = float(split_func(line))
    except Exception as e:
        print(f"Error running MPI program with {num_processes} processes: {e}")

    return result

def get_S_E(t_1, t_n, n):
     S = round(t_1 / t_n, 4)
     E = round(S / n, 4)
     return S, E


def plot_matrix_performance(data, save_path):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    p_list = []
    s_row_list = []
    e_row_list = []
    s_col_list = []
    e_col_list = []
    s_block_list = []
    e_block_list = []
    n = data[0][0]

    for sublist in data:
        p_list.append(sublist[1])
        
        S_row, E_row = sublist[2]
        s_row_list.append(S_row)
        e_row_list.append(E_row)

        S_col, E_col = sublist[3]
        s_col_list.append(S_col)
        e_col_list.append(E_col)

        S_block, E_block = sublist[4]
        s_block_list.append(S_block)
        e_block_list.append(E_block)
    
    ax1.plot(p_list, s_row_list, marker='o', label='S_row')
    ax1.plot(p_list, s_col_list, marker='o', label='S_col')
    ax1.plot(p_list, s_block_list, marker='o', label='S_block')
    ax1.set_title(f'Скорость обработки матрицы {n}x{n}')
    ax1.set_xlabel('Количество процессов')
    ax1.set_ylabel('Скорость')
    ax1.legend()
    ax1.grid()
    
    ax2.plot(p_list, e_row_list, marker='o', label='E_row')
    ax2.plot(p_list, e_col_list, marker='o', label='E_col')
    ax2.plot(p_list, e_block_list, marker='o', label='E_block')
    ax2.set_title(f'Эффективность обработки матрицы {n}x{n}')
    ax2.set_xlabel('Количество процессов')
    ax2.set_ylabel('Эффективность')
    ax2.legend()
    ax2.grid()

    plt.tight_layout()
    plt.savefig(save_path)
    plt.close()

def main() -> None:
    matrix_sizes = [576, 2304, 3636]
    num_processes = [1, 4, 9]

    os.makedirs(f'{ROOT_DIR}/src/task1/task_1_benchmark', exist_ok=True)
    for size in matrix_sizes:
        data = []
        # os.environ["MAT_SIZE"] = str(size)
        avg_time_row_1 = None
        avg_time_col_1 = None
        avg_time_block_1 = None
        os.makedirs(f'{ROOT_DIR}/src/task1/task_1_benchmark/size={size}', exist_ok=True)
        table = MarkdownTable(
            alignment='center',
            headers=[
                'Размер матрицы **A**',
                'Количество процессов **P**',
                'Разбиение по строкам (S, E)',
                'Разбиение по столбцам (S, E)',
                'Разбиение на блоки (S, E)']
        )
        for processes in tqdm.tqdm(num_processes, desc=f'[Size={size}]'):
            result = run_mpi_program(processes, size)

            if processes == 1:
                S_row = E_row = '-'
                S_col = E_col = '-'
                S_block = E_block = '-'
                avg_time_row_1 = result['row_split']
                avg_time_col_1 = result['col_split']
                avg_time_block_1 = result['block_split']
            else:
                 S_row, E_row = get_S_E(avg_time_row_1, result['row_split'], processes)
                 S_col, E_col = get_S_E(avg_time_col_1, result['col_split'], processes)
                 S_block, E_block = get_S_E(avg_time_block_1, result['block_split'], processes)
                 data.append([size, processes, (S_row, E_row), (S_col, E_col), (S_block, E_block)])

            table.add_row([
                f"{size}x{size}", processes, 
                (S_row, E_row), (S_col, E_col), (S_block, E_block)])
        
        table.save_to_file(f'{ROOT_DIR}/src/task1/task_1_benchmark/size={size}/table.md')
        plot_matrix_performance(data, f'{ROOT_DIR}/src/task1/task_1_benchmark/size={size}/plot.png')
        


if __name__ == '__main__':
    main()
