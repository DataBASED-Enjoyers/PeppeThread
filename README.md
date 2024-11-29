# PeppeMPI

## Task1 start

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
```

## clang-format start

```bash
sudo apt install clang-format
find src -name "*.c" -exec clang-format -i --verbose {} +
clang-format -i include/*.h --verbose
```

## MPI start

```bash
sudo apt-get install libopenmpi-dev
mpic++ path_to_file.c -o path_to_object.o
mpirun -np 4 path_to_object.o
```

## Python utils start

```bash
cd utils
python3 -m venv .venv
source .venv/bin/activate
pip3 install numpy pandas matplotlib
```
