#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int* init_matrix(long size);
int* init_vector(long size, int value);
int compare_results(int n, int *result1, int *result2);
void seq_matmul(int n, int *matrix, int *vector, int *result);
void print_vector(int *vector, int n);
void distr_vector(int *vector, int *local_vector, int rank, int nprocs, long n);
void distr_mat(int *matrix, int *local_matrix, int rank, int nprocs, long n, long rows_per_proc);
void calc_matmul(int *local_matrix, int *local_vector, int *local_result, long rows_per_proc, long n);
 

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
        // Initialize matrix and vector on root process
        matrix = init_matrix(n);
        vector = init_vector(n, 1);  // Example vector filled with 1
        sequential_result = init_vector(n, 0);

        // Perform sequential multiplication for correctness checking
        seq_matmul(n, matrix, vector, sequential_result);
    } else {
        // Allocate memory for vector on non-root processes
        vector = init_vector(n, 1);
    }

    local_matrix = (int *)malloc(sizeof(int) * rows_per_proc * n);
    local_result = init_vector(rows_per_proc, 0);

    double start, end;

    // Проверка для умножения с разбиением по строкам
    start = MPI_Wtime();
    

    printf("matrix distr %d\n",rank);
    // Distribute rows of the matrix across all processes
    distr_mat(matrix, local_matrix, rank, nprocs, n, rows_per_proc);

    printf("vector cast %d\n",rank);
    printf("vector 0 %d", vector[0]);
    // Distribute the vector across all processes    
    distr_vector(vector, local_vector, rank, nprocs, n);

    printf("after vector cast %d\n",rank);

    printf("qweqwr %d\n",rank);
    // Perform local row-wise matrix-vector multiplication
    calc_matmul(local_matrix, vector, local_result, rows_per_proc, n);

    printf("dsf %d\n",rank);
    printf("locres %d\n", local_result[0]);
    printf("locres %d\n", local_result[1]);
    printf("locres %d\n", local_result[2]);
    printf("locres %d\n", local_result[3]);

    // Gather results on the root process
    if (rank == 0) {
        resultvector = init_vector(n, 0);
    }
    
    int *recvcounts = NULL, *displs = NULL;

    if (rank == 0) {
        recvcounts = (int *)malloc(nprocs * sizeof(int));
        displs = (int *)malloc(nprocs * sizeof(int));

        long rows = n / nprocs;
        int remainder = n % nprocs;
        int offset = 0;

        for (int i = 0; i < nprocs; i++) {
            recvcounts[i] = rows;
            if (i == nprocs - 1) {
                recvcounts[i] += remainder; // Add remainder rows to the last process
            }
            displs[i] = offset;
            offset += recvcounts[i];
        }
    }

    // Gather results from all processes
    MPI_Gatherv(local_result, rows_per_proc, MPI_INT,
                resultvector, recvcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);


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
void seq_matmul(int n, int *matrix, int *vector, int *result) {
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

// Distribute the vector using MPI_Bcast
void distr_vector(int *vector, int *local_vector, int rank, int nprocs, long n) {
    if (rank == 0) {
        // Root process has the full vector
        MPI_Bcast(vector, n, MPI_INT, 0, MPI_COMM_WORLD);
    } else {
        // Other processes receive the vector
        MPI_Bcast(local_vector, n, MPI_INT, 0, MPI_COMM_WORLD);
        for (long i = 0; i < n; i++) {
            vector[i] = local_vector[i];
        }
    }
}

// Distribute the matrix rows using MPI_Scatterv
void distr_mat(int *matrix, int *local_matrix, int rank, int nprocs, long n, long rows_per_proc) {
    int *sendcounts = NULL, *displs = NULL;

    if (rank == 0) {
        sendcounts = (int *)malloc(nprocs * sizeof(int));
        displs = (int *)malloc(nprocs * sizeof(int));

        int rows = n / nprocs;        
        int remainder = n % nprocs;
        int local_rows = rows + (rank < remainder ? 1 : 0);
        int offset = 0;

        for (int i = 0; i < nprocs; i++) {
            sendcounts[i] = (rows + (i < remainder ? 1 : 0)) * n;
            displs[i] = (i * rows + (i < remainder ? i : remainder)) * n;
        }
    }

    MPI_Scatterv(matrix, sendcounts, displs, MPI_INT,
                 local_matrix, rows_per_proc * n, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        free(sendcounts);
        free(displs);
    }
}

// Distribute the matrix rows using MPI_Scatterv
void gather_mat(int *matrix, int *local_matrix, int rank, int nprocs, long n, long rows_per_proc) {
    int *sendcounts = NULL, *displs = NULL;

    if (rank == 0) {
        sendcounts = (int *)malloc(nprocs * sizeof(int));
        displs = (int *)malloc(nprocs * sizeof(int));

        int rows = n / nprocs;        
        int remainder = n % nprocs;
        int local_rows = rows + (rank < remainder ? 1 : 0);
        int offset = 0;

        for (int i = 0; i < nprocs; i++) {
            sendcounts[i] = (rows + (i < remainder ? 1 : 0)) * n;
            displs[i] = (i * rows + (i < remainder ? i : remainder)) * n;
        }
    }

    MPI_Scatterv(matrix, sendcounts, displs, MPI_INT,
                 local_matrix, rows_per_proc * n, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        free(sendcounts);
        free(displs);
    }
}

// Perform row-wise matrix-vector multiplication
void calc_matmul(int *local_matrix, int *local_vector, int *local_result, long rows_per_proc, long n) {
    for (long i = 0; i < rows_per_proc; i++) {
        local_result[i] = 0;
        for (long j = 0; j < n; j++) {
            local_result[i] += local_matrix[i * n + j] * local_vector[j];
        }
    }
}