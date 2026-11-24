# AI usage — hw09

Tools: OpenCode (DeepSeek V4.1 Flash Free) for writing `ht.h`, `ht.c`,
`tests/test_ht.c`, the `Makefile`, and this README. gcc under WSL
(Ubuntu) with `-Wall -Wextra` and again with
`-fsanitize=address,undefined` for the real verification; the Table 2
chain lengths were checked by running the tests, not by trusting the
analysis.

One thing a model got wrong: the very first repository command was
written as a POSIX one-liner

```
cd C:\ECE361_C && git status --short && git log --oneline -3
```

which the agent ran in PowerShell, where `&&` is not a statement
separator, so it failed with `The token '&&' is not a valid statement
separator in this version` before git ever ran. The fix was to call
`git -C C:\ECE361_C ...` as separate commands (and to keep all real
builds in WSL), which is the form used for the rest of the work.
