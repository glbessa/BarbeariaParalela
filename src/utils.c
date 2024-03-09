#ifndef UTILS_H
#define UTILS_H

#include "utils.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int get_cut_hair_duration() {
    return rand() % 4 + 3;
}

int get_next_round_clients_delay() {
    return rand() % 8 + 3;
}

int get_next_round_num_clients() {
    return rand() % 5 + 1;
}

void print_help() {
    printf("Uso: barbearia_paralela TEMPO_SIMULACAO\nRealiza uma simulação de TEMPO_SIMULACAO segundos.\n");
}

void print_stats(int total_clients, int clients_served, int abandoning_clients) {
    printf("Número total de clientes: %i\nNúmero de clientes atendidos: %i\nNúmeros de clientes não atendidos: %i\n", total_clients, clients_served, abandoning_clients);
}



#endif