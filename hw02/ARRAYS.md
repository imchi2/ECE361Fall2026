# The arrays in the original `readings.c`

Background reading for HW2. Everything here refers to the **starter**
program — the single-file `readings.c` inside `hw02_starter.zip`, before
it was split into modules. Line numbers are that file's.

It is study material, not a graded deliverable: it explains what the
original code did and why, so that the refactor in
[`DIAGRAM.md`](DIAGRAM.md) makes sense.

---

## 1. The problem the arrays solve

Each input line is one reading with three numbers:

```
120     25.8   70.1
```

The program must remember **all** the readings, because it cannot
compute a minimum or a histogram until it has seen every line. It reads
first, computes later. That means we need storage that holds up to 1000
readings — and an array is exactly that: a row of same-sized boxes in a
row.

## 2. Line 25 — the size

```c
#define MAX_READINGS 1000
```

This is **not** a variable. `#define` is handled by the preprocessor
*before* the compiler runs: everywhere the text `MAX_READINGS` appears,
the compiler sees `1000` instead. So this line:

- **Saves you from typing 1000 in several places.** If you later decide
  the limit is 5000, you change one line and every use follows.
- **Gives the number a meaning.** `if (count == 1000)` makes a reader
  wonder *what is 1000?* `if (count == MAX_READINGS)` answers itself.
- **Costs no memory.** There is no "MAX_READINGS variable" in your
  program; it exists only during preprocessing.

The other three — `LINE_LEN`, `NUM_BINS`, `BIN_WIDTH` — work the same
way.

## 3. Lines 30–35 — the declarations

```c
int   ticks[MAX_READINGS];
float temps[MAX_READINGS];
float hums[MAX_READINGS];
int   count = 0;
int   skipped = 0;
float threshold = 30.0f;
```

Read line 31 aloud: *"temps is an array of 1000 floats."* Breaking down
the syntax:

| Piece | Meaning |
|---|---|
| `float` | the type of **each slot** — every element is a `float` |
| `temps` | the name of the whole array |
| `[1000]` | how many slots, side by side in memory |

Actual storage: `ticks` is 1000 × 4 bytes = 4000, and so is each of
`temps` and `hums`. About 12 KB total for the three.

**Indexing starts at 0, not 1.** The valid indexes of a 1000-element
array are `0` through `999`. There is no `temps[1000]` — that slot does
not exist. This is the single most common C mistake, so it is worth
saying twice.

**These six are globals**, because they are written outside any
function. Two consequences:

- They exist for the whole run, and **any** function can read or write
  them. `max_temp()` on line 38 reads `temps` and `count` without them
  being passed in — that only works because they are global.
- Globals are **automatically zeroed** at program start, even without
  `= 0`. The `= 0` on lines 33–34 is legal but technically redundant.

This is precisely what the HW2 header comment complains about ("global
variables"): nothing tells you *who* changed `count`. That is why the
refactor moved all of this into `struct dataset` passed by pointer.

## 4. The core idea: three arrays that are really one

This is the design decision to understand. The three arrays are
**parallel arrays** — index `i` means the *same reading* in all three:

| `i` | `ticks[i]` | `temps[i]` | `hums[i]` |
|---|---|---|---|
| 0 | 0 | 24.7 | 68.5 |
| 1 | 60 | 25.0 | 68.8 |
| 2 | 120 | 25.8 | 70.1 |

Row `i` is one reading, split across three arrays. The index is the join.

**Why they are split rather than one array of structs:** the statistics
code wants *just* the temperatures. Writing `min(temps, count)` is one
clean argument; with an array of structs you would have to say "the
`temp` member of every element," which in C means a loop or a helper.
Splitting keeps the numeric code simple.

**Why `ticks` is `int` and the other two `float`:** a tick count is a
whole number (seconds elapsed); 24.7 is not. One array holds one type —
that is a rule of C — so different types need different arrays.

**Why this is fragile:** nothing *forces* the three to stay in sync. If
one line incremented `count` twice, or wrote to `temps` but not `hums`,
you would get nonsense instead of a compile error. The refactor keeps
the same three arrays but tucks them inside `struct dataset`, so at
least they travel together.

## 5. `count` — the most important variable

```c
int   count = 0;
```

`count` is **how many slots hold real readings.** The invariant — the
promise the whole program relies on:

> Slots `0` through `count - 1` hold valid readings. Slots `count`
> onward are irrelevant.

Two jobs at once:

- **It is the length** for every loop: `for (i = 0; i < count; i++)`.
- **It is the next free index** where a new reading gets written.

Those are the same number on purpose. After 5 readings, slots 0–4 are
full and slot 5 is the next empty one. This dual role is why line 71
can use `count` as the write position and line 72 can do `count++` to
claim it.

## 6. Lines 61–75 — storing readings

```c
while (fgets(line, sizeof line, stdin) != NULL) {
    int i = 0;
    while (line[i] == ' ' || line[i] == '\t')
        i++;
    if (line[i] == '\n' || line[i] == '\0' || line[i] == '#')
        continue;
    if (count == MAX_READINGS) {
        fprintf(stderr, "warning: more than %d readings, the rest are ignored\n", MAX_READINGS);
        break;
    }
    if (sscanf(line, "%d %f %f", &ticks[count], &temps[count], &hums[count]) == 3)
        count++;
    else
        skipped++;
}
```

**`fgets(line, sizeof line, stdin)`** — reads one line into the `line`
array (see §9). `sizeof line` is `128`, so it never writes past the end.
Stops at `NULL` = end of input.

**Lines 62–64** — skip leading spaces/tabs so an indented `#` comment is
still recognised as a comment. Note this `i` is a *local* index into
`line`, unrelated to the reading index.

**Line 65–66** — blank line (`\n`), nothing left (`\0`), or comment
(`#`): `continue` jumps to the next loop iteration. These lines take
**no slot** in the arrays.

**Line 67 — the bounds check, and its position matters:**

```c
if (count == MAX_READINGS) {
```

This must run *before* anything writes to the arrays. If you wrote first
and checked after, the 1001st reading would land in `temps[1000]` —
**outside the array**. C will not stop you; you would silently corrupt
whatever memory sits next to it, and the bug would show up somewhere
unrelated, much later. Checking first makes the limit a decision
instead of a crash.

Why is it *after* the comment check? So a comment appearing once the
arrays are full does not print a spurious warning — comments were never
going to be stored anyway.

**Line 71 — the heart of it:**

```c
sscanf(line, "%d %f %f", &ticks[count], &temps[count], &hums[count])
```

`sscanf` reads from the `line` string instead of the keyboard, matching
the format `"%d %f %f"` (an int, then two floats), and **writes each
result through the addresses you give it.** That is what `&` means:
*"address of."*

- `&temps[count]` = "the slot at index `count`; write the parsed value
  there."
- So for `count == 2` and the line `120 25.8 70.1`, it stores `120`
  into `ticks[2]`, `25.8` into `temps[2]`, `70.1` into `hums[2]`.

`&` is required because `sscanf` must modify your variables, and a
plain `temps[count]` would only hand it a value.

**`== 3`** — `sscanf` returns how many items it successfully converted.
Three means the line really held three numbers. Exactly 3 → `count++`
(claim the slot; the next reading goes to index 3). Fewer →
`skipped++`, and **`count` does not move**, so the half-written slot
gets overwritten by the next attempt. No garbage is ever counted.

**An important consequence:** the index counts *readings*, not *lines*.
Comments and blank lines are never stored, so index 2 is the 3rd
reading, whatever line number it was on in the file.

## 7. Lines 77–105 — reading them back

```c
printf("readings: %d\n", count);
```

Just the length, no array involved.

```c
if (count == 0) {
    printf("no readings, no summary\n");
    return 0;
}
```

**This guard exists for the arrays.** The next lines do `temps[0]` and
`tsum / count`. With `count == 0` you would read `temps[0]` — a slot
that was never filled — and divide by zero. So the empty case leaves
before anything touches an array. Every later use of the arrays is safe
*because* this check came first.

```c
float tmin = temps[0];
float tsum = 0.0f;
for (int i = 0; i < count; i++) {
    if (temps[i] < tmin)
        tmin = temps[i];
    tsum += temps[i];
}
```

- **`tmin = temps[0]`** — seed the minimum with the *first real
  element*. A tempting shortcut is `tmin = 0`, but the readings could
  all be negative, and then 0 is not the minimum. Starting from an
  actual element guarantees the answer comes from the data.
- **`tsum = 0.0f`** — running total, starts empty. The `f` marks it a
  `float`; without it the compiler would treat it as a `double` and
  warn about mixing types.
- **`i < count`, not `i <= count`** — with `count == 3`, slots 0, 1, 2
  are valid; `<=` would also read `temps[3]`, which holds nothing
  useful. Off-by-one, again.
- **`tsum += temps[i]`** adds the element to the total — it is
  `tsum = tsum + temps[i]`, shortened.

Lines 95–104 repeat this for humidity. The comment even admits it:
*"the same loop again."* It is identical logic on a different array —
exactly what the refactor's `stats_of()` collapses into one function
called twice.

## 8. Lines 129–136 — `bins`, an array used as counters

```c
int bins[NUM_BINS] = {0};
for (int i = 0; i < count; i++) {
    int b = (int) (temps[i] / BIN_WIDTH);
    if (temps[i] < 0.0f)
        b = 0;
    if (b >= NUM_BINS)
        b = NUM_BINS - 1;
    bins[b]++;
}
```

Different idea: **each slot counts something.** There are 20 bins for
0–100 °C in 5 °C steps:

| `bins[0]` | `bins[1]` | `bins[2]` | ... |
|---|---|---|---|
| 0–5 °C | 5–10 °C | 10–15 °C | ... |

`bins[b]++` means "slot `b`, add one" — the array holds the histogram.

**Why `= {0}` is mandatory here.** Unlike the globals in §3, `bins` is
a **local** variable, and C does *not* zero locals. Uninitialized, it
contains whatever that memory held before. `{0}` sets the first element
to 0 — and for an array, that first value is **spread across every
element**. (Setting all 20 explicitly would be `= {0, 0, 0, ...}`, or
just `{0}`.) Skip this and your histogram includes random counts from a
previous program.

**Choosing the bin:** `(int)(temps[i] / 5.0f)` divides then **truncates
toward zero** — that is what the cast does. `27.6 / 5.0 = 5.52`, cast
to `5`, so 27.6 °C lands in `bins[5]`, the 25–30 °C bucket.

**The two clamps are array safety**, same spirit as the check on line
67:

- `-3.0 / 5.0 = -0.6` → cast gives `0`… but a truly negative result
  like `-1.5 → -1` would index `bins[-1]`, which does not exist. So
  `if (temps[i] < 0.0f) b = 0;` forces it into the first bucket.
- `150 / 5 = 30`, and `bins[30]` is out of range in a 20-element
  array. So anything ≥ 100 °C is pushed to `bins[19]`, the last bucket.

Then lines 139–145 walk `b` from 0 to 19, skip empty buckets, and print
`bins[b]` asterisks — the stored count becomes the bar length.

## 9. Line 46 — `line`, a different kind of array

```c
char line[LINE_LEN];
```

This one holds **text**, not numbers: 128 `char` slots, so 128 bytes.

- It is **local** to `main`, so it is uninitialized garbage until
  `fgets` fills it.
- **`sizeof line` is 128** because the compiler knows the array's size
  right there. (A function that merely receives a pointer would only
  know the byte count of a pointer — that is why `fgets` is handed the
  size explicitly.)
- Characters are indexed the same way: `line[0]` is the first
  character. `line[i] == '\n'` on line 65 asks *"is the character at
  position i a newline?"* — and `'\0'` is C's end-of-string marker.

## 10. One more array use — line 126

```c
ticks[best_start], ticks[best_start + best - 1]
```

Index **arithmetic** to find a tick value. If the longest run started
at index `7` and was `25` readings long, the last reading in it is at
`7 + 25 - 1 = 31`. The `-1` is because the run's length, counted
starting at the start index, ends one before `start + length`. Get that
wrong by one and you print a tick one reading too far.

## 11. Rules worth carrying forward

1. Indexes run `0` to `size - 1`. Never `temps[1000]`.
2. Loop with `i < count`, never `i <= count`.
3. Check the bound **before** writing, never after.
4. Globals are zeroed for free; **locals need `= {0}` explicitly**.
5. Seed a minimum with a real element (`temps[0]`), not a magic `0`.
6. Use a named constant (`MAX_READINGS`), not a bare `1000`.
7. C will not check your bounds. `temps[-1]` and `temps[9999]` compile
   fine and fail somewhere else.

## 12. What the refactor changed

| Original | Now |
|---|---|
| `ticks`, `temps`, `hums`, `count`, `skipped` as 5 globals | 5 members of `struct dataset`, owned by `main`, passed as `&ds` |
| input logic inline in `main` | `read_dataset()` fills the struct |
| temp loop + copy-pasted humidity loop + recursive `max_temp()` | one `stats_of()` per array |
| `bins` local to `main` | local to `hist_print()`, same `= {0}` |
| `line[128]` local to `main` | local to `read_dataset()` |

The arrays themselves are unchanged — same types, same `MAX_READINGS`,
same indexing rules. What changed is **who can touch them**: now only
the one function that holds the pointer. That is the entire point of
the homework.

---

See also: [`DIAGRAM.md`](DIAGRAM.md) for the module split,
[`README.md`](README.md) for how to build and test it.
