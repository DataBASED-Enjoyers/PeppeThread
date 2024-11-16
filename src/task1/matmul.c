#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

void row_split_multiplication(int rank, int size, int n, int *matrix, int *vector, int *result);
void column_split_multiplication(int rank, int size, int n, int *matrix, int *vector, int *result);
void block_split_multiplication(int rank, int size, int n, int *matrix, int *vector, int *result);

void row_split_multiplication(int rank, int size, int n, int *matrix, int *vector, int *result) {
    int rows_per_process = n / size;
    int *local_matrix = (int*)malloc(rows_per_process * n * sizeof(int));
    int *local_result = (int*)malloc(rows_per_process * sizeof(int));

    MPI_Scatter(matrix, rows_per_process * n, MPI_INT, local_matrix, rows_per_process * n, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(vector, n, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i = 0; i < rows_per_process; i++) {
        local_result[i] = 0;
        for (int j = 0; j < n; j++) {
            local_result[i] += local_matrix[i * n + j] * vector[j];
        }
    }

    MPI_Gather(local_result, rows_per_process, MPI_INT, result, rows_per_process, MPI_INT, 0, MPI_COMM_WORLD);
    free(local_matrix);
    free(local_result);
}

void column_split_multiplication(int rank, int size, int n, int *matrix, int *vector, int *result) {
    int cols_per_process = n / size;
    int *local_matrix = (int*)malloc(n * cols_per_process * sizeof(int));
    int *local_vector = (int*)malloc(cols_per_process * sizeof(int));
    int *local_result = (int*)malloc(n * sizeof(int));

    MPI_Scatter(matrix, n * cols_per_process, MPI_INT, local_matrix, n * cols_per_process, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(vector, cols_per_process, MPI_INT, local_vector, cols_per_process, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i = 0; i < n; i++) {
        local_result[i] = 0;
        for (int j = 0; j < cols_per_process; j++) {
            local_result[i] += local_matrix[i * cols_per_process + j] * local_vector[j];
        }
    }

    MPI_Reduce(local_result, result, n, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    free(local_matrix);
    free(local_vector);
    free(local_result);
}

void block_split_multiplication(int rank, int size, int n, int *matrix, int *vector, int *result) {
    int block_size = n / size;
    int *local_matrix = (int*)malloc(block_size * block_size * sizeof(int));
    int *local_vector = (int*)malloc(block_size * sizeof(int));
    int *local_result = (int*)malloc(block_size * sizeof(int));

    MPI_Scatter(matrix, block_size * block_size, MPI_INT, local_matrix, block_size * block_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(vector, block_size, MPI_INT, local_vector, block_size, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i = 0; i < block_size; i++) {
        local_result[i] = 0;
        for (int j = 0; j < block_size; j++) {
            local_result[i] += local_matrix[i * block_size + j] * local_vector[j];
        }
    }

    MPI_Gather(local_result, block_size, MPI_INT, result, block_size, MPI_INT, 0, MPI_COMM_WORLD);
    free(local_matrix);
    free(local_vector);
    free(local_result);
}

int main(int argc, char *argv[]) {
    int n = 1024; // Размер матрицы и вектора
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int *matrix = NULL;
    int *vector = NULL;
    int *result = NULL;

    if (rank == 0) {
        matrix = (int*)malloc(n * n * sizeof(int));
        vector = (int*)malloc(n * sizeof(int));
        result = (int*)malloc(n * sizeof(int));

        for (int i = 0; i < n; i++) {
            vector[i] = 1;
            for (int j = 0; j < n; j++) {
                matrix[i * n + j] = 1;
            }
        }
    }

    double start, end;

    // Разбиение по строкам
    start = MPI_Wtime();
    row_split_multiplication(rank, size, n, matrix, vector, result);
    end = MPI_Wtime();
    if (rank == 0) {
        printf("Row-split time: %f seconds\n", end - start);
    }

    // Разбиение по столбцам
    start = MPI_Wtime();
    column_split_multiplication(rank, size, n, matrix, vector, result);
    end = MPI_Wtime();
    if (rank == 0) {
        printf("Column-split time: %f seconds\n", end - start);
    }

    // Разбиение по блокам
    start = MPI_Wtime();
    block_split_multiplication(rank, size, n, matrix, vector, result);
    end = MPI_Wtime();
    if (rank == 0) {
        printf("Block-split time: %f seconds\n", end - start);
    }

    if (rank == 0) {
        free(matrix);
        free(vector);
        free(result);
    }

    MPI_Finalize();
    return 0;
}
