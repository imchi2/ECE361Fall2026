# ECE 361 I/O module emulator, version 4.0
#   make        builds the demo
#   make test   builds and runs the self-checking tests, with the sanitizers
#   make clean  removes everything that was built
# Works with gcc on Linux and WSL, and with gcc (which is clang) on macOS.

CC      ?= gcc
CFLAGS  = -std=c11 -Wall -Wextra -pedantic -g
SAN     = -fsanitize=address,undefined -fno-omit-frame-pointer

all: demo_iom361_r4

demo_iom361_r4: demo_iom361_r4.c iom361_r4.c iom361_r4.h
	$(CC) $(CFLAGS) -o $@ demo_iom361_r4.c iom361_r4.c

test_iom361_r4: test_iom361_r4.c iom361_r4.c iom361_r4.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_iom361_r4.c iom361_r4.c -lm

test: test_iom361_r4
	./test_iom361_r4

clean:
	rm -rf demo_iom361_r4 test_iom361_r4 *.o *.dSYM

.PHONY: all test clean
