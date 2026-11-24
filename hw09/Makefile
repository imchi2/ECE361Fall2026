# hw09 - ECE 361 homework 9: a counting hash table (ht.h/ht.c) with
# separate chaining, and its tests.
#
# Build only under WSL (Ubuntu), never with the native Windows gcc:
#
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw09 && make"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw09 && make test"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw09 && make memcheck"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw09 && make clean"
#
# What the flags mean:
#     -std=c11        required by the assignment; the code itself only
#                     uses features that existed by C99
#     -Wall -Wextra   warnings, and the build must stay warning-free
#     -g3 -O0         debug information and no optimisation, so a
#                     debugger shows real variables and exact lines
#     -MMD -MP        write a .d file per object, listing the headers
#                     it used - each .o therefore knows the headers it
#                     includes, and editing a header rebuilds that
#                     object
#     -I.             add hw09/ itself to the header search path, so
#                     tests/test_ht.c can still #include "ht.h"

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g3 -O0 -I. -MMD -MP

# Every .c file of this homework, as an explicit list rather than a
# wildcard, for readability: the ADT and its test program. `make`
# compiles each one to its own .o file.
SRCS    := ht.c tests/test_ht.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

# The one program the Makefile links.
TEST_BIN := tests/test_ht

.PHONY: all test memcheck clean

# Default target: build the test program. ht.o and tests/test_ht.o are
# built along the way, so a plain `make` still catches a warning in
# every source file.
all: $(TEST_BIN)

# Pattern rule: ht.c becomes ht.o, tests/test_ht.c becomes
# tests/test_ht.o, and so on.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the unit tests from the test object and the ADT object.
# `$^` expands to the list of prerequisites above, so this reads:
# gcc <flags> tests/test_ht.o ht.o -o tests/test_ht
$(TEST_BIN): tests/test_ht.o ht.o
	$(CC) $(CFLAGS) $^ -o $@

# Run the unit tests. The program prints PASS or FAIL per check and a
# summary, and exits nonzero if anything failed, which stops make.
test: $(TEST_BIN)
	./$(TEST_BIN)

# Rebuild everything with the address and undefined-behaviour
# sanitizers, then run the tests again: a double free, a use after
# free, a bad read or a leak is reported and fails the run. This finds
# leaks on Linux only; on macOS use instead:
#     leaks --atExit -- ./tests/test_ht
memcheck:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined" test

# Remove everything the build produced, including the test program.
# "rm -f" ignores files that are not there, so this works on a fresh
# checkout as well as after a build.
clean:
	rm -f $(OBJS) $(DEPS) $(TEST_BIN)

# Read back the header dependencies written by -MMD. They are included
# last so the rules above are already known when they load.
-include $(DEPS)
