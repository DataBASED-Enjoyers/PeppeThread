#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int *init_matrix(long size);
int *init_vector(long size, int value);
int compare_results(int n, int *result1, int *result2);
void seq_matmul(int n, int *matrix, int *vector, int *result);
void print_vector(int *vector, int n);
void distr_vector(int *vector, int *local_vector, int rank, int nprocs, long n,
                  long chunksize);
void distr_mat(int *matrix, int *local_matrix, int rank, int nprocs, long n,
               long chunksize);
void calc_reduce_scatter(int *local_matrix, int *local_vector,
                         int *resultvector, int rank, int nprocs, long n,
                         long chunksize);

int main(int argc, char *argv[]) {
    int rank, nprocs;
    long n, chunksize, columncount;
    int *matrix = NULL, *vector = NULL;
    int *local_matrix = NULL, *local_vector = NULL;
    int *resultvector = NULL;
    int *sequential_result = NULL;

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

    sequential_result = init_vector(n, 1);

    if (rank == 0) {
        matrix = init_matrix(n);
        vector = init_vector(n, 1);

        seq_matmul(n, matrix, vector, sequential_result);
    }

    resultvector = init_vector(n, 1);

    columncount = n - ((n / nprocs) * (nprocs - 1));
    chunksize = (rank == nprocs - 1) ? columncount : (n / nprocs);
    columncount = (columncount > chunksize)
                      ? columncount
                      : chunksize;

    local_matrix = (int *)malloc(sizeof(int) * n * columncount);
    local_vector = (int *)malloc(sizeof(int) * chunksize);

    double start, end;

    start = MPI_Wtime();

    distr_vector(vector, local_vector, rank, nprocs, n, chunksize);

    distr_mat(matrix, local_matrix, rank, nprocs, n, chunksize);

    calc_reduce_scatter(local_matrix, local_vector, resultvector, rank, nprocs,
                        n, chunksize);

    end = MPI_Wtime();
    if (rank == 0) {
        printf("Column-split time: %f seconds\n", end - start);

#ifdef VERBOSE
        printf("Column-split time result: ");
        print_vector(resultvector, n);
        if (compare_results(n, resultvector, sequential_result)) {
            printf("Column-split time multiplication is correct.\n");
        } else {
            printf("Column-split time multiplication is incorrect.\n");
        }
#endif
    }

    if (rank == 0) {
        free(vector);
        free(matrix);
    }
    free(resultvector);
    free(local_vector);
    free(local_matrix);

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
            matrix[i * size + j] =
                (int)(i + j + 1);
        }
    }

    return matrix;
}

int *init_vector(long size, int value) {
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

void distr_mat(int *matrix, int *local_matrix, int rank, int nprocs, long n,
               long chunksize) {
    int sendcounts[nprocs], displs[nprocs];
    MPI_Datatype MPI_coltype, MPI_resized_coltype;

    int chunk = n / nprocs;
    int lastchunk = n - (chunk * (nprocs - 1));

    MPI_Type_vector(n, 1, n, MPI_INT, &MPI_coltype);
    MPI_Type_create_resized(MPI_coltype, 0, sizeof(int), &MPI_resized_coltype);
    MPI_Type_commit(&MPI_resized_coltype);

    for (int i = 0; i < nprocs; i++) {
        sendcounts[i] = (i == nprocs - 1) ? lastchunk : chunk;
        displs[i] = i * chunk;
    }

    MPI_Scatterv(matrix, sendcounts, displs, MPI_resized_coltype, local_matrix,
                 chunksize * n, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Type_free(&MPI_resized_coltype);
}

void distr_vector(int *vector, int *local_vector, int rank, int nprocs, long n,
                  long chunksize) {
    int sendcounts[nprocs], displs[nprocs];
    int chunk = n / nprocs;
    int lastchunk = n - (chunk * (nprocs - 1));

    for (int i = 0; i < nprocs; i++) {
        sendcounts[i] = (i == nprocs - 1) ? lastchunk : chunk;
        displs[i] = i * chunk;
    }

    MPI_Scatterv(vector, sendcounts, displs, MPI_INT, local_vector, chunksize,
                 MPI_INT, 0, MPI_COMM_WORLD);
}

void calc_reduce_scatter(int *local_matrix, int *local_vector,
                         int *resultvector, int rank, int nprocs, long n,
                         long chunksize) {
    int *intermediate_result = init_vector(n, 0);
    int recvcounts[nprocs];

    for (int i = 0; i < nprocs; i++) {
        recvcounts[i] = n;
    }

    for (long i = 0; i < chunksize; i++) {
        for (long j = 0; j < n; j++) {
            intermediate_result[j] += local_matrix[i * n + j] * local_vector[i];
        }
    }

    MPI_Reduce_scatter(intermediate_result, resultvector, recvcounts, MPI_INT,
                       MPI_SUM, MPI_COMM_WORLD);

    free(intermediate_result);
}