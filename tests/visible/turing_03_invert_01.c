#include "../helpers/turing_helpers.h"

int main(void) {
    TuringMachine *tm = create_machine("01");
    CHECK_TEST(tm != NULL, "create_machine returned NULL");
    CHECK_TEST(configure_inverter_machine(tm), "configure failed");
    run_machine(tm);

    char *result = get_tape_content(tm);
    CHECK_TEST(result != NULL, "failed to read result");
    CHECK_TEST(strcmp(result, "10") == 0, "expected 10");

    free(result);
    destroy_machine(tm);
    return 0;
}
