# hw05 — The combination lock as a table-driven finite state machine

This homework turns a written description into a finite state machine: four
states, four events, and a 4×4 transition table where one array lookup
replaces a chain of `if`s. The code is **3-1-4**, typed on a keypad, with a
door and a 5-second timer.

| File | What it is |
| ---- | ---------- |
| `lock.h` | Part 1: the three enums, the `transition_t` struct, the `extern` table, five prototypes |
| `lock.c` | Part 1: `lock_table` (all 16 entries) plus `lock_step`, `lock_event`, the name functions |
| `main.c` | Part 2: reads stdin, prints one line per event, then `final: STATE` |
| `data/codes.txt` | Part 2: a sample input of our own |
| `tests/test_lock.c` | Part 2: the test program (60 checks) |
| `Makefile` | `make`, `make test`, `make bonus`, `make clean` |
| `check.h` / `check.c` | Bonus: `fsm_check`, the static table checker |
| `tests/test_check.c` | Bonus: tests for the checker (4 checks) |
| `BONUS.md` | Bonus: the checker's output on three broken tables |
| `README.md` / `AI_USAGE.md` | This file, and how AI was used |

## The lock, in five bullets

- The code is **3-1-4**, typed on a keypad. Each right digit moves on; the
  third one opens the door (`UNLOCK`).
- A wrong digit beeps (`BEEP`) and starts over from the beginning, even in
  the middle of the code.
- While a code is being typed, a 5-second timer runs; if it runs out, the
  lock starts over, silently (`A_NONE`, no beep).
- While open, digits do nothing. Closing the door, or the timer running
  out, locks it again (`LOCK`).
- Closing the door while it is already locked, or while a code is being
  typed, changes nothing.

## State diagram

Arrows show the nine transitions that **change** state. The seven that
**stay where they are** have no arrow — they are listed under the picture,
so no loop is ever ambiguous.

```
                  3 / A_NONE          1 / A_NONE          4 / A_UNLOCK
     LOCKED  ================>  GOT1  ================>  GOT2  ================>  OPEN
        ^                         |                          |                        |
        |                         | wrong digit / A_BEEP      | wrong digit / A_BEEP    | door c or t
        |                         | timeout t / A_NONE        | timeout t / A_NONE      | / A_LOCK
        |                         |                          |                        |
        +<------------------------+--------------------------+------------------------+
```

Read it as: every vertical line drops onto the bottom rail, and the rail
carries everything back into `LOCKED` (the `<` shows the direction).

The seven self-transitions, spelled out:

```
      LOCKED --wrong digit / A_BEEP--> LOCKED     (beep, restart here)
      LOCKED --timeout t   / A_NONE--> LOCKED     (nothing to restart)
      LOCKED --door c      / A_NONE--> LOCKED     (already locked)
      GOT1   --door c      / A_NONE--> GOT1       (code in progress)
      GOT2   --door c      / A_NONE--> GOT2       (code in progress)
      OPEN   --right digit / A_NONE--> OPEN       (digits do nothing)
      OPEN   --wrong digit / A_NONE--> OPEN       (digits do nothing)
```

9 arrows + 7 stays = 16 = exactly the table below, so the picture and the
table cannot drift apart.

## Transition table

`from —event / action→ to`, all 16 cells:

| state \ event | `E_RIGHT` (right digit) | `E_WRONG` (wrong digit) | `E_CLOSE` (`c`) | `E_TIMEOUT` (`t`) |
| ------------- | ----------------------- | ----------------------- | --------------- | ----------------- |
| **LOCKED** | GOT1 / `A_NONE` | LOCKED / `A_BEEP` | LOCKED / `A_NONE` | LOCKED / `A_NONE` |
| **GOT1** | GOT2 / `A_NONE` | LOCKED / `A_BEEP` | GOT1 / `A_NONE` | LOCKED / `A_NONE` |
| **GOT2** | OPEN / `A_UNLOCK` | LOCKED / `A_BEEP` | GOT2 / `A_NONE` | LOCKED / `A_NONE` |
| **OPEN** | OPEN / `A_NONE` | OPEN / `A_NONE` | LOCKED / `A_LOCK` | LOCKED / `A_LOCK` |

The same thing as a list, one line per cell:

```
LOCKED -> GOT1    : digit 3, right  (E_RIGHT)   / A_NONE
LOCKED -> LOCKED  : wrong digit     (E_WRONG)   / A_BEEP
LOCKED -> LOCKED  : door c          (E_CLOSE)   / A_NONE
LOCKED -> LOCKED  : timeout t       (E_TIMEOUT) / A_NONE

GOT1   -> GOT2    : digit 1, right  (E_RIGHT)   / A_NONE
GOT1   -> LOCKED  : wrong digit     (E_WRONG)   / A_BEEP
GOT1   -> GOT1    : door c          (E_CLOSE)   / A_NONE
GOT1   -> LOCKED  : timeout t       (E_TIMEOUT) / A_NONE

GOT2   -> OPEN    : digit 4, right  (E_RIGHT)   / A_UNLOCK
GOT2   -> LOCKED  : wrong digit     (E_WRONG)   / A_BEEP
GOT2   -> GOT2    : door c          (E_CLOSE)   / A_NONE
GOT2   -> LOCKED  : timeout t       (E_TIMEOUT) / A_NONE

OPEN   -> OPEN    : any digit       (E_RIGHT)   / A_NONE
OPEN   -> OPEN    : any digit       (E_WRONG)   / A_NONE
OPEN   -> LOCKED  : door c          (E_CLOSE)   / A_LOCK
OPEN   -> LOCKED  : timeout t       (E_TIMEOUT) / A_LOCK
```

## Building and running

Everything is built and run under WSL (Ubuntu), never with a native Windows
gcc. From PowerShell:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make bonus"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make clean"
```

Or from a WSL shell already inside `hw05/`, just `make`, `make test`,
`make bonus`, `make clean`.

- `make` links the `lock` program and compiles **every** `.c` file to its
  own `.o` with `gcc -std=c11 -Wall -Wextra` — must produce **no warnings**.
- `make test` builds `tests/test_lock` and runs it: one `PASS`/`FAIL` line
  per check, a summary, and a **nonzero exit status if any check failed**,
  which makes `make test` fail too.
- `make bonus` builds `tests/test_check` and runs the checker tests.
- `make clean` removes everything the build produced.
- Each `.o` writes a `.d` file listing the headers it includes (that is
  what `-MMD -MP` does), so editing `lock.h` rebuilds everything that
  includes it.

Try the program on our data file:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && ./lock < data/codes.txt"
```

It prints one line per event in the spec's format, then the final state:

```
LOCKED 3 RIGHT -> GOT1 NONE
GOT1 3 WRONG -> LOCKED BEEP
...
GOT2 4 RIGHT -> OPEN UNLOCK
OPEN c CLOSE -> LOCKED LOCK
final: OPEN
```

Characters that are not events (letters, spaces, the newline at the end of
each line) are consumed silently — that is what `lock_event` returning `-1`
means, and it is why a data file can look like ordinary text.

## How the code works

### `lock_step` — one table lookup

```c
transition_t entry = lock_table[s][e];   /* the one lookup */
*action = entry.action;                  /* out-parameter: how to get 2 results from 1 function */
return entry.next;
```

No `if`, no `switch` on the state — an explicit Satisfactory criterion.
The entry itself says where to go and what to do, so `lock_step` never
needs to know what a state *means*.

### `lock_event` — character → event

Checked in order, first match wins:

1. an out-of-range state → `-1` (there is no "next digit" to ask about),
2. `'c'` → `E_CLOSE` and `'t'` → `E_TIMEOUT`, in **every** state — the
   table decides what they do,
3. a digit `'0'`–`'9'` → `E_RIGHT` when it is the next digit of the code
   for this state (`'3'` in LOCKED, `'1'` in GOT1, `'4'` in GOT2),
   otherwise `E_WRONG`,
4. anything else → `-1` (not an event).

### `main` — the loop

```
start in LOCKED
read a character
loop while it is not EOF:
    event = lock_event(state, c)
    if event < 0: skip it, read the next character
    otherwise:    next = lock_step(...); print the line; state = next
print "final: STATE"
```

There is **no real 5-second timer** in `main` — no `alarm()`, no signals.
The character `'t'` stands in for "the timer ran out", so a timeout can be
triggered from a file and tested.

## Decisions we had to make (and defend)

The spec leaves three points open; these are our readings, tested and
documented:

1. **A wrong digit in LOCKED beeps.** "A wrong digit beeps and starts over
   from the beginning" is read as a general rule: in LOCKED you are already
   at the beginning, so you stay and beep — `{LOCKED, A_BEEP}`.
2. **Digits while open report `E_WRONG`.** The rule is "a digit is `E_RIGHT`
   if it is the *next* digit of the code"; while open the code is complete,
   so no digit is next. Both OPEN cells are `{OPEN, A_NONE}` anyway, so
   "digits do nothing" holds either way — the event only matters for the
   printed line.
3. **A timeout in LOCKED is filled with `A_NONE`.** The timer only runs
   while a code is being typed, so this cell can never fire in practice —
   but all 16 entries must be written, and "nothing to reset" is the honest
   answer. Leaving it as `A_UNDEFINED` would be a bug the bonus checker
   reports.

Also worth knowing: `lock_step` does **no range checking** on purpose (the
spec asks for exactly one lookup), and `state_name` / `event_name` /
`action_name` return `"?"` for anything out of range instead of reading
past their arrays.

## The tests

`tests/test_lock.c` runs **60 automated checks**:

| Section | Checks | What it pins down |
| ------- | -----: | ----------------- |
| all 16 table entries | 16 | every cell's next state *and* action, against a second copy of the table written in the test itself |
| `lock_event`: digits | 9 | a right and a wrong digit in each typing state, `'0'` (nowhere in the code), digits while open |
| `lock_event`: `c` and `t` | 8 | both events in all four states |
| `lock_event`: not events | 5 | `'x'`, `'A'`, space, `'\n'`, `'?'` → `-1` |
| `lock_event`: bad state | 2 | out-of-range state → `-1` |
| name functions | 16 | all 13 names plus the three `"?"` cases (uses `strcmp` — plain standard C) |
| sequences | 4 | `3 1 4` opens, `3 3 1 4` does not, `3 1 t 4` does not, `3 1 4 c` locks again — each also checks the *last action* (`A_UNLOCK`, `A_BEEP`, `A_LOCK`) |

The expected table lives **again** in the test file on purpose: if the test
read `lock_table` to get its expected answers, a wrong entry would agree
with itself and pass.

A `FAIL` does not stop the run — each check reports on its own, so one
broken entry cannot hide the other fifteen — but the exit code is still
nonzero, which is what `make test` looks at.

## The bonus: `fsm_check`

```c
int fsm_check(const transition_t table[N_STATES][N_EVENTS],
              state_t start, FILE *out);
```

Four passes over the table, one printed line per problem, returns how many
it found (0 = clean):

1. **left out** — the action is `A_UNDEFINED` (the value a cell keeps when
   nobody wrote it, which is why it is 0),
2. **out of range** — a `next` that is not one of `LOCKED`..`OPEN`,
3. **unreachable** — a state that cannot be reached from `start`, found by
   searching **breadth first** over the table,
4. **no way out** — every event from a state points straight back at it.

The checker never *runs* the machine: the whole behaviour lives in data,
so the data can be inspected statically. Run it with `make bonus`; the
output on three deliberately broken tables, and why each break is a real
bug for a lock, is in [`BONUS.md`](BONUS.md).

## The Shine question to be ready for

> *"Why does 3 3 1 4 not open, and which table entry would you change so
> that it does?"*

`3` moves LOCKED → GOT1. The second `3` is **wrong in GOT1** (GOT1 wants
`1`), so `[GOT1][E_WRONG] = {LOCKED, A_BEEP}` resets the machine; the `1`
and the `4` then arrive in LOCKED, where both are wrong. The entry to
change is **`[GOT1][E_WRONG]`** — make it `{GOT1, A_NONE}` (ignore a wrong
digit instead of resetting) and `3 3 1 4` becomes GOT1 → GOT1 → GOT2 →
OPEN. That is also *why* it would be a bad lock: wrong digits would no
longer reset, so an attacker could pad any guess with junk digits.
