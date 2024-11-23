#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define IDX(i, j, n) ((i) * (n) + (j)) // Индекс для 2D матрицы

void initialize_matrices(int *A, int *B, int *C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A[IDX(i, j, n)] = 1;
            B[IDX(i, j, n)] = 1;
            C[IDX(i, j, n)] = 0;
        }
    }
}

void print_matrix(const char *label, int *matrix, int n) {
    printf("%s:\n", label);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", matrix[IDX(i, j, n)]);
        }
        printf("\n");
    }
}

void distribute_blocks(int *A, int *B, int *A_local, int *B_local, int n, int block_size, int rank, int size) {
    int q = sqrt(size); // Размерность решётки процессов

    // Тип данных для блоков
    MPI_Datatype block_type;
    MPI_Type_vector(block_size, block_size, n, MPI_INT, &block_type);
    MPI_Type_create_resized(block_type, 0, sizeof(int), &block_type);
    MPI_Type_commit(&block_type);

    int *sendcounts = NULL, *displs = NULL;
    if (rank == 0) {
        sendcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));
        for (int i = 0; i < q; i++) {
            for (int j = 0; j < q; j++) {
                int idx = i * q + j;
                sendcounts[idx] = 1;
                displs[idx] = IDX(i * block_size, j * block_size, n);
            }
        }
    }

    // Разбрасывание блоков матрицы A и B
    MPI_Scatterv(A, sendcounts, displs, block_type, A_local,
                 block_size * block_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(B, sendcounts, displs, block_type, B_local,
                 block_size * block_size, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        free(sendcounts);
        free(displs);
    }

    MPI_Type_free(&block_type);

    // Вывод локальных блоков для отладки
    printf("Process %d: Local A block:\n", rank);
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%d ", A_local[IDX(i, j, block_size)]);
        }
        printf("\n");
    }

    printf("Process %d: Local B block:\n", rank);
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%d ", B_local[IDX(i, j, block_size)]);
        }
        printf("\n");
    }
}

void matrix_multiply(int *A_local, int *B_local, int *C_local, int block_size) {
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            for (int k = 0; k < block_size; k++) {
                C_local[IDX(i, j, block_size)] +=
                    A_local[IDX(i, k, block_size)] * B_local[IDX(k, j, block_size)];
            }
        }
    }

    // Вывод результатов умножения для отладки
    printf("C_local after multiplication:\n");
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            printf("%d ", C_local[IDX(i, j, block_size)]);
        }
        printf("\n");
    }
}

void gather_results(int *C_local, int *C, int n, int block_size, int rank, int size) {
    int q = sqrt(size);
    int *recvcounts = NULL, *recvdispls = NULL;

    if (rank == 0) {
        recvcounts = (int *)malloc(size * sizeof(int));
        recvdispls = (int *)malloc(size * sizeof(int));
        for (int i = 0; i < q; i++) {
            for (int j = 0; j < q; j++) {
                int idx = i * q + j;
                recvcounts[idx] = block_size * block_size;  // Убедитесь, что count правильный
                recvdispls[idx] = IDX(i * block_size, j * block_size, n);
            }
        }
    }

    // Вывод recvcounts и recvdispls для отладки
    if (rank == 0) {
        printf("recvcounts:\n");
        for (int i = 0; i < size; i++) {
            printf("%d ", recvcounts[i]);
        }
        printf("\nrecvdispls:\n");
        for (int i = 0; i < size; i++) {
            printf("%d ", recvdispls[i]);
        }
        printf("\n");
    }

    // Используем recvcounts и recvdispls для правильного сбора блоков
    MPI_Gatherv(C_local, block_size * block_size, MPI_INT, C,
                recvcounts, recvdispls, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        free(recvcounts);
        free(recvdispls);
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = 4; // Размер матрицы (например, 4x4)
    int q = sqrt(size); // Размерность решётки процессов

    if (q * q != size) {
        if (rank == 0) {
            fprintf(stderr, "Number of processes must be a perfect square.\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (n % q != 0) {
        if (rank == 0) {
            fprintf(stderr, "Matrix size must be divisible by the grid size.\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int block_size = n / q; // Размер блока для каждого процесса

    // Локальные блоки
    int *A_local = (int *)malloc(block_size * block_size * sizeof(int));
    int *B_local = (int *)malloc(block_size * block_size * sizeof(int));
    int *C_local = (int *)calloc(block_size * block_size, sizeof(int));

    // Глобальные матрицы
    int *A = NULL, *B = NULL, *C = NULL;
    if (rank == 0) {
        A = (int *)malloc(n * n * sizeof(int));
        B = (int *)malloc(n * n * sizeof(int));
        C = (int *)malloc(n * n * sizeof(int));
        initialize_matrices(A, B, C, n);
    }

    // Замер времени
    double start_time = MPI_Wtime();

    // Распределение блоков матриц
    distribute_blocks(A, B, A_local, B_local, n, block_size, rank, size);

    // Умножение локальных блоков
    matrix_multiply(A_local, B_local, C_local, block_size);

    // Сборка результата в глобальную матрицу
    gather_results(C_local, C, n, block_size, rank, size);

    // Замер времени
    double end_time = MPI_Wtime();
    if (rank == 0) {
        printf("Time taken for matrix multiplication: %f seconds\n", end_time - start_time);
        print_matrix("Result matrix", C, n);
        free(A);
        free(B);
        free(C);
    }

    free(A_local);
    free(B_local);
    free(C_local);

    MPI_Finalize();

    return 0;
}
