#include "../helpers/string_helpers.h"

int main(void) {
    char *result = process_string("aab");
    CHECK_TEST(result != NULL, "process_string returned NULL");
    CHECK_TEST(strcmp(result, "aab#aab") == 0, "unexpected result");
    free(result);
    return 0;
}
