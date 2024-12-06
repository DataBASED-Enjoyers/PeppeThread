#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int *init_matrix(long size);
int compare_results(int n, int *result1, int *result2);
void seq_matmul(int n, int *matrix, int *vector, int *result);
void print_vector(int *vector, int n);
void distr_mat(int *matrix, int *local_matrix, int rank, int nprocs, int n);
void gather_mat(int *matrix, int *local_matrix, int rank, int nprocs, int n);
void calc_matmul(int *local_matrix, int *local_vector, int *local_result,
                 int rows_per_proc, int n);

int main(int argc, char *argv[]) {
    int rank, nprocs;
    int n;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc != 2) {
        if (rank == 0) {
            printf(
                "Usage: mpirun -n <nodecount> ./program_name <matrix_size>\n");
        }
        MPI_Finalize();
        return 1;
    }
    n = atol(argv[1]);
    if (n < nprocs) {
        if (rank == 0) {
            printf(
                "Matrix size must be greater than the number of processes.\n");
        }
        MPI_Finalize();
        return 1;
    }

    int *matrix = NULL;
    int *vector = (int *)malloc(n * sizeof(int));
    int *resultvector = (rank == 0) ? (int *)malloc(n * sizeof(int)) : NULL;
    int *sequential_result =
        (rank == 0) ? (int *)malloc(n * sizeof(int)) : NULL;

    if (rank == 0) {
        matrix = init_matrix(n);
        for (int i = 0; i < n; i++) {
            vector[i] = 1;
        }

        seq_matmul(n, matrix, vector, sequential_result);
    }

    int rows_per_process = n / nprocs;
    int extra_rows = n % nprocs;
    int local_rows = rows_per_process + (rank < extra_rows ? 1 : 0);

    double start, end;

    int *local_matrix = (int *)malloc(sizeof(int) * local_rows * n);
    int *local_result = (int *)malloc(local_rows * sizeof(int));

    start = MPI_Wtime();

    distr_mat(matrix, local_matrix, rank, nprocs, n);
    MPI_Bcast(vector, n, MPI_INT, 0, MPI_COMM_WORLD);
    calc_matmul(local_matrix, vector, local_result, local_rows, n);

    gather_mat(resultvector, local_result, rank, nprocs, n);

    end = MPI_Wtime();

    free(local_matrix);
    free(local_result);

    if (rank == 0) {
        printf("Row-split time: %f seconds\n", end - start);

#ifdef VERBOSE
        printf("Row-split result: ");
        print_vector(resultvector, n);
        if (compare_results(n, resultvector, sequential_result)) {
            printf("Row-split multiplication is correct.\n");
        } else {
            printf("Row-split multiplication is incorrect.\n");
        }
#endif
    }

    MPI_Finalize();
    return 0;
}

int *init_matrix(long size) {
    int *matrix = (int *)malloc(sizeof(int) * size * size);
    if (!matrix) {
        printf("Error allocating memory for the matrix!\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    for (long i = 0; i < size; i++) {
        for (long j = 0; j < size; j++) {
            matrix[i * size + j] = (int)(i + j + 1);
        }
    }

    return matrix;
}

int compare_results(int n, int *result1, int *result2) {
    for (int i = 0; i < n; i++) {
        if (result1[i] != result2[i]) {
            return 0;
        }
    }
    return 1;
}

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

void distr_mat(int *matrix, int *local_matrix, int rank, int nprocs, int n) {
    int *sendcounts = NULL, *displs = NULL;
    int rows_per_proc = n / nprocs;
    int extra_rows = n % nprocs;
    int local_rows = rows_per_proc + (rank < extra_rows ? 1 : 0);

    if (rank == 0) {
        sendcounts = (int *)malloc(nprocs * sizeof(int));
        displs = (int *)malloc(nprocs * sizeof(int));

        for (int i = 0; i < nprocs; i++) {
            sendcounts[i] = (rows_per_proc + (i < extra_rows ? 1 : 0)) * n;
            displs[i] =
                (i * rows_per_proc + (i < extra_rows ? i : extra_rows)) * n;
        }
    }

    MPI_Scatterv(matrix, sendcounts, displs, MPI_INT, local_matrix,
                 local_rows * n, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        free(sendcounts);
        free(displs);
    }
}

void gather_mat(int *resultvector, int *local_result, int rank, int nprocs,
                int n) {
    int *recv_counts = NULL, *recv_displs = NULL;
    int rows_per_proc = n / nprocs;
    int extra_rows = n % nprocs;
    int local_rows = rows_per_proc + (rank < extra_rows ? 1 : 0);

    if (rank == 0) {
        recv_counts = (int *)malloc(nprocs * sizeof(int));
        recv_displs = (int *)malloc(nprocs * sizeof(int));

        for (int i = 0; i < nprocs; i++) {
            recv_counts[i] = (rows_per_proc + (i < extra_rows ? 1 : 0));
            recv_displs[i] =
                (i * rows_per_proc + (i < extra_rows ? i : extra_rows));
        }
    }

    MPI_Gatherv(local_result, local_rows, MPI_INT, resultvector, recv_counts,
                recv_displs, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        free(recv_counts);
        free(recv_displs);
    }
}

void calc_matmul(int *local_matrix, int *local_vector, int *local_result,
                 int rows_per_proc, int n) {
    for (int i = 0; i < rows_per_proc; i++) {
        local_result[i] = 0;
        for (long j = 0; j < n; j++) {
            local_result[i] += local_matrix[i * n + j] * local_vector[j];
        }
    }
}