#include <stdlib.h>
#include <string.h>
#include "machine_turing.h"

#define MAX_STEPS 100000      // Ограничение на число шагов
#define INITIAL_TAPE_SIZE 256 // Начальный размер ленты


TuringMachine* create_machine(const char *tape_initial) {
   
}

void add_transition(TuringMachine *tm, char current_state, char read_symbol, 
                    char write_symbol, char direction, char next_state) {
   
}


void run_machine(TuringMachine *tm) {
}

void destroy_machine(TuringMachine *tm) {
}

char* process_string(const char *input) {
    
}