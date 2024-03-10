#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

// work on linux and windows :)
#if defined(_WIN32) || defined(_WIN64)
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
#define NUM_COMB_SCISSORS 0
#define NUM_MAX_CLIENTS 1000

void initialize();
void * client_action(void *args);
void * barber_action(void *args);

// create thread buffers
pthread_t barbers[NUM_BARBERS];
pthread_t clients[NUM_MAX_CLIENTS];
int num_clients = 0;
int num_clients_served = 0;
int num_abandoning_clients = 0;
pthread_mutex_t num_clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_clients_served_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_abandoning_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// create semaphores of resources
struct {
    pthread_mutex_t mutex;
    sem_t full;
    sem_t empty;

    int in;
    int out;
    int buffer[NUM_BARBERS];
} cutting_chairs;

sem_t waiting_chairs;

sem_t scissors;
sem_t combs;

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    srand(time(NULL)); // seed random number generator

    initialize();

    // get extra arguments
    int simulation_duration = atoi(argv[1]);
    int simulation_endtime = time(NULL) + simulation_duration;
    int simulation_timestep;

    printf("Initializing simulation of %i seconds...\n", simulation_duration);

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
        #if defined(_WIN32) || defined(_WIN64)
            Sleep(get_next_round_clients_delay() * 1000); // sleep in miliseconds
        #else
            sleep(get_next_round_clients_delay()); // sleep in seconds
        #endif

        simulation_timestep = time(NULL); // update actual time
    } while (simulation_timestep < simulation_endtime);

    print_stats(num_clients, num_clients_served, num_abandoning_clients);

    // destroy everything
    sem_destroy(&(cutting_chairs.full));
    sem_destroy(&(cutting_chairs.empty));
    sem_destroy(&(scissors));
    sem_destroy(&(combs));

    pthread_mutex_destroy(&num_clients_mutex);
    pthread_mutex_destroy(&(cutting_chairs.mutex));
    pthread_mutex_destroy(&num_clients_served_mutex);
    pthread_mutex_destroy(&num_abandoning_clients_mutex);

    return 0;
}

void initialize() {
    // initialize cutting_chairs
    pthread_mutex_init(&(cutting_chairs.mutex), NULL);
    cutting_chairs.in = 0;
    cutting_chairs.out = 0;
    sem_init(&(cutting_chairs.full), 0, 0);
    sem_init(&(cutting_chairs.empty), 0, NUM_BARBERS);

    // initialize all semaphores
    sem_init(&waiting_chairs, 0, NUM_WAIT_CHAIRS);
    sem_init(&scissors, 0, NUM_COMB_SCISSORS);
    sem_init(&combs, 0, NUM_COMB_SCISSORS);
}

// Producer
void * client_action(void *args) {
    int *id = (int *) args;

    if (sem_trywait(&(waiting_chairs)) == 0) {
        //Função de acesso a recurso de cadeira de espera
        printf("Client %i is waiting in the waiting chairs...\n", *id);
        fflush(stdout);

        sem_wait(&(cutting_chairs.empty));
        pthread_mutex_lock(&(cutting_chairs.mutex));
        *(cutting_chairs.buffer + cutting_chairs.in) = get_cut_hair_duration();
        if (cutting_chairs.in != 0)
            cutting_chairs.out += 1;
        cutting_chairs.in += 1;
        pthread_mutex_unlock(&(cutting_chairs.mutex));
        sem_post(&(cutting_chairs.full));

        sem_post(&waiting_chairs);

        printf("Client %i will cut its hair...\n", *id);
        fflush(stdout);
    } else {
        pthread_mutex_lock(&num_abandoning_clients_mutex);
        num_abandoning_clients += 1;
        pthread_mutex_unlock(&num_abandoning_clients_mutex);

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

    while (1) {
        printf("Barber %i is sleeping...\n", *id);
        fflush(stdout);

        sem_wait(&scissors); // get scissors
        sem_wait(&combs); // get comb

        sem_wait(&(cutting_chairs.full));
        pthread_mutex_lock(&(cutting_chairs.mutex)); // blocking producer-consumer
        cut_time = *(cutting_chairs.buffer + cutting_chairs.out); // get cut time generated by client
        cutting_chairs.in -= 1; // move producer's pointer
        if (cutting_chairs.in != 0)
            cutting_chairs.out -= 1; // move consumer's pointer
        pthread_mutex_unlock(&(cutting_chairs.mutex)); // unlock producer-consumer
        sem_post(&(cutting_chairs.empty)); // increase empty chairs

        printf("Barber %i is cutting the client hair for %i seconds...\n", *id, cut_time);
        fflush(stdout);

        #if defined(_WIN32) || defined(_WIN64)
            Sleep(cut_time * 1000); // sleep in miliseconds
        #else
            sleep(cut_time); // sleep in seconds
        #endif

        pthread_mutex_lock(&num_clients_served_mutex);
        num_clients_served += 1;
        pthread_mutex_unlock(&num_clients_served_mutex);

        sem_post(&scissors); // return scissors
        sem_post(&combs); // return comb
    }

    free(id);

    return NULL;
}