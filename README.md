# PeppeMPI

## clang-format quick-start

```shell
sudo apt install clang-format
find src -name "*.c" -exec clang-format -i --verbose {} +
clang-format -i include/*.h --verbose
```
