# BONUS.md — the table checker on three broken tables

The bonus is `fsm_check` in `check.c` / `check.h`:

```c
int fsm_check(const transition_t table[N_STATES][N_EVENTS],
              state_t start, FILE *out);
```

It reads a transition table the way you would on paper — it never runs the
machine — prints **one line per problem** to `out`, and returns how many it
found (0 means clean). Four passes, four kinds of problem:

1. **left out** — an entry whose action is `A_UNDEFINED`. A cell nobody
   wrote keeps `{0, 0}` = `{LOCKED, A_UNDEFINED}`, and 0 is exactly what
   an omitted entry contains.
2. **out of range** — a `next` state that is not one of `LOCKED`..`OPEN`.
   The next `lock_step` would index `lock_table` with it: undefined
   behaviour in C.
3. **unreachable** — a state that cannot be reached from `start`, found by
   searching **breadth first** over the table.
4. **no way out** — a state where every one of the four events points
   straight back at itself, so entering it means staying forever.

Run it with:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make bonus"
```

`tests/test_check.c` checks the *counts*: the real table must come back
with 0, and each broken copy with the number predicted below.

---

## First: the real table is clean

```
$ ./tests/test_check
---- the real lock_table (expect no problems) ----
PASS  fsm_check(lock_table) found 0 problems
```

No problem lines at all: every entry is written, every `next` is a real
state, every state is reachable from `LOCKED`, and every state has an
event that leaves it. (This is also a test of the checker itself — a
checker that always complains would fail here.)

---

## Broken copy 1: the unlock cell was forgotten

The bug to simulate: someone writing the table by hand skips
`[GOT2][E_RIGHT]` — the cell for "the last correct digit". An unwritten
cell contains `{0, 0}`, i.e. `{LOCKED, A_UNDEFINED}`:

```c
copy_table(broken);
broken[GOT2][E_RIGHT].next = LOCKED;
broken[GOT2][E_RIGHT].action = A_UNDEFINED;
```

**Output:**

```
left out: [GOT2][RIGHT] was never written (action A_UNDEFINED)
unreachable: OPEN cannot be reached from LOCKED
PASS  fsm_check(forgotten cell) found 2 problems
```

**Why each is a real bug for this lock:**

- **left out `[GOT2][RIGHT]`** — the correct final digit does the wrong
  thing: instead of `{OPEN, A_UNLOCK}` the machine reports `A_UNDEFINED`
  and drops back to `LOCKED`. The correct code can never open the door,
  and the program would print `... -> LOCKED UNDEFINED`. Note how
  *plausible* the broken cell looks: its `next` is `LOCKED`, which is a
  perfectly legal state — only the action field gives it away. That is
  why check 1 looks at `A_UNDEFINED` and not at `next`.
- **unreachable `OPEN`** — a knock-on effect of the same missing cell:
  `[GOT2][E_RIGHT]` was the *only* way into `OPEN`, so now no sequence of
  events can ever reach it. One forgotten entry does not stay a one-cell
  problem, and the breadth-first search proves it: starting at `LOCKED`
  and following every in-range `next`, `OPEN` is never visited.

---

## Broken copy 2: a next state one past the end

The bug to simulate: a typo writes `N_STATES` (4) as a next state — one
past the last real state:

```c
copy_table(broken);
broken[GOT2][E_TIMEOUT].next = (state_t)N_STATES;
```

**Output:**

```
out of range: [GOT2][TIMEOUT].next is 4, which is not a state
PASS  fsm_check(out of range) found 1 problem
```

**Why this is a real bug for this lock:** after typing `3 1`, a timeout
sets the state to 4. The very next `lock_step` executes
`lock_table[4][e]` — reading past the end of a four-row array, which is
undefined behaviour in C: it can print garbage states (`state_name(4)`
would print `"?"`), crash, or read whatever happens to sit in memory,
including a value that *looks* like `OPEN`. For a lock this is the worst
kind of bug: undefined behaviour triggered by waiting out the timer
mid-entry, i.e. by an attacker who can simply pause.

It is the only problem reported here on purpose: everything is still
reachable through the *other* events, every state still has a way out,
and every action is filled in — the other three checks correctly stay
quiet, which is what makes the single line worth trusting.

---

## Broken copy 3: OPEN has no way out

The bug to simulate: someone "fixes" the lock so that closing the door or
timing out while open no longer locks the machine:

```c
copy_table(broken);
broken[OPEN][E_CLOSE].next = OPEN;
broken[OPEN][E_CLOSE].action = A_NONE;
broken[OPEN][E_TIMEOUT].next = OPEN;
broken[OPEN][E_TIMEOUT].action = A_NONE;
```

**Output:**

```
no way out: every event from OPEN stays in OPEN
PASS  fsm_check(no way out) found 1 problem
```

**Why this is a real bug for this lock:** all four events from `OPEN` now
point at `OPEN`, so once the door is opened it can never be locked again —
the `c` key and the timer both do nothing. The lock is a one-way door:
you can get in, and the machine stays open until the program exits. That
is a security failure, not a cosmetic one, and it is invisible in testing
that only checks "does the code open the door".

Nothing else is wrong with this copy — every cell is written, every `next`
is a real state, and `OPEN` is still reachable — so again exactly one line
comes out.

---

## Summary of the three runs

| Copy | Mutation | Problems | Checker lines |
| ---- | -------- | -------: | ------------- |
| real table | — | 0 | *(none)* |
| 1 | `[GOT2][E_RIGHT]` forgotten | 2 | `left out` + `unreachable: OPEN` |
| 2 | `[GOT2][E_TIMEOUT].next = 4` | 1 | `out of range` |
| 3 | `OPEN` swallows `c` and `t` | 1 | `no way out` |

Three copies cover all four check types (copy 1 triggers two at once,
which is itself the lesson: one missing cell usually breaks more than one
thing).

## For the Shine

Two questions worth having an answer ready for:

- **Why breadth-first for reachability?** The table is a graph: states are
  nodes, `next` values are edges. BFS from `start` marks every state you
  can get to; anything unmarked can never happen. BFS (rather than
  depth-first) mirrors how the lock actually works — it explores by
  "number of steps from the start", and with a 4-node graph either would
  give the same answer, but BFS is the natural fit for "can this ever be
  reached".
- **Why does check 4 look at all four events?** A state only has "no way
  out" when *every* event returns to it — one event pointing anywhere else
  is enough escape. The loop sets a flag on the first escape and stops
  (`break`), so the check reads as "is there any way out?".
