# AI usage — hw07

Tools: OpenCode (MiMo) for writing `istack.h`, `istack.c`, `balance.c`,
`tests/test_istack.c`, `regress.sh` and the `Makefile`; gcc/ASan under
WSL for the real verification.

One thing a model got wrong: the first version of
`tests/test_istack.c` checked the values kept across the doubling
starting at `i = 8`, so it expected `80` first — but the 9th push had
stored `90`, and the check failed until the loop was corrected to pop
`90` first and then `70` down to `0`. Also, an early `regress.sh` kept
the `.txt` suffix when building the expected-file names, so every case
looked missing until `name=${name%.txt}` was added.
