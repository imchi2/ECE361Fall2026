# AI usage for hw02

## Tools used, and what they were used for

- **OpenCode (the coding agent) with the MiMo model
  (`mimo-v2.6-flash-free`)** — the main tool. Used to:
  - work out what was in `hw02_starter/` and what the homework was
    asking for, since there is no spec PDF for HW2 in this repository;
  - design the module split and write it up in `DIAGRAM.md`,
  - write `readings.h`, `input.h` / `input.c`, `stats.h` / `stats.c`,
    `run.h` / `run.c`, `histogram.h` / `histogram.c`, the rewritten
    `readings.c` driver, and the `Makefile`,
  - write the comments in the code and these two documents.
- **PowerShell (`System.IO.Compression.ZipFile`)** — to unpack the
  original single-file `readings.c` out of `hw02_starter.zip` into the
  session temp directory, and to move the starter files up from
  `hw02_starter/` into `hw02/`. Used to get at the *original* program
  so it could be compared against the refactor.
- **Python 3 (standard library only), run under WSL** — a short script
  (`gen_extra.py`, kept in the session temp directory, not in this
  repository) that generated the extra test inputs: indented comments,
  malformed lines, out-of-range temperatures, two equal-length runs,
  and a 1200-reading file to trip the overflow warning.
- **gcc, GNU make, diff and sh under WSL (Ubuntu)** — not AI. Every
  claim above was checked by building and running:
  `make` (warning-free), `make test` (4/4 PASS), `make clean`, and a
  differential harness (`compare.sh`, also outside the repository) that
  ran the original binary and the refactored one on the same stdin and
  arguments and compared stdout, stderr and the exit status across 47
  cases.

## One thing a model got wrong, and how it was found

**The include guards did not match their own `#ifndef`.** The model
wrote `readings.h` as

```c
#ifndef READINGS_H
#define READ_READINGS_H_GUARD
```

and `input.h` the same way with `INPUT_H_GUARD`. The guard only works
when the macro named by `#ifndef` is the one `#define` creates; as
written, the second inclusion of the header would slip past the guard
entirely. Nothing in the build would have complained, and the symptom
would only appear much later, the first time a translation unit
included a header twice — which these did not, so it would have shipped
as a latent bug.

**How it was found:** by re-reading the header immediately after
writing it, not by the compiler. Both were corrected to `#define
READINGS_H` and `#define INPUT_H` before the first build. The lesson is
that a warning-free build proves the code compiles, not that a guard
does its job — guards need to be read, or the file needs to be included
twice on purpose.

## One thing that had to be fixed

**The model's own comparison harness reported a failure that was the
harness's fault.** The usage message echoes `argv[0]`, so running the
original binary from the temp directory and the refactored one from
`hw02/` necessarily printed two different program paths — and the test
flagged that as a behavioural difference.

It was caught by reading the diff instead of just the `FAIL` line: the
only differing line was the program name inside the usage text. The
harness now normalises that one token (`sed 's|^usage: .*|usage: PROG|'`)
before comparing stderr, and re-running reports 47 cases with 0
failing. A test that fails has to be proven wrong before the code is
blamed for it.

## One more thing worth recording

The first attempt to drive WSL from PowerShell put shell variables
inside a double-quoted PowerShell string:

```powershell
wsl -d Ubuntu -- sh -c "python3 $T/gen_extra.py"
```

PowerShell expands `$T` itself — to nothing — so WSL received
`python3 /gen_extra.py` and Python reported `No such file or
directory`. The fix was to stop interpolating and pass the path
literally in a single-quoted path argument. Shell layers do not nest
the way they look like they do.
