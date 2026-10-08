# hw02 - ECE 361 homework 2, splitting a working program into modules.
#
# Build only under WSL (Ubuntu), never with the native Windows gcc:
#
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw02 && make"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw02 && make test"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw02 && make clean"
#
# What the flags mean:
#     -std=c11        the flag the assignment specifies; the code itself only
#                     uses features that existed by C99
#     -Wall -Wextra   warnings, and the build must stay warning-free
#     -g3 -O0         debug information and no optimisation, so a debugger
#                     shows real variables and exact line numbers
#     -MMD -MP        write a .d file per object, listing the headers it
#                     used, so editing a header rebuilds that object
#     -I.             add hw02/ itself to the header search path

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g3 -O0 -I. -MMD -MP

# Every .c file of this homework, as an explicit list rather than a
# wildcard, for readability: the driver plus the four modules it calls.
# `make` compiles each one to its own .o file.
SRCS    := readings.c input.c stats.c run.c histogram.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

# The linked program that `make test` runs. regress.sh also expects this
# name in the current directory, so it must stay "readings".
BIN     := readings

.PHONY: all test clean

# Default target: compile every source file to its own object file, then
# link them into the program.
all: $(BIN)

# Pattern rule: readings.c becomes readings.o, input.c becomes input.o,
# and so on.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the program from every object file. `$^` expands to the list of
# prerequisites above, so this reads: gcc <flags> readings.o -o readings
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Build and run the regression test. regress.sh diffs every output against
# the golden files in expected/ and exits nonzero on any difference, which
# makes `make test` fail too.
test: $(BIN)
	sh regress.sh

# Remove everything the build produced. "rm -f" ignores files that are not
# there, so this works on a fresh checkout as well as after a build.
clean:
	rm -f $(OBJS) $(DEPS) $(BIN)

# Read back the header dependencies written by -MMD. They are included last
# so the rules above are already known when they load.
-include $(DEPS)
