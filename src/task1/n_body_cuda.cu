#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>
#include <cuda_runtime.h>

#define G 6.67430e-11 // гравитационная постоянная

#define checkCudaErrors(val) CheckCuda((val), #val, __FILE__, __LINE__)
inline void CheckCuda(cudaError_t result, char const *const func, const char *const file, int const line) {
    if (result != cudaSuccess) {
        std::cerr << "CUDA error at " << file << ":" << line << " code=" << (int)result << " \"" << func << "\" " 
                  << cudaGetErrorString(result) << std::endl;
        exit(1);
    }
}

struct Body {
    double x, y;      // Координаты
    double vx, vy;    // Скорости
    double mass;      // Масса
};

__global__ void computeForces(Body* bodies, double* Fx, double* Fy, int n) {
    extern __shared__ Body sharedBodies[];

    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    double ix = bodies[i].x;
    double iy = bodies[i].y;
    double imass = bodies[i].mass;

    double forceX = 0.0;
    double forceY = 0.0;

    int tiles = (n + blockDim.x - 1) / blockDim.x;

    for (int tile = 0; tile < tiles; tile++) {
        int idx = tile * blockDim.x + threadIdx.x;
        if (idx < n) {
            sharedBodies[threadIdx.x] = bodies[idx];
        }
        __syncthreads();

        int count = (tile == tiles - 1) ? (n - tile * blockDim.x) : blockDim.x;
        for (int j = 0; j < count; j++) {
            int actual_j = tile * blockDim.x + j;
            if (actual_j != i) {
                double dx = sharedBodies[j].x - ix;
                double dy = sharedBodies[j].y - iy;
                double distSqr = dx * dx + dy * dy + 1e-10;
                double invDist = rsqrt(distSqr);
                double invDist3 = invDist * invDist * invDist;
                double F = G * imass * sharedBodies[j].mass * invDist3;

                forceX += F * dx;
                forceY += F * dy;
            }
        }
        __syncthreads();
    }

    Fx[i] = forceX;
    Fy[i] = forceY;
}


__global__ void updateBodies(Body* bodies, double* Fx, double* Fy, int n, double dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    double imass = bodies[i].mass;
    bodies[i].vx += (Fx[i] / imass) * dt;
    bodies[i].vy += (Fy[i] / imass) * dt;

    bodies[i].x += bodies[i].vx * dt;
    bodies[i].y += bodies[i].vy * dt;
}

int main(int argc, char* argv[]) {
    int n;
    double dt = 1e-3;
    int steps = 10000; // например, 10 тыс. шагов

    std::string input_filename = "src/task1/input.txt";
    std::string output_filename = "src/task1/output.csv";
    std::string trajectory_filename = "src/task1/trajectory.csv";

    int blockSize = 256; // дефолтный размер CUDA-блока
    if (argc > 1) {
        blockSize = atoi(argv[1]);
    }

    bool makeTrajectories = false;
    if (argc > 2) {
        if (atoi(argv[2]) == 1) {
            makeTrajectories = true;
        }
    }

    // Читаем входные данные
    std::ifstream input(input_filename);
    input >> n;
    std::vector<Body> h_bodies(n);
    for (int i = 0; i < n; i++) {
        input >> h_bodies[i].x >> h_bodies[i].y >> h_bodies[i].vx >> h_bodies[i].vy >> h_bodies[i].mass;
    }
    input.close();

    Body* d_bodies;
    double *d_Fx, *d_Fy;
    checkCudaErrors(cudaMalloc(&d_bodies, n * sizeof(Body)));
    checkCudaErrors(cudaMalloc(&d_Fx, n * sizeof(double)));
    checkCudaErrors(cudaMalloc(&d_Fy, n * sizeof(double)));
    checkCudaErrors(cudaMemcpy(d_bodies, h_bodies.data(), n * sizeof(Body), cudaMemcpyHostToDevice));

    int gridSize = (n + blockSize - 1) / blockSize;

    std::ofstream traj_out(trajectory_filename);
    if (makeTrajectories) {
        // Записываем заголовок
        traj_out << "t";
        for (int i = 0; i < n; i++) {
            traj_out << ",x_" << i << ",y_" << i << ",vx_" << i << ",vy_" << i;
        }
        traj_out << "\n";
    }

    double t = 0.0;

    if (makeTrajectories) {
        // Записываем начальное состояние
        traj_out << t;
        for (int i = 0; i < n; i++) {
            traj_out << "," << h_bodies[i].x << "," << h_bodies[i].y << "," << h_bodies[i].vx << "," << h_bodies[i].vy;
        }
        traj_out << "\n";
    }

    int output_interval = 100;

    // Создаем CUDA events для замеров GPU-времени
    cudaEvent_t startEvent, stopEvent;
    checkCudaErrors(cudaEventCreate(&startEvent));
    checkCudaErrors(cudaEventCreate(&stopEvent));

    float total_gpu_time_ms = 0.0f;

    // Начинаем замер CPU-времени
    auto start_time = std::chrono::high_resolution_clock::now();

    // Используем только blockSize для shared memory
    size_t sharedMemSize = blockSize * sizeof(Body);

    // Основной цикл
    for (int step = 1; step <= steps; step++) {
        // Запуск GPU замера
        checkCudaErrors(cudaEventRecord(startEvent, 0));

        computeForces<<<gridSize, blockSize, sharedMemSize>>>(d_bodies, d_Fx, d_Fy, n);
        updateBodies<<<gridSize, blockSize>>>(d_bodies, d_Fx, d_Fy, n, dt);

        checkCudaErrors(cudaPeekAtLastError());
        checkCudaErrors(cudaDeviceSynchronize());

        // Остановка GPU замера
        checkCudaErrors(cudaEventRecord(stopEvent, 0));
        checkCudaErrors(cudaEventSynchronize(stopEvent));
        float gpu_time_ms = 0.0f;
        checkCudaErrors(cudaEventElapsedTime(&gpu_time_ms, startEvent, stopEvent));
        total_gpu_time_ms += gpu_time_ms;

        t = step * dt;

        // Запись промежуточных состояний
        if (step % output_interval == 0 && makeTrajectories) {
            checkCudaErrors(cudaMemcpy(h_bodies.data(), d_bodies, n * sizeof(Body), cudaMemcpyDeviceToHost));
            traj_out << t;
            for (int i = 0; i < n; i++) {
                traj_out << "," << h_bodies[i].x << "," << h_bodies[i].y << "," << h_bodies[i].vx << "," << h_bodies[i].vy;
            }
            traj_out << "\n";
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;

    traj_out.close();

    // Копируем конечные результаты
    checkCudaErrors(cudaMemcpy(h_bodies.data(), d_bodies, n * sizeof(Body), cudaMemcpyDeviceToHost));

    std::ofstream output(output_filename);
    for (int i = 0; i < n; i++) {
        output << h_bodies[i].x << "," << h_bodies[i].y << ",";
    }
    output.close();

    // Освобождение памяти
    checkCudaErrors(cudaFree(d_bodies));
    checkCudaErrors(cudaFree(d_Fx));
    checkCudaErrors(cudaFree(d_Fy));

    // Уничтожаем events
    checkCudaErrors(cudaEventDestroy(startEvent));
    checkCudaErrors(cudaEventDestroy(stopEvent));

    // Выводим результаты
    std::cout << "Total computation time: " << diff.count() << " s\n";
    std::cout << "Total GPU time accumulated: " << (total_gpu_time_ms/1000.0) << " s\n";
    std::cout << "Average GPU time per step: " << (total_gpu_time_ms / steps) << " ms\n";

    return 0;
}
