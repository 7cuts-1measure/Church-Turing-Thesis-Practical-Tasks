#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "machine_turing.h"

#define MAX_STEPS 10000000      // Ограничение на число шагов
#define INITIAL_TAPE_SIZE 256 // Начальный размер ленты


#define START_STATE 'q'
#define END_STATE '!'

#define INITIAL_TRANSITION_CAPACITY 5

TuringMachine* create_machine(const char *tape_initial) {
    TuringMachine *tm = (TuringMachine *) malloc(sizeof(TuringMachine));
    tm->tape                 = (char *) malloc(INITIAL_TAPE_SIZE);
    memset(tm->tape, '_', INITIAL_TAPE_SIZE);
    memcpy(tm->tape, tape_initial, INITIAL_TAPE_SIZE);
    tm->tape_size            = INITIAL_TAPE_SIZE;
    tm->head                 = 0;
    tm->transitions          = malloc(sizeof(Transition) * INITIAL_TAPE_SIZE);
    tm->current_state        = START_STATE;
    tm->num_transitions      = 0;
    tm->transitions_capacity = INITIAL_TRANSITION_CAPACITY;
    return tm;
}

void add_transition(TuringMachine *tm, char current_state, char read_symbol, 
                    char write_symbol, char direction, char next_state) {
    Transition tr = {
        .current_state = current_state,
        .read_symbol   = read_symbol,
        .write_symbol  = write_symbol,
        .direction     = direction,
        .next_state    = next_state
    };
    
    if (tm->num_transitions >= tm->transitions_capacity) {
        tm->transitions_capacity *= 2;
        tm->transitions = (Transition *) realloc(tm->transitions, sizeof(Transition) * tm->transitions_capacity);
    }
    tm->transitions[tm->num_transitions++] = tr;
}


Transition current_transition(TuringMachine *tm) {
    Transition tr;
    for (int i = 0; i < tm->num_transitions; ++i) {
        tr = tm->transitions[i];
        if (tr.current_state == tm->current_state
            && tr.read_symbol == tm->tape[tm->head]) {
            return tr;
        }
    }
    return (Transition){0,0,0,0,0};
}

void apply_transition(TuringMachine *tm, Transition tr) {
    tm->tape[tm->head] = tr.write_symbol;
    tm->head = tr.direction == 'L' ? tm->head - 1 :
               tr.direction == 'R' ? tm->head + 1 : tm->head;

    if (tm->head > tm->tape_size) {
        tm->tape = realloc(tm->tape, tm->tape_size * 2 * sizeof(char));
    }
    tm->current_state = tr.next_state;
}


void run_machine(TuringMachine *tm) {
    for (int i = 0; i < MAX_STEPS; i++) {
        Transition tr = current_transition(tm);
        if (tr.current_state == END_STATE) 
            break;
        apply_transition(tm, tr);
    }
}

void destroy_machine(TuringMachine *tm) {
    free(tm->transitions);
    free(tm->tape);
    memset(tm, 0, sizeof(TuringMachine));
    free(tm);
}

char *cstr2tape(const char *cstr) {
    size_t sz = strlen(cstr) + 4;
    char *tape = malloc(sz + 1);
    tape[sz] = '\0'; 
    sprintf(tape, "__%s__", cstr);
    return tape;
}

char *tape2cstr(const char *tape) {
    int start, end;
    for (int i = 0; ; i++) {
        if (tape[i] != '_') {
            start = i;
            break;
        }
    }

    for (int i = start; ;i++) {
        if (tape[i] == '_') {
            end = i;
            break;
        } 
    }

    size_t len = end - start;
    char *str = malloc(len + 1);
    memcpy(str, tape + start, len);
    str[len] = '\0';
    return str;
}

char* process_string(const char *input) {
    int tape_len = strlen(input) + 4;
    char * tape = cstr2tape(input);
    if (tape_len < INITIAL_TAPE_SIZE) {
        tape = realloc(tape, INITIAL_TAPE_SIZE);
        memset(tape + tape_len, '_', INITIAL_TAPE_SIZE - tape_len);
    }

    TuringMachine *tm = create_machine(tape);
    tm->head = 2;
//add_transition(tm,   curr_st ,rd_sym, wr_sym, direction, next_state)
    add_transition(tm, 'q', 'a', 'a', 'R', 'q');
    add_transition(tm, 'q', 'b', 'b', 'R', 'q');
    add_transition(tm, 'q', '_', '#', 'L', 'w');

    add_transition(tm, 'w', 'a', 'a', 'L', 'w');
    add_transition(tm, 'w', 'b', 'b', 'L', 'w');
    add_transition(tm, 'w', '#', '#', 'R', 'e');
    add_transition(tm, 'w', '_', '#', 'R', 'e'); 
    
    add_transition(tm, 'e', 'a', 'a', 'N', 't');
    add_transition(tm, 'e', 'b', 'b', 'N', 'i'); 
    add_transition(tm, 'e', '#', '#', 'N', 'p'); 
    
    add_transition(tm, 't', 'a', '#', 'L', 't');
    add_transition(tm, 't', '#', 'a', 'R', 'y'); 
    add_transition(tm, 't', '_', 'a', 'R', 'y');
    
    add_transition(tm, 'y', 'a', 'a', 'R', 'y'); 
    add_transition(tm, 'y', 'b', 'b', 'R', 'y');
    add_transition(tm, 'y', '#', '#', 'R', 'y'); 
    add_transition(tm, 'y', '_', 'a', 'L', 'u');
    
    add_transition(tm, 'u', 'a', 'a', 'L', 'u'); 
    add_transition(tm, 'u', 'b', 'b', 'L', 'u'); 
    add_transition(tm, 'u', '#', '#', 'L', 'w');

    add_transition(tm, 'i', 'b', '#', 'L', 'i'); 
    add_transition(tm, 'i', '#', 'b', 'R', 'o');
        
    add_transition(tm, 'o', 'a', 'a', 'R', 'o'); 
    add_transition(tm, 'o', 'b', 'b', 'R', 'o');
    add_transition(tm, 'o', '#', '#', 'R', 'o');
    add_transition(tm, 'o', '_', 'b', 'L', 'u');

    add_transition(tm, 'p', '#', '#', 'R', 'f'); 

    add_transition(tm, 'f', 'a', '#', 'L', 'f'); 
    add_transition(tm, 'f', 'b', 'b', 'N', 'g'); 
    add_transition(tm, 'f', '#', 'a', 'R', 'p');
    add_transition(tm, 'f', '_', '#', 'L', 'h'); 
    
    add_transition(tm, 'g', 'a', 'a', 'N', 'f');
    add_transition(tm, 'g', 'b', '#', 'L', 'g');
    add_transition(tm, 'g', '#', 'b', 'R', 'p');
    add_transition(tm, 'g', '_', '#', 'L', 'h');
    
    add_transition(tm, 'h', '#', '_', 'R', 'h');
    add_transition(tm, 'h', '_', '_', 'N', '!');

    run_machine(tm);
    char *result = tape2cstr(tm->tape);
    destroy_machine(tm);
    
    free(tape);
    return result;
}