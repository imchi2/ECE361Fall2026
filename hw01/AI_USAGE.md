# AI usage for hw01

## Tools used, and what they were used for

- **OpenCode (the coding agent) with the MiMo model
  (`mimo-v2.6-flash-free`)** — the main tool. Used to:
  - read and interpret the homework spec (`ece361_hw01_fall_2026_r1.pdf`),
  - write and review `bits.h` / `bits.c` (Part 2), `status.h` /
    `status.c` (Part 3), `tests/test_bits.c` (Part 4), and the
    `Makefile`,
  - write the comments in the code and these two documents,
  - keep `AGENTS.md` and `.gitignore` in working order.
- **Python 3 (standard library only: `re`, `zlib`), run under WSL** — to
  extract the text of the homework PDF. This machine has no PDF tools (no
  `pdftotext`, no `pip`, and the chat tool refuses PDFs), so a small script
  decompressed the PDF's content streams and printed the spec text. This
  was used only to *read* the assignment, never to write code.
- **gcc, GNU make and git under WSL (Ubuntu)** — not AI. Every claim in the
  README was checked by actually building and running the code
  (`make`, `make test`, `make clean`), and `git check-ignore` was used
  whenever a git question came up.

## One thing a model got wrong, and how it was found

**The `.gitignore` pattern for the test binary.** The model first wrote the
pattern as `tests/test_bits`. A pattern that contains a slash is anchored
to the folder that holds `.gitignore`, so `tests/test_bits` only matches a
`tests/` folder at the very top of the repository — it would never match
`hw01/tests/test_bits`, and the compiled test binary could have been
committed by accident.

**How it was found:** not by reading it, but by checking it. The command

```sh
git check-ignore hw01/tests/test_bits
```

printed nothing for that path (while it did print the other paths I asked
about), which means "not ignored". The fix was to drop the slash: the
pattern `test_bits` contains no slash, so git treats it as a name that may
appear at any depth, and it matches `hw01/tests/test_bits` and any future
`hwNN/tests/test_bits`. Re-running `git check-ignore` on all five paths
confirmed the fix.

(While fixing that, the file itself turned out to be UTF-16 encoded, which
git cannot parse at all — no pattern in it matched anything. That was found
because `git status` kept listing the spec PDF as an untracked file. It was
rewritten as plain UTF-8.)

## One thing that had to be fixed

The first draft of the test program was over-engineered: it captured
`print_binary`'s output by redirecting stdout into a file with POSIX
functions (`dup`, `dup2`, `freopen`) and then compared the captured lines
with `strcmp`. That is a lot of unfamiliar machinery for a first C course.

It was caught on review: the acceptance criterion is simply that the code
compiles and the PASS/FAIL lines can be checked by eye. The file was
rewritten to what it is now — plain value comparisons plus a labelled
"check by eye" section for `print_binary`, using only `<stdio.h>`,
`<stdint.h>` and `<stdbool.h>`.