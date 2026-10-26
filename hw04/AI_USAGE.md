# AI usage for hw04

## Tools used, and what they were used for

- **OpenCode (the coding agent) with the MiMo model
  (`mimo-v2.6-flash-free`)** — the main tool. Used to:
  - read and interpret the homework spec (`ece361_hw04_fall_2026.pdf`), which
    arrives as part 1 (the ADT), part 2 (program, tests, Makefile), part 3
    (the opaque-type write-up), and the ungraded project next step,
  - write `tally.h`, `tally.c`, `events.c`, `data/door.txt`,
    `tests/test_tally.c` and the `Makefile`,
  - write the comments in that code and these two documents,
  - review the result against the spec's requirement list (exactly the
    given interface, no malloc, sorted names, safe copy, PASS/FAIL per test,
    nonzero exit on failure, one `.d` file per object).
- **Python 3 (standard library only: `re`, `zlib`), run from PowerShell** —
  to extract the text of the homework PDF. As in hw03, this machine has no
  PDF tools (no `pdftotext`, no `pip`, and the chat tool refuses PDFs), so a
  small script decompressed the PDF's content streams, tokenised the text
  operators and mapped the bytes through each font's `/ToUnicode` table. Used
  only to *read* the assignment, never to write code.
- **gcc, GNU make and git under WSL (Ubuntu)** — not AI. Every claim in the
  README was checked by actually building and running it (`make`,
  `make test`, `make clean`, `./events < data/door.txt`), the Part 3 error
  message was copied from a real `gcc` run rather than written from memory,
  and `git check-ignore` was used to confirm the compiled binaries are not
  tracked.

## One thing a model got wrong, and how it was found

**The test's own pool accounting, twice wrong in one section.** The
independence section of `tests/test_tally.c` originally created two tallies
while `pool[1..3]` still held three of the four slots — only **one** free
slot existed, so `tally_create()` handed back `NULL` for the second tally and
two checks failed:

```
FAIL  could not get two separate tallies
FAIL  sizes are 1 and 0, expected 1 and 1
```

The model had also left `tally_destroy(pool[1..3])` *after* that section,
commented with a claim ("the pool still holds pool[1..3]: one free slot, so
exactly two more creates can succeed together") that contradicted its own
arithmetic: three slots held means one free, not two.

**How it was found:** `make test` printed the two FAILs — the test suite did
exactly what it exists to do, and it caught the model, not the library. The
fix was to release `pool[1..3]` *before* the section, with a comment that
now says what is really true, and to drop the now-redundant destroys from
after it. Re-running gave `29 test(s), 0 failure(s)`.

**The lesson kept:** a comment that explains a count is a promise; when the
code and the comment disagree, one of them is wrong, and the run tells you
which.

## One thing that had to be fixed during review

**The newline removal in `events.c`** was first written as a two-step test
(`strchr` to detect, `strcspn` to replace) — correct, but indirect, and this
homework asks for beginner-readable code. It was rewritten to find the
newline once and store through the pointer:

```c
char *nl = strchr(line, '\n');
if (nl != NULL) *nl = '\0';
else { /* no newline: line outgrew the buffer, drain the rest */ }
```

The `else` branch also matters: a line longer than `LINE_MAX` arrives in
pieces from `fgets`, and without draining the remainder, the next `fgets`
would start mid-line and count a fragment as a second name.

## How the deliverable was checked

- `make` under WSL: **no warnings** with `gcc -std=c11 -Wall -Wextra`.
- `make test`: **29 tests, 0 failures**, `ALL TESTS PASSED`. The failure path
  was proved by the pool bug above for real — `FAIL` lines appeared, the
  summary said `2 failure(s)`, and `make` stopped with `Error 1`.
- `./events < data/door.txt` prints 16 sorted names and
  `names: 16, lines skipped: 5` — the arithmetic was checked by hand against
  the data file: 1 over-long name + 4 new names after the tally filled = 5,
  and the 16 that fit are exactly the first 16 distinct names offered.
- `make clean` leaves only `tally.h`, `tally.c`, `events.c`, `data/`,
  `tests/test_tally.c`, the Makefile and the docs — and
  `git check-ignore -v hw04/events hw04/tests/test_tally` confirms both
  binaries are ignored by the new `events` and `test_tally` patterns in
  `.gitignore` (the sources are **not** ignored — checked too).
- Part 3 was performed as written: a file including only `tally.h` with
  `t->n = 0` was compiled, the real gcc message pasted into the README, and
  the experiment file kept outside the repository so the pushed code compiles.
- Every new file was written with LF endings, as `.gitattributes` requires,
  so `make test` behaves the same on a fresh clone.

## The ungraded project next step (not part of HW4)

The same session also produced the scaffolding the sheet marks "not graded":
`project/sensor.h` (`reading_t`), `project/fsm.h` (the states),
`project/store.h` (`record_t` + opaque `store_t`) and `project/store.c` as a
sorted array. Verified with a smoke test of 32 checks under
`-fsanitize=address,undefined`, committed separately. The HW4 Shine does not
ask about any of it.
