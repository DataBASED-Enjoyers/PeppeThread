#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int* init_matrix(long size);
int* init_vector(long size, int value);
int compare_results(int n, int *result1, int *result2);
void seq_matmul(int n, int *matrix, int *vector, int *result);
void print_vector(int *vector, int n);
void distr_vector(int *vector, int *local_vector, int rank, int nprocs, long n, long chunksize);
void distr_mat(int *matrix, int *local_matrix, int rank, int nprocs, long n, long chunksize);
void calc_reduce_scatter(int *local_matrix, int *local_vector, int *resultvector, int rank, int nprocs, long n, long chunksize);


int main(int argc, char *argv[]) {
    int rank, nprocs;
    long n, rows_per_proc;
    int *matrix = NULL, *vector = NULL;
    int *local_matrix = NULL, *local_vector = NULL;
    int *local_result = NULL;
    int *resultvector = NULL;
    int *sequential_result = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc != 2) {
        if (rank == 0) {
            printf("Usage: mpirun -n <nodecount> ./program_name <matrix_size>\n");
        }
        MPI_Finalize();
        return 1;
    }

    n = atol(argv[1]);
    if (n < nprocs) {
        if (rank == 0) {
            printf("Matrix size must be greater than the number of processes.\n");
        }
        MPI_Finalize();
        return 1;
    }

    // Determine rows per process
    rows_per_proc = n / nprocs;
    if (rank == nprocs - 1) {
        rows_per_proc += n % nprocs; // Add remainder to the last process
    }

    sequential_result = init_vector(n, 1);

    // Initialize matrix and vector on the root process
    if (rank == 0) {
        matrix = init_matrix(n);
        vector = init_vector(n, 1);  // Example vector filled with 1
        
        // Выполняем последовательное умножение для проверки
        seq_matmul(n, matrix, vector, sequential_result);
    }

    local_matrix = (int *)malloc(sizeof(int) * rows_per_proc * n);
    local_vector = init_vector(n, 0);
    local_result = init_vector(rows_per_proc, 0);

    double start, end;

    // Проверка для умножения с разбиением по строкам
    start = MPI_Wtime();
    row_split_multiplication(rank, nprocs, n, matrix, vector, resultvector);
    end = MPI_Wtime();
    if (rank == 0) {
        printf("Row-split time: %f seconds\n", end - start);
        printf("Row-split result: ");
        print_vector(resultvector, n);
        if (compare_results(n, resultvector, sequential_result)) {
            printf("Row-split multiplication is correct.\n");
        } else {
            printf("Row-split multiplication is incorrect.\n");
        }
    }

    MPI_Finalize();
    return 0;
}

// Initializes a matrix (1D array representation of 2D matrix)
int* init_matrix(long size) {
    int *matrix = (int *)malloc(sizeof(int) * size * size);
    if (!matrix) {
        printf("Error allocating memory for the matrix!\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    for (long i = 0; i < size; i++) {
        for (long j = 0; j < size; j++) {
            matrix[i * size + j] = (int)(i + j + 1);  // Example: Fill with column indices
        }
    }

    return matrix;
}

// Initializes a vector
int* init_vector(long size, int value) {
    int *vector = (int *)malloc(sizeof(int) * size);
    if (!vector) {
        printf("Error allocating memory for the vector!\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    for (long i = 0; i < size; i++) {
        vector[i] = value;
    }

    return vector;
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