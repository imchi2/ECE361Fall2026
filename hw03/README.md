# hw03 — A pointer library

HW3 is about pointers: passing an *address* so a function can change the
caller's own variables, return more than one answer, and walk an array
without ever writing a subscript. Part 1 is the library, Part 2 is the
test program and the Makefile.

## Files

| File | What it is |
| ---- | ---------- |
| `ptr.h` | the four prototypes, each with a comment saying what it takes, returns and assumes |
| `ptr.c` | the four implementations — pointer arithmetic only, no indexed subscripts |
| `tests/test_ptr.c` | the test program: 17 checks, PASS/FAIL per test, summary, nonzero exit on failure |
| `Makefile` | builds `ptr.o` and `tests/test_ptr` with `gcc -std=c11 -Wall -Wextra` |
| `README.md` | this file |
| `AI_USAGE.md` | which AI tools were used, and for what |

## The four functions

```c
void swap_float(float *a, float *b);
int minmax(const float *a, int n, float *min, float *max);
const float *find_above(const float *a, int n, float threshold);
void reverse(float *a, int n);
```

| Function | Takes | Returns | Assumes |
| -------- | ----- | ------- | ------- |
| `swap_float` | the addresses of two floats | nothing — the two caller variables exchange values | both pointers are valid, never NULL |
| `minmax` | an array, its length `n`, and two addresses to write into | `0` when stored; **`−1`** when nothing was done | `−1` when `n < 1` or any of `a`, `min`, `max` is NULL, and then `*min` and `*max` are untouched |
| `find_above` | an array, its length, a threshold | a pointer to the **first** value strictly greater than the threshold, scanning left to right; `NULL` if there is none | `NULL` also when `a` is NULL or `n < 1`; equal to the threshold does not count |
| `reverse` | an array and its length | nothing — the array really is reversed in place | nothing happens when `a` is NULL or `n < 2`; the ends are exchanged with `swap_float` |

`minmax`'s failure value is `−1`, not `1`, and it *stores* the two answers
rather than returning them — there is no way to hand back two floats from
one `int` return value, so the outputs go through pointers instead.

## Building and testing

Everything is built and run under WSL (Ubuntu), never with a native Windows
gcc. From PowerShell:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw03 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw03 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw03 && make clean"
```

Or from a WSL shell already inside `hw03/`, just `make`, `make test`,
`make clean`.

- `make` builds **`ptr.o`** (the library on its own) and **`tests/test_ptr`**
  with `gcc -std=c11 -Wall -Wextra` and must produce **no warnings**.
- `make test` links those objects and runs the test program, which prints
  one `PASS` or `FAIL` line per check plus a summary, and **returns nonzero
  if any check fails** — so `make test` itself fails too.
- `make clean` removes everything the build produced (`*.o`, `*.d` and the
  test binary).

Each `.o` lists the headers it includes: `-MMD -MP` writes a `.d` file per
object, and the `-include` at the bottom of the Makefile reads them back, so
editing `ptr.h` rebuilds exactly the objects that include it (as in HW2).

## The pointer-arithmetic rule

`ptr.c` never writes `a[i]`. Every array is walked by a pointer:

```c
const float *p;          /* a pointer into the array          */
for (p = a; p < a + n; p++)   /* start, stop one past the end, step   */
    if (*p > threshold) return p;   /* *p is the value under the pointer */
```

`a + n` is the address just past the last element. C allows that address to
be computed and compared (but not dereferenced), which is what makes
`p < a + n` the correct "still inside the array" test. `p++` advances by
`sizeof(float)`, so it lands on the next element, not 4 bytes further on.

`reverse` keeps two pointers — `left` at the first element and `right` at
the last — and moves them toward each other with `left++` and `right--`,
swapping through `swap_float` on the way. The loop stops at
`left < right`, so for an odd length the middle element is simply never
touched.

The test program is not bound by that rule: it may use `even[0]` while it
reads the answers back.

## The tests

`tests/test_ptr.c` runs **17 checks**, each printing `PASS` or `FAIL`:

| Function | Checks |
| -------- | ------ |
| `swap_float` | the two values really exchange |
| `minmax` | several values; one value; `n = 0`; `a = NULL`; `min = NULL`; `max = NULL` — every rejection also asserts the outputs are **unchanged** (they start as sentinels, so any write is visible) |
| `find_above` | two matches → the first one; a value equal to the threshold is skipped; nothing greater → `NULL`; `a = NULL`; `n = 0` |
| `reverse` | an even length (4) and an odd length (5); `n = 1`; `n = 0`; `a = NULL` |

The program prints a summary (`17 test(s), 0 failure(s)`) and returns `0`
only when everything passed, `1` otherwise.

Floats are compared with `==` on purpose: every expected number is a
constant the test itself wrote or a copy of one, never the result of
arithmetic, so the bits are identical and `==` is exact.

## Shine prep (Oct 19–21)

The Shine is 10–15 minutes, about your own code, with no notes, no second
screen, no AI, no other person, webcam on. The spec names two questions:

- **Boxes and arrows for `swap_float`.** `swap_float(&x, &y)` passes the
  *addresses* of `x` and `y`. Inside, `a` holds `&x` and `b` holds `&y`;
  `t` is a local copy of `*a`, then `*a = *b` writes the caller's `x`, and
  `*b = t` writes the caller's `y`. Three boxes, two arrows into them from
  the caller's frame. **Without the `*`**, `*a = *b` would become
  `a = b` — assigning one *address* to the local pointer — the caller's
  variables would never change, and `swap_float` would do nothing at all.
- **Why does `minmax` return `−1` instead of storing something, and why is
  `a` a `const float *`?** It returns an `int` *status*: `0` = "the answers
  were stored", `−1` = "I refused, and I wrote nothing" (bad `n` or a NULL
  pointer). The actual answers cannot be returned that way — a function has
  one return value and `minmax` has two results — so they go out through
  `min` and `max`. `a` is `const float *` because `minmax` only *reads* the
  array: the `const` documents that, and it lets a caller pass a read-only
  array (or even a literal) without a cast or a compiler complaint.
