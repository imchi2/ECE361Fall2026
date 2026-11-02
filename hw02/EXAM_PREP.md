# hw02 oral exam prep — the module split

The one-sentence answer: the original was **one file with 6 global
variables and a `main()` that did everything**; the refactor splits it
into **five modules that share one struct**, with the rule: *same
printed output, byte for byte* (checked by `regress.sh`, 4/4 PASS, plus
a 47-case differential test against the original binary).

The last section of this file is the **original starter code with notes
in the margin** showing where every piece went.

## What changed, piece by piece

### 1. Globals → one struct passed around (the biggest change)

**Before:** `ticks[]`, `temps[]`, `hums[]`, `count`, `skipped`,
`threshold` were globals — any line of the program could change them at
any time.

**After:** `readings.h` defines one type:

```c
struct dataset {
    int   ticks[MAX_READINGS];
    float temps[MAX_READINGS];
    float hums[MAX_READINGS];
    int   count;
    int   skipped;
};
```

`main()` (which stayed in `readings.c`) declares it **locally**
(`struct dataset ds;`) and hands `&ds` to whoever needs it.

**Why:** with globals you can't tell who changed what. With a local
struct passed as a parameter, the data flow is visible in the function
signatures.

**C concepts to know cold:**

- A `struct` bundles related variables into one type. `.` accesses a
  member (`ds.count`); `->` accesses it through a pointer (`ds->count`
  means `(*ds).count`).
- Passing `&ds` passes the *address*, so `read_dataset()` writes into
  the caller's struct — no copy.
- **Exam trap:** globals are automatically zero-initialized; **stack
  locals are garbage**. That is why the new `main()` has
  `ds.count = 0; ds.skipped = 0;` — the old code got the zeroing for
  free. (The arrays are left uninitialized on purpose: they are written
  before they are read.)

### 2. Reading loop → `input.c`

The `fgets`/`sscanf` loop moved verbatim into
`read_dataset(FILE *in, struct dataset *ds)`. `LINE_LEN` moved with it,
because only the input module cares about line length.

**Why take `FILE *in` instead of using `stdin` directly?** The module
no longer depends on *where* the bytes come from — `main()` passes
`stdin`, but the same function could read any file. The dependency is
explicit in the signature.

**Detail preserved:** `sscanf` writes straight into
`ds->ticks[ds->count]` etc., and `count` only goes up if all 3
conversions succeeded — that is why one index can be shared by all
three arrays.

### 3. Two copy-pasted loops + recursion → `stats.c`

**Before:** temperature min/mean was one loop, humidity min/max/mean
was a *copy of the same loop* (the starter comment even admitted:
"the same loop again"), and the max was found by a **recursive**
`max_temp()` sitting right next to them — two different styles for one
job.

**After:** one function, called twice:

```c
struct summary stats_of(const float *values, int n);
// main: stats_of(ds.temps, ds.count)  and  stats_of(ds.hums, ds.count)
```

It computes min, max, **and** mean in a single pass, and returns all
three in a small struct.

**C concepts:**

- **Arrays decay to pointers.** When you pass `ds.temps`, the function
  receives a pointer to the first element — it has *no idea* how long
  the array is. That is why `n` must be passed separately. (`sizeof`
  won't help inside the function.)
- **`const float *values`** = "I promise not to modify the data." The
  compiler enforces it, and it documents intent.
- **Returning a struct by value** is legal C and handy: three answers
  come back in one object.
- **Why kill the recursion?** `max_temp(0)` recursed ~1000 frames deep
  to find one maximum, while a loop does it with zero stack. Recursion
  shines on divide-and-conquer problems; a linear scan is not one.

### 4. Inline run logic → `run.c`

The longest-run loop moved into `longest_run()`, returning
`struct run { int length; int start; }` — **indexes, not tick numbers**.
`main()` looks up `ds.ticks[r.start]` itself.

**Why indexes?** The module stays a generic "scan a float array" helper
that doesn't need to know ticks exist. And `length == 0` doubles as the
"never happened" signal, so `main` prints `never`.

**Behavior preserved on purpose:** the tie rule. `run > best.length`
(strictly greater) means an *earlier* run wins ties — changing it to
`>=` would silently change the printed tick numbers.

### 5. Histogram → `histogram.c`

Binning and printing moved together. `NUM_BINS`/`BIN_WIDTH` are now
**private to `histogram.c`**.

**Why does this module print, but `stats.c` doesn't?** Deliberate
split:

- The summary lines (`temperature:`, `humidity:`, `above ...`) are
  `main()`'s job — computation and formatting are separate.
- The histogram's wording is *welded to the bin edges*
  (`%5.1f to %5.1f`), so keeping format and counting in one file stops
  them from drifting apart.

### 6. The header rules (this is what "module" means in C)

- Each module is a **`.c` (code) + `.h` (declarations)** pair. The
  header is the *menu*: it tells other files what the function looks
  like without showing the body.
- **Include guards** (`#ifndef STATS_H` / `#define STATS_H` /
  `#endif`) prevent "redefinition" compile errors if a header gets
  pulled in twice.
- **Only `readings.h` is shared** — modules never include each other's
  headers, so dependencies point one way: everything hangs off the data
  type, and only `main()` knows all the modules.
- C has **no `private` keyword** — you hide something (like
  `LINE_LEN`) simply by *not* putting it in the header.

### 7. The build changed too

Old: one file, one compile. New: each `.c` compiles to its **own `.o`**
(separate compilation), then the **linker** stitches the objects into
`./readings`. The `.d` files (`-MMD -MP`) record which headers each
object used, so editing `stats.h` rebuilds exactly the right files.
Benefit: recompile only what changed, and each file can be understood
alone.

## Subtle "output must not change" details (examiners love these)

1. **Floating-point addition is not associative**, so `stats_of()` sums
   in index order, same as the original loops — otherwise the printed
   mean could differ in the last digit.
2. **`count == 0` early return stayed in `main()`** — `stats_of()`
   would compute `0 / 0` otherwise.
3. The **capacity check comes after** the blank/comment check in
   `read_dataset()`, so a comment arriving when the arrays are full
   doesn't trigger a spurious warning.
4. Histogram **clamping** (below 0 C → bin 0, >= 100 C → last bin) kept
   as-is.

## Likely oral questions → 10-second answers

- **"Why refactor at all?"** One file doing everything is hard to test,
  reuse, and reason about. After the split, each module has one job,
  and `stats_of()` is genuinely reused (called for both temp and
  humidity).
- **"Why is `MAX_READINGS` in the shared header but `LINE_LEN`
  isn't?"** `MAX_READINGS` sizes the arrays inside `struct dataset` —
  anyone declaring the struct needs it. `LINE_LEN` is the input
  module's private business.
- **"Why does `read_dataset` get `&ds` but `stats_of` only gets
  `ds.temps`?"** The first writes into the whole struct → needs a
  pointer to it. The second only reads one array → a pointer to its
  first element suffices.
- **"What does `const` buy you?"** A compiler-checked promise the
  function won't modify the caller's data.
- **"Why initialize `ds.count` but the old global needed no help?"**
  Globals are zero-initialized by the language; stack variables start
  as garbage.
- **"Where is `main()` now?"** Still in `readings.c` — the original
  file name was kept (the Makefile's `SRCS` confirms it; `DIAGRAM.md`
  calls it `main.c`, which is the same file by role).
- **Fun fact if they probe deep:** in `run.c`, the struct *tag* `run`
  and the local variable `run` coexist legally — C keeps tags and
  variables in separate namespaces.

## Known discrepancies (be ready if asked)

1. **`DIAGRAM.md` calls the driver `main.c`, but the file is actually
   `readings.c`.** Same role, original name kept.
2. **`histogram.h`'s comment claims it "prints nothing at all when
   `ds->count` is 0" — but there is no such guard in the code**;
   `printf("histogram:\n")` runs unconditionally. It never matters in
   practice because `main()` returns early on empty input. Honest
   answer if asked: "the module alone would print the header line with
   no bars; the driver never lets that happen."

---

## The original `hw02_starter/readings.c`, annotated

Notes in the right margin (`<<<`) say where each piece went.

```c
/*
 * readings.c: ECE 361, Fall 2026, HW2 starter
 * ... (header comment: "It is also one file, with global variables,
 * repeated code, and a main() that does everything.")
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_READINGS 1000      /* <<< readings.h: it sizes the arrays in
                                      struct dataset, so it is SHARED   */
#define LINE_LEN     128       /* <<< input.c: private to the module
                                      that reads lines                  */
#define NUM_BINS     20        /* <<< histogram.c: private              */
#define BIN_WIDTH    5.0f      /* <<< histogram.c: private              */

/* <<< ALL SIX GLOBALS DIE. Bundled into struct dataset (readings.h),
   declared locally in main(), passed as &ds. threshold becomes a
   local in main() and a parameter to longest_run(). */
int   ticks[MAX_READINGS];
float temps[MAX_READINGS];
float hums[MAX_READINGS];
int   count = 0;               /* <<< globals are zero-init'd for free;
                                  the new stack struct needs
                                  ds.count = 0 by hand — exam trap!     */
int   skipped = 0;
float threshold = 30.0f;

/* <<< DELETED. Recursion (~1000 stack frames) to find one maximum,
   while min/mean used loops. Replaced by one iterative pass in
   stats.c stats_of(). */
float max_temp(int i) {
    if (i == count - 1)
        return temps[i];
    float rest = max_temp(i + 1);
    return temps[i] > rest ? temps[i] : rest;
}

int main(int argc, char *argv[]) {
    char line[LINE_LEN];       /* <<< input.c: the buffer belongs to
                                      read_dataset()                    */

    /* <<< argv parsing STAYS in readings.c main() — it is the
       driver's job. threshold is now a local float. */
    if (argc > 2) {
        fprintf(stderr, "usage: %s [threshold] < readings.txt\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        char *end;
        threshold = strtof(argv[1], &end);
        if (*end != '\0') {
            fprintf(stderr, "error: threshold '%s' is not a number\n", argv[1]);
            return 1;
        }
    }

    /* <<< THIS WHOLE LOOP -> input.c read_dataset(stdin, &ds).
       Logic kept verbatim, same order of checks:
       1. skip leading spaces/tabs (indented comments still count)
       2. drop blank lines and '#' comments
       3. THEN the capacity check (a comment on a full array must
          not trigger the warning)
       4. sscanf into the arrays; count++ only if 3 conversions   */
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

    /* <<< these two printfs STAY in main() — they are the summary */
    printf("readings: %d\n", count);
    printf("skipped:  %d\n", skipped);
    if (count == 0) {
        /* <<< STAYS in main(), and MUST stay before stats_of():
           0 / 0 otherwise */
        printf("no readings, no summary\n");
        return 0;
    }

    /* <<< THIS LOOP + max_temp(0) -> stats.c stats_of(ds.temps, n).
       min AND max AND mean in one pass now; sum kept in index order
       because float addition is not associative. */
    float tmin = temps[0];
    float tsum = 0.0f;
    for (int i = 0; i < count; i++) {
        if (temps[i] < tmin)
            tmin = temps[i];
        tsum += temps[i];
    }
    printf("temperature: min %6.1f  max %6.1f  mean %6.2f C\n", tmin, max_temp(0), tsum / count);
    /*    ^ this printf STAYS in main(); only the numbers moved   */

    /* <<< THE COPY-PASTED LOOP ("the same loop again") -> the SAME
       function: stats_of(ds.hums, n). One function, two arrays —
       that is the reuse argument for the exam. */
    float hmin = hums[0];
    float hmax = hums[0];
    float hsum = 0.0f;
    for (int i = 0; i < count; i++) {
        if (hums[i] < hmin)
            hmin = hums[i];
        if (hums[i] > hmax)
            hmax = hums[i];
        hsum += hums[i];
    }
    printf("humidity:    min %6.1f  max %6.1f  mean %6.2f %%RH\n", hmin, hmax, hsum / count);

    /* <<< -> run.c longest_run(ds.temps, n, threshold), returns
       struct run { length, start } as INDEXES. main() looks up the
       tick numbers itself. Tie rule kept: '>' not '>=', so the
       earlier run wins. length == 0 means "never". */
    int run = 0, best = 0, best_start = -1, start = 0;
    for (int i = 0; i < count; i++) {
        if (temps[i] > threshold) {
            if (run == 0)
                start = i;
            run++;
            if (run > best) {
                best = run;
                best_start = start;
            }
        } else {
            run = 0;
        }
    }
    /* <<< both printfs STAY in main() */
    if (best == 0)
        printf("above %.1f C: never\n", threshold);
    else
        printf("above %.1f C: longest run %d readings, from tick %d to tick %d\n",
               threshold, best, ticks[best_start], ticks[best_start + best - 1]);

    /* <<< -> histogram.c hist_print(&ds). Counting AND printing moved
       together, because the wording is welded to the bin edges.
       Clamping kept: below 0 C -> bin 0, >= 100 C -> last bin.
       Empty bins print nothing. */
    int bins[NUM_BINS] = {0};
    for (int i = 0; i < count; i++) {
        int b = (int) (temps[i] / BIN_WIDTH);
        if (temps[i] < 0.0f)
            b = 0;
        if (b >= NUM_BINS)
            b = NUM_BINS - 1;
        bins[b]++;
    }
    printf("histogram:\n");
    for (int b = 0; b < NUM_BINS; b++) {
        if (bins[b] == 0)
            continue;
        printf("  %5.1f to %5.1f C | ", b * BIN_WIDTH, (b + 1) * BIN_WIDTH);
        for (int k = 0; k < bins[b]; k++)
            putchar('*');
        printf(" %d\n", bins[b]);
    }
    return 0;
}
```

## Cheat sheet: original line → new home

| Original (`hw02_starter/readings.c`) | New home |
|---|---|
| 6 globals (lines 30–35) | `struct dataset` in `readings.h`, owned locally by `main()` |
| `MAX_READINGS` | `readings.h` (part of the shared type) |
| `LINE_LEN` | `input.c` (private) |
| `NUM_BINS`, `BIN_WIDTH` | `histogram.c` (private) |
| `max_temp()` recursion (38–43) | deleted; `stats.c stats_of()` finds max iteratively |
| argv parsing (48–59) | stays in `readings.c main()`; `threshold` now a local |
| reading loop (61–75) | `input.c read_dataset(FILE *in, struct dataset *ds)` |
| `readings:`/`skipped:`/`no readings` (77–82) | stays in `main()` (count==0 guard must precede division) |
| temperature loop + humidity loop (85–105) | `stats_of()` called twice — the reuse win |
| run logic (108–126) | `run.c longest_run()` → `struct run {length, start}`; main looks up ticks |
| histogram fill + print (129–146) | `histogram.c hist_print(const struct dataset *ds)` |
