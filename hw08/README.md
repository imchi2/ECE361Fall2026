# hw08 — the bst ADT, and the crash in the original search

ECE 361, Fall 2026. TutorialsPoint's `BinarySearchTree.c` — one global
tree, printing inside `search`, no `free` — turned into a small ADT
with no globals. `bst.c` carries the ported tree; `treestats.c` shows
what insertion order does to the height; the bonus adds `bst_remove`.

## Build and test

Everything is built and run under WSL (Ubuntu), never with the native
Windows gcc:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make memcheck"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make bonus"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw08 && make clean"
```

- `make` builds `treestats` (and compiles every `.c` to its own `.o`),
  with no warnings under `-std=c11 -Wall -Wextra`.
- `make test` runs the unit tests in `tests/test_bst.c` (PASS/FAIL per
  check, a summary, nonzero exit on failure), then checks `treestats`
  against the three outputs the spec fixes, with their exit statuses:

  ```
  treestats seven keys: "n 7, height 3" (exit 0)
  treestats empty input: "empty tree" (exit 0)
  treestats bad input:   "not a number after 4 values" (exit 1)
  ```

- `make memcheck` rebuilds everything with
  `-fsanitize=address,undefined` and runs `make test` again. It finds
  leaks on Linux only; on macOS use
  `leaks --atExit -- ./tests/test_bst` instead.
- `make bonus` rebuilds with the sanitizers and runs
  `tests/test_remove.c`, the `bst_remove` tests.
- `make clean` removes the objects, the dependency files, and all
  three programs.

## Part 2: the crash

The port of `search()` was done faithfully first, loop and all. The
original loop is

```c
while (current->data != data) { ... }
```

which reads `current->data` **before** anything checks `current` for
`NULL`. The input that crashes it is the empty tree: `bst_contains` on
a tree that never had a key inserted dereferences `NULL` immediately.
The test that triggers it is `test_search_empty` in
`tests/test_bst.c` (its `bst_contains` call is at line 70), which runs
first so the crash is the first thing seen. Under `make memcheck` the
first lines of the report are

```
bst.c:193:19: runtime error: member access within null pointer of type 'struct node'
AddressSanitizer:DEADLYSIGNAL
=================================================================
==1241==ERROR: AddressSanitizer: SEGV on unknown address 0x000000000000 (pc 0x55777f12e64f bp 0x7ffd2645cac0 sp 0x7ffd2645ca90 T0)
==1241==The signal is caused by a READ memory access.
==1241==Hint: address points to the zero page.
    #0 0x55777f12e64f in bst_contains /mnt/c/ECE361_C/hw08/bst.c:193
    #1 0x55777f12c515 in test_search_empty tests/test_bst.c:70
```

(the UBSan line comes first, because the undefined-behaviour sanitizer
reports the `NULL` member access before AddressSanitizer prints the
SEGV). The original never noticed because its `main()` always inserts
the seven keys before it searches, so `root` is never `NULL` when
`search()` runs — the empty tree simply never occurs in its own demo.

The fix is one term in the loop condition: test `current != NULL`
first, so `&&` short-circuits and `current->data` is only read when
`current` is a real node. The walk is otherwise unchanged, and the
visit count is the number of nodes whose data was compared, found or
not (on the seven-key tree: 31 found in 3 visits, 15 not found in 3).

## Part 3: what insertion order does

`./treestats < file` reads integers until the end of the input,
inserts them in that order, and prints one line:

| input | output | exit status |
| --- | --- | --- |
| `27 14 35 10 19 31 42` | `n 7, height 3` | 0 |
| (no numbers) | `empty tree` | 0 |
| `1 2 3 4 x` | `not a number after 4 values` | 1 |

Two runs on 1 to 1023:

```
$ seq 1 1023 | ./treestats
n 1023, height 1023

$ seq 1 1023 | sort -R | ./treestats
n 1023, height 23
```

(`sort -R` shuffles freshly every run, so the second height changes
from run to run; the point is that it stays near 20–30, not 1023.)

Increasing order makes every key the right child of the last, so the
tree is one right-going chain and the height equals the number of
keys — a search can take up to 1023 visits, one per key. A shuffled
order keeps the tree bushy: the height is about 23, so a search takes
at most about 23 visits. The insertion order alone decides the shape
of the tree, and the shape decides how many visits a search costs.

## The tree (bst.h, bst.c)

Both structs live in `bst.c` and nowhere else; `bst.h` only says
`typedef struct bst bst_t`, so callers hold an opaque pointer. Nothing
is global: every function takes the tree it works on, and two trees in
one program are independent (tested).

`bst_insert` is the original's `insert()`: smaller keys go left, all
others (including an equal key) go right, so a duplicate is kept, one
step to the right. The new node is attached where the walk falls off
the tree. The size is counted on every successful insert.

`bst_contains` is the original's `search()` without the printing, plus
the visit count and the empty-tree fix above.

`bst_destroy` and `bst_height` recurse: a tree is a recursive shape, so
"free the children, then the node" and "one plus the taller subtree"
read exactly like the definitions. The recursion depth is the height,
which the tests keep at 1000 nodes at most.

## Files

| File | What it is |
| --- | --- |
| `bst.h` | the interface, one comment per function |
| `bst.c` | the tree; the only place the structs are defined |
| `treestats.c` | Part 3, the tree-shape probe |
| `tests/test_bst.c` | unit tests for the ADT, including the crash input |
| `tests/test_remove.c` | bonus tests for `bst_remove` |
| `BinarySearchTree.c` | the original, annotated line by line |
| `Makefile` | `make`, `make test`, `make memcheck`, `make bonus`, `make clean` |
| `BONUS.md` | the three removals, drawn |
