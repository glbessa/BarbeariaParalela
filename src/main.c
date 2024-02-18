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
sem_t cut_chairs_barbers;
sem_t cut_chairs_clients;
sem_t wait_chairs;
sem_t comb_scissors;

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    srand(time(NULL)); // seed random number generator

    // initialize all semaphores
    sem_init(&cut_chairs_barbers, 0, 0);
    sem_init(&cut_chairs_clients, 0, NUM_BARBERS);
    sem_init(&wait_chairs, 0, NUM_WAIT_CHAIRS);
    sem_init(&comb_scissors, 0, NUM_COMB_SCISSORS);

    int simulation_duration = atoi(argv[1]);
    int simulation_endtime = time(NULL) + simulation_duration;
    int simulation_timestep;

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
            int *id = malloc(sizeof(int));
            *id = i;
            // create clients (threads) for this iteration
            pthread_mutex_lock(&num_clients_mutex); // num_clients has to be a mutex
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

void * client_action(void *args) {
    int *id = (int *) args;

    if (sem_trywait(&wait_chairs)) {
        //Função de acesso a recurso de cadeira de espera
        
        print("Client %i: Waiting in the waiting chairs...\n", *id);
        fflush(stdout);
        
        sem_wait(&cut_chairs_clients);
        sem_post(&wait_chairs);

        sem_post(&cut_chairs_barbers);

        printf("Client %i: Cutting the hair...\n", *id);
        fflush(stdout);

        // cutting the hair...
        #ifdef _WIN32 || _WIN64
            Sleep(get_cut_hair_duration() * 1000); // sleep in miliseconds
        #else
            sleep(get_cut_hair_duration()); // sleep in seconds
        #endif

        sem_post(&cut_chairs_clients);
    } else {
        print("The client %i left the barbershop!\n", *id);
        fflush(stdout);
    }
    
    // decrease number of clients
    pthread_mutex_lock(&num_clients_mutex);
    num_clients--;
    pthread_mutex_unlock(&num_clients_mutex);
    
    free(id);

    return NULL;
}

void * barber_action(void *args) {
    int *id = (int *) args;

    while (pthread_mutex_trylock(&simulation_status_mutex) == -1) {
        printf("Barber %i is sleeping...", *id);
        fflush(stdout);

        sem_wait(&cut_chairs_barbers);

        sem_wait(&comb_scissors);

        // problema: como o barbeiro vai saber quanto tempo vai levar para contar o cabelo do cliente?
        // possivel solucao: produtor-consumidor

        printf("Barber %i is cutting the client hair...\n", *id);
        fflush(stdout);

        sem_post(&comb_scissors);
    }

    free(id);

    return NULL;
}