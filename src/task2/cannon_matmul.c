#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>
#include <math.h>

int allocMatrix(int*** mat, int rows, int cols) {
	int* p = (int*)malloc(sizeof(int*) * rows * cols);
	if (!p) {
		return -1;
	}
	
	*mat = (int**)malloc(rows * sizeof(int*));
	if (!mat) {
		free(p);
		return -1;
	}

	// Set up the pointers into the contiguous memory
	for (int i = 0; i < rows; i++) {
		(*mat)[i] = &(p[i * cols]);
	}
	return 0;
}

int freeMatrix(int ***mat) {
	free(&((*mat)[0][0]));
	free(*mat);
	return 0;
}

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

void printMatrix(int **mat, int size) {
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			printf("%d ", mat[i][j]);
		}
		printf("\n");
	}
}

void createMatrix(int*** mat, int size, int isIdentity) {
    // Выделение памяти под матрицу
    if (allocMatrix(mat, size, size) != 0) {
        printf("[ERROR] Matrix allocation failed!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Заполнение матрицы
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (isIdentity && i == j) {
                (*mat)[i][j] = 1;
            } else if (isIdentity) {
                (*mat)[i][j] = 0;
            } else {
                (*mat)[i][j] = rand() % 10;
            }
        }
    }
}


int main(int argc, char* argv[]) {
    MPI_Comm cartComm;
    int dim[2], period[2], reorder;
    int coord[2], id;
    int **A = NULL, **B = NULL, **C = NULL;
    int **localA = NULL, **localB = NULL, **localC = NULL;
    int rows = 0;
    int columns;
    int count = 0;
    int worldSize;
    int procDim;
    int blockDim;
    int left, right, up, down;
    int bCastData[4];

	int is_B_identity = 1;

    // Инициализация MPI
    MPI_Init(&argc, &argv);

    // Получение размера мира и ранга
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Считывание переменной MAT_SIZE из окружения
    char* matSizeEnv = getenv("MAT_SIZE");
    if (!matSizeEnv) {
        if (rank == 0) {
            printf("[ERROR] MAT_SIZE environment variable not set!\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    rows = columns = atoi(matSizeEnv);

    if (rank == 0) {

        double sqroot = sqrt(worldSize);
        if ((sqroot - floor(sqroot)) != 0) {
            printf("[ERROR] Number of processes must be a perfect square!\n");
            MPI_Abort(MPI_COMM_WORLD, 2);
        }
        int intRoot = (int)sqroot;
        if (columns % intRoot != 0 || rows % intRoot != 0) {
            printf("[ERROR] Number of rows/columns not divisible by %d!\n", intRoot);
            MPI_Abort(MPI_COMM_WORLD, 3);
        }
        procDim = intRoot;
        blockDim = columns / intRoot;

        srand(time(NULL));
        createMatrix(&A, rows, 0);
        createMatrix(&B, rows, is_B_identity);

        // printf("Matrix A:\n");
        // printMatrix(A, rows);
        // printf("Matrix B:\n");
        // printMatrix(B, rows);

        if (allocMatrix(&C, rows, columns) != 0) {
            printf("[ERROR] Matrix allocation for C failed!\n");
            MPI_Abort(MPI_COMM_WORLD, 4);
        }

        bCastData[0] = procDim;
        bCastData[1] = blockDim;
        bCastData[2] = rows;
        bCastData[3] = columns;
    }

    MPI_Bcast(&bCastData, 4, MPI_INT, 0, MPI_COMM_WORLD);
    procDim = bCastData[0];
    blockDim = bCastData[1];
    rows = bCastData[2];
    columns = bCastData[3];

    dim[0] = procDim; dim[1] = procDim;
    period[0] = 1; period[1] = 1;
    reorder = 1;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &cartComm);

    allocMatrix(&localA, blockDim, blockDim);
    allocMatrix(&localB, blockDim, blockDim);

	int globalSize[2] = { rows, columns };
	int localSize[2] = { blockDim, blockDim };
	int starts[2] = { 0,0 };
	MPI_Datatype type, subarrtype;
	MPI_Type_create_subarray(2, globalSize, localSize, starts, MPI_ORDER_C, MPI_INT, &type);
	MPI_Type_create_resized(type, 0, blockDim * sizeof(int), &subarrtype);
	MPI_Type_commit(&subarrtype);

	int *globalptrA = NULL;
	int *globalptrB = NULL;
	int *globalptrC = NULL;
	if (rank == 0) {
		globalptrA = &(A[0][0]);
		globalptrB = &(B[0][0]);
		globalptrC = &(C[0][0]);
	}

	int* sendCounts = (int*)malloc(sizeof(int) * worldSize);
	int* displacements = (int*)malloc(sizeof(int) * worldSize);
	if (rank == 0) {
		for (int i = 0; i < worldSize; i++) {
			sendCounts[i] = 1;
		}
		int disp = 0;
		for (int i = 0; i < procDim; i++) {
			for (int j = 0; j < procDim; j++) {
				displacements[i * procDim + j] = disp;
				disp += 1;
			}
			disp += (blockDim - 1)* procDim;
		}
	}

	MPI_Scatterv(
		globalptrA, sendCounts, displacements, subarrtype, 
		&(localA[0][0]), rows * columns / (worldSize), MPI_INT,
		0, MPI_COMM_WORLD
	);

	MPI_Scatterv(
		globalptrB, sendCounts, displacements, subarrtype, 
		&(localB[0][0]), rows * columns / (worldSize), MPI_INT,
		0, MPI_COMM_WORLD
	);

	if (allocMatrix(&localC, blockDim, blockDim) != 0) {
		printf("[ERROR] Matrix alloc for localC in rank %d failed!\n", rank);
		MPI_Abort(MPI_COMM_WORLD, 7);
	}

	MPI_Cart_coords(cartComm, rank, 2, coord);
	MPI_Cart_shift(cartComm, 1, coord[0], &left, &right);
	MPI_Sendrecv_replace(&(localA[0][0]), blockDim * blockDim, MPI_INT, left, 1, right, 1, cartComm, MPI_STATUS_IGNORE);
	MPI_Cart_shift(cartComm, 0, coord[1], &up, &down);
	MPI_Sendrecv_replace(&(localB[0][0]), blockDim * blockDim, MPI_INT, up, 1, down, 1, cartComm, MPI_STATUS_IGNORE);


	for (int i = 0; i < blockDim; i++) {
		for (int j = 0; j < blockDim; j++) {
			localC[i][j] = 0;
		}
	}

	int** multiplyRes = NULL;
	if (allocMatrix(&multiplyRes, blockDim, blockDim) != 0) {
		printf("[ERROR] Matrix alloc for multiplyRes in rank %d failed!\n", rank);
		MPI_Abort(MPI_COMM_WORLD, 8);
	}
	for (int k = 0; k < procDim; k++) {
		matrixMultiply(localA, localB, blockDim, blockDim, &multiplyRes);

		for (int i = 0; i < blockDim; i++) {
			for (int j = 0; j < blockDim; j++) {
				localC[i][j] += multiplyRes[i][j];
			}
		}
		

		MPI_Cart_shift(cartComm, 1, 1, &left, &right);
		MPI_Cart_shift(cartComm, 0, 1, &up, &down);
		MPI_Sendrecv_replace(&(localA[0][0]), blockDim * blockDim, MPI_INT, left, 1, right, 1, cartComm, MPI_STATUS_IGNORE);
		MPI_Sendrecv_replace(&(localB[0][0]), blockDim * blockDim, MPI_INT, up, 1, down, 1, cartComm, MPI_STATUS_IGNORE);
	}
	
	MPI_Gatherv(&(localC[0][0]), rows * columns / worldSize, MPI_INT,
		globalptrC, sendCounts, displacements, subarrtype,
		0, MPI_COMM_WORLD);

	freeMatrix(&localC);
	freeMatrix(&multiplyRes);

	if (rank == 0) {
		bool is_answer_correct = true;
		for (int i = 0; i < rows; ++i) {
			for (int j = 0; j < columns; ++j) {
				if (A[i][j] != C[i][j] && is_B_identity == 1) {
					is_answer_correct = false;
					break;
				}
			}
			if (!is_answer_correct) {
					break;
			}
		}
		if (is_answer_correct) {
			printf("Correct matmul!\n");
		}
		else {
			printf("Incorrect matmul!\n");
		}
		// printMatrix(C, rows);
	}
	
	MPI_Finalize();
	return 0;
}
