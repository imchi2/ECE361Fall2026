# hw02 — Splitting a working program into modules

The starter for this homework was a single 148-line `readings.c` that
worked but did everything in one place: four global arrays, a `main()`
that parsed arguments, read stdin, computed every statistic and printed
every line, the temperature and humidity summaries written out twice,
and a recursive helper next to two iterative ones. HW2 asks for the
program to be split into modules **without changing what it prints**.

The refactor is described in [`DIAGRAM.md`](DIAGRAM.md), which holds the
module diagram and the rules that keep it from turning into a hairball.

## Files

| File | What it is |
| ---- | ---------- |
| `readings.h` | the shared header: `struct dataset` and `MAX_READINGS`. No logic |
| `readings.c` | the driver: command line, calls, and the summary `printf` lines |
| `input.h` / `input.c` | reading stdin into a `struct dataset` |
| `stats.h` / `stats.c` | min, max and mean of a float array |
| `run.h` / `run.c` | longest run of readings above the threshold |
| `histogram.h` / `histogram.c` | the 5 C temperature bins, and printing them |
| `DIAGRAM.md` | the module diagram: what each module owns, and the rules that keep the dependencies one-directional |
| `ARRAYS.md` | background reading on the original `readings.c`: the parallel arrays, `count`, and the bounds rules (line numbers refer to the starter) |
| `regress.sh` | the regression test: diffs every output against `expected/` |
| `Makefile` | builds each `.c` to its own `.o`, links `./readings`, runs the test |
| `../.gitattributes` | forces LF line endings on checkout — see below |
| `data/` | the input files |
| `expected/` | the golden outputs — the definition of "unchanged" |
| `hw02_starter.zip` | the original download, kept unmodified |

## What the program does

Reads `tick temperature humidity` from standard input, one reading per
line:

```
0       24.7   68.5
60      25.0   68.8
```

Blank lines and lines whose first non-blank character is `#` are
ignored. A line that does not hold three numbers is counted as
`skipped`. Then it prints the count, the minimum/maximum/mean of both
temperature and humidity, the longest run of consecutive readings above
a threshold, and a histogram of the temperatures in 5 C bins.

```sh
./readings < data/normal.txt           # threshold defaults to 30.0 C
./readings 25 < data/edge.txt          # or give the threshold yourself
```

## Building and running

Everything is built and run under WSL (Ubuntu), never with a native
Windows gcc. From PowerShell:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw02 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw02 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw02 && make clean"
```

Or from a WSL shell already inside `hw02/`, just `make`, `make test`,
`make clean`.

- `make` compiles each of the five `.c` files to its own `.o` file with
  `gcc -std=c11 -Wall -Wextra` and must produce **no warnings**.
- `make test` links them into `./readings` and runs `regress.sh`, which
  runs the program on every file in `data/` and `diff`s the output
  against the same name in `expected/`, plus `edge.txt` at a threshold
  of 25. Any difference fails the target.
- `make clean` removes everything the build produced.

Editing a header rebuilds exactly the objects that include it, via the
`.d` files that `-MMD -MP` writes; the `-include` at the bottom of the
Makefile reads them back.

## Why the repository forces LF line endings

The repo has `core.autocrlf=true`, so a clone on a Windows machine
would otherwise check every text file out with CRLF endings — and
`make test` would fail for two reasons that gcc never warns about:

- **`regress.sh` would not run at all.** `sh` reads it line by line,
  and a trailing `\r` makes dash report
  `Syntax error: word unexpected (expecting "do")` on the first `for`.
- **The regression would fail even if it ran.** `regress.sh` does
  `diff - expected/<name>` while the program writes LF to stdout, so a
  CRLF copy of the golden file never matches. And the *input* files
  change the output too: a blank line read as `"\r\n"` no longer
  matches the `'\n'` test, falls through to `sscanf`, fails to parse as
  three numbers, and bumps the printed `skipped:` count.

`.gitattributes` therefore sets `* text=auto eol=lf` (and `*.zip
-text`), which overrides `autocrlf` on checkout. This was checked by
cloning the repository with `core.autocrlf=true` and confirming that
`Makefile`, `regress.sh`, `readings.c`, every file in `data/` and
`expected/` contains no CR byte, that `hw02_starter.zip` hashes
identically to the original, and that `make test` still reports 4/4
PASS in that fresh clone.

## Design decisions

- **No globals.** `main()` owns one `struct dataset` on the stack and
  passes `&ds` to each module. Nothing else in the program can reach in
  and change the readings.
- **One shared header.** `readings.h` is the only file more than one
  module includes. The modules never include each other, so the
  dependencies point in one direction: everything hangs off the data
  type, and only `main()` knows about the modules.
- **Each constant lives with its owner.** `LINE_LEN` is private to
  `input.c`, `NUM_BINS` and `BIN_WIDTH` are private to `histogram.c`,
  and only `MAX_READINGS` (which is part of the shared type) is in the
  shared header.
- **Where the printing goes.** `main()` prints `readings:`, `skipped:`,
  `temperature:`, `humidity:` and the `above ... C:` line, because those
  are the summary. `histogram.c` prints its own section, header line and
  bars together, because the wording is welded to the bin edges.
- **Recursion removed.** The old `max_temp()` recursed to find a maximum
  while min and mean were found with a plain loop right beside it.
  `stats_of()` is one iterative pass that computes all three, for
  either array.
- **Summation order preserved.** Floating-point addition is not
  associative, so `stats_of()` adds in index order — the same order the
  original loops used. That is what keeps the printed means identical
  rather than merely close.
- **`count == 0` is checked in `main()`.** The driver returns before
  calling `stats_of()`, which would otherwise divide by zero on an
  empty input, and the empty-input output (`no readings, no summary`)
  comes out exactly where it did before.

## How the refactor was checked

Two levels, both run under WSL:

1. **The shipped regression** — `make test` runs `regress.sh`: 4 checks
   over `data/` and `expected/`, all passing.
2. **A differential test against the original program.** The original
   single-file `readings.c` was unpacked from `hw02_starter.zip`,
   compiled beside the refactored binary, and both were run on the same
   stdin and arguments while stdout, stderr and the exit status were
   compared. The inputs covered what the golden files do not: indented
   comments, blank and malformed lines, temperatures below 0 C and at
   or above 100 C (the clamped histogram ends), two equal-length runs
   (the tie rule), a run that never happens, more than `MAX_READINGS`
   readings (the `stderr` warning), and thresholds of `0`, `25`,
   `30.5`, `100` and `-5`, plus the argument-error paths.
   **47 cases, all identical.** The scaffolding lives outside this
   repository, in the session temp directory, so the graded folder
   holds only homework files.

## Input rules (unchanged from the starter)

- Leading spaces and tabs are skipped before anything is decided, so an
  indented `#` comment is still a comment.
- Blank lines and `#` comments are not data and are not counted as
  skipped.
- A line with three parseable numbers is a reading, whatever follows
  them; anything else increments `skipped`.
- After 1000 readings, a warning goes to **stderr** and the rest of the
  input is ignored.
- Histogram bins are 5 C wide from 0 to 100 C. Below 0 C is clamped into
  the first bin and 100 C or above into the last, so every reading
  lands in one of the 20 printed ranges.
- The longest-run rule is strict (`>` threshold), and on a tie the
  **earlier** run wins, because a strictly longer run is what replaces
  the best answer.
