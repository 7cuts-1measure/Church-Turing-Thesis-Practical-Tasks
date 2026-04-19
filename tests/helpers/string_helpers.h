#ifndef TESTS_HELPERS_STRING_HELPERS_H
#define TESTS_HELPERS_STRING_HELPERS_H

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

static char *naive_double(const char *s) {
    if (s == NULL) {
        return NULL;
    }

    size_t len = strlen(s);
    char *result = (char *)malloc(len * 2 + 2);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, s, len);
    result[len] = '#';
    memcpy(result + len + 1, s, len);
    result[len * 2 + 1] = '\0';
    return result;
}

#endif
