# hw06 - ECE 361 homework 6: replay a scenario file into a trace.
#
# Build only under WSL (Ubuntu), never with the native Windows gcc:
#
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make test"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make clean"
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
#     -I.             add hw06/ itself to the header search path, so
#                     tests/test_hw06.c can still #include "sensor.h"

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g3 -O0 -I. -MMD -MP

# Every .c file of this homework, as an explicit list rather than a
# wildcard, for readability: the three modules, the replay program, the
# provided iom361 library, and the test program. `make` compiles each
# one to its own .o file.
SRCS    := sensor.c panel.c scenario.c main.c iom361_r4.c tests/test_hw06.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

# The two programs the Makefile can link.
REPLAY_BIN := replay
TEST_BIN   := tests/test_hw06

.PHONY: all test clean

# Default target: link replay, and compile every source file to its own
# object so a plain `make` still catches a warning in the test program,
# not just in the replay tool.
all: $(REPLAY_BIN) $(OBJS)

# Pattern rule: sensor.c becomes sensor.o, and tests/test_hw06.c
# becomes tests/test_hw06.o, and so on.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link replay from the three modules, main, and the iom361 library.
# `$^` expands to the list of prerequisites above, so this reads:
# gcc <flags> sensor.o panel.o scenario.o main.o iom361_r4.o -o replay
$(REPLAY_BIN): sensor.o panel.o scenario.o main.o iom361_r4.o
	$(CC) $(CFLAGS) $^ -o $@

# Link the unit tests WITHOUT main.o: test_hw06.c has its own main().
$(TEST_BIN): tests/test_hw06.o sensor.o panel.o scenario.o iom361_r4.o
	$(CC) $(CFLAGS) $^ -o $@

# Run the unit tests first: they exit nonzero on a failure, which stops
# make before the regression test runs. Then replay every scenario in
# scenarios/ and diff each trace against expected/, as in HW2.
test: $(TEST_BIN) $(REPLAY_BIN)
	./$(TEST_BIN)
	sh regress.sh

# Remove everything the build produced, including both programs.
# "rm -f" ignores files that are not there, so this works on a fresh
# checkout as well as after a build.
clean:
	rm -f $(OBJS) $(DEPS) $(REPLAY_BIN) $(TEST_BIN)

# Read back the header dependencies written by -MMD. They are included
# last so the rules above are already known when they load.
-include $(DEPS)
