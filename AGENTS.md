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
- `.gitignore` excludes `*.pdf`, `*.o`, `*.exe` — spec PDFs and build output are
  intentionally never committed. Don't fight it.
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

## Reading the spec PDFs (the obvious approaches fail)

- `read` rejects PDFs: *"this model does not support pdf input"*.
- No PDF tools in either environment: `python3` has **no pip** (`No module named
  pip` → no pypdf/pdfminer/fitz, verified on Ubuntu WSL too) and there is no
  `pdftotext`.
- Working recipe: pipe a Python script to `python -` (or `python3 -`) that
  regex-splits `(\d+) 0 obj` bodies, `zlib.decompress`s `/FlateDecode` streams,
  tokenizes the content stream for `(str)`, `<hex>`, `[`/`]`, `Tj`, `TJ`, and
  maps bytes through each font's `/ToUnicode` CMap (`beginbfchar`,
  `beginbfrange`). Only two fonts have a CMap; the rest decode as
  latin1/utf-16be.
- Trap that silently produces empty output: to find a `TJ` array, search
  backwards for the `[` token specifically — searching for any bracket token
  finds the closing `]`, and every text run comes out blank.

## Building and verifying (hw01)

- `make` must produce **zero warnings** (`-std=c11 -Wall -Wextra -g3 -O0 -MMD
  -MP`); `make clean` must work. Header edits rebuild the right object via the
  `-MMD` `.d` files included at the bottom of the Makefile.
- `SRCS` is an explicit list (`SRCS := bits.c`) — deliberately not a wildcard,
  for readability. **Add `status.c` to it by hand in Part 3.**
- `make test` is wired up but currently stops with
  `No rule to make target 'tests/test_bits.o'`, because `tests/test_bits.c`
  does not exist yet (Part 4).
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

## Status (as of 2026-10-04)

- hw01 due **Sun Oct 4, 11:59pm**. Nothing committed yet (`main` has zero
  commits; `.gitignore`, `AGENTS.md`, and `hw01/` are untracked).
- Done: Part 2 (`bits.h`, `bits.c`, `Makefile`) — helpers inlined and the
  Makefile reduced to plain POSIX; verified under WSL: warning-free build,
  `make clean` works, header edits trigger rebuilds, and a 44-case throwaway
  suite passed (since deleted).
- Outstanding: Part 3 `status.h`/`status.c` (and add `status.c` to `SRCS`),
  Part 4 `tests/test_bits.c`, `hw01/README.md`, `hw01/AI_USAGE.md`, top-level
  `README.md` (name), first commit.
