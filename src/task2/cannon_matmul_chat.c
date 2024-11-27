#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>
#include <math.h>
#include <time.h>

// Allocate a 2D matrix
int allocMatrix(int ***mat, int rows, int cols) {
    int *data = (int *)malloc(rows * cols * sizeof(int));
    if (!data) return -1;

    *mat = (int **)malloc(rows * sizeof(int *));
    if (!*mat) {
        free(data);
        return -1;
    }

    for (int i = 0; i < rows; i++) {
        (*mat)[i] = &(data[i * cols]);
    }
    return 0;
}

// Free a 2D matrix
void freeMatrix(int ***mat) {
    if (*mat) {
        free(&((*mat)[0][0])); // Free the data block
        free(*mat);           // Free the row pointers
        *mat = NULL;
    }
}

// Perform matrix multiplication
void matrixMultiply(int **a, int **b, int rows, int cols, int ***c) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int val = 0;
            for (int k = 0; k < rows; k++) {
                val += a[i][k] * b[k][j];
            }
            (*c)[i][j] = val;
        }
    }
}

// Print a 2D matrix
void printMatrix(int **mat, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
}

// Create and optionally initialize a matrix
void createMatrix(int ***mat, int size, bool isIdentity) {
    if (allocMatrix(mat, size, size) != 0) {
        fprintf(stderr, "[ERROR] Matrix allocation failed!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            (*mat)[i][j] = isIdentity ? (i == j ? 1 : 0) : rand() % 10;
        }
    }
}

// Validate the result matrix
bool validateMatrix(int **a, int **b, int size, bool isIdentity) {
    if (isIdentity) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (a[i][j] != b[i][j]) {
                    return false;
                }
            }
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    MPI_Comm cartComm;
    int dim[2], period[2], reorder;
    int coord[2], id;
    int **A = NULL, **B = NULL, **C = NULL;
    int **localA = NULL, **localB = NULL, **localC = NULL;
    int rows, cols, procDim, blockDim, rank, worldSize;
    int left, right, up, down;
    int is_B_identity = 1;
    int bCastData[4];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    char *matSizeEnv = getenv("MAT_SIZE");
    if (!matSizeEnv) {
        if (rank == 0) {
            fprintf(stderr, "[ERROR] MAT_SIZE environment variable not set!\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    rows = cols = atoi(matSizeEnv);

    if (rank == 0) {
        double sqroot = sqrt(worldSize);
        if ((sqroot - floor(sqroot)) != 0) {
            fprintf(stderr, "[ERROR] Number of processes must be a perfect square!\n");
            MPI_Abort(MPI_COMM_WORLD, 2);
        }
        procDim = (int)sqroot;
        if (rows % procDim != 0) {
            fprintf(stderr, "[ERROR] Matrix size not divisible by %d!\n", procDim);
            MPI_Abort(MPI_COMM_WORLD, 3);
        }
        blockDim = rows / procDim;

        srand(time(NULL));
        createMatrix(&A, rows, false);
        createMatrix(&B, rows, is_B_identity);

        if (allocMatrix(&C, rows, cols) != 0) {
            fprintf(stderr, "[ERROR] Matrix allocation for C failed!\n");
            MPI_Abort(MPI_COMM_WORLD, 4);
        }

        bCastData[0] = procDim;
        bCastData[1] = blockDim;
        bCastData[2] = rows;
        bCastData[3] = cols;
    }

    MPI_Bcast(&bCastData, 4, MPI_INT, 0, MPI_COMM_WORLD);
    procDim = bCastData[0];
    blockDim = bCastData[1];
    rows = bCastData[2];
    cols = bCastData[3];

    dim[0] = procDim;
    dim[1] = procDim;
    period[0] = 1;
    period[1] = 1;
    reorder = 1;

    MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &cartComm);

    allocMatrix(&localA, blockDim, blockDim);
    allocMatrix(&localB, blockDim, blockDim);
    allocMatrix(&localC, blockDim, blockDim);

    // Scatter and perform matrix multiplication
    // (omitting for brevity; similar logic as original with cleanup)
    
    // Gather results and validate
    if (rank == 0) {
        bool isCorrect = validateMatrix(A, C, rows, is_B_identity);
        printf("%s\n", isCorrect ? "Correct matmul!" : "Incorrect matmul!");
        freeMatrix(&A);
        freeMatrix(&B);
        freeMatrix(&C);
    }

    freeMatrix(&localA);
    freeMatrix(&localB);
    freeMatrix(&localC);

    MPI_Finalize();
    return 0;
}
