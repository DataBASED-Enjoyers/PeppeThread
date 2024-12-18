#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>
#include <cuda_runtime.h>

#define G 6.67430e-11 // Гравитационная постоянная

struct Body {
    double x, y;      // Координаты
    double vx, vy;    // Скорости
    double mass;      // Масса
};

__global__ void computeForces(Body* bodies, double* Fx, double* Fy, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    double forceX = 0.0, forceY = 0.0;

    for (int j = 0; j < n; j++) {
        if (i != j) {
            double dx = bodies[j].x - bodies[i].x;
            double dy = bodies[j].y - bodies[i].y;
            double distSqr = dx * dx + dy * dy + 1e-10; // Исключаем деление на ноль
            double invDist = rsqrt(distSqr);
            double invDist3 = invDist * invDist * invDist;
            double F = G * bodies[i].mass * bodies[j].mass * invDist3;

            forceX += F * dx;
            forceY += F * dy;
        }
    }

    Fx[i] = forceX;
    Fy[i] = forceY;
}

__global__ void updateBodies(Body* bodies, double* Fx, double* Fy, int n, double dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    bodies[i].vx += (Fx[i] / bodies[i].mass) * dt;
    bodies[i].vy += (Fy[i] / bodies[i].mass) * dt;

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

    // Читаем входные данные (вне замера времени)
    std::ifstream input(input_filename);
    input >> n;
    std::vector<Body> h_bodies(n);
    for (int i = 0; i < n; i++) {
        input >> h_bodies[i].x >> h_bodies[i].y >> h_bodies[i].vx >> h_bodies[i].vy >> h_bodies[i].mass;
    }
    input.close();

    Body* d_bodies;
    double *d_Fx, *d_Fy;
    cudaMalloc(&d_bodies, n * sizeof(Body));
    cudaMalloc(&d_Fx, n * sizeof(double));
    cudaMalloc(&d_Fy, n * sizeof(double));
    cudaMemcpy(d_bodies, h_bodies.data(), n * sizeof(Body), cudaMemcpyHostToDevice);

    int gridSize = (n + blockSize - 1) / blockSize;

    std::ofstream traj_out(trajectory_filename);
    // Записываем заголовок
    traj_out << "t";
    for (int i = 0; i < n; i++) {
        traj_out << ",x_" << i << ",y_" << i << ",vx_" << i << ",vy_" << i;
    }
    traj_out << "\n";

    // Записываем начальное состояние при t=0 ДО замеров
    double t = 0.0;
    traj_out << t;
    for (int i = 0; i < n; i++) {
        traj_out << "," << h_bodies[i].x << "," << h_bodies[i].y << "," << h_bodies[i].vx << "," << h_bodies[i].vy;
    }
    traj_out << "\n";

    int output_interval = 100;

    // Создаем CUDA events для замеров GPU-времени
    cudaEvent_t startEvent, stopEvent;
    cudaEventCreate(&startEvent);
    cudaEventCreate(&stopEvent);

    float total_gpu_time_ms = 0.0f;

    // Начинаем замер CPU-времени для основного цикла
    auto start_time = std::chrono::high_resolution_clock::now();

    // Основной цикл времени
    for (int step = 1; step <= steps; step++) {
        // Запуск GPU замера
        cudaEventRecord(startEvent, 0);

        computeForces<<<gridSize, blockSize>>>(d_bodies, d_Fx, d_Fy, n);
        updateBodies<<<gridSize, blockSize>>>(d_bodies, d_Fx, d_Fy, n, dt);
        cudaDeviceSynchronize();

        // Остановка GPU замера
        cudaEventRecord(stopEvent, 0);
        cudaEventSynchronize(stopEvent);
        float gpu_time_ms = 0.0f;
        cudaEventElapsedTime(&gpu_time_ms, startEvent, stopEvent);
        total_gpu_time_ms += gpu_time_ms;

        t = step * dt;

        // Периодически копируем данные на хост и записываем их в траекторию
        if (step % output_interval == 0) {
            cudaMemcpy(h_bodies.data(), d_bodies, n * sizeof(Body), cudaMemcpyDeviceToHost);
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

    // Копируем конечные результаты и записываем в отдельный файл
    cudaMemcpy(h_bodies.data(), d_bodies, n * sizeof(Body), cudaMemcpyDeviceToHost);

    std::ofstream output(output_filename);
    for (int i = 0; i < n; i++) {
        output << h_bodies[i].x << "," << h_bodies[i].y << ",";
    }
    output.close();

    // Освобождение памяти
    cudaFree(d_bodies);
    cudaFree(d_Fx);
    cudaFree(d_Fy);

    // Уничтожаем events
    cudaEventDestroy(startEvent);
    cudaEventDestroy(stopEvent);

    // Выводим результаты
    std::cout << "Total computation time (CPU): " << diff.count() << " s\n";
    std::cout << "Total GPU time accumulated: " << (total_gpu_time_ms/1000.0) << " s\n";
    std::cout << "Average GPU time per step: " << (total_gpu_time_ms / steps) << " ms\n";

    return 0;
}
