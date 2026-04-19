#include "../helpers/turing_helpers.h"

int main(void) {
    TuringMachine *tm = create_machine("0101");
    CHECK_TEST(tm != NULL, "create_machine returned NULL");
    run_machine(tm);

    char *result = get_tape_content(tm);
    CHECK_TEST(result != NULL, "failed to read result");
    CHECK_TEST(strcmp(result, "0101") == 0, "expected unchanged tape");

    free(result);
    destroy_machine(tm);
    return 0;
}
