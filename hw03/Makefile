# hw03 - ECE 361 homework 3, a pointer library.
#
# Build only under WSL (Ubuntu), never with the native Windows gcc:
#
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw03 && make"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw03 && make test"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw03 && make clean"
#
# What the flags mean:
#     -std=c11        the flag the assignment specifies; the code itself only
#                     uses features that existed by C99
#     -Wall -Wextra   warnings, and the build must stay warning-free
#     -g3 -O0         debug information and no optimisation, so a debugger
#                     shows real variables and exact line numbers
#     -MMD -MP        write a .d file per object, listing the headers it
#                     used, so editing a header rebuilds that object
#     -I.             add hw03/ itself to the header search path, so
#                     tests/test_ptr.c can still #include "ptr.h"

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g3 -O0 -I. -MMD -MP

# Every .c file of this homework, as an explicit list rather than a
# wildcard, for readability: the library from Part 1 and the test program
# from Part 2. `make` compiles each one to its own .o file.
SRCS    := ptr.c tests/test_ptr.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

# The linked test program that `make test` runs.
TEST_BIN := tests/test_ptr

.PHONY: all test clean

# Default target: ptr.o (the library on its own) plus the test program,
# which is what Part 2 asks `make` to build.
all: ptr.o $(TEST_BIN)

# Pattern rule: ptr.c becomes ptr.o, tests/test_ptr.c becomes
# tests/test_ptr.o, and so on. `$<` is the first prerequisite, `$@` the
# target.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the test program from every object file. `$^` expands to the list
# of prerequisites above, so this reads: gcc <flags> ptr.o
# tests/test_ptr.o -o tests/test_ptr
$(TEST_BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Build the tests and run them. The program exits nonzero if any test
# fails, which makes `make test` fail too.
test: $(TEST_BIN)
	./$(TEST_BIN)

# Remove everything the build produced. "rm -f" ignores files that are not
# there, so this works on a fresh checkout as well as after a build.
clean:
	rm -f $(OBJS) $(DEPS) $(TEST_BIN)

# Read back the header dependencies written by -MMD. They are included last
# so the rules above are already known when they load.
-include $(DEPS)
