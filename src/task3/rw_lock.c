#include "rw_lock.h"

void rwlock_init(rwlock_t* rwlock) {
    pthread_mutex_init(&rwlock->lock, NULL);
    pthread_cond_init(&rwlock->readers_cond, NULL);
    pthread_cond_init(&rwlock->writers_cond, NULL);
    rwlock->readers = 0;
    rwlock->writers = 0;
    rwlock->waiting_writers = 0;
}

void rwlock_destroy(rwlock_t* rwlock) {
    pthread_mutex_destroy(&rwlock->lock);
    pthread_cond_destroy(&rwlock->readers_cond);
    pthread_cond_destroy(&rwlock->writers_cond);
}

void rwlock_rdlock(rwlock_t* rwlock) {
    pthread_mutex_lock(&rwlock->lock);
    while (rwlock->writers > 0 || rwlock->waiting_writers > 0) {
        pthread_cond_wait(&rwlock->readers_cond, &rwlock->lock);
    }
    rwlock->readers++;
    pthread_mutex_unlock(&rwlock->lock);
}

void rwlock_wrlock(rwlock_t* rwlock) {
    pthread_mutex_lock(&rwlock->lock);
    rwlock->waiting_writers++;
    while (rwlock->writers > 0 || rwlock->readers > 0) {
        pthread_cond_wait(&rwlock->writers_cond, &rwlock->lock);
    }
    rwlock->waiting_writers--;
    rwlock->writers++;
    pthread_mutex_unlock(&rwlock->lock);
}

void rwlock_unlock(rwlock_t* rwlock) {
    pthread_mutex_lock(&rwlock->lock);
    if (rwlock->writers > 0) {
        rwlock->writers--;
    } else if (rwlock->readers > 0) {
        rwlock->readers--;
    }

    if (rwlock->writers == 0 && rwlock->waiting_writers > 0) {
        pthread_cond_signal(&rwlock->writers_cond);
    } else if (rwlock->writers == 0 && rwlock->readers == 0) {
        pthread_cond_broadcast(&rwlock->readers_cond);
    }
    pthread_mutex_unlock(&rwlock->lock);
}
