# PeppeMPI

## Выполнили:
* Красильников Николай Владимирович (21 ПМИ-1)
* Еременко Данил Тимофеевич (21 ПМИ-1)
* Кудасов Максим Игоревич (21 ПМИ-2)

---

## Preparations

Install and activate python virtual environment.
```bash
cd utils
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
cd ..
```

## Task 1

[Task 1 Report](https://github.com/DataBASED-Enjoyers/PeppeThread/blob/lab2/src/task1/report_task_1.md)

Default template for running these apps:
```bash
mpirun -np <num_of_threads> <path_to_compiled_file> <matrix_size>
```

Row-split multiplication.
```bash
mpic++ src/task1/matmul_row.c -o src/task1/matmul_row.o
mpirun -np 4 src/task1/matmul_row.o 576
```

Column-split multiplication.
```bash
mpic++ src/task1/matmul_col.c -o src/task1/matmul_col.o
mpirun -np 4 src/task1/matmul_col.o 576
```

Block-split multiplication.
```bash
mpic++ src/task1/matmul_chess.c -o src/task1/matmul_chess.o
mpirun -np 4 src/task1/matmul_chess.o 576
```

If you want to see debug info then build program with `-DVERBOSE` key.

Example:
```bash
mpic++ src/task1/matmul_row.c -o src/task1/matmul_row.o -DVERBOSE
mpirun -np 4 src/task1/matmul_row.o 576
```

Run program generating graph:
```bash
python3 utils/matmul_split_metrics.py
```

---

## Task 2

[Task 2 Report](https://github.com/DataBASED-Enjoyers/PeppeThread/blob/lab2/src/task2/report_task_2.md)

Default template to run this app:
```bash
export MAT_SIZE=<square_matrix_size>
mpirun -np <num_of_threads> <path_to_compiled_file>
```

Quick Start:
```bash
export MAT_SIZE=500
mpic++ /src/task2/cannon_matmul.c -o /src/task2/cannon_matmul.o
mpirun -np 4 /src/task2/cannon_matmul.o
```

Run program generating graph:
```bash
python3 utils/cannon_matmul_metrics.py
```

---

## Utilities

### clang-format

```bash
sudo apt install clang-format
find src -name "*.c" -exec clang-format -i --verbose {} +
clang-format -i include/*.h --verbose
```

### MPI start

```bash
sudo apt-get install libopenmpi-dev
mpic++ path_to_file.c -o path_to_object.o
mpirun -np 4 path_to_object.o
```
