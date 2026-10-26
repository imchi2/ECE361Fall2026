# hw04 — A tally ADT

HW4 is the first abstract data type: a **tally** that counts how often each
name occurs, such as the events your project device sees. Callers use it only
through `tally.h`; only `tally.c` knows what is inside a `tally_t`. Part 2
adds a small program, tests, and a Makefile; Part 3 writes down what that
opacity prevents.

## Files

| File | What it is |
| ---- | ---------- |
| `tally.h` | the interface: the three limits, the opaque `tally_t`, six prototypes, one comment per function (what it takes, returns, assumes) |
| `tally.c` | the implementation: a sorted array of names and counts in a fixed pool of `TALLY_MAX` tallies — no malloc, no globals beyond the pool |
| `events.c` | reads standard input with `fgets`, skips blank and `#` lines, tallies the rest, prints the tally and `names: N, lines skipped: S` |
| `data/door.txt` | my own data file: comments, blank lines, repeats, one over-long name, and enough names to fill the tally |
| `tests/test_tally.c` | the test program: 29 checks, PASS/FAIL per test, summary, nonzero exit on failure |
| `Makefile` | builds `events` with `gcc -std=c11 -Wall -Wextra`, `make test` runs the tests, `make clean` removes what was built |
| `README.md` | this file |
| `AI_USAGE.md` | which AI tools were used, for what, one thing a model got wrong |

## The interface

```c
#define TALLY_MAX     4    /* tallies that can exist at the same time */
#define TALLY_NAMES  16    /* different names one tally can hold */
#define TALLY_LEN    15    /* longest name, without the '\0' */

typedef struct tally tally_t;      /* opaque */

tally_t *tally_create(void);
void     tally_destroy(tally_t *t);
int      tally_add(tally_t *t, const char *name);
int      tally_count(const tally_t *t, const char *name);
int      tally_size(const tally_t *t);
void     tally_print(const tally_t *t, FILE *out);
```

| Function | Takes | Returns | Assumes |
| -------- | ----- | ------- | ------- |
| `tally_create` | nothing | a new empty tally, or `NULL` when all `TALLY_MAX` are in use | the caller hands it back with `tally_destroy` |
| `tally_destroy` | a tally (or `NULL`) | nothing | the pointer must not be used again afterwards |
| `tally_add` | a tally and a name | `0` counted; **nonzero** = refused: NULL args, empty name, name longer than `TALLY_LEN`, or *new* name in a full tally | a name already in the tally **still counts** when full — full only blocks new names; the name is copied, so the caller may reuse its buffer |
| `tally_count` | a tally and a name | how often it was added; `0` if never, or for NULL | comparison is exact `strcmp`: `"Alice"` ≠ `"alice"` |
| `tally_size` | a tally | how many *different* names; `0` for NULL | a full tally has size `TALLY_NAMES` |
| `tally_print` | a tally and a stream | nothing | the tally keeps names sorted, so printing is a plain walk in `strcmp` order; NULL tally or NULL out writes nothing |

**Decisions the spec left open:** `tally_add` reports refusal through its
`int` return (`0` = counted, nonzero = refused) because `events.c` has to
count refused lines — the spec asks for `lines skipped: S`, and S is exactly
the number of nonzero returns. A refused `tally_add` changes **nothing**:
every check runs before any write.

## The sorted array

`struct tally` holds `e[TALLY_NAMES]` entries (a name and a count) plus `n`,
the number in use. The entries are **kept sorted by name**:

- **finding** a name scans in order and stops as soon as it meets an entry
  that sorts *after* the wanted name — that proves the name is absent;
- **printing** walks `0 .. n-1` and is alphabetical for free;
- **inserting** finds the slot where the name belongs, moves entries
  `i..n-1` up by one, then copies the name in, character by character, only
  after the length check — no `strcpy`, no write past `name[TALLY_LEN + 1]`.

The name copy never trusts the input: `strlen` runs first, and an empty or
over-long name is refused before a single byte is written.

## Building and testing

Everything is built and run under WSL (Ubuntu), never with a native Windows
gcc. From PowerShell:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw04 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw04 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw04 && make clean"
```

- `make` builds **`events`** (`tally.o` + `events.o`) with
  `gcc -std=c11 -Wall -Wextra` and produces **no warnings**.
- `make test` builds `tests/test_tally` and runs it: **29 tests, 0 failures**,
  `ALL TESTS PASSED`. It returns nonzero if any check fails, so `make test`
  fails too.
- `make clean` removes the objects, the `.d` files, and both binaries.

Each `.o` lists the headers it includes: `-MMD -MP` writes a `.d` file per
object and the `-include` at the bottom reads them back, so editing `tally.h`
rebuilds exactly the objects that include it.

### Running the program

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw04 && ./events < data/door.txt"
```

With `data/door.txt` this prints the 16 names in alphabetical order with
their counts, then:

```
names: 16, lines skipped: 5
```

The 5 skipped lines are the one over-long name plus 4 new names that arrived
after the tally was already full (the tally holds `TALLY_NAMES = 16`
different names). Blank lines and `#` comments are never offered to
`tally_add`, so they are **not** part of S.

## Part 3: what the opaque type prevents

Part 3 asks you to try to reach inside a `tally_t` from a file that includes
**only** `tally.h`, write down the compiler's message, and then remove the
line (the code pushed must compile).

The experiment — a file containing nothing but `#include "tally.h"` and one
offending line:

```c
tally_t *t = NULL;
t->n = 0;   /* reach inside the opaque type */
```

The compiler's message, in full:

```
part3.c: In function 'main':
part3.c:8:6: error: invalid use of incomplete typedef 'tally_t' {aka 'struct tally'}
    8 |     t->n = 0; /* the line Part 3 asks me to try */
      |      ^~
```

**What it means.** `tally.h` contains only

```c
typedef struct tally tally_t;   /* no braces, no members */
```

so from `part3.c`'s point of view `struct tally` is a *name without a
definition* — an incomplete type. C does not know how big it is, does not
know it has an `n`, and therefore refuses the `->` outright. The line was
removed afterwards, so the pushed code compiles: the error exists only in
this write-up.

**What the opaque type prevents, and why it is useful.** It prevents any
file except `tally.c` from seeing the tally's members. `events.c` cannot
write `t->n = 0`, cannot skip an entry, cannot forget to move the sorted
tail — it can only call the six functions, each of which keeps its promises
(NULL stays safe, a refused add changes nothing, the sort order survives).
That is useful because the *representation can change without touching the
callers*: `tally.c` could stop being a sorted array and become a hash table
tomorrow, and `events.c`, the tests, and every other caller would still
compile and behave the same — only `tally.c` would be edited. The compiler
enforces the boundary: breaking it is a build error, not a bug you find at
runtime.

## The tests

`tests/test_tally.c` runs **29 checks**, each printing `PASS` or `FAIL`:

| Group | Checks |
| ----- | ------ |
| create | a new tally exists; size 0 |
| add / count | counting one name up to 4; four additions of one name → size 1; a name never added → 0 |
| name lengths | the boundary names really are `TALLY_LEN` and `TALLY_LEN + 1` characters (checked with `strlen`, not trusted); empty name refused; exactly `TALLY_LEN` accepted; `TALLY_LEN + 1` refused; refusals changed nothing |
| NULL | `tally_add`, `tally_count`, `tally_size`, `tally_print` all stay quiet on NULL |
| full tally | fills to `TALLY_NAMES`; a new name is refused; an existing name still counts (5); the refusal did not grow the tally |
| the pool | `TALLY_MAX` tallies at once; the next create returns `NULL`; create after destroy comes back **empty** |
| independence | two tallies at once; adding to one leaves the other at 0; each holds only its own name |
| print order | alphabetical with counts (`alice 2`, `bob 1`, `carol 1`, `dave 1`) read back from a `tmpfile()`; nothing after the last line |

The program prints `29 test(s), 0 failure(s)` and `ALL TESTS PASSED`, and
returns `0` only when everything passed.

## Shine prep (opens Mon Oct 26, due Wed Oct 28)

The Shine is 10–15 minutes about your own code, no notes, no AI. The spec
names two topics:

- **`tally_add` when a new name goes in.** Every check runs first: NULL
  arguments, empty name, `strlen > TALLY_LEN`, then "is it already here?"
  (bump the count and return 0), then "is the array full?" (refuse). Only
  then does the insert happen: find the slot where the name sorts, move
  entries `i..n-1` up by one (`for (j = n; j > i; j--) e[j] = e[j-1];`),
  copy the name character by character up to and including the `'\0'`, set
  `count = 1`, `n++`. The copy is safe because the length was checked
  first — the destination is `name[TALLY_LEN + 1]` and `len + 1` bytes fit.
  Note the order of checks 3 and 4: an *existing* name is found before the
  fullness test, which is why it still counts in a full tally.
- **`events.c` never looks inside a `tally_t`, and what that prevents.**
  `events.c` includes `tally.h` and calls `tally_create/add/size/print/destroy`
  — nothing else. It could not look inside even if it wanted to: the
  incomplete-type error above proves it. That prevents callers from
  corrupting the sort order or the count of used slots, and it means the
  representation can be swapped (array → tree/hash) without editing a single
  caller. The compiler enforces it, so the boundary cannot erode silently.
