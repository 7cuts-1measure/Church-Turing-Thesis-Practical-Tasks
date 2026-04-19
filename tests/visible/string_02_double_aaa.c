#include "../helpers/string_helpers.h"

int main(void) {
    char *result = process_string("aaa");
    CHECK_TEST(result != NULL, "process_string returned NULL");
    CHECK_TEST(strcmp(result, "aaa#aaa") == 0, "unexpected result");
    free(result);
    return 0;
}
