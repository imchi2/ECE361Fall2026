# AI usage for hw05

## Tools used, and what they were used for

- **OpenCode (the coding agent) with the MiMo model
  (`mimo-v2.6-flash-free`)** — the main tool. Used to:
  - read and interpret the homework spec (`ece361_hw05_fall_2026.pdf`),
  - write and review `lock.h` / `lock.c` (Part 1), `main.c` and
    `data/codes.txt` (Part 2), `tests/test_lock.c`, the `Makefile`,
    the bonus `check.h` / `check.c` / `tests/test_check.c`,
  - write the state diagram, the transition table and the explanations
    in `README.md` and `BONUS.md`,
  - draft the first version of the state diagram as a Mermaid graph
    (see below — it had to be redrawn twice).
- **Python 3 (standard library only: `re`, `zlib`), run on this
  machine** — to extract the text of the homework PDF. There are no PDF
  tools anywhere here (no `pdftotext`, no `pip`, and the chat tool
  refuses PDFs), so a small script split the PDF's objects, decompressed
  its `/FlateDecode` content streams, and mapped the bytes through each
  font's `/ToUnicode` table. Used only to *read* the assignment, never to
  write code.
- **gcc, GNU make and git under WSL (Ubuntu)** — not AI. Every claim in
  the README was checked by actually building and running the code:
  `make` (zero warnings), `make test` (60/60), `make bonus` (4/4),
  `make clean`, `./lock < data/codes.txt`, and `git check-ignore` for
  every gitignore question.

## One thing a model got wrong, and how it was found

**The first state diagram was unreadable.** The model drew the machine as
a Mermaid `stateDiagram-v2` where every "stay in the same state" case was
a self-loop edge with its own label. `LOCKED` has three of those loops and
`GOT1` has one, and the rendered labels overlapped: it was impossible to
tell which `action_t` belonged to which loop — exactly the thing a state
diagram is supposed to make obvious.

**How it was found:** the user read it and said so — it was not caught by
re-reading the picture, because the model had checked that the *content*
was right (all 16 transitions present) and not that the *layout* was
legible.

**What fixed it:** two redraws. First, a version with a rule that removes
loops entirely — arrows for transitions that change state, the
"nothing changes" cases written inside the state boxes. Then, because the
user asked for something simpler, the plain-text arrow list in
`README.md`. The final ASCII diagram in the README keeps the same rule:
nine arrows for the nine state changes, and the seven self-transitions
listed underneath, so no two labels ever sit on the same loop.

**The lesson worth keeping:** a diagram can be *correct* and still fail at
its only job. Checking "did I include every transition?" is not the same
as checking "can someone else read this?" — and only the second one was
ever going to be graded, both by the user and by the grader.

## One other thing that had to be fixed

The first run of the PDF extractor printed a stray `(` before every
single character (`C(h(r(is(to(f` instead of `Christof`). The bug was an
off-by-one in the script's string reader: it counted the *opening*
parenthesis of each `( ... )` string as content, because it incremented
the nesting depth before deciding whether to append the character. It was
found by comparing the output against the PDF on screen — the text was
obviously wrong — and fixed by consuming the opening parenthesis first
and only appending *nested* ones. The same script also needed its output
reconfigured to UTF-8 after the first successful extraction died on a
`−` (U+2212 minus sign) that the Windows console codec could not encode.
