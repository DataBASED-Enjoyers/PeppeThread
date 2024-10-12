# PeppeThread

## clang-format

```bash
sudo apt install clang-format
clang-format -i src/*.c --verbose
clang-format -i src/*.h --verbose
```

## Lab1

```bash
gcc pi.c -o pi.o
./pi.o
```

## Lab2

```bash
gcc mandelbrot.c -o mandelbrot.o -lm
./mandelbrot.o
```

Plot graph for lab2

```bash
cd utils
python3 -m venv .venv
source .venv/bin/activate
pip install numpy pandas matplotlib
python3 -m main
```

## Lab3

```bash
gcc rw_lock.c -o wr_lock.o
./rw_lock.o
```
