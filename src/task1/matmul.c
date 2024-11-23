#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK_RESULTS_CORRECTNESS

void row_split_multiplication(int rank, int size, int n, int *matrix,
                              int *vector, int *result);
void column_split_multiplication(int rank, int size, int n, int *matrix,
                                 int *vector, int *result);
void block_split_multiplication(int rank, int size, int n, int *matrix,
                                int *vector, int *result);

// Умножение с разбиением по строкам
void row_split_multiplication(int rank, int size, int n, int *matrix,
                              int *vector, int *result) {
    int rows_per_process = n / size;
    int *local_matrix = (int *)malloc(rows_per_process * n * sizeof(int));
    int *local_result = (int *)malloc(rows_per_process * sizeof(int));

    MPI_Scatter(matrix, rows_per_process * n, MPI_INT, local_matrix,
                rows_per_process * n, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(vector, n, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i = 0; i < rows_per_process; i++) {
        local_result[i] = 0;
        for (int j = 0; j < n; j++) {
            local_result[i] += local_matrix[i * n + j] * vector[j];
        }
    }

    MPI_Gather(local_result, rows_per_process, MPI_INT, result,
               rows_per_process, MPI_INT, 0, MPI_COMM_WORLD);

    free(local_matrix);
    free(local_result);
}

// Умножение с разбиением по столбцам
void column_split_multiplication(int rank, int size, int n, int *matrix,
                                 int *vector, int *result) {
    int cols_per_process = n / size;
    int *local_matrix = (int *)malloc(n * cols_per_process * sizeof(int));
    int *local_result = (int *)malloc(n * sizeof(int));

    MPI_Scatter(matrix, n * cols_per_process, MPI_INT, local_matrix,
                n * cols_per_process, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(vector, n, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i = 0; i < n; i++) {
        local_result[i] = 0;
        for (int j = 0; j < cols_per_process; j++) {
            local_result[i] += local_matrix[i * cols_per_process + j] * vector[j + rank * cols_per_process];
        }
    }

    MPI_Reduce(local_result, result, n, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    free(local_matrix);
    free(local_result);
}

// Умножение с разбиением по блокам
void block_split_multiplication(int rank, int size, int n, int *matrix,
                                int *vector, int *result) {
    int block_size = n / size;
    int *local_matrix = (int *)malloc(block_size * n * sizeof(int));
    int *local_result = (int *)malloc(block_size * sizeof(int));

    // Разделяем матрицу по строкам для каждого блока
    MPI_Scatter(matrix, block_size * n, MPI_INT, local_matrix,
                block_size * n, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(vector, n, MPI_INT, 0, MPI_COMM_WORLD);

    // Умножаем блок на вектор
    for (int i = 0; i < block_size; i++) {
        local_result[i] = 0;
        for (int j = 0; j < n; j++) {
            local_result[i] += local_matrix[i * n + j] * vector[j];
        }
    }

    // Собираем результаты
    MPI_Gather(local_result, block_size, MPI_INT, result,
               block_size, MPI_INT, 0, MPI_COMM_WORLD);

    free(local_matrix);
    free(local_result);
}

// Проверка корректности результатов
int compare_results(int n, int *result1, int *result2) {
    for (int i = 0; i < n; i++) {
        if (result1[i] != result2[i]) {
            return 0; // Результаты отличаются
        }
    }
    return 1; // Результаты совпадают
}

// Последовательное умножение матрицы на вектор
void sequential_multiplication(int n, int *matrix, int *vector, int *result) {
    for (int i = 0; i < n; i++) {
        result[i] = 0;
        for (int j = 0; j < n; j++) {
            result[i] += matrix[i * n + j] * vector[j];
        }
    }
}

#ifdef CHECK_RESULTS_CORRECTNESS
int main(int argc, char *argv[]) {
    int n = atoi(getenv("MAT_SIZE")); // Размер матрицы и вектора
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int *matrix = NULL;
    int *vector = (int *)malloc(n * sizeof(int));
    int *result = (rank == 0) ? (int *)malloc(n * sizeof(int)) : NULL;
    int *sequential_result = (rank == 0) ? (int *)malloc(n * sizeof(int)) : NULL;

    if (rank == 0) {
        matrix = (int *)malloc(n * n * sizeof(int));
        for (int i = 0; i < n; i++) {
            vector[i] = 1; // Заполняем вектор значениями
            for (int j = 0; j < n; j++) {
                matrix[i * n + j] = i + j + 1; // Заполняем матрицу
            }
        }
        // Выполняем последовательное умножение
        sequential_multiplication(n, matrix, vector, sequential_result);
    }

    double start, end;

    // Проверка для умножения с разбиением по строкам
    start = MPI_Wtime();
    row_split_multiplication(rank, size, n, matrix, vector, result);
    end = MPI_Wtime();
    if (rank == 0) {
        printf("Row-split time: %f seconds\n", end - start);
        if (compare_results(n, result, sequential_result)) {
            printf("Row-split multiplication is correct.\n");
        } else {
            printf("Row-split multiplication is incorrect.\n");
        }
    }

    // Проверка для умножения с разбиением по столбцам
    start = MPI_Wtime();
    column_split_multiplication(rank, size, n, matrix, vector, result);
    end = MPI_Wtime();
    if (rank == 0) {
        printf("Column-split time: %f seconds\n", end - start);
        if (compare_results(n, result, sequential_result)) {
            printf("Column-split multiplication is correct.\n");
        } else {
            printf("Column-split multiplication is incorrect.\n");
        }
    }

    // Проверка для умножения с разбиением по блокам
    start = MPI_Wtime();
    block_split_multiplication(rank, size, n, matrix, vector, result);
    end = MPI_Wtime();
    if (rank == 0) {
        printf("Block-split time: %f seconds\n", end - start);
        if (compare_results(n, result, sequential_result)) {
            printf("Block-split multiplication is correct.\n");
        } else {
            printf("Block-split multiplication is incorrect.\n");
        }
    }

    if (rank == 0) {
        free(matrix);
        free(result);
        free(sequential_result);
    }
    free(vector);

    MPI_Finalize();
    return 0;
}
#endif
