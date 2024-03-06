#ifndef MPMC_H

#include <stdlib.h>
#include <semaphore.h>

typedef struct mpmc_channel mpmc_channel {
    int buffer_size;
    int in;
    int out;

    sem_t fulls;
    sem_t empties;
    pthread_mutex_t mutex;
    void *buffer;
};

mpmc_channel *mpmc_create_channel(int buffer_size) {
    return NULL;
}

#endif