# PeppeMPI

## Выполнили

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

CUDA setup:

* Check nvcc

```bash
nvcc --version
```

If you get no version then write in config:

```bash
export CUDA_HOME=/usr/local/cuda
export PATH=${CUDA_HOME}/bin:${PATH}   
export LD_LIBRARY_PATH=${CUDA_HOME}/lib64:$LD_LIBRARY_PATH
```

## Task 1

[Task 1 Report](https://github.com/DataBASED-Enjoyers/PeppeThread/blob/lab2/src/task1/report_task_1.md)

### Example

```bash
python utils/generate_n_points.py <n_points>
nvcc src/task1/n_body_cuda.cu -o src/task1/n_body_cuda
./src/task1/n_body_cuda
```

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
