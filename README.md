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
```

## Lab2

```bash
gcc mandelbrot.c -o mandelbrot.o -lm
```

Plot utils

```bash
cd utils
python3 -m venv .venv
source .venv/bin/activate
pip install numpy pandas matplotlib
python3 -m main
```
