#ifndef TESTS_HELPERS_TURING_HELPERS_H
#define TESTS_HELPERS_TURING_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "machine_turing.h"

#define CHECK_TEST(cond, msg)         \
    do {                              \
        if (!(cond)) {                \
            fprintf(stderr, "%s\n", msg); \
            return 1;                 \
        }                             \
    } while (0)

static char *get_tape_content(const TuringMachine *tm) {
    if (tm == NULL || tm->tape == NULL || tm->tape_size <= 0) {
        return NULL;
    }

    int start = 0;
    int end = tm->tape_size - 1;

    while (start < tm->tape_size && tm->tape[start] == '_') {
        start++;
    }
    while (end >= 0 && tm->tape[end] == '_') {
        end--;
    }

    if (start > end) {
        char *empty = (char *)malloc(1);
        if (empty == NULL) {
            return NULL;
        }
        empty[0] = '\0';
        return empty;
    }

    int len = end - start + 1;
    char *result = (char *)malloc((size_t)len + 1);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, tm->tape + start, (size_t)len);
    result[len] = '\0';
    return result;
}

static int configure_inverter_machine(TuringMachine *tm) {
    if (tm == NULL) {
        return 0;
    }

    const char candidates[] = { '0', 's', 'S', 'q', 'Q', 'I' };
    int n = (int)(sizeof(candidates) / sizeof(candidates[0]));

    for (int i = 0; i < n; i++) {
        add_transition(tm, candidates[i], '0', '1', 'R', candidates[i]);
        add_transition(tm, candidates[i], '1', '0', 'R', candidates[i]);
        add_transition(tm, candidates[i], '_', '_', 'N', 'A');
    }

    return 1;
}

static int configure_replace_0_with_1_machine(TuringMachine *tm) {
    if (tm == NULL) {
        return 0;
    }

    const char candidates[] = { '0', 's', 'S', 'q', 'Q', 'I' };
    int n = (int)(sizeof(candidates) / sizeof(candidates[0]));

    for (int i = 0; i < n; i++) {
        add_transition(tm, candidates[i], '0', '1', 'R', candidates[i]);
        add_transition(tm, candidates[i], '1', '1', 'R', candidates[i]);
        add_transition(tm, candidates[i], '_', '_', 'N', 'A');
    }

    return 1;
}

static int configure_move_right_to_end_machine(TuringMachine *tm) {
    if (tm == NULL) {
        return 0;
    }

    const char candidates[] = { '0', 's', 'S', 'q', 'Q', 'I' };
    int n = (int)(sizeof(candidates) / sizeof(candidates[0]));

    for (int i = 0; i < n; i++) {
        add_transition(tm, candidates[i], '0', '0', 'R', candidates[i]);
        add_transition(tm, candidates[i], '1', '1', 'R', candidates[i]);
        add_transition(tm, candidates[i], '_', '_', 'N', 'A');
    }

    return 1;
}

#endif
