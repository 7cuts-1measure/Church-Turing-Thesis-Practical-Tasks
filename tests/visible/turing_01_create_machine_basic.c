#include "../helpers/turing_helpers.h"

int main(void) {
    TuringMachine *tm = create_machine("01");
    CHECK_TEST(tm != NULL, "create_machine returned NULL");
    CHECK_TEST(tm->tape != NULL, "tape is NULL");

    char *content = get_tape_content(tm);
    CHECK_TEST(content != NULL, "failed to read tape");
    CHECK_TEST(strcmp(content, "01") == 0, "unexpected tape content");

    free(content);
    destroy_machine(tm);
    return 0;
}
