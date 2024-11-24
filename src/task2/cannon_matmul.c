#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int rank, size;

    // Чтение размера матрицы из окружения
    int N = atoi(getenv("MAT_SIZE")); // Размер матрицы и вектора
    if (N <= 0) {
        if (rank == 0) {
            printf("Ошибка: неверный размер матрицы (проверьте переменную окружения MAT_SIZE).\n");
        }
        MPI_Finalize();
        return -1;
    }

    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Проверка на соответствие числа процессов
    if (N % size != 0) {
        if (rank == 0) {
            printf("Количество процессов должно быть кратно размеру матрицы.\n");
        }
        MPI_Finalize();
        return -1;
    }

    // Инициализация данных
    int *A = (int*)malloc(N * N * sizeof(int));  // Явное приведение типа
    int *B = (int*)malloc(N * sizeof(int));      // Явное приведение типа
    int *C = (int*)malloc(N * sizeof(int));      // Явное приведение типа

    // Блоки данных для текущего процесса
    int block_size = N / size;  // Блок матрицы для каждого процесса
    int *A_block = (int*)malloc(block_size * N * sizeof(int));  // Явное приведение типа
    int *C_block = (int*)malloc(block_size * sizeof(int));      // Явное приведение типа

    // Инициализация данных
    if (rank == 0) {
        // Заполнение матрицы A и вектора B
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i * N + j] = i * N + j + 1;
            }
            B[i] = i + 1;
        }
    }

    // Засекаем время до начала работы
    double start_time = MPI_Wtime();

    // Разделение матрицы A между процессами
    MPI_Scatter(A, block_size * N, MPI_INT, A_block, block_size * N, MPI_INT, 0, MPI_COMM_WORLD);

    // Распространение вектора B ко всем процессам
    MPI_Bcast(B, N, MPI_INT, 0, MPI_COMM_WORLD);

    // Умножение блоков матрицы A на вектор B
    for (int i = 0; i < block_size; i++) {
        C_block[i] = 0;
        for (int j = 0; j < N; j++) {
            C_block[i] += A_block[i * N + j] * B[j];
        }
    }

    // Собираем результаты в главный процесс
    MPI_Gather(C_block, block_size, MPI_INT, C, block_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Синхронизация всех процессов
    MPI_Barrier(MPI_COMM_WORLD);

    // Засекаем время после выполнения
    double end_time = MPI_Wtime();

    // Вывод результата в главном процессе
    if (rank == 0) {
        // printf("Результат умножения матрицы на вектор:\n");
        // for (int i = 0; i < N; i++) {
        //     printf("%d ", C[i]);
        // }
        // printf("\n");
        printf("Время выполнения: %.6f секунд\n", end_time - start_time);
    }

    // Завершение работы MPI
    free(A);
    free(B);
    free(C);
    free(A_block);
    free(C_block);

    MPI_Finalize();
    return 0;
}
