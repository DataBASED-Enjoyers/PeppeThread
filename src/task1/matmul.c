#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
    int cols_per_process = n / size; // Number of columns per process
    int extra_cols = n % size;       // Extra columns to distribute

    // Number of local columns for this process
    int local_cols = cols_per_process + (rank < extra_cols ? 1 : 0);

    // Allocate memory for local matrix and local vector
    int *local_matrix = (int *)malloc(n * local_cols * sizeof(int));
    if (!local_matrix) {
        fprintf(stderr, "Process %d: Failed to allocate memory for local_matrix\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    int start_col = rank * cols_per_process + (rank < extra_cols ? rank : extra_cols);

    // Populate the local matrix
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < local_cols; j++) {
            int global_col_idx = start_col + j; // Global column index
            local_matrix[i * local_cols + j] = matrix[i * n + global_col_idx];
        }
    }

    // Allocate memory for local vector
    int *local_vector = (int *)malloc(local_cols * sizeof(int));
    if (!local_vector) {
        fprintf(stderr, "Process %d: Failed to allocate memory for local_vector\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Populate the local vector
    for (int i = 0; i < local_cols; i++) {
        int global_col_idx = start_col + i;
        local_vector[i] = vector[global_col_idx];
    }

    // Allocate memory for local result
    int *local_result = (int *)calloc(n, sizeof(int)); // Initialize to 0
    if (!local_result) {
        fprintf(stderr, "Process %d: Failed to allocate memory for local_result\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Perform local matrix-vector multiplication
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < local_cols; j++) {
            local_result[i] += local_matrix[i * local_cols + j] * local_vector[j];
        }
    }

    // Synchronize all processes before reduction
    MPI_Barrier(MPI_COMM_WORLD);

    // Reduce results to the root process
    MPI_Reduce(local_result, result, n, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Free allocated memory
    free(local_matrix);
    free(local_vector);
    free(local_result);
}



// Block-split multiplication
void block_split_multiplication(int rank, int size, int n, int *matrix, int *vector, int *result) {
    int sqrt_size = (int)sqrt(size);
    if (sqrt_size * sqrt_size != size) {
        if (rank == 0) {
            fprintf(stderr, "Number of processes must be a perfect square.\n");
        }
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    int block_size = n / sqrt_size + (n % sqrt_size != 0);
    int *local_matrix = (int *)malloc(block_size * block_size * sizeof(int));
    int *local_vector = (int *)malloc(block_size * sizeof(int));
    int *local_result = (int *)calloc(block_size, sizeof(int));

    MPI_Comm grid_comm;
    int dims[2] = {sqrt_size, sqrt_size};
    int periods[2] = {0, 0};
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &grid_comm);

    int coords[2];
    MPI_Cart_coords(grid_comm, rank, 2, coords);

    // Scatter data and perform computation
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            local_result[i] += local_matrix[i * block_size + j] * local_vector[j];
        }
    }

    MPI_Gather(local_result, block_size, MPI_INT, result, block_size, MPI_INT, 0, grid_comm);

    free(local_matrix);
    free(local_vector);
    free(local_result);
    MPI_Comm_free(&grid_comm);
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
    int *result1 = (rank == 0) ? (int *)malloc(n * sizeof(int)) : NULL;
    int *result2 = (rank == 0) ? (int *)malloc(n * sizeof(int)) : NULL;
    int *result3 = (rank == 0) ? (int *)malloc(n * sizeof(int)) : NULL;
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
    // start = MPI_Wtime();
    // row_split_multiplication(rank, size, n, matrix, vector, result1);
    // end = MPI_Wtime();
    // if (rank == 0) {
    //     printf("Row-split time: %f seconds\n", end - start);
    //     printf("Row-split result: ");
    //     print_vector(result1, n);
    //     if (compare_results(n, result1, sequential_result)) {
    //         printf("Row-split multiplication is correct.\n");
    //     } else {
    //         printf("Row-split multiplication is incorrect.\n");
    //     }
    // }
    
    // MPI_Barrier(MPI_COMM_WORLD);  // Synchronize before timing

    // Проверка для умножения с разбиением по столбцам
    start = MPI_Wtime();
    column_split_multiplication(rank, size, n, matrix, vector, result2);
    end = MPI_Wtime();
    if (rank == 0) {
        printf("Column-split time: %f seconds\n", end - start);
        printf("Column-split result: ");
        print_vector(result2, n);
        if (compare_results(n, result2, sequential_result)) {
            printf("Column-split multiplication is correct.\n");
        } else {
            printf("Column-split multiplication is incorrect.\n");
        }        
    }
    
    MPI_Barrier(MPI_COMM_WORLD);  // Synchronize before timing

    // Проверка для умножения с разбиением по блокам
    // start = MPI_Wtime();
    // block_split_multiplication(rank, size, n, matrix, vector, result3);
    // end = MPI_Wtime();
    // if (rank == 0) {
        
    //     print_vector(sequential_result, n);
    //     printf("Block-split time: %f seconds\n", end - start);
    //     printf("Block-split result: ");
    //     print_vector(result3, n);
    //     if (compare_results(n, result3, sequential_result)) {
    //         printf("Block-split multiplication is correct.\n");
    //         print_vector(result3, n);
    //         print_vector(sequential_result, n);
    //     } else {
    //         printf("Block-split multiplication is incorrect.\n");
    //         print_vector(result3, n);
    //         print_vector(sequential_result, n);
    //     }
    // }
    
    MPI_Barrier(MPI_COMM_WORLD); 

    if (rank == 0) {
        free(matrix);
        free(result1);
        free(result2);
        free(result3);
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