# hw01 - ECE 361 homework 1, bit-manipulation library.
#
# Build only under WSL (Ubuntu), never with the native Windows gcc:
#
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make test"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make clean"
#
# What the flags mean:
#     -std=c11        required by the assignment; the code itself only uses
#                     features that existed by C99
#     -Wall -Wextra   warnings, and the build must stay warning-free
#     -g3 -O0         debug information and no optimisation, so a debugger
#                     shows real variables and exact line numbers
#     -MMD -MP        write a .d file per object, listing the headers it
#                     used, so editing a header rebuilds that object

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g3 -O0 -MMD -MP

# The source files of the library: bits.c from Part 2, status.c from
# Part 3. Kept as an explicit list rather than a wildcard, for readability.
SRCS    := bits.c status.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

# The test program lives in its own folder.
TEST_BIN   := tests/test_bits
TEST_OBJ   := tests/test_bits.o
TEST_DEP   := tests/test_bits.d

.PHONY: all test clean

# Default target: compile every source file to its own object file.
all: $(OBJS)

# Pattern rule: bits.c becomes bits.o, status.c becomes status.o, and so on.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the test program against the library objects.
$(TEST_BIN): $(TEST_OBJ) $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Build the tests and run them. The program exits nonzero if any test fails,
# which makes `make test` fail too.
test: $(TEST_BIN)
	./$(TEST_BIN)

# Remove everything the build produced. "rm -f" ignores files that are not
# there, so this works on a fresh checkout as well as after a build.
clean:
	rm -f $(OBJS) $(DEPS) $(TEST_OBJ) $(TEST_DEP) $(TEST_BIN)

# Read back the header dependencies written by -MMD. They are included last
# so the rules above are already known when they load.
-include $(DEPS)
-include $(TEST_DEP)
