CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -pedantic -I.

# Команда build должна собирать объектный файл решения.
build:
	$(CC) $(CFLAGS) -c machine_turing.c -o machine_turing.o

# Команда test должна собирать и запускать конкретный test runner.
# make test TEST_FILE=/path/to/test.c
test: build
	test -n "$(TEST_FILE)"
	$(CC) $(CFLAGS) "$(TEST_FILE)" machine_turing.o -o test_runner
	./test_runner
