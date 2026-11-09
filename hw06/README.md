# hw06 — Replaying a scenario into a trace

This homework builds the device-independent half of the project: reading the
sensor and the switches through `iom361`'s registers, lighting LEDs without
disturbing the others, keeping the clock, and replaying a scenario file into
a trace of one line per tick. The device here is deliberately simple — **a
press of switch k toggles LED k**; in the project, a state machine takes its
place.

| File | What it is |
| ---- | ---------- |
| `sensor.h` / `sensor.c` | Part 1: `reading_t` and `sensor_read` — read, convert, round, stamp |
| `panel.h` / `panel.c` | Part 2: `panel_pressed`, `panel_released`, `panel_led`, `panel_toggle` |
| `scenario.h` / `scenario.c` | Part 3: `scenario_t`, `scenario_load`, `scenario_at` (no malloc, ≤ 1000 lines) |
| `main.c` | Part 3: `./replay scenario.txt` — the per-tick loop and the trace line |
| `iom361_r4.h` / `iom361_r4.c` | The provided register model, copied in unchanged |
| `tests/test_hw06.c` | Part 4: the unit tests (20 checks) |
| `scenarios/` | Part 4: two scenario files of our own |
| `expected/` | Part 4: their expected traces |
| `regress.sh` | Part 4: replays every scenario and `diff`s it against `expected/` |
| `Makefile` | `make`, `make test`, `make clean` |
| `README.md` / `AI_USAGE.md` | This file, and how AI was used |

## The device, in five bullets

- Every tick, the scenario's line **in force** sets the world: the tick's
  temperature, humidity and switches become what the emulated hardware
  holds. Values hold between lines.
- The clock is `start + tick`, one second per tick, printed in **UTC** with
  `gmtime` and `strftime`, so a trace is the same on every computer.
- The sensor and the switches are read **back through the registers**
  (`sensor_read`, `iom361_readReg`) — never from the scenario struct — so
  the trace shows what a real board would have seen, rounding and all.
- Each switch that **just went down** (off → on since the previous tick)
  toggles its LED. A release changes nothing; a held switch does nothing.
- One line per tick on stdout; everything that is not a trace line goes to
  stderr, because stdout is diffed against `expected/`.

## Building and running

Everything is built and run under WSL (Ubuntu), never with a native Windows
gcc. From PowerShell:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make clean"
```

Or from a WSL shell already inside `hw06/`, just `make`, `make test`,
`make clean`.

- `make` links `replay` from all five modules (`sensor`, `panel`,
  `scenario`, `main`, `iom361_r4`) **and** compiles every `.c` file —
  including the test program — to its own `.o` with
  `gcc -std=c11 -Wall -Wextra`: must produce **no warnings**.
- `make test` first runs `tests/test_hw06` (one `PASS`/`FAIL` line per
  check, a summary, **nonzero exit status on any failure**, which stops
  make there), then runs `sh regress.sh`, which replays every file in
  `scenarios/` and `diff`s each trace against the file of the same name in
  `expected/`.
- `make clean` removes everything the build produced.
- Each `.o` writes a `.d` file listing the headers it includes (that is
  what `-MMD -MP` does), so editing `scenario.h` rebuilds everything that
  includes it.

Try it:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && ./replay scenarios/press.txt"
```

```
2026-10-06 08:00:00 temp=20.0 rh=50.0 switches=0x0 pressed=0x0 leds=0x0
2026-10-06 08:00:01 temp=20.0 rh=50.0 switches=0x0 pressed=0x0 leds=0x0
2026-10-06 08:00:02 temp=21.5 rh=40.0 switches=0x1 pressed=0x1 leds=0x1
2026-10-06 08:00:03 temp=21.5 rh=40.0 switches=0x1 pressed=0x0 leds=0x1
2026-10-06 08:00:04 temp=21.5 rh=40.0 switches=0x0 pressed=0x0 leds=0x1
...
```

Read line 3 as: at tick 2 the world pressed switch 0 (`switches=0x1`), it
was off the tick before (`pressed=0x1`), so LED 0 toggled on (`leds=0x1`).
Line 5 is the release: `switches` drops to 0, but LEDs do **not** change —
only presses toggle.

## How the code works

### `sensor_read` — read first, change nothing on failure

```
read TEMP_REG   -> if it fails, return its code now (*r untouched)
read HUMID_REG  -> if it fails, return its code now (*r untouched)
keep bits 19..0 of each
convert with the formulas from iom361_r4.h (in double)
round each to 0.1
store temp, humidity and `now` into *r, then return 0
```

The order is the spec's "returns the iom361 return code of a failed read;
then it changes nothing": both reads land in local variables first, so a
failure on the second read cannot leave `*r` half-updated. Rounding is
hand-rolled (`value * 10`, add `0.5` for positives / subtract it for
negatives, truncate to `int`, divide by 10) — no `math.h`, no `-lm`, and it
works for the negative end of the temperature range.

### `panel_led` / `panel_toggle` — read-modify-write

```
reject n outside 0..31 with -1, before touching anything
mask = 1 << n
read LEDS_REG
change only bit n (set/clear for panel_led, flip for panel_toggle)
write the whole word back
```

Writing a word built from just `mask` would switch every other LED off —
the register holds **all** LEDs in one word, so the other bits have to be
read first and carried through. `panel_pressed` / `panel_released` are pure
bit arithmetic: `now & ~before` (newly on) and `before & ~now` (newly off).

### `scenario_load` — one pass, line by line

For every physical line: cut the comment at `#`, skip leading whitespace,
trim trailing whitespace, skip it if nothing is left. The first content
line must be `start <timestamp>`; every later line must be
`tick temp rh switches`, four fields exactly, `tick >= 0`, strictly
increasing, switches parsed as hexadecimal (`strtoul`, base 16, the `0x`
prefix allowed), and at most 1000 lines. The first line that breaks any
rule is returned — its **physical** line number, comments and blanks
counted, so the number matches what an editor shows.

`scenario_at(s, tick)` walks the lines in order and returns the last one
whose tick has arrived — that is what "the values hold between lines"
means. Before the first line nothing is in force yet: it returns `NULL`.

### `main` — the per-tick loop

```
load the scenario, refuse to continue if it is wrong or has no lines
iom361_initialize(4, 4) then iom361_set_display(false)
before = 0                                     (all switches start off)
for tick = 0 to the last line's tick:
    line = scenario_at(tick)                   (may be NULL early on)
    if a line is in force: set the world (_iom361_set_sensor1, _iom361_set_switches)
    now = start + tick
    sensor_read(base, now, &r)                 (through the registers)
    switches = read SWITCHES_REG               (through the registers)
    pressed = panel_pressed(switches, before); before = switches
    for each bit set in pressed: panel_toggle(base, k)
    leds = read LEDS_REG
    print the trace line (UTC via gmtime + strftime)
```

## Decisions we had to make (and defend)

The spec leaves several points open; these are our readings, tested and
documented:

1. **A switch already on at tick 0 counts as pressed.** `before` starts at
   0, so the first tick sees every initially-on switch as an off → on
   edge. The alternative (starting `before` from the first line's switches)
   would swallow a press the scenario clearly asks for at tick 0.
2. **Releases never touch the LEDs.** The spec says "a press of switch k
   toggles LED k" and the loop toggles "for every switch k just pressed" —
   `panel_released` exists and is tested, but the replay does not call it.
3. **Ticks must strictly increase.** Two lines at the same tick would fight
   over which one is in force, so a repeat is rejected like a decrease.
4. **Line numbers count every physical line**, comments and blanks
   included. A file that never declares a start line is wrong from line 1
   (an empty file lands there too); a file that cannot be opened is also
   reported as line 1, because `scenario_load`'s contract is "0 or a
   nonzero line number" and 0 must keep meaning "fine".
5. **Before the first line, the board keeps its initial state.**
   `scenario_at` returns `NULL` there, and `main` simply does not set the
   world — those ticks print the values `iom361_initialize` starts with.
   (Both of our scenarios start at tick 0, so this path is only exercised
   by the unit tests.)
6. **A file with only a start line loads, but will not replay.** Parsing
   succeeded, so `scenario_load` returns 0; `main` then refuses with "no
   tick lines to replay", because "the last tick" would not exist.
7. **Return codes:** `sensor_read` passes the iom361 code straight
   through; `panel_led` / `panel_toggle` return `-1` only for a bad `n`
   and the iom361 code for register failures; `scenario_load` returns `0`
   or the first wrong line, as the spec demands.
8. **Switch fields wider than 32 bits are rejected** rather than
   truncated — a silent truncation would hide a typo in the scenario.

## The tests

`tests/test_hw06.c` runs **20 automated checks**:

| Section | Checks | What it pins down |
| ------- | -----: | ----------------- |
| sensor | 2 | a reading of 91.0 °C reads back as 91.0 (the register can only hold 90.99998 — this tests the rounding), the timestamp is stamped, and a failed read leaves all three fields untouched |
| panel: edges | 4 | pressed/released for a press, a release, and a held switch; only the newly pressed bit counts |
| panel: LEDs | 5 | `panel_led` and `panel_toggle` keep the other LEDs; both reject `n` = −1 and 32 without touching the register |
| scenario | 9 | a load with comments and blank lines; the parsed field values; values holding between lines, at a line's tick, after the last line, and `NULL` before the first; no start line → line 1; a missing field → its line; decreasing ticks → its line; a repeated tick → its line |

A `FAIL` does not stop the run — each check reports on its own — but the
exit code is nonzero, which is what `make test` looks at.

## The regression test

```
scenarios/press.txt  ->  expected/press.txt   (7 ticks: press, release, press)
scenarios/hold.txt   ->  expected/hold.txt    (13 ticks: one press, held to the end)
```

`regress.sh` replays each scenario into a temp file (so a crashing
`./replay` is caught by its exit status, not hidden behind a pipe),
`diff`s it against `expected/`, prints `PASS`/`FAIL` per scenario, shows
the diff when one fails, and exits 1 if anything differed — so `make test`
fails with it. The expected traces were checked line by line against the
scenario files by hand before being saved, so the test compares against
the spec's answer, not against whatever the program happened to print.

## The Shine question to be ready for

> *"Why does your `panel_led` read the register before it writes it, and
> what goes wrong if it doesn't?"*

Because `LEDS_REG` is **one word holding all 32 LEDs**, and the call must
change exactly one bit of it. The read brings in the current state of the
other LEDs; the modify changes bit `n` only; the write puts the whole word
back. If you skip the read and write `1 << n` (or `~(1 << n)`) directly,
you overwrite every other bit with 0: lighting LED 2 on a board where LED 0
is lit would silently switch LED 0 off — with 4 LEDs, one `panel_led` call
would show up in the trace as `leds=` losing bits that nothing asked it to
touch. On real hardware there is a second, subtler problem: between the
read and the write an interrupt could change the register, and your write
would put the stale value back — which is why read-modify-write is a
classic race there. (Here nothing else writes the register, so the
clobbering is the answer that matters.)
