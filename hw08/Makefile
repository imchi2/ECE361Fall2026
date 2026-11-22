# hw08 - ECE 361 homework 8: porting TutorialsPoint's binary search
# tree into a small ADT (bst.h/bst.c), a tree-shape probe (treestats),
# and the bonus bst_remove.
#
# Build only under WSL (Ubuntu), never with the native Windows gcc:
#
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make test"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make memcheck"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make bonus"
#     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make clean"
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
#     -I.             add hw08/ itself to the header search path, so
#                     tests/test_bst.c can still #include "bst.h"

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g3 -O0 -I. -MMD -MP

# Every .c file of this homework, as an explicit list rather than a
# wildcard, for readability: the ADT, the tree-shape probe, and the
# two test programs. `make` compiles each one to its own .o file.
SRCS    := bst.c treestats.c tests/test_bst.c tests/test_remove.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

# The three programs the Makefile can link.
TREESTATS_BIN := treestats
TEST_BIN      := tests/test_bst
BONUS_BIN     := tests/test_remove

.PHONY: all test memcheck bonus clean

# Default target: link treestats, and compile every source file to its
# own object so a plain `make` still catches a warning in a test
# program, not just in treestats.
all: $(TREESTATS_BIN) $(OBJS)

# Pattern rule: bst.c becomes bst.o, tests/test_bst.c becomes
# tests/test_bst.o, and so on.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link treestats from the probe and the ADT it uses.
# `$^` expands to the list of prerequisites above, so this reads:
# gcc <flags> treestats.o bst.o -o treestats
$(TREESTATS_BIN): treestats.o bst.o
	$(CC) $(CFLAGS) $^ -o $@

# Link the unit tests WITHOUT treestats.o: test_bst.c has its own main().
$(TEST_BIN): tests/test_bst.o bst.o
	$(CC) $(CFLAGS) $^ -o $@

# The bonus tests, same pattern.
$(BONUS_BIN): tests/test_remove.o bst.o
	$(CC) $(CFLAGS) $^ -o $@

# Run the unit tests first (they exit nonzero on a failure, which
# stops make), then check treestats against the three outputs the spec
# fixes, with their exit statuses. `$$` is a literal $ for the shell,
# and each recipe line is one shell command joined by \.
test: $(TEST_BIN) $(TREESTATS_BIN)
	./$(TEST_BIN)
	@out=$$(echo "27 14 35 10 19 31 42" | ./$(TREESTATS_BIN)); status=$$?; \
	printf 'treestats seven keys: "%s" (exit %s)\n' "$$out" "$$status"; \
	test "$$out" = "n 7, height 3" && test "$$status" -eq 0
	@out=$$(printf "" | ./$(TREESTATS_BIN)); status=$$?; \
	printf 'treestats empty input: "%s" (exit %s)\n' "$$out" "$$status"; \
	test "$$out" = "empty tree" && test "$$status" -eq 0
	@out=$$(echo "1 2 3 4 x" | ./$(TREESTATS_BIN)); status=$$?; \
	printf 'treestats bad input:   "%s" (exit %s)\n' "$$out" "$$status"; \
	test "$$out" = "not a number after 4 values" && test "$$status" -eq 1

# Rebuild everything with the address and undefined-behaviour
# sanitizers, then run the tests again: a double free, a use after
# free, a bad read or a leak is reported and fails the run. This finds
# leaks on Linux only; on macOS use instead:
#     leaks --atExit -- ./tests/test_bst
memcheck:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined" test

# Bonus: rebuild with the sanitizers and run the bst_remove tests.
bonus:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined" $(BONUS_BIN)
	./$(BONUS_BIN)

# Remove everything the build produced, including all three programs.
# "rm -f" ignores files that are not there, so this works on a fresh
# checkout as well as after a build.
clean:
	rm -f $(OBJS) $(DEPS) $(TREESTATS_BIN) $(TEST_BIN) $(BONUS_BIN)

# Read back the header dependencies written by -MMD. They are included
# last so the rules above are already known when they load.
-include $(DEPS)
