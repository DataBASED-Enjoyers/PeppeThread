#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

double f(double x, double y) {
    return sin(M_PI * x) * sin(M_PI * y);
}

void initialize_boundary_conditions(double** u, int N, double c) {
    for (int i = 0; i <= N; i++) {
        u[i][0] = c;         // Нижняя граница
        u[i][N] = c;         // Верхняя граница
        u[0][i] = c;         // Левая граница
        u[N][i] = c;         // Правая граница
    }
}

// Метод Гаусса-Зейделя
void gauss_seidel(double** u, double** u_new, int N, double h, double eps, int max_iter) {
    int iter = 0;
    double diff;

    do {
        diff = 0.0;

        // Параллельные вычисления с OpenMP
        #pragma omp parallel for reduction(max : diff)
        for (int i = 1; i < N; i++) {
            for (int j = 1; j < N; j++) {
                u_new[i][j] = 0.25 * (u[i+1][j] + u[i-1][j] + u[i][j+1] + u[i][j-1] - h * h * f(i * h, j * h));
                diff = fmax(diff, fabs(u_new[i][j] - u[i][j]));
            }
        }

        // Копируем новую матрицу в старую
        #pragma omp parallel for
        for (int i = 1; i < N; i++) {
            for (int j = 1; j < N; j++) {
                u[i][j] = u_new[i][j];
            }
        }

        iter++;
    } while (diff > eps && iter < max_iter);

    printf("Converged in %d iterations with max difference %.10f\n", iter, diff);
}

// Функция для сохранения данных в CSV
void save_to_csv(double** u, int N, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Записываем данные в CSV (построчно)
    for (int i = 0; i <= N; i++) {
        for (int j = 0; j <= N; j++) {
            fprintf(file, "%.10f", u[i][j]);
            if (j < N) fprintf(file, ",");  // Разделитель для CSV
        }
        fprintf(file, "\n");  // Конец строки
    }

    fclose(file);
    printf("Data saved to %s\n", filename);
}

int main(int argc, char* argv[]) {
    // Проверка аргументов командной строки
    if (argc != 3) {
        printf("Usage: %s <number_of_points> <number_of_threads>\n", argv[0]);
        return 1;
    }

    // Чтение параметров из argv
    int N = atoi(argv[1]);          // Число точек сетки
    int num_threads = atoi(argv[2]); // Число потоков OpenMP

    // Проверка на корректность
    if (N <= 1 || num_threads <= 0) {
        printf("Error: number_of_points must be > 1 and number_of_threads must be > 0\n");
        return 1;
    }

    // Установка числа потоков для OpenMP
    omp_set_num_threads(num_threads);

    // Параметры задачи
    double c = 1.0;            // Значение на границах
    double eps = 1e-6;         // Погрешность
    int max_iter = 100000;      // Максимальное число итераций
    double h = 1.0 / N;        // Шаг сетки

    // Выделяем память для температурной матрицы
    double** u = (double**)malloc((N + 1) * sizeof(double*));
    double** u_new = (double**)malloc((N + 1) * sizeof(double*));
    for (int i = 0; i <= N; i++) {
        u[i] = (double*)calloc(N + 1, sizeof(double));
        u_new[i] = (double*)calloc(N + 1, sizeof(double));
    }

    // Инициализация граничных условий
    initialize_boundary_conditions(u, N, c);

    // Замер времени
    double start_time = omp_get_wtime();

    // Решение методом Гаусса-Зейделя
    gauss_seidel(u, u_new, N, h, eps, max_iter);

    double end_time = omp_get_wtime();
    double elapsed_time = end_time - start_time;

    // Вывод времени выполнения
    printf("Elapsed time: %.6f seconds\n", elapsed_time);

    // Сохраняем результаты в CSV
    save_to_csv(u, N, "src/task2/benchmarks/output.csv");

    // Освобождение памяти
    for (int i = 0; i <= N; i++) {
        free(u[i]);
        free(u_new[i]);
    }
    free(u);
    free(u_new);

    return 0;
}
