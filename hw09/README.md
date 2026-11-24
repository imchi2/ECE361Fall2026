# hw09 — a counting hash table

ECE 361, Fall 2026. One small ADT: a hash table with separate
chaining whose nodes count how often each key was added. `ht.c`
carries the table; `tests/test_ht.c` tests it.

## Build and test

Everything is built and run under WSL (Ubuntu), never with the native
Windows gcc:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw09 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw09 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw09 && make memcheck"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw09 && make clean"
```

- `make` builds `tests/test_ht` (and compiles every `.c` to its own
  `.o`), with no warnings under `-std=c11 -Wall -Wextra`.
- `make test` runs the tests. Each check prints `PASS` or `FAIL`, then
  a summary line; the program exits nonzero if anything failed, which
  stops make:

  ```
  ...
  39 checks, 0 failed
  ```
- `make memcheck` rebuilds everything with
  `-fsanitize=address,undefined` and runs `make test` again. It finds
  leaks on Linux only; on macOS use `leaks --atExit -- ./tests/test_ht`
  instead.
- `make clean` removes the objects, the dependency files, and the test
  program.

## Table 2: why row 3 gives 10, and why 1009 buckets fix it

Row 3 adds the 1000 multiples of 10 from 10 to 10000 with 1000
buckets: every key is a multiple of 10, so its remainder modulo 1000
is also a multiple of 10, and only the 100 buckets 0, 10, 20, …,
990 can ever be used — the 1000 keys pile up in those 100 bucket
chains, 10 per chain. With 1009 buckets the modulus is prime and
shares no factor with 10, so the same 1000 keys get 1000 different
remainders and each chain holds exactly one key.

## The table (ht.h, ht.c)

Both structs live in `ht.c` and nowhere else; `ht.h` only says
`typedef struct ht ht_t`, so callers hold an opaque pointer. Nothing
is global: every function takes the table it works on, and two tables
in one program are independent (tested).

A table is a bucket array plus two counts:

```
struct ht
+-----------+     buckets
| *buckets -+---> [0] -> node -> node -> NULL
| nbuckets  |     [1] -> NULL
| size      |     [2] -> node -> NULL
+-----------+      ...
```

- `ht_create(nbuckets)` allocates the table, then the bucket array;
  every slot is set to `NULL`. 0 buckets returns `NULL` because no key
  could ever be stored. If the second allocation fails, the first is
  freed before returning, so a failed create leaks nothing.
- `ht_add` computes `key % nbuckets`, walks that one chain, and raises
  the node's count if the key is already there. A key that is not
  there yet gets a new node with count 1 at the **front** of the chain,
  which is O(1) and needs no tail pointer. `size` grows only for a new
  key, so it counts different keys. `-1` is returned for a `NULL`
  table or a failed `malloc`, and in both cases the table is unchanged.
- `ht_count` walks the same chain and returns the stored count, or 0
  when the key is missing.
- `ht_size` returns the maintained `size`, so it does not walk the
  chains.
- `ht_longest_chain` walks every bucket, counts its nodes, and keeps
  the largest length; an empty table has no nodes, so the answer is 0.
- `ht_destroy` frees every node of every chain (remembering `next`
  before each `free`), then the bucket array, then the table; `NULL`
  does nothing.

The hash is `key % nbuckets`, as the spec fixes. Keys are never
negative, so the remainder names a real bucket; the code casts the key
to `size_t` first so a slip can never produce a negative index.

### The Shine warm-up, drawn

Adding 12 5 22 5 with 10 buckets:

| bucket | chain (front first) | count at that node |
| --- | --- | --- |
| 2 | `22 -> 12` | 1 and 1 |
| 5 | `5` | 2 |

12 % 10 = 2 and 22 % 10 = 2, so 22 was pushed to the front of
bucket 2 ahead of 12; 5 % 10 = 5, and the second 5 raised that node's
count from 1 to 2 instead of adding a node. Counting 32 therefore
looks in bucket 2 and compares the two nodes, 22 then 12: **2
comparisons**.

## Files

| File | What it is |
| --- | --- |
| `ht.h` | the interface, one comment per function (takes, returns, assumes) |
| `ht.c` | the table; the only place the structs are defined |
| `tests/test_ht.c` | the unit tests, including the four Table 2 rows |
| `Makefile` | `make`, `make test`, `make memcheck`, `make clean` |
| `AI_USAGE.md` | which tools were used, and one thing a model got wrong |
