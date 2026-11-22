# AGENTS.md

ECE 361 (Fall 2026) coursework submission repo. Each `hwNN/` and `project/` is a
graded deliverable read by the "Shine" grader, which looks for **exactly these
paths and file names** — never rename, move, or add top-level folders.

## Ground rules

- Repo must stay **public** (Shine reads it). Remote:
  `https://github.com/imchi2/ECE361Fall2026.git`, branch `main`.
- **Commit is fine, never `git push`** — the user pushes.
- Specs are the PDFs inside each hw dir (e.g. `hw01/ece361_hw01_fall_2026_r1.pdf`).
  Read them first; grading criteria are listed at the end of each spec.
- `.gitignore` excludes `*.pdf`, `*.o`, `*.d`, `*.exe`, `*.tar.gz` (source
  tarballs such as `iom361_r4.tar.gz` stay out; the extracted tree is
  committed instead), scratch files (`tmp_scenario_*.txt`, `regress_out.tmp`),
  and one bare name per homework binary (`test_bits`, `test_ptr`,
  `test_tally`, `events`, `readings`, `lock`, `test_lock`, `test_check`,
  `replay`, `test_hw06`, `balance`, `test_istack`) — no slashes, so each
  matches in any `hwNN/` folder. Spec PDFs and build output are intentionally
  never committed. Don't fight it. It used to be UTF-16 (git parsed no
  pattern at all, so the spec PDF showed as untracked); it is plain UTF-8
  now — keep it that way.
- Every homework also needs `README.md` and `AI_USAGE.md` **inside its own
  folder**, plus a top-level `README.md` with the student's name. `AI_USAGE.md`
  must name the tools used and one thing a model got wrong.

## Build environment: WSL Ubuntu only

All compiling, running, and testing happens inside WSL. From PowerShell:

```
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make clean"
```

- The repo is `/mnt/c/ECE361_C` inside WSL. Always pass `-d Ubuntu`: the only
  other distro is `docker-desktop`, which is not a dev environment.
- Verified toolchain: Ubuntu 26.04.1, gcc 15.2.0, GNU Make 4.4.1, git, python3.
- **Do not build natively on Windows**, even though MSYS2 gcc 16.2 and GnuWin32
  make 3.81 are on PATH and will happily succeed. A native build proves nothing
  about the environment that gets graded.
- Makefiles are plain POSIX: `rm -f`, `./tests/test_bits`, forward slashes.
  `hw01/Makefile` used to carry an `rm`/`del` PATH-scanning branch for native
  Windows; it was removed when builds moved to WSL-only. Do not reintroduce
  shell-portability cleverness.

## C style for this course (the user is learning C fundamentals)

These are explicit user requirements, not defaults:

- **C99 is the ceiling.** No features added after C99: no `_Static_assert`,
  `_Generic`, anonymous struct/union members, `alignas`/`alignof`, `stdalign.h`,
  `stdnoreturn.h`, `threads.h`, binary literals (`0b1010`), `static_assert` as a
  keyword. Designated initializers, `<stdbool.h>`, `<stdint.h>`, and declaring a
  loop variable in `for` are C99 and fine.
- **Keep `-std=c11` in `CFLAGS` anyway** — the assignment specifies the exact
  command `gcc -std=c11 -Wall -Wextra`. The flag permits C99 code; don't "fix"
  it to `-std=c99`.
- **Beginner-readable over clever.** No one-liners: if a statement or expression
  can be split across lines for clarity, split it. Prefer plain `if`/`else` and
  `for` loops over recursion, function pointers, argument macros, compound
  literals, or nested ternaries.
- **Inline helpers rather than adding them.** Repeating a straightforward
  validation block in three functions is *preferred* over a shared helper.
  DRY is explicitly not a goal here.
- **Explain the why.** Every decision, boundary, and non-obvious bit trick gets
  a comment in plain language; code and tests must be understandable by a
  classmate who just learned C.

## Reading the spec PDFs

- `read` still rejects PDFs: *"this model does not support pdf input"*. Never
  pass a spec PDF to `read` — extract text first.
- **`pypdf` is available in WSL and is the preferred tool** (verified 2026-11-21
  on `hw01/ece361_hw01_fall_2026_r1.pdf`: 3 pages, 3122 chars on page 0, clean
  text). There is **no `pdftotext`** binary, but that no longer matters:

  ```
  wsl -d Ubuntu -- python3 -c "from pypdf import PdfReader; \
      print(PdfReader('/mnt/c/ECE361_C/hw01/ece361_hw01_fall_2026_r1.pdf').pages[0].extract_text())"
  ```
- Multi-line Python does not survive PowerShell — inline `python3 -c "..."` with
  newlines gets parsed as PowerShell and fails with `ReservedKeywordNotAllowed`.
  For anything past one line, **write a `.py` file** (temp dir:
  `C:\Users\Xinyi Xu\AppData\Local\Temp\opencode`) and run it, e.g.
  `wsl -d Ubuntu -- sh -c "python3 '/mnt/c/Users/Xinyi Xu/AppData/Local/Temp/opencode/probe.py' <pdf>"`.
  Single-line `python3 -c '...'` with semicolons is fine.
- Windows-side fallback: **`pymupdf` 1.28.2** is installed for Python 3.13 and
  works, **but it mangles the curly apostrophe** — `Tuesday's` comes out as
  `Tuesday\ufffd\ufffd\ufffds` where pypdf gives `Tuesday's` correctly. Prefer
  pypdf; use pymupdf only when a file must be read from Windows without WSL.
- **Old note, now obsolete — do not re-derive it.** This section used to claim
  "no pip in either environment" and carry a hand-rolled recipe: regex-split
  `(\d+) 0 obj`, `zlib.decompress` `/FlateDecode`, tokenize `(str)`/`<hex>`/
  `Tj`/`TJ`, map bytes through each font's `/ToUnicode` CMap. pip works in both
  environments now (WSL pip 25.1.1, Windows pip 26.1.2) and pypdf is installed.
  The old recipe's `TJ`-bracket trap (search backwards for the `[` token, or
  every run comes back blank) is only relevant if pypdf ever fails on a PDF —
  reach for it then, not by default.

## Building and verifying (hw01)

- `make` must produce **zero warnings** (`-std=c11 -Wall -Wextra -g3 -O0 -MMD
  -MP`); `make clean` must work. Header edits rebuild the right object via the
  `-MMD` `.d` files included at the bottom of the Makefile.
- `SRCS` is an explicit list (`bits.c status.c tests/test_bits.c`) —
  deliberately not a wildcard, for readability. Plain `make` compiles every
  `.c` to its own `.o`, which is what Part 4 asks for.
- `CFLAGS` carries `-I.` so `tests/test_bits.c` can `#include "bits.h"`:
  quoted includes search the *includer's* folder first (`tests/`), then the
  `-I` paths.
- `make test` links `tests/test_bits` from the objects and runs it. The
  program prints PASS/FAIL per check plus a summary, returns 1 if anything
  failed (which fails `make test`), and checks `print_binary` by eye: the
  expected text comes first, the real output follows the arrow. **User
  decision: no file I/O, no `unistd.h`, no `strcmp` in the tests — keep it
  that simple.**
- To try code without polluting the deliverable, build in a temp dir under WSL:
  `gcc -std=c11 -Wall -Wextra -I /mnt/c/ECE361_C/hw01 <temp>/smoke.c /mnt/c/ECE361_C/hw01/bits.c`

## Conventions decided with the user (do not "fix" these)

- Part 2 out-of-range policy is **reject deterministically, not clamp**:
  `get_field` → 0, `set_field` → word unchanged, `sign_extend` → 0,
  `print_binary` → prints nothing. Validation runs *before* any shift/mask so
  rejected input never reaches undefined behaviour. Documented in the README and
  tested — don't switch it to clamping.
- The validation and mask building are **repeated inside each function on
  purpose** (see the style rules above); do not factor them into helpers.
- `print_binary` emits **no trailing newline**; groups of four are counted from
  the MSB side, so width 6 prints `10 1111`.
- Part 3 requires named constants for every field position/width ("no magic
  numbers"), and `(status & 0x08) == 0` — remember `==` binds tighter than `&`.
- `bits.c` is reused by the term project ("Keep bits.c clean") — favour clear,
  commented, tested code over cleverness.
- Part 3 invalid-mode policy (spec: "decide how your `status_t` reports it"):
  `status_t` keeps the raw `int mode` (0..7, even when invalid) **and** an
  extra `bool mode_valid` that is false for modes 5..7. Documented in
  `status.h`; the README must say so too.
- Part 3 named constants (`STATUS_POS_*`, `STATUS_WIDTH_*`, `STATUS_MODE_*`)
  live in `status.h`, not `status.c`, so the word layout reads as interface
  and tests/callers can use the `STATUS_MODE_*` values.

## Status (as of 2026-11-21)

- hw01–hw07 are all committed (21 commits, latest 2026-11-16). hw07 (istack
  ADT + bracket checker) was due Sun Nov 15; its Shine closed Wed Nov 18.
  Homework topics: hw01 bits library, hw02 module split, hw03 pointer
  library, hw04 tally ADT + events, hw05 state machines, hw06
  sensor/panel/scenario replay, hw07 istack.
- Uncommitted leftovers in `hw07/`: a one-character style edit to
  `istack_is_empty` (`s->count == 0` parenthesized) and an untracked
  `copy_istack.c` scratch rewrite of the ADT. Neither is part of the graded
  submission — keep or discard, but don't commit the scratch copy blindly.
- `iom361_r4/` is committed at top level (user decision 2026-11-21,
  exception to the "no new top-level folders" rule): the extracted
  hardware-model source — `iom361_r4.c/.h`, Makefile, README, demo, test.
  The tarball's macOS `._iom361_r4.h` metadata file is deliberately left
  untracked; the `.tar.gz` itself is gitignored.
- Term project is **Project 15 "Turnstile"** (spec:
  `project/ece361_project_15_Xu_Xinyi.pdf`), freeze **Sun Dec 6, 11:59pm**.
  Assigned variant: key K3, structure S2 (FIFO queue of pending paid
  passages, capacity 5), report Q4 (temperature rate-of-change anomalies,
  threshold 3.0 °C). Committed so far: `sensor.h`, `fsm.h`, `store.h`, and
  a sorted-array `store.c` placeholder (swap the inside for a tree after
  week 8 without touching the interface).
- Project spec requires `project/` to contain `iom361_r4.h`/`.c`
  **unchanged**, plus Makefile (`make` builds `turnstile`, `make test`,
  `make clean`), `tests/`, `scenarios/`, `SPEC.md`, `AI_USAGE.md`,
  `ASSIGNMENT.md`. Status word must use hw01 `get_field`/`set_field`; time
  is `time_t` UTC via `gmtime`/`strftime`.
