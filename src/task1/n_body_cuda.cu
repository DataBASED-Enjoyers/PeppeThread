#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

#define G 6.67430e-11 // Гравитационная постоянная
#define BLOCK_SIZE 512 // Размер блока CUDA

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

// CUDA ядро для обновления позиций и скоростей методом Эйлера
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
    double dt = 1e-4;
    int steps = 10000;

    std::ifstream input("input.txt");
    input >> n;

    // Выделение памяти для тел
    std::vector<Body> h_bodies(n);
    for (int i = 0; i < n; i++) {
        input >> h_bodies[i].x >> h_bodies[i].y >> h_bodies[i].vx >> h_bodies[i].vy >> h_bodies[i].mass;
    }
    input.close();

    Body* d_bodies;
    double *d_Fx, *d_Fy;

    // Выделение памяти на GPU
    cudaMalloc(&d_bodies, n * sizeof(Body));
    cudaMalloc(&d_Fx, n * sizeof(double));
    cudaMalloc(&d_Fy, n * sizeof(double));

    // Копирование данных на GPU
    cudaMemcpy(d_bodies, h_bodies.data(), n * sizeof(Body), cudaMemcpyHostToDevice);

    int gridSize = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Основной цикл времени
    for (int step = 0; step < steps; step++) {
        computeForces<<<gridSize, BLOCK_SIZE>>>(d_bodies, d_Fx, d_Fy, n);
        updateBodies<<<gridSize, BLOCK_SIZE>>>(d_bodies, d_Fx, d_Fy, n, dt);
        cudaDeviceSynchronize();
    }

    // Копирование результатов обратно на CPU
    cudaMemcpy(h_bodies.data(), d_bodies, n * sizeof(Body), cudaMemcpyDeviceToHost);

    // Запись результатов в файл
    std::ofstream output("output.csv");
    for (int i = 0; i < n; i++) {
        output << h_bodies[i].x << "," << h_bodies[i].y << ",";
    }
    output.close();

    // Освобождение памяти
    cudaFree(d_bodies);
    cudaFree(d_Fx);
    cudaFree(d_Fy);

    return 0;
}
