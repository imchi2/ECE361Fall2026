# C Readings Refactor — Module Split Notes

The original was one file with 6 global variables and a `main()` that did everything; the refactor splits it into five modules that share one struct, with the rule: **same printed output, byte for byte.**

## What changed, piece by piece

### 1. Globals → one struct passed around (the biggest change)

**Before:** `ticks[]`, `temps[]`, `hums[]`, `count`, `skipped`, `threshold` were globals — any line of the program could change them at any time.

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

`main()` declares it locally (`struct dataset ds;`) and hands `&ds` to whoever needs it.

**Why:** with globals you can't tell who changed what. With a local struct passed as a parameter, the data flow is visible in the function signatures.

**C concepts to know cold:**

- A struct bundles related variables into one type. `.` accesses a member (`ds.count`); `->` accesses it through a pointer (`ds->count` means `(*ds).count`).
- Passing `&ds` passes the address, so `read_dataset()` writes into the caller's struct — no copy.
- **Exam trap:** globals are automatically zero-initialized; stack locals are garbage. That's exactly why the new `main()` has `ds.count = 0; ds.skipped = 0;` — the old code got the zeroing for free, the new code must do it by hand. (The arrays are left uninitialized on purpose: they're written before they're read.)

### 2. Reading loop → input.c

The `fgets`/`sscanf` loop moved verbatim into `read_dataset(FILE *in, struct dataset *ds)`. `LINE_LEN` moved with it, because only the input module cares about line length.

**Why take `FILE *in` instead of using `stdin` directly?** The module no longer depends on where the bytes come from — `main()` passes `stdin`, but the same function could read any file. The dependency is explicit in the signature.

**Detail preserved:** `sscanf` writes straight into `ds->ticks[ds->count]` etc., and `count` only goes up if all 3 conversions succeeded — that's why one index can be shared by all three arrays.

### 3. Two copy-pasted loops + recursion → stats.c

**Before:** temperature min/mean was one loop, humidity min/max/mean was a copy of the same loop (the starter comment even admitted it), and the max was found by a recursive `max_temp()` sitting right next to them — two different styles for one job.

**After:** one function, called twice:

```c
struct summary stats_of(const float *values, int n);
// main: stats_of(ds.temps, ds.count)  and  stats_of(ds.hums, ds.count)
```

It computes min, max, and mean in a single pass, and returns all three in a small struct.

**C concepts:**

- **Arrays decay to pointers.** When you pass `ds.temps`, the function receives a pointer to the first element — it has no idea how long the array is. That's why `n` must be passed separately. (`sizeof` won't help inside the function.)
- `const float *values` = "I promise not to modify the data." The compiler enforces it, and it documents intent.
- Returning a struct by value is legal C and handy: three answers come back in one object.
- **Why kill the recursion?** `max_temp(0)` recursed ~1000 frames deep to find one maximum, while a loop does it with zero stack. Recursion shines on divide-and-conquer problems; a linear scan is not one.

### 4. Inline run logic → run.c

The longest-run loop moved into `longest_run()`, returning `struct run { int length; int start; }` — **indexes, not tick numbers**. `main()` looks up `ds.ticks[r.start]` itself.

**Why indexes?** The module stays a generic "scan a float array" helper that doesn't need to know ticks exist. And `length == 0` doubles as the "never happened" signal, so `main` prints `never`.

**Behavior preserved on purpose:** the tie rule. `run > best.length` (strictly greater) means an earlier run wins ties — changing it to `>=` would silently change the printed tick numbers.

### 5. Histogram → histogram.c

Binning and printing moved together. `NUM_BINS`/`BIN_WIDTH` are now private to `histogram.c`.

**Why does this module print, but stats.c doesn't?** Deliberate split:

- The summary lines (`temperature:`, `humidity:`, `above ...`) are `main()`'s job — computation and formatting are separate.
- The histogram's wording is welded to the bin edges (`%5.1f to %5.1f`), so keeping format and counting in one file stops them from drifting apart.

### 6. The header rules (this is what "module" means in C)

- Each module is a `.c` (code) + `.h` (declarations) pair. The header is the **menu**: it tells other files what the function looks like without showing the body.
- Include guards (`#ifndef STATS_H` / `#define STATS_H` / `#endif`) prevent "redefinition" compile errors if a header gets pulled in twice.
- Only `readings.h` is shared — modules never include each other's headers, so dependencies point one way: everything hangs off the data type, and only `main()` knows all the modules.
- C has no `private` keyword — you hide something (like `LINE_LEN`) simply by not putting it in the header.

### 7. The build changed too

Old: one file, one compile. New: each `.c` compiles to its own `.o` (separate compilation), then the linker stitches the objects into `./readings`. The `.d` files (`-MMD -MP`) record which headers each object used, so editing `stats.h` rebuilds exactly the right files. Benefit: recompile only what changed, and each file can be understood alone.

## Subtle "output must not change" details (examiners love these)

1. Floating-point addition is not associative, so `stats_of()` sums in index order, same as the original loops — otherwise the printed mean could differ in the last digit.
2. `count == 0` early return stayed in `main()` — `stats_of()` would compute 0 / 0 otherwise.
3. The capacity check comes **after** the blank/comment check in `read_dataset()`, so a comment arriving when the arrays are full doesn't trigger a spurious warning.
4. Histogram clamping (below 0 °C → bin 0, ≥100 °C → last bin) kept as-is.

## Likely oral questions → 10-second answers

- **"Why refactor at all?"** One file doing everything is hard to test, reuse, and reason about. After the split, each module has one job, and `stats_of()` is genuinely reused (called for both temp and humidity).
- **"Why is `MAX_READINGS` in the shared header but `LINE_LEN` isn't?"** `MAX_READINGS` sizes the arrays inside `struct dataset` — anyone declaring the struct needs it. `LINE_LEN` is the input module's private business.
- **"Why does `read_dataset` get `&ds` but `stats_of` only gets `ds.temps`?"** The first writes into the whole struct → needs a pointer to it. The second only reads one array → a pointer to its first element suffices.
- **"What does `const` buy you?"** A compiler-checked promise the function won't modify the caller's data.
- **"Why initialize `ds.count` but the old global needed no help?"** Globals are zero-initialized by the language; stack variables start as garbage.
- **"Fun fact if they probe deep:"** in `run.c`, the struct tag `run` and the local variable `run` coexist legally — C keeps tags and variables in separate namespaces.

## Two discrepancies worth knowing before the exam

1. `DIAGRAM.md` calls the driver `main.c`, but the file is actually `readings.c` — the original name was kept (the Makefile's `SRCS` confirms it). If asked "where's `main()`?", the answer is `readings.c`.
2. `histogram.h`'s comment claims it "prints nothing at all when `ds->count` is 0" — but the code has no such guard; `printf("histogram:\n")` runs unconditionally. It never matters in practice because `main()` returns early on empty input, but if an examiner asks "what does `hist_print` print for an empty dataset?", the honest answer is: **the header line with no bars**. You may want to fix either the comment or add the guard before submission.
