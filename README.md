# PeppeMPI

## Task1 start

```bash
mpic++ src/task1/matmul.c -o src/task1/matmul.o
mpirun -np 4 src/task1/matmul.o
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