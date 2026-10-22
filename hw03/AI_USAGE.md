# AI usage for hw03

## Tools used, and what they were used for

- **OpenCode (the coding agent) with the MiMo model
  (`mimo-v2.6-flash-free`)** — the main tool. Used to:
  - read and interpret the homework spec (`ece361_hw03_fall_2026.pdf`),
  - write `ptr.h`, `ptr.c`, `tests/test_ptr.c` and the `Makefile`,
  - write the comments in that code and these two documents,
  - review the result against the spec's bullet list (pointer arithmetic
    only, `const`, one comment per function, PASS/FAIL per test, and so on).
- **Python 3 (standard library only: `re`, `zlib`), run from PowerShell** —
  to extract the text of the homework PDF. This machine has no PDF tools (no
  `pdftotext`, no `pip`, and the chat tool refuses PDFs), so a small script
  decompressed the PDF's content streams, tokenised the text operators and
  mapped the bytes through each font's `/ToUnicode` table. Used only to
  *read* the assignment, never to write code.
- **gcc, GNU make and git under WSL (Ubuntu)** — not AI. Every claim in the
  README was checked by actually building and running it (`make`,
  `make test`, `make clean`), and `git check-ignore` was used to confirm the
  compiled test binary is not tracked.

## One thing a model got wrong, and how it was found

**The failure value of `minmax`.** The first read of the spec said
`minmax` returns **`1`** when it refuses to do anything. The spec actually
says **`−1`**: *"stores the smallest and largest of `a[0..n-1]` in `*min`
and `*max` and returns 0; returns −1 and changes nothing if `n < 1` or any
pointer is NULL."* The Shine asks the same thing in words — *"Why does your
minmax return −1 instead of storing something?"* — so `1` would have been
wrong in the code, in the README and in the answer to the examiner.

**Why it happened:** the PDF text extractor was broken, and the wrong value
came from its broken output rather than from the PDF. Two bugs, both found
by not trusting it:

1. each page's `/Font` resources were looked up with the name still
   carrying its leading `/` (`/F7` against a table keyed `F7`), so the
   lookup always missed and every font silently fell back to latin-1;
2. `/Resources<<...>>` was read non-greedily, so it stopped at the first
   nested `>>` (the `/ExtGState` sub-dictionary) and the font map came back
   empty.

Text drawn with the embedded subset fonts therefore decoded as shifted,
NUL-separated letters — `Homework 3` printed as `+ R P H Z R U N F` — and
the `−1` was one of the casualties.

**How it was found:** the raw content stream was dumped and inspected
directly. The header and the code-font paragraphs decoded perfectly
(`Christof Teuscher`, `the four functions of the pointer library`) while
whole body paragraphs did not — that asymmetry pointed at the font lookup,
not at the PDF. Both bugs were fixed, the spec was re-extracted, and the
return value came back as `−1`. The value in `ptr.c`, the README and the
tests was checked against that second extraction.

## One thing that had to be fixed

**Comments that looked like indexed subscripts.** Part 1 forbids `a[i]`
inside `ptr.c`. The first draft obeyed that in the code but broke it in the
prose: comments such as `/* walks a[1], a[2], ... */` and `*p == a[0]`
contain the exact text a grader (or a simple `grep`) would search for.

It was caught on review by grepping `ptr.c` for `[` — and the file header
even said *"there is not one `a[i]` in here"*, which is itself a match. All
of the comments were reworded to talk about "the first element", "the last
element" and "one past the end". Re-running the grep now finds no `[`
anywhere in `ptr.c`, in code or in comments.

## How the deliverable was checked

- `make` under WSL: **no warnings** with `gcc -std=c11 -Wall -Wextra`.
- `make test`: **17 tests, 0 failures**, `ALL TESTS PASSED`.
- The failure path was proved, not assumed: one expectation was
  temporarily changed to a value that cannot hold, `make test` printed
  `FAIL`, the summary showed `1 failure(s)` and `make` stopped with
  `Error 1`. The line was put back and the suite run again to green.
- `make clean` leaves only `ptr.h`, `ptr.c`, `tests/test_ptr.c`, the
  Makefile and the docs — and `git check-ignore -v hw03/tests/test_ptr`
  confirms the compiled binary is ignored by the `test_ptr` pattern in
  `.gitignore`.
- Every new file was checked for CR bytes (there are none): `.gitattributes`
  forces LF, so `make` behaves the same on a fresh clone.
