#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <iomanip>
#include <functional> // Для std::hash
#include "timer.h"    // Подключаем ваш файл timer.h

void compute_pi(long trials, std::atomic<long>& hits) {
    // Инициализируем генератор случайных чисел с уникальным зерном
    unsigned int seed = static_cast<unsigned int>(
        std::chrono::system_clock::now().time_since_epoch().count() +
        std::hash<std::thread::id>{}(std::this_thread::get_id())
    );

    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> distribution(-1.0, 1.0);

    long local_hits = 0;

    for (long i = 0; i < trials; ++i) {
        double x = distribution(generator);
        double y = distribution(generator);

        if (x * x + y * y <= 1.0) {
            ++local_hits;
        }
    }

    hits += local_hits;
}

int main(int argc, char* argv[]) {
    // Измерение времени начала работы программы
    double start_time, end_time;
    GET_TIME(start_time);

    // if (argc != 3) {
    //     std::cerr << "Использование: ./program nthreads ntrials" << std::endl;
    //     return 1;
    // }

    // int nthreads = std::stoi(argv[1]);
    // long ntrials = std::stol(argv[2]);

    int nthreads = 100;
    long ntrials = 1000000;

    long trials_per_thread = ntrials / nthreads;

    std::atomic<long> total_hits(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < nthreads; ++i) {
        long trials = trials_per_thread;

        if (i == nthreads - 1) {
            trials += ntrials % nthreads;
        }

        threads.emplace_back(compute_pi, trials, std::ref(total_hits));
    }

    for (auto& t : threads) {
        t.join();
    }

    // Измерение времени окончания работы программы
    GET_TIME(end_time);

    double pi_estimate = 4.0 * static_cast<double>(total_hits) / static_cast<double>(ntrials);
    std::cout << "nthreads: " << nthreads << std::endl;
    std::cout << "ntrials: " << ntrials << std::endl;
    std::cout << "Estimated π = " << std::setprecision(15) << pi_estimate << std::endl;

    // Вывод времени выполнения программы
    double elapsed_time = end_time - start_time;
    std::cout << "Execution time: " << elapsed_time << " seconds." << std::endl;

    return 0;
}
