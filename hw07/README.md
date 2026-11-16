# hw07 — the istack ADT and a bracket checker

ECE 361, Fall 2026. Three pieces: `istack`, a stack of `int` that grows
on the heap (`malloc`, `realloc`, `free`); `balance`, a bracket checker
that uses it; and tests plus a memory check that catch bugs in both.

## Build and test

Everything is built and run under WSL (Ubuntu), never with the native
Windows gcc:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw07 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw07 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw07 && make memcheck"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw07 && make clean"
```

- `make` builds `balance` (and compiles every `.c` to its own `.o`),
  with no warnings under `-std=c11 -Wall -Wextra`.
- `make test` runs both tests: the unit tests in `tests/test_istack.c`
  (PASS/FAIL per check, a summary, nonzero exit on failure), then
  `regress.sh`, which runs `./balance` on every file in `inputs/` and
  compares both the output and the exit status with `expected/`.
- `make memcheck` rebuilds everything with
  `-fsanitize=address,undefined` and runs `make test` again. It finds
  leaks on Linux only; on macOS use
  `leaks --atExit -- ./tests/test_istack` instead.
- `make clean` removes the objects, the dependency files, and both
  programs.

## Running the bracket checker

```sh
./balance < file
```

It reads standard input one character at a time, pushes each `(`, `[`,
`{` and pops for each `)`, `]`, `}`. Every bracket counts, also the ones
in comments and strings — it does not know what a comment is. It stops
at the first error and prints exactly one line; lines and columns count
from 1:

```
balanced: 6 pairs, deepest 3                  exit status 0
line 1, column 8: found ')', expected ']'     exit status 1
line 1, column 12: found ')', nothing open    exit status 1
end of input: 2 still open, innermost '('     exit status 1
```

`deepest` is the most brackets open at once. `balance` destroys its
stack on every path — the success path, all three error paths, and the
early out-of-memory paths.

## The stack (`istack.h`, `istack.c`)

The struct is completed in `istack.c` and nowhere else, so callers only
ever hold an opaque pointer. The values live in one array on the heap;
the room starts at `ISTACK_START` (8) and doubles when full, and never
shrinks. When a push finds the stack full it calls `realloc` **into a
second pointer** first, so a failure leaves the original array and every
value in it untouched. Every `malloc` has its `free`: `istack_create`
frees the struct if the array fails, `istack_to_array` returns `NULL`
rather than half a result, and `istack_destroy` frees both blocks (and
does nothing for `NULL`).

## Part 4: breaking it on purpose

Each bug was made in turn, run under `make memcheck`, and then undone.
The first line of each report is copied below, with what happened in
memory. **The code in this repository is the fixed version**: after
undoing all three, `make memcheck` runs clean, with no sanitizer report.

### (1) Destroy the same stack twice

```
==737==ERROR: AddressSanitizer: heap-use-after-free on address 0x7250410e0040 at pc 0x578e5b83d4fb bp 0x7ffe1589b760 sp 0x7ffe1589b750
```

The first destroy frees the struct, so the second one reads `s->data`
out of memory that is already free — the report points at
`istack_destroy` in `istack.c`, with the earlier `free` shown just below
it.

### (2) Push onto a stack after destroying it

```
==784==ERROR: AddressSanitizer: heap-use-after-free on address 0x71f1d1ee0048 at pc 0x5eb29f8a25b8 bp 0x7ffdc31b3210 sp 0x7ffdc31b3200
```

`istack_push` reads the freed struct's fields (`count`/`capacity`) to
decide whether there is room, so it writes a value into memory that
belongs to the allocator again — use after free, reported at the first
field read in `istack.c`.

### (3) Leave out `istack_destroy` in `balance.c`

```
==830==ERROR: LeakSanitizer: detected memory leaks
```

Without the destroy the 24-byte struct and the array inside it are
never handed back, so at exit LeakSanitizer reports a direct leak of 24
bytes allocated by `istack_create` — the stack simply outlived the
program.

## Files

| File | What it is |
| --- | --- |
| `istack.h` | the interface, one comment per function |
| `istack.c` | the stack, the only place the struct is defined |
| `balance.c` | the bracket checker |
| `tests/test_istack.c` | unit tests for the stack |
| `inputs/` | six regression inputs (balanced, deeper than 8, all three errors, empty) |
| `expected/` | expected output (`.out`) and exit status (`.status`) per input |
| `regress.sh` | compares `./balance` against `expected/` |
| `Makefile` | `make`, `make test`, `make memcheck`, `make clean` |
