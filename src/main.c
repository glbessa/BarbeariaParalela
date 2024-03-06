#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

// work on linux and windows :)
#ifdef _WIN32 || _WIN64
    #include <Windows.h>
#else
    #include <unistd.h>
#endif

#include "utils.h"

// Quantidade de cadeiras de barbear: 8
// Quantidade de barbeiros: 8
// Quantidade de clientes: Máximo 5 clientes num intervalo de 1 até 10 segundos
// Quantidade de cadeiras da sala de espera: 16
// Quantidade de pentes: 4
// Quantidade de tesouras: 4
// Tempo de corte de cabelo: 3 - 6 segundos

// Threads: Barbeiros e clientes
// Recursos: Cadeiras de barbear e cadeiras da sala de espera

#define NUM_BARBERS 8
#define NUM_WAIT_CHAIRS 16
#define NUM_COMB_SCISSORS (NUM_BARBERS / 2)
#define NUM_MAX_CLIENTS 1000

void * client_action(void *args);
void * barber_action(void *args);

typedef struct ClientArgs ClientArgs;
struct ClientArgs {
    int *id;
};

// create thread buffers
pthread_mutex_t simulation_status_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t barbers[NUM_BARBERS];
pthread_t clients[NUM_MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t num_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// create semaphores of resources
struct {
    pthread_mutex_t mutex;
    sem_t full;
    sem_t empty;
    int in;
    int out;
    int buffer[NUM_WAIT_CHAIRS];
} wait_chairs;

sem_t scissors;
sem_t combs;

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    srand(time(NULL)); // seed random number generator

    // initialize wait_chairs
    pthread_mutex_init(&(wait_chairs.mutex), NULL);
    wait_chairs.in = 0;
    wait_chairs.out = 0;
    sem_init(&(wait_chairs.full), 0, 0);
    sem_init(&(wait_chairs.empty), 0, NUM_WAIT_CHAIRS);

    // initialize all semaphores
    sem_init(&scissors, 0, NUM_COMB_SCISSORS);
    sem_init(&combs, 0, NUM_COMB_SCISSORS);

    // get extra arguments
    int simulation_duration = atoi(argv[1]);
    int simulation_endtime = time(NULL) + simulation_duration;
    int simulation_timestep;

    printf("Initializing simulation of %i seconds...\n", simulation_duration);

    pthread_mutex_lock(&simulation_status_mutex);

    for (int i = 0; i < NUM_BARBERS; i++) {
        // create all barbers
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&barbers[i], NULL, barber_action, id);
    }    

    do {
        // all simulation is inside here
        for (int i = 0; i < get_next_round_num_clients(); i++) {
            pthread_mutex_lock(&num_clients_mutex); // num_clients has to be a mutex
            int *id = malloc(sizeof(int));
            *id = num_clients;
            pthread_create(&clients[num_clients++], NULL, client_action, id);
            pthread_mutex_unlock(&num_clients_mutex);
        }

        // wait some time for the next iteration
        #ifdef _WIN32 || _WIN64
            Sleep(get_next_round_clients_delay() * 1000); // sleep in miliseconds
        #else
            sleep(get_next_round_clients_delay()); // sleep in seconds
        #endif

        simulation_timestep = time(NULL); // update actual time
    } while (simulation_timestep < simulation_endtime);

    // wait all threads finish their actions
    for (int i = 0; i < NUM_BARBERS; i++) {
        // wait all clients
        pthread_join(barbers[i], NULL);
    }

    pthread_mutex_unlock(&simulation_status_mutex);

    for (int i = 0; i < NUM_BARBERS; i++) {
        // wait all barbers
        pthread_join(barbers[i], NULL);
    }

    // destroy everything

    return 0;
}

// Producer
void * client_action(void *args) {
    int *id = (int *) args;
    int cut_time;
    
    if (sem_trywait(&wait_chairs)) {
        //Função de acesso a recurso de cadeira de espera
        printf("Client %i: Waiting in the waiting chairs...\n", *id);
        fflush(stdout);

        cut_time = get_cut_hair_duration();

        pthread_mutex_lock(&(wait_chairs.mutex));
        while (sem_trywait(&(wait_chairs.full)) != 0) {
            pthread_mutex_unlock(&(wait_chairs.mutex));
            #ifdef _WIN32 || _WIN64
                Sleep(1 * 1000); // sleep in miliseconds
            #else
                sleep(1); // sleep in seconds
            #endif
            pthread_mutex_lock(&(wait_chairs.mutex));
        } // decrease occupied chairs
        sem_wait(&(wait_chairs.empty));
        wait_chairs.buffer[wait_chairs.in];
        wait_chairs.in += 1;
        sem_post(&(wait_chairs.full));
        pthread_mutex_unlock(&(wait_chairs.mutex));

        printf("Client %i: Cutting the hair...\n", *id);
        fflush(stdout);

    } else {
        printf("The client %i has left the barbershop!\n", *id);
        fflush(stdout);
    }
    
    free(id);

    return NULL;
}

// Consumer
void * barber_action(void *args) {
    int *id = (int *) args;
    int cut_time;

    while (pthread_mutex_trylock(&simulation_status_mutex) == -1) {
        printf("Barber %i is sleeping...", *id);
        fflush(stdout);

        pthread_mutex_lock(&(wait_chairs.mutex)); // blocking producer-consumer
        while (sem_trywait(&(wait_chairs.full)) != 0) {
            pthread_mutex_unlock(&(wait_chairs.mutex));
            #ifdef _WIN32 || _WIN64
                Sleep(1 * 1000); // sleep in miliseconds
            #else
                sleep(1); // sleep in seconds
            #endif
            pthread_mutex_lock(&(wait_chairs.mutex));
        } // decrease occupied chairs
        cut_time = wait_chairs.buffer[wait_chairs.out]; // get cut time generated by client
        wait_chairs.out -= 1; // move consumer's pointer
        sem_post(&(wait_chairs.empty)); // increase empty chairs
        pthread_mutex_unlock(&(wait_chairs.mutex)); // unlock producer-consumer

        printf("Chegou aqui!");

        sem_wait(&scissors); // get scissors
        sem_wait(&combs); // get comb

        printf("Barber %i is cutting the client hair for %i seconds...\n", *id, cut_time);
        fflush(stdout);

        #ifdef _WIN32 || _WIN64
            Sleep(cut_time * 1000); // sleep in miliseconds
        #else
            sleep(cut_time); // sleep in seconds
        #endif

        sem_post(&scissors); // return scissors
        sem_post(&combs); // return comb
    }

    free(id);

    return NULL;
}