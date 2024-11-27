#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

//
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
    int extra_rows = n % size;

    int start_row = rank * rows_per_process + (rank < extra_rows ? rank : extra_rows);
    int local_rows = rows_per_process + (rank < extra_rows ? 1 : 0);

    int *local_matrix = (int *)malloc(local_rows * n * sizeof(int));
    int *local_result = (int *)malloc(local_rows * sizeof(int));

    // Распределяем строки между процессами
    int *send_counts = NULL;
    int *displs = NULL;

    if (rank == 0) {
        send_counts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));

        for (int i = 0; i < size; i++) {
            send_counts[i] = (rows_per_process + (i < extra_rows ? 1 : 0)) * n;
            displs[i] = (i * rows_per_process + (i < extra_rows ? i : extra_rows)) * n;
        }
    }

    MPI_Scatterv(matrix, send_counts, displs, MPI_INT, local_matrix,
                 local_rows * n, MPI_INT, 0, MPI_COMM_WORLD);

    // Рассылаем вектор всем процессам
    MPI_Bcast(vector, n, MPI_INT, 0, MPI_COMM_WORLD);

    // Умножаем строки на вектор
    for (int i = 0; i < local_rows; i++) {
        local_result[i] = 0;
        for (int j = 0; j < n; j++) {
            local_result[i] += local_matrix[i * n + j] * vector[j];
        }
    }

    // Собираем результаты
    int *recv_counts = NULL;
    int *recv_displs = NULL;

    if (rank == 0) {
        recv_counts = (int *)malloc(size * sizeof(int));
        recv_displs = (int *)malloc(size * sizeof(int));

        for (int i = 0; i < size; i++) {
            recv_counts[i] = rows_per_process + (i < extra_rows ? 1 : 0);
            recv_displs[i] = i * rows_per_process + (i < extra_rows ? i : extra_rows);
        }
    }

    MPI_Gatherv(local_result, local_rows, MPI_INT, result, recv_counts, recv_displs, MPI_INT, 0, MPI_COMM_WORLD);

    free(local_matrix);
    free(local_result);
    if (rank == 0) {
        free(send_counts);
        free(displs);
        free(recv_counts);
        free(recv_displs);
    }
}

void column_split_multiplication(int rank, int size, int n, int *matrix, int *vector, int *result) {
    int cols_per_process = n / size;
    int remainder = n % size;
    int my_cols = (rank < remainder) ? cols_per_process + 1 : cols_per_process;

    // Allocate space for local matrix and results
    int *local_matrix = (int*)malloc(n * my_cols * sizeof(int));
    int *local_result = (int*)calloc(n, sizeof(int)); // Initialize to 0

    // Create sendcounts and displacements for MPI_Scatterv
    int *sendcounts = NULL;
    int *displs = NULL;
    if (rank == 0) {
        sendcounts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));
        int offset = 0;
        for (int i = 0; i < size; i++) {
            int cols = (i < remainder) ? cols_per_process + 1 : cols_per_process;
            sendcounts[i] = cols * n;
            displs[i] = offset;
            offset += sendcounts[i];
        }
    }

    // Scatter the matrix columns among processes
    MPI_Scatterv(matrix, sendcounts, displs, MPI_INT, local_matrix, my_cols * n, MPI_INT, 0, MPI_COMM_WORLD);

    // Broadcast the full vector to all processes
    MPI_Bcast(vector, n, MPI_INT, 0, MPI_COMM_WORLD);

    // Compute the local results (processes with no columns do nothing)
    if (my_cols > 0) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < my_cols; j++) {
                int global_col_idx = rank * cols_per_process + j + (rank < remainder ? rank : remainder);
                local_result[i] += local_matrix[i * my_cols + j] * vector[global_col_idx];
            }
        }
    }

    // Synchronize before reducing
    MPI_Barrier(MPI_COMM_WORLD);

    // Reduce the local results to the final result on the root process
    // MPI_Reduce(local_result, result, n, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Free allocated memory
    free(local_matrix);
    free(local_result);
    if (rank == 0) {
        free(sendcounts);
        free(displs);
    }
}





// Умножение с разбиением по блокам
void block_split_multiplication(int rank, int size, int n, int *matrix,
                                int *vector, int *result) {
    int rows_per_process = n / size;        // Количество строк на процесс
    int extra_rows = n % size;              // Остаток строк

    int local_rows = rows_per_process + (rank < extra_rows ? 1 : 0); // Локальное количество строк

    // Локальная матрица и результат
    int *local_matrix = (int *)malloc(local_rows * n * sizeof(int));
    int *local_result = (int *)calloc(local_rows, sizeof(int));

    int *send_counts = NULL;
    int *displs = NULL;

    if (rank == 0) {
        send_counts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));
        int offset = 0;
        for (int i = 0; i < size; i++) {
            int rows = rows_per_process + (i < extra_rows ? 1 : 0);
            send_counts[i] = rows * n;        // Количество элементов для процесса
            displs[i] = offset;               // Смещение для процесса
            offset += send_counts[i];         // Увеличиваем смещение
        }
    }

    // Распределение строк матрицы между процессами
    MPI_Scatterv(matrix, send_counts, displs, MPI_INT, local_matrix,
                 local_rows * n, MPI_INT, 0, MPI_COMM_WORLD);

    // Рассылаем вектор всем процессам
    MPI_Bcast(vector, n, MPI_INT, 0, MPI_COMM_WORLD);

    // Вычисляем локальный результат
    for (int i = 0; i < local_rows; i++) {
        local_result[i] = 0;
        for (int j = 0; j < n; j++) {
            local_result[i] += local_matrix[i * n + j] * vector[j];
        }
    }

    // Собираем результаты в массив result
    MPI_Gatherv(local_result, local_rows, MPI_INT, result, send_counts, displs, MPI_INT, 0, MPI_COMM_WORLD);

    // Освобождаем память
    free(local_matrix);
    free(local_result);
    if (rank == 0) {
        free(send_counts);
        free(displs);
    }
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

void print_vector(int *vector, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", vector[i]);
    }
    printf("\n");
}

#ifdef CHECK_RESULTS_CORRECTNESS
int main(int argc, char *argv[]) {
    int n = 4;  // Размер матрицы и вектора для теста (4x4)
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

        // Инициализируем матрицу и вектор
        int matrix_data[4][4] = {
            {1, 2, 3, 4},
            {5, 6, 7, 8},
            {9, 10, 11, 12},
            {13, 14, 15, 16}
        };

        for (int i = 0; i < n; i++) {
            vector[i] = 1;  // Заполняем вектор единицами
            for (int j = 0; j < n; j++) {
                matrix[i * n + j] = matrix_data[i][j];  // Заполняем матрицу
            }
        }

        // Выполняем последовательное умножение для проверки
        sequential_multiplication(n, matrix, vector, sequential_result);
    }

    MPI_Barrier(MPI_COMM_WORLD);  // Synchronize before timing

    double start, end;

    // Проверка для умножения с разбиением по строкам
    start = MPI_Wtime();
    row_split_multiplication(rank, size, n, matrix, vector, result);
    end = MPI_Wtime();
    if (rank == 0) {
        printf("Row-split time: %f seconds\n", end - start);
        printf("Row-split result: ");
        print_vector(result, n);
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
        printf("Column-split result: ");
        print_vector(result, n);
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
        printf("Block-split result: ");
        print_vector(result, n);
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


    // if (rank == 0) {
    //     matrix = (int *)malloc(n * n * sizeof(int));

    //     // Инициализируем матрицу и вектор
    //     if (matrix == NULL) {
    //         fprintf(stderr, "Memory allocation failed on process %d\n", rank);
    //         MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    //     }
    //     for (int i = 0; i < n; i++) {
    //         vector[i] = 1;
    //         for (int j = 0; j < n; j++) {
    //             matrix[i * n + j] = i + j + 1;
    //         }
    //     }