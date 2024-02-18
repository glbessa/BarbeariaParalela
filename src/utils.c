#ifndef UTILS_H
#define UTILS_H

#include "utils.h"

#include <stdlib.h>
#include <time.h>

int get_cut_hair_duration() {
    return rand() % 4  + 3;
}

int get_next_round_clients_delay() {
    return rand() % 8 + 3;
}

int get_next_round_num_clients() {
    return rand() % 5 + 1;
}

void print_help() {

}




#endif