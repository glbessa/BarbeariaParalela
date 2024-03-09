#ifndef MPMC_H

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct mpmc_channel mpmp_channel;
struct mpmc_channel {
    int buffer_size;
    int pos;

    sem_t fulls;
    sem_t empties;
    pthread_mutex_t mutex;
    void **buffer;
};

mpmc_channel *mpmc_create_channel(int buffer_size) {
    mpmc_channel *channel = (mpmc_channel *) malloc(sizeof(mpmc_channel));
    if (channel == NULL) {
        return NULL;
    }

    channel->buffer_size = buffer_size;
    channel->pos = 0;
    channel->buffer = (void **) malloc(sizeof(void *) * buffer_size);
    if (channel->buffer == NULL) {
        free(channel);
        return NULL;
    }

    for (int i = 0; i < buffer_size; i++) {
        *(channel->buffer + i) = NULL;
    }

    sem_init(&channel->fulls, 0, 0);
    sem_init(&channel->empties, 0, buffer_size);

    return NULL;
}

void *mpmc_receive(mpmc_channel *channel) {
    sem_wait(&channel->fulls);

    pthread_mutex_lock(&(channel->mutex));
    void *data = *(channel->buffer + channel->pos);
    channel->pos = (channel->pos - 1) % channel->buffer_size;
    pthread_mutex_unlock(&channel->mutex);

    sem_post(&channel->empties);

    return data;
}

void *mpmc_tryreceive(mpmc_channel *channel) {
    if (sem_trywait(&channel->fulls) == 0) {
        pthread_mutex_lock(&(channel->mutex));
        void *data = *(channel->buffer + channel->pos);
        channel->pos = (channel->pos - 1) % channel->buffer_size;
        pthread_mutex_unlock(&channel->mutex);

        sem_post(&channel->empties);

        return data;
    }

    return NULL;
}

void mpmc_send(mpmc_channel *channel, void *data) {
    sem_wait(&channel->empties);

    pthread_mutex_lock(&(channel->mutex));
    *(channel->buffer + channel->pos) = data;
    channel->pos = (channel->pos + 1) % channel->buffer_size;
    pthread_mutex_unlock(&channel->mutex);

    sem_post(&channel->fulls);
}

void mpmc_trysend(mpmc_channel *channel, void *data) {
    if (sem_trywait(&channel->empties) == 0) {
        pthread_mutex_lock(&(channel->mutex));
        *(channel->buffer + channel->pos) = data;
        channel->pos = (channel->pos + 1) % channel->buffer_size;
        pthread_mutex_unlock(&channel->mutex);

        sem_post(&channel->fulls);
    }
}

void mpmc_destroy_channel(mpmc_channel *channel) {
    sem_destroy(&channel->fulls);
    sem_destroy(&channel->empties);
    free(channel->buffer);
    free(channel);
}

#endif