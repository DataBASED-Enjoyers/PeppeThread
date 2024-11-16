# PeppeMPI

## clang-format quick-start:

```bash
sudo apt install clang-format
find src -name "*.c" -exec clang-format -i --verbose {} +
clang-format -i include/*.h --verbose
```

## compile mpi:

```bash
sudo apt-get install libopenmpi-dev
mpic++ path_to_file.c -o path_to_object.o
```
