#include <iostream>
#include <cuda_runtime.h>

int main() {
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    for (int dev = 0; dev < deviceCount; dev++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, dev);
        std::cout << "Device " << dev << ": " << prop.name << "\n";
        std::cout << "Max threads per block: " << prop.maxThreadsPerBlock << "\n";
    }
    return 0;
}
