# PeppeThread

## Task1
[Task 1 report](https://github.com/DataBASED-Enjoyers/PeppeThread/blob/main/src/task1/report_task_1.md)

```bash
gcc -Iinclude src/task1/pi.c -o src/task1/pi.o
./src/task1/pi.o <nthreads> <ntrials>
```

## Task2
[Task 2 report](https://github.com/DataBASED-Enjoyers/PeppeThread/blob/main/src/task2/report_task_2.md)

```bash
gcc -Iinclude src/task2/mandelbrot.c -o src/task2/mandelbrot.o -lm
./src/task2/mandelbrot.o <nthreads> <npoints>
```

Plot graph for lab2

```bash
cd utils
python3 -m venv .venv
source .venv/bin/activate
pip3 install numpy pandas matplotlib
python3 -m main
```

## Task3
[Task 3 report](https://github.com/DataBASED-Enjoyers/PeppeThread/blob/main/src/task3/report_task_3.md)
* rw_lock original

```bash
gcc -Iinclude src/task3/pth_ll_rwl.c -o src/task3/pth_ll_rwl.o
./src/task3/pth_ll_rwl.o <thread_count>
```

* rw_lock implementation

```bash
gcc -Iinclude src/task3/ppt_ll_rwl.c src/task3/rw_lock.c -o src/task3/ppt_ll_rwl.o
./src/task3/ppt_ll_rwl.o <thread_count>
```

## clang-format

```bash
sudo apt install clang-format
find src -name "*.c" -exec clang-format -i --verbose {} +
clang-format -i include/*.h --verbose
```
