# AI usage for hw06

## Tools used, and what they were used for

- **OpenCode (the coding agent) with the MiMo model
  (`mimo-v2.6-flash-free`)** — the main tool. Used to:
  - read and interpret both PDFs: the homework spec
    (`ece361_hw06_fall_2026.pdf`) and the project sheet
    (`project/ece361_project_15_Xu_Xinyi.pdf`) for the `reading_t`
    fields, the scenario file format and the register conversion
    formulas,
  - plan the code (the walkthrough of what each function's pseudocode
    had to decide, question by question, before any C was written),
  - write and review `sensor.h` / `sensor.c` (Part 1),
    `panel.h` / `panel.c` (Part 2), `scenario.h` / `scenario.c` and
    `main.c` (Part 3), `tests/test_hw06.c`, `scenarios/`, the
    `Makefile`, `regress.sh`, `README.md` and this file (Part 4),
  - hand-check every line of both expected traces against the scenario
    files before they were saved, so `expected/` is the spec's answer
    rather than a recording of whatever the program printed.
- **Python 3 (standard library only: `re`, `zlib`), run on this
  machine** — to extract the text of the two PDFs. There are no PDF
  tools anywhere here (no `pdftotext`, no `pip`, and the chat tool
  refuses PDFs), so a small script split the PDF's objects, decompressed
  its `/FlateDecode` content streams, and mapped the bytes through each
  font's `/ToUnicode` table. Used only to *read* the assignments, never
  to write code.
- **gcc, GNU make and git under WSL (Ubuntu)** — not AI. Every claim in
  the README was checked by actually building and running the code:
  `make` (zero warnings, from a `make clean` state), `make test`
  (20/20 unit checks plus both regressions), `make clean`, the CLI
  error paths (`./replay` with no arguments, a missing file, a scenario
  without a start line, a start line with nothing to replay — each
  exits 1), and `git check-ignore` for every gitignore question.

## One thing a model got wrong, and how it was found

**The first build was not warning-free.** The test program was written
following hw01/hw05's "one variable per result type, stored first then
compared then printed" habit, and the model declared an `actual_i32`
alongside the others — but no check in hw06 ever produces an `int`
result that way (`rc` and `actual_u32` covered everything). The very
first `make` flagged it:

```
tests/test_hw06.c:84:9: warning: unused variable 'actual_i32'
```

**How it was found:** the build itself. Zero warnings is an explicit
grading criterion here, so the run could not be accepted as-is.

**What fixed it:** deleting the declaration. The lesson worth keeping:
carrying a template forward from the previous homework means re-checking
every piece of it against the new code — a pattern that was right for
hw01's checks was dead weight in hw06's, and the compiler noticed before
any human did.

## One other thing that had to be fixed

**The first instinct for `scenario_at` was a binary search** — the
lines are sorted by tick, so a log-time lookup "obviously" fits. That is
exactly the kind of clever this course keeps marking down: at most 1000
lines, called once per tick, a plain linear scan with an early `break`
(it stops at the first line past the tick) is both fast enough and
readable by someone who just learned C. It was caught while reviewing
the plan against the course's style rules, before any code existed, and
the final `scenario_at` is the linear version with a comment explaining
why the loop may stop early.

Separately, the smoke checks for the CLI error paths were first run as
one `wsl ... sh -c "..."` command with nested double quotes; the inner
`echo "exit=$?"` closed the outer quote early (`Syntax error:
Unterminated quoted string`) and half the output came back as garbage.
Found by reading the mangled output, fixed by writing the checks into a
script file and piping it through `tr -d '\r' | sh` — the same lesson as
above, in a different costume: when the output looks wrong, believe it.
