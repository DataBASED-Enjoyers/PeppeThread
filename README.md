# PeppeMPI

## Выполнили

* Красильников Николай Владимирович (21 ПМИ-1)
* Еременко Данил Тимофеевич (21 ПМИ-1)
* Кудасов Максим Игоревич (21 ПМИ-2)

---

CUDA setup:

* Check nvcc

```bash
nvcc --version
```

If you get no version then write in config:

```bash
export CUDA_HOME=/usr/local/cuda
export PATH=${CUDA_HOME}/bin:${PATH}
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
export CUDACXX=/usr/local/cuda/bin/nvcc
```

## Task 1

[Task 1 Report](https://github.com/DataBASED-Enjoyers/PeppeThread/blob/lab2/src/task1/report_task_1.md)

### Example

To run app with `n_points` and `n_threads` use the code below: 
```bash
python utils/generate_n_points.py <n_points>
nvcc src/task1/n_body_cuda.cu -o src/task1/n_body_cuda
./src/task1/n_body_cuda <n_threads>
```

If you want to run with checking the results:
```bash
python utils/generate_n_points.py <n_points>
nvcc src/task1/n_body_cuda.cu -o src/task1/n_body_cuda
./src/task1/n_body_cuda <n_threads>
python utils/check_n_body_correctness.py
```

Run benchmarks:
```bash
nvcc src/task1/n_body_cuda.cu -o src/task1/n_body_cuda
python utils/make_benchmarks.py
```

All files will be generated in directory `src/task1/benchmarks`.

---

## Task 2

[Task 2 Report](https://github.com/DataBASED-Enjoyers/PeppeThread/blob/lab2/src/task2/report_task_2.md)

---

## Utilities

### clang-format

```bash
sudo apt install clang-format
find src -name "*.c" -exec clang-format -i --verbose {} +
clang-format -i include/*.h --verbose
```
