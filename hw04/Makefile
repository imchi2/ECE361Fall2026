# hw04 - ECE 361 homework 4, the tally ADT.
#
# Build only under WSL (Ubuntu), never with the native Windows gcc:
#
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw04 && make"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw04 && make test"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw04 && make clean"
#
# What the flags mean:
#     -std=c11        the flag the assignment specifies; the code itself only
#                     uses features that existed by C99
#     -Wall -Wextra   warnings, and the build must stay warning-free
#     -g3 -O0         debug information and no optimisation, so a debugger
#                     shows real variables and exact line numbers
#     -I.             add hw04/ itself to the header search path, so
#                     tests/test_tally.c can still #include "tally.h"
#     -MMD -MP        write a .d file per object, listing the headers it
#                     used, so editing a header rebuilds that object

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g3 -O0 -I. -MMD -MP

# The library from Part 1 and the program from Part 2, as an explicit
# list rather than a wildcard, for readability. `make` compiles each
# one to its own .o file, then links them into the events program.
SRCS    := tally.c events.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

# The program `make` produces and the assignment asks for.
BIN     := events

# The test program `make test` builds and runs. It links tally.o with
# tests/test_tally.o - tally.o is shared with events, compiled once.
TEST_BIN := tests/test_tally
TEST_OBJS := tally.o tests/test_tally.o
TEST_DEPS := $(TEST_OBJS:.o=.d)

.PHONY: all test clean

# Default target: the events program, which is what Part 2 asks
# `make` to build.
all: $(BIN)

# Link events from its objects. `$^` expands to the list of
# prerequisites: gcc <flags> tally.o events.o -o events
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Pattern rule: tally.c becomes tally.o, tests/test_tally.c becomes
# tests/test_tally.o, and so on. `$<` is the first prerequisite, `$@`
# the target.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the test program from its objects.
$(TEST_BIN): $(TEST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Build the tests and run them. The program exits nonzero if any test
# fails, which makes `make test` fail too.
test: $(TEST_BIN)
	./$(TEST_BIN)

# Remove everything the build produced: objects, dependency files, and
# both binaries. "rm -f" ignores files that are not there, so this
# works on a fresh checkout as well as after a build.
clean:
	rm -f $(OBJS) $(DEPS) tests/test_tally.o tests/test_tally.d $(BIN) $(TEST_BIN)

# Read back the header dependencies written by -MMD. They are included
# last so the rules above are already known when they load.
-include $(DEPS) $(TEST_DEPS)
