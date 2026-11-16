# hw07 - ECE 361 homework 7: a stack of int that grows, and a
# bracket checker that uses it.
#
# Build only under WSL (Ubuntu), never with the native Windows gcc:
#
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw07 && make"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw07 && make test"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw07 && make memcheck"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw07 && make clean"
#
# What the flags mean:
#     -std=c11        required by the assignment; the code itself only uses
#                     features that existed by C99
#     -Wall -Wextra   warnings, and the build must stay warning-free
#     -g3 -O0         debug information and no optimisation, so a debugger
#                     shows real variables and exact line numbers
#     -MMD -MP        write a .d file per object, listing the headers it
#                     used - each .o therefore knows the headers it
#                     includes, and editing a header rebuilds that object
#     -I.             add hw07/ itself to the header search path, so
#                     tests/test_istack.c can still #include "istack.h"

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g3 -O0 -I. -MMD -MP

# Every .c file of this homework, as an explicit list rather than a
# wildcard, for readability: the stack, the bracket checker, and the
# test program. `make` compiles each one to its own .o file.
SRCS    := istack.c balance.c tests/test_istack.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

# The two programs the Makefile can link.
BALANCE_BIN := balance
TEST_BIN    := tests/test_istack

.PHONY: all test memcheck clean

# Default target: link balance, and compile every source file to its own
# object so a plain `make` still catches a warning in the test program,
# not just in the balance tool.
all: $(BALANCE_BIN) $(OBJS)

# Pattern rule: istack.c becomes istack.o, and tests/test_istack.c
# becomes tests/test_istack.o, and so on.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link balance from the bracket checker and the stack it uses.
# `$^` expands to the list of prerequisites above, so this reads:
# gcc <flags> balance.o istack.o -o balance
$(BALANCE_BIN): balance.o istack.o
	$(CC) $(CFLAGS) $^ -o $@

# Link the unit tests WITHOUT balance.o: test_istack.c has its own main().
$(TEST_BIN): tests/test_istack.o istack.o
	$(CC) $(CFLAGS) $^ -o $@

# Run both tests: the unit tests first (they exit nonzero on a failure,
# which stops make before the regression test runs), then every file in
# inputs/ against expected/ in regress.sh.
test: $(TEST_BIN) $(BALANCE_BIN)
	./$(TEST_BIN)
	sh regress.sh

# Rebuild everything with the address and undefined-behaviour
# sanitizers, then run the tests again: a double free, a use after free
# or a leak is reported and fails the run. This finds leaks on Linux
# only; on macOS use instead:
#     leaks --atExit -- ./tests/test_istack
memcheck:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined" test

# Remove everything the build produced, including both programs.
# "rm -f" ignores files that are not there, so this works on a fresh
# checkout as well as after a build.
clean:
	rm -f $(OBJS) $(DEPS) $(BALANCE_BIN) $(TEST_BIN) regress_out.tmp

# Read back the header dependencies written by -MMD. They are included
# last so the rules above are already known when they load.
-include $(DEPS)
