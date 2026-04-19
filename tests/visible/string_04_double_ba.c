#include "../helpers/string_helpers.h"

int main(void) {
    char *result = process_string("ba");
    CHECK_TEST(result != NULL, "process_string returned NULL");
    CHECK_TEST(strcmp(result, "ba#ba") == 0, "unexpected result");
    free(result);
    return 0;
}
