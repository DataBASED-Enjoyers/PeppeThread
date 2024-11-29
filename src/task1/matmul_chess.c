#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define ROOT 0

#ifdef VERBOSE
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

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
    for (int i = 0; i < mat_size; i++) {
        for (int j = 0; j < mat_size; j++) {
            printf("%6.2f ", matrix[i * mat_size + j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_vector(const double *vector, int size) {
    for (int i = 0; i < size; i++) {
        printf("%6.2f ", vector[i]);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    int rank, size;
    double start_time, end_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Получение размера матрицы из переменной окружения
    const char *mat_size_env = getenv("MAT_SIZE");
    int mat_size = mat_size_env ? atoi(mat_size_env) : 4; // По умолчанию 4

    // Проверка на совместимость размера матрицы и числа процессов
    if (mat_size % size != 0) {
        if (rank == ROOT) {
            fprintf(stderr, "Matrix size must be divisible by the number of processes!\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Проверка на "квадратность" числа процессов
    if ((int)sqrt(size) * (int)sqrt(size) != size) {
        if (rank == ROOT) {
            fprintf(stderr, "The number of processes must be a perfect square!\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int local_size = mat_size / size; // Размер локального блока

    // Выделяем память для локальных и глобальных данных
    double *matrix = NULL, *vector = NULL, *result = NULL;
    double *local_matrix = (double *)malloc(local_size * mat_size * sizeof(double));
    double *local_result = (double *)malloc(local_size * sizeof(double));
    double *local_vector = (double *)malloc(mat_size * sizeof(double)); // копия вектора

    if (rank == ROOT) {
        matrix = (double *)malloc(mat_size * mat_size * sizeof(double));
        vector = (double *)malloc(mat_size * sizeof(double));
        result = (double *)malloc(mat_size * sizeof(double));

        initialize_data(matrix, vector, mat_size);
        
        printf("Matrix:\n");
        print_matrix(matrix, mat_size);
        printf("Vector:\n");
        print_vector(vector, mat_size);
    }

    start_time = MPI_Wtime();

    // Распространение вектора и разбиение матрицы по процессам
    MPI_Scatter(matrix, local_size * mat_size, MPI_DOUBLE,
                local_matrix, local_size * mat_size, MPI_DOUBLE,
                ROOT, MPI_COMM_WORLD);

    MPI_Bcast(vector, mat_size, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);

    DEBUG_PRINT("Process %d received matrix block:\n", rank);
    for (int i = 0; i < local_size; i++) {
        DEBUG_PRINT("  Row %d: ", i);
        for (int j = 0; j < mat_size; j++) {
            DEBUG_PRINT("%6.2f ", local_matrix[i * mat_size + j]);
        }
        DEBUG_PRINT("\n");
    }

    DEBUG_PRINT("Process %d received vector:\n", rank);
    for (int i = 0; i < mat_size; i++) {
        DEBUG_PRINT("%6.2f ", vector[i]);
    }
    DEBUG_PRINT("\n");

    // Вычисление локального результата
    for (int i = 0; i < local_size; i++) {
        local_result[i] = 0.0;
        for (int j = 0; j < mat_size; j++) {
            local_result[i] += local_matrix[i * mat_size + j] * vector[j];
        }
    }

    DEBUG_PRINT("Process %d computed local result:\n", rank);
    for (int i = 0; i < local_size; i++) {
        DEBUG_PRINT("  %6.2f\n", local_result[i]);
    }

    // Сбор результатов от всех процессов
    MPI_Gather(local_result, local_size, MPI_DOUBLE,
               result, local_size, MPI_DOUBLE,
               ROOT, MPI_COMM_WORLD);

    end_time = MPI_Wtime();

    // Вывод результата
    if (rank == ROOT) {
        printf("Result:\n");
        print_vector(result, mat_size);
        printf("Execution time: %f seconds\n", end_time - start_time);
    }

    // Освобождение памяти
    free(local_matrix);
    free(local_result);
    free(local_vector);
    if (rank == ROOT) {
        free(matrix);
        free(vector);
        free(result);
    }

    MPI_Finalize();
    return 0;
}
