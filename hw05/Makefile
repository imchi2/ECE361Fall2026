# hw05 - ECE 361 homework 5, the combination lock as a table-driven FSM.
#
# Build only under WSL (Ubuntu), never with the native Windows gcc:
#
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make test"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make bonus"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make clean"
#
# What the flags mean:
#     -std=c11        required by the assignment; the code itself only uses
#                     features that existed by C99
#     -Wall -Wextra   warnings, and the build must stay warning-free
#     -g3 -O0         debug information and no optimisation, so a debugger
#                     shows real variables and exact line numbers
#     -MMD -MP        write a .d file per object, listing the headers it
#                     used, so editing a header rebuilds that object
#     -I.             add hw05/ itself to the header search path, so
#                     tests/test_lock.c can still #include "lock.h"

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g3 -O0 -I. -MMD -MP

# Every .c file of this homework, as an explicit list rather than a
# wildcard, for readability: the machine, the program that drives it,
# the bonus checker, and the two test programs. `make` compiles each
# one to its own .o file.
SRCS    := main.c lock.c check.c tests/test_lock.c tests/test_check.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

# The three programs the Makefile can link.
LOCK_BIN  := lock
TEST_BIN  := tests/test_lock
CHECK_BIN := tests/test_check

.PHONY: all test bonus clean

# Default target: link the lock program, and compile every source file
# to its own object so a plain `make` still catches a warning in the
# test programs and the bonus checker, not just in the program itself.
all: $(LOCK_BIN) $(OBJS)

# Pattern rule: main.c becomes main.o, and tests/test_lock.c becomes
# tests/test_lock.o, and so on.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the lock program from its two objects. `$^` expands to the list
# of prerequisites above, so this reads: gcc <flags> main.o lock.o -o lock
$(LOCK_BIN): main.o lock.o
	$(CC) $(CFLAGS) $^ -o $@

# Link the main test program from the tests object and the machine.
$(TEST_BIN): tests/test_lock.o lock.o
	$(CC) $(CFLAGS) $^ -o $@

# Build the tests and run them. The program exits nonzero if any test
# fails, which makes `make test` fail too.
test: $(TEST_BIN)
	./$(TEST_BIN)

# The bonus: link the checker tests, then run them. Same nonzero-exit
# rule as `make test`.
$(CHECK_BIN): tests/test_check.o lock.o check.o
	$(CC) $(CFLAGS) $^ -o $@

bonus: $(CHECK_BIN)
	./$(CHECK_BIN)

# Remove everything the build produced, including the three programs.
# "rm -f" ignores files that are not there, so this works on a fresh
# checkout as well as after a build.
clean:
	rm -f $(OBJS) $(DEPS) $(LOCK_BIN) $(TEST_BIN) $(CHECK_BIN)

# Read back the header dependencies written by -MMD. They are included
# last so the rules above are already known when they load.
-include $(DEPS)
