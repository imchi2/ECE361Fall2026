# HW2 module diagram

ECE 361, Fall 2026, homework 2: split `readings.c` into modules without
changing what the program prints.

## Before: one file

`readings.c` works, but everything lives in one place:

- four global arrays (`ticks`, `temps`, `hums`, `count`) plus `skipped`
  and `threshold`;
- `main()` that parses arguments, reads stdin, computes every statistic,
  and prints every line;
- the temperature and humidity summaries are the same loop written twice
  (the comment on line 94 even admits it: *"the same loop again"*);
- `max_temp()` recurses to find the largest temperature, while min and
  mean are found with a plain loop right next to it;
- the histogram bins are filled and printed inside `main()`.

## After: five modules

```
                        +--------------------------------------+
                        |  main.c   (the driver)               |
                        |  - parse argv -> threshold           |
                        |  - call each module in order         |
                        |  - print the summary lines           |
                        +-------+------+----------+------------+
                                |      |          |
        +-----------------------+      |          +------------------+
        v                              v                             v
+-------------------+        +------------------+          +------------------+
| input.h / input.c |        | stats.h/stats.c |          | run.h / run.c    |
| read_dataset()    |        | stats_of()      |          | longest_run()    |
|                   |        |                  |          |                  |
| reads stdin line  |        | one pass over a  |          | longest stretch  |
| by line, skips     |        | float array ->   |          | of temps above   |
| blank/# lines,    |        | {min, max, mean} |          | threshold ->     |
| counts malformed, |        |                  |          | {length, start}  |
| warns on overflow |        | replaces BOTH    |          |                  |
|                   |        | copy-pasted loops|          | replaces the     |
|                   |        | AND max_temp()   |          | inline run logic |
|                   |        | (recursion dies) |          |                  |
+--------+----------+        +--------+---------+          +--------+---------+
         |                            |                             |
         | fills                      | reads                       | reads
         v                            v                             v
   +--------------------------------------------------------------------------+
   |  readings.h   - the one shared header: struct dataset { ticks[],         |
   |                 temps[], hums[], count, skipped }  + MAX_READINGS        |
   +--------------------------------------------------------------------------+

                        +--------------------------------------+
                        |  histogram.h / histogram.c          |
                        |  hist_print(dataset*)               |
                        |  - fills 20 bins (5 C) in one loop  |
                        |  - prints "histogram:" and the bars |
                        +--------------------------------------+
                          (called last by main, reads dataset)
```

## What each module owns

| File | Owns | Replaces |
|---|---|---|
| `readings.h` | The data type only. No logic, just the struct plus `MAX_READINGS`. | The four untyped globals |
| `input.c` | Turning bytes into a `dataset`. | The `fgets`/`sscanf` loop inside `main()` |
| `stats.c` | Numeric summaries. | The temperature loop, the humidity loop, and the recursive `max_temp()` — one iterative pass covers all three |
| `run.c` | "Longest run above the threshold". | Lines 108–121 inline in `main()` |
| `histogram.c` | Binning plus its own output format. | Lines 129–146 inline in `main()` |
| `main.c` | Argument parsing, call order, and the summary `printf` lines. | Everything else; should shrink to roughly 40 lines |

## Rules that keep it a diagram instead of a hairball

1. **No globals.** `main()` owns the `dataset` and passes `&ds` down.
   Every module receives it as a parameter.
2. **One header per module, and `main()` includes them all.** A module
   never includes another module's header — they only meet at
   `readings.h`, so every arrow points one way.
3. **The output stays byte-identical.** `main()` still prints
   `readings:`, `skipped:`, `temperature:`, `humidity:` and the
   `above ... C:` line; `histogram.c` prints its own two-part lines
   because that format is welded to the bin data; `input.c` keeps the
   overflow warning on `stderr`; `main()` keeps the `count == 0` early
   return, because `stats.c` would divide by zero on an empty input.

## Checking the refactor

```
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw02 && make test"
```

`regress.sh` runs the program on every file in `data/` and `diff`s the
output against the same name in `expected/`, plus `edge.txt` with a
threshold of 25. All four must PASS — that is the definition of
"without changing what it prints".
