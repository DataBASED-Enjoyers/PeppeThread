#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROOT 0

// Функция для инициализации матрицы и вектора случайными числами
void initialize_data(double *matrix, double *vector, int mat_size) {
    for (int i = 0; i < mat_size * mat_size; i++) {
        matrix[i] = i + 1;
    }
    for (int i = 0; i < mat_size; i++) {
        vector[i] = i + 1;
    }
}

// Функция для проверки результата
void print_matrix(const double *matrix, int mat_size) {
#ifdef VERBOSE
    for (int i = 0; i < mat_size; i++) {
        for (int j = 0; j < mat_size; j++) {
            printf("%6.2f ", matrix[i * mat_size + j]);
        }
        printf("\n");
    }
    printf("\n");
#endif
}

void print_vector(const double *vector, int size) {
#ifdef VERBOSE
    for (int i = 0; i < size; i++) {
        printf("%6.2f ", vector[i]);
    }
    printf("\n");
#endif
}

void distr_vec(double *vector, double *local_vector, int rank, int nprocs,
               int n, int chunksize) {
    int grid_size = (int)sqrt(nprocs);  // сетка процессоров
    int sendcounts[nprocs], displs[nprocs];
    double *temp_vec = (double *)malloc(chunksize * sizeof(double));

    if (rank == ROOT) {
        for (int p = 0; p < nprocs; ++p) {
            int shift = (p % grid_size) * chunksize;

            for (int i = 0; i < chunksize; ++i) {
                temp_vec[i] = vector[shift + i];
            }

            if (p == ROOT) {
                memcpy(local_vector, temp_vec, chunksize * sizeof(double));
            } else {
                MPI_Send(temp_vec, chunksize, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        MPI_Recv(local_vector, chunksize, MPI_DOUBLE, ROOT, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
    }
    free(temp_vec);
}

void distr_mat(double *matrix, double *local_matrix, int rank, int nprocs,
               int n, int chunksize) {
    int grid_size = (int)sqrt(nprocs);  // сетка процессоров
    int local_size = chunksize * chunksize;
    double *temp_block = (double *)malloc(local_size * sizeof(double));

    if (rank == ROOT) {
        for (int i = 0; i < nprocs; i++) {
            int start_row =
                (i / grid_size) * chunksize;  // Начальная строка блока
            int start_col =
                (i % grid_size) * chunksize;  // Начальный столбец блока

            // Заполняем временный блок для процесса
            for (int row = 0; row < chunksize; row++) {
                for (int col = 0; col < chunksize; col++) {
                    int global_row = start_row + row;
                    int global_col = start_col + col;
                    temp_block[row * chunksize + col] =
                        matrix[global_row * n + global_col];
                }
            }

            if (i == ROOT) {
                // Копируем блок ROOT-процесса в его локальную матрицу
                memcpy(local_matrix, temp_block, local_size * sizeof(double));
            } else {
                // Отправляем блок остальным процессам
                MPI_Send(temp_block, local_size, MPI_DOUBLE, i, 0,
                         MPI_COMM_WORLD);
            }
        }
    } else {
        // Получаем блок для текущего процесса
        MPI_Recv(local_matrix, local_size, MPI_DOUBLE, ROOT, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
    }
    free(temp_block);
}

void calc_loc_matmul(double *local_matrix, double *local_vector,
                     double *local_result, int chunksize, int rank) {
    for (int i = 0; i < chunksize; i++) {
        local_result[i] = 0.0;
        for (int j = 0; j < chunksize; j++) {
            local_result[i] +=
                local_matrix[i * chunksize + j] * local_vector[j];
        }
    }

#ifdef VERBOSE
    printf("Process %d computed local result:\n", rank);
    for (int i = 0; i < chunksize; i++) {
        printf("  %6.2f\n", local_result[i]);
    }
#endif
}

void calc_reduce_result(double *result, double *local_result, int rank,
                        int nprocs, int n, int chunksize) {
    int grid_size = (int)sqrt(nprocs);  // сетка процессоров
    double *partial_result = (double *)malloc(n * sizeof(double));
    for (int i = 0; i < n; ++i) {
        partial_result[i] = 0;
    }

    for (int i = 0; i < chunksize; ++i) {
        partial_result[i + (int)(rank / grid_size) * chunksize] +=
            local_result[i];
    }

    MPI_Reduce(partial_result, result, n, MPI_DOUBLE, MPI_SUM, ROOT,
               MPI_COMM_WORLD);

#ifdef VERBOSE
    printf("Process %i: RESULT = ", rank);
    for (int i = 0; i < n; ++i) {
        printf("%f ", partial_result[i]);
    }
    printf("\n");
#endif

    free(partial_result);
}

int main(int argc, char **argv) {
    int rank, size;
    double start_time, end_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 2) {
        if (rank == 0) {
            printf(
                "Usage: mpirun -n <nodecount> ./program_name <matrix_size>\n");
        }
        MPI_Finalize();
        return 1;
    }
    int mat_size = atoi(argv[1]);
    int num_of_blocks_by_dim = (int)sqrt(size);
    int block_size = mat_size / num_of_blocks_by_dim;

    // Проверка на совместимость размера матрицы и числа процессов
    if (mat_size % size != 0) {
        if (rank == ROOT) {
            fprintf(
                stderr,
                "Matrix size must be divisible by the number of processes!\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Проверка на "полную квадратность" числа процессов
    if (num_of_blocks_by_dim * num_of_blocks_by_dim != size) {
        if (rank == ROOT) {
            fprintf(stderr,
                    "The number of processes must be a perfect square!\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Выделяем память для локальных и глобальных данных
    double *matrix = NULL, *vector = NULL, *result = NULL;
    double *local_matrix =
        (double *)malloc(block_size * block_size * sizeof(double));
    double *local_result = (double *)malloc(block_size * sizeof(double));
    double *local_vector = (double *)malloc(block_size * sizeof(double));

    if (rank == ROOT) {
        matrix = (double *)malloc(mat_size * mat_size * sizeof(double));
        vector = (double *)malloc(mat_size * sizeof(double));
        result = (double *)malloc(mat_size * sizeof(double));

        initialize_data(matrix, vector, mat_size);

#ifdef VERBOSE
        printf("Matrix:\n");
        print_matrix(matrix, mat_size);
        printf("Vector:\n");
        print_vector(vector, mat_size);
#endif
    }

    start_time = MPI_Wtime();

    // Распространение вектора и разбиение матрицы по процессам
    distr_mat(matrix, local_matrix, rank, size, mat_size, block_size);

#ifdef VERBOSE
    printf("Process %d received matrix block:\n", rank);
    for (int i = 0; i < block_size; i++) {
        printf("  Row %d: ", i);
        for (int j = 0; j < block_size; j++) {
            printf("%6.2f ", local_matrix[i * block_size + j]);
        }
        printf("\n");
    }
#endif

    distr_vec(vector, local_vector, rank, size, mat_size, block_size);

#ifdef VERBOSE
    printf("Process %d received vector:\n", rank);
    for (int i = 0; i < block_size; i++) {
        printf("%6.2f ", local_vector[i]);
    }
    printf("\n");
#endif

    calc_loc_matmul(local_matrix, local_vector, local_result, block_size, rank);

    calc_reduce_result(result, local_result, rank, size, mat_size, block_size);

    end_time = MPI_Wtime();

    // Вывод результата
    if (rank == ROOT) {
        printf("Block-split time: %f seconds\n", end_time - start_time);

#ifdef VERBOSE
        printf("Block-split result:\n");
        print_vector(result, mat_size);
#endif
    }

    // Освобождение памяти
    free(local_matrix);
    free(local_vector);
    free(local_result);
    if (rank == ROOT) {
        free(matrix);
        free(vector);
        free(result);
    }

    MPI_Finalize();
    return 0;
}
