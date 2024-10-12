#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Для функции sleep()

typedef struct {
    pthread_mutex_t lock;            // Мьютекс для защиты структуры
    pthread_cond_t readers_cond;     // Условная переменная для читателей
    pthread_cond_t writers_cond;     // Условная переменная для писателей
    int readers;                     // Счётчик активных читателей
    int writers;                     // Счётчик активных писателей
    int waiting_writers;             // Счётчик писателей, ожидающих доступа
} rwlock_t;

void rwlock_init(rwlock_t* rwlock) {
    pthread_mutex_init(&rwlock->lock, NULL);
    pthread_cond_init(&rwlock->readers_cond, NULL);
    pthread_cond_init(&rwlock->writers_cond, NULL);
    rwlock->readers = 0;
    rwlock->writers = 0;
    rwlock->waiting_writers = 0;
    printf("rwlock initialized\n");
}

void rwlock_destroy(rwlock_t* rwlock) {
    pthread_mutex_destroy(&rwlock->lock);
    pthread_cond_destroy(&rwlock->readers_cond);
    pthread_cond_destroy(&rwlock->writers_cond);
    printf("rwlock destroyed\n");
}

void rwlock_rlock(rwlock_t* rwlock) {
    pthread_mutex_lock(&rwlock->lock);
    while (rwlock->writers > 0 || rwlock->waiting_writers > 0) {
        printf("Reader waiting for writers to finish\n");
        pthread_cond_wait(&rwlock->readers_cond, &rwlock->lock);
    }
    rwlock->readers++;
    printf("Reader acquired read lock, active readers: %d\n", rwlock->readers);
    pthread_mutex_unlock(&rwlock->lock);
}

void rwlock_wlock(rwlock_t* rwlock) {
    pthread_mutex_lock(&rwlock->lock);
    rwlock->waiting_writers++;
    printf("Writer waiting, waiting writers: %d\n", rwlock->waiting_writers);
    while (rwlock->writers > 0 || rwlock->readers > 0) {
        pthread_cond_wait(&rwlock->writers_cond, &rwlock->lock);
    }
    rwlock->waiting_writers--;
    rwlock->writers++;
    printf("Writer acquired write lock\n");
    pthread_mutex_unlock(&rwlock->lock);
}

void rwlock_unlock(rwlock_t* rwlock) {
    pthread_mutex_lock(&rwlock->lock);
    if (rwlock->writers > 0) {
        rwlock->writers--;
        printf("Writer released write lock\n");
    } else if (rwlock->readers > 0) {
        rwlock->readers--;
        printf("Reader released read lock, active readers: %d\n", rwlock->readers);
    }

    if (rwlock->writers == 0 && rwlock->waiting_writers > 0) {
        printf("Signaling writer\n");
        pthread_cond_signal(&rwlock->writers_cond);
    } else if (rwlock->writers == 0 && rwlock->readers == 0) {
        printf("Broadcasting to all waiting readers\n");
        pthread_cond_broadcast(&rwlock->readers_cond);
    }
    pthread_mutex_unlock(&rwlock->lock);
}

// Пример использования read-write lock в потоках
void* reader(void* arg) {
    rwlock_t* rwlock = (rwlock_t*)arg;
    rwlock_rlock(rwlock);
    printf("Reader is reading\n");
    sleep(1); // Чтение занимает некоторое время
    rwlock_unlock(rwlock);
    return NULL;
}

void* writer(void* arg) {
    rwlock_t* rwlock = (rwlock_t*)arg;
    rwlock_wlock(rwlock);
    printf("Writer is writing\n");
    sleep(2); // Запись занимает некоторое время
    rwlock_unlock(rwlock);
    return NULL;
}

int main() {
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
