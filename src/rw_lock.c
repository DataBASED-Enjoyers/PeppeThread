#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Для функции sleep()
#include <string.h> // Для обработки аргументов командной строки

int verbose = 0; // Флаг для контроля логирования

typedef struct {
    pthread_mutex_t lock;            // Мьютекс для защиты структуры
    pthread_cond_t readers_cond;     // Условная переменная для читателей
    pthread_cond_t writers_cond;     // Условная переменная для писателей
    int readers;                     // Счётчик активных читателей
    int writers;                     // Счётчик активных писателей
    int waiting_writers;             // Счётчик писателей, ожидающих доступа
} rwlock_t;

void log_message(const char* message) {
    if (verbose) {
        printf("%s\n", message);
    }
}

void rwlock_init(rwlock_t* rwlock) {
    pthread_mutex_init(&rwlock->lock, NULL);
    pthread_cond_init(&rwlock->readers_cond, NULL);
    pthread_cond_init(&rwlock->writers_cond, NULL);
    rwlock->readers = 0;
    rwlock->writers = 0;
    rwlock->waiting_writers = 0;
    log_message("rwlock initialized");
}

void rwlock_destroy(rwlock_t* rwlock) {
    pthread_mutex_destroy(&rwlock->lock);
    pthread_cond_destroy(&rwlock->readers_cond);
    pthread_cond_destroy(&rwlock->writers_cond);
    log_message("rwlock destroyed");
}

void rwlock_rlock(rwlock_t* rwlock) {
    pthread_mutex_lock(&rwlock->lock);
    while (rwlock->writers > 0 || rwlock->waiting_writers > 0) {
        log_message("Reader waiting for writers to finish");
        pthread_cond_wait(&rwlock->readers_cond, &rwlock->lock);
    }
    rwlock->readers++;
    char buf[100];
    snprintf(buf, sizeof(buf), "Reader acquired read lock, active readers: %d", rwlock->readers);
    log_message(buf);
    pthread_mutex_unlock(&rwlock->lock);
}

void rwlock_wlock(rwlock_t* rwlock) {
    pthread_mutex_lock(&rwlock->lock);
    rwlock->waiting_writers++;
    char buf[100];
    snprintf(buf, sizeof(buf), "Writer waiting, waiting writers: %d", rwlock->waiting_writers);
    log_message(buf);
    while (rwlock->writers > 0 || rwlock->readers > 0) {
        pthread_cond_wait(&rwlock->writers_cond, &rwlock->lock);
    }
    rwlock->waiting_writers--;
    rwlock->writers++;
    log_message("Writer acquired write lock");
    pthread_mutex_unlock(&rwlock->lock);
}

void rwlock_unlock(rwlock_t* rwlock) {
    pthread_mutex_lock(&rwlock->lock);
    if (rwlock->writers > 0) {
        rwlock->writers--;
        log_message("Writer released write lock");
    } else if (rwlock->readers > 0) {
        rwlock->readers--;
        char buf[100];
        snprintf(buf, sizeof(buf), "Reader released read lock, active readers: %d", rwlock->readers);
        log_message(buf);
    }

    if (rwlock->writers == 0 && rwlock->waiting_writers > 0) {
        log_message("Signaling writer");
        pthread_cond_signal(&rwlock->writers_cond);
    } else if (rwlock->writers == 0 && rwlock->readers == 0) {
        log_message("Broadcasting to all waiting readers");
        pthread_cond_broadcast(&rwlock->readers_cond);
    }
    pthread_mutex_unlock(&rwlock->lock);
}

// Пример использования read-write lock в потоках
void* reader(void* arg) {
    rwlock_t* rwlock = (rwlock_t*)arg;
    rwlock_rlock(rwlock);
    log_message("Reader is reading");
    sleep(1); // Чтение занимает некоторое время
    rwlock_unlock(rwlock);
    return NULL;
}

void* writer(void* arg) {
    rwlock_t* rwlock = (rwlock_t*)arg;
    rwlock_wlock(rwlock);
    log_message("Writer is writing");
    sleep(2); // Запись занимает некоторое время
    rwlock_unlock(rwlock);
    return NULL;
}

int main(int argc, char* argv[]) {
    // Проверяем аргументы командной строки
    if (argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = 1; // Включаем логирование
    }

    pthread_t r1, r2, r3, w1, w2;
    rwlock_t rwlock;

    rwlock_init(&rwlock);

    // Создаем потоки читателей и писателей
    pthread_create(&r1, NULL, reader, &rwlock);
    pthread_create(&r2, NULL, reader, &rwlock);
    pthread_create(&w1, NULL, writer, &rwlock);
    pthread_create(&r3, NULL, reader, &rwlock);
    pthread_create(&w2, NULL, writer, &rwlock);

    // Ждем завершения потоков
    pthread_join(r1, NULL);
    pthread_join(r2, NULL);
    pthread_join(w1, NULL);
    pthread_join(r3, NULL);
    pthread_join(w2, NULL);

    rwlock_destroy(&rwlock);

    return 0;
}
