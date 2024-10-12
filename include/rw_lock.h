#ifndef _RW_LOCK_H_
#define _RW_LOCK_H_

#include <pthread.h>

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t readers_cond;
    pthread_cond_t writers_cond;
    int readers;
    int writers;
    int waiting_writers;
} rwlock_t;

void log_message(const char* message);
void rwlock_init(rwlock_t* rwlock);

void rwlock_destroy(rwlock_t* rwlock);

void rwlock_rdlock(rwlock_t* rwlock);
void rwlock_wrlock(rwlock_t* rwlock);

void rwlock_unlock(rwlock_t* rwlock);
#endif