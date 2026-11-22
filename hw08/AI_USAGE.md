# AI usage — hw08

Tools: OpenCode (DeepSeek V4.1 Flash Free) for writing `bst.h`,
`bst.c`, `treestats.c`, `tests/test_bst.c`, `tests/test_remove.c`, the
`Makefile`, `README.md` and `BONUS.md`; gcc with
`-fsanitize=address,undefined` under WSL for the real verification,
including reproducing the empty-tree crash before fixing it.

One thing a model got wrong: when drafting the crash section of the
README, the model first quoted the AddressSanitizer
`==...==ERROR: AddressSanitizer: SEGV ...` line as "the first line of
the report". The actual first line printed is the UBSan line
`bst.c:193:19: runtime error: member access within null pointer of type
'struct node'`, which fires before ASan prints its block; the README
now quotes both, in the order they appeared.
