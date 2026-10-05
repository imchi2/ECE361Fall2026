# hw01 — Bit manipulation and the thermostat status word

This homework is a small C library in four parts: printing binary numbers,
cutting fields out of (and poking fields into) a 32-bit word, sign-extending
two's-complement values, and unpacking a thermostat's 16-bit status word.

## Files

| File | What it is |
| ---- | ---------- |
| `bits.h` / `bits.c` | Part 2: the bit-manipulation library |
| `status.h` / `status.c` | Part 3: the status word layout and `status_unpack` |
| `tests/test_bits.c` | Part 4: the test program |
| `Makefile` | builds every `.c` to its own `.o` and runs the tests |

## What the library does

### Part 2: `bits.h` / `bits.c`

Bits are numbered from 0, the least significant bit.

- `void print_binary(uint32_t x, int width)` — prints the lowest `width`
  bits of `x`, most significant bit first, in groups of four separated by a
  single space. `print_binary(0x2C, 8)` writes `0010 1100`. It emits **no
  trailing newline**; the caller prints its own.
- `uint32_t get_field(uint32_t word, int pos, int width)` — returns bits
  `pos` to `pos + width - 1` of `word`, shifted down so the field starts at
  bit 0.
- `uint32_t set_field(uint32_t word, int pos, int width, uint32_t value)` —
  returns `word` with bits `pos` to `pos + width - 1` replaced by the lowest
  `width` bits of `value`. Every other bit is unchanged.
- `int32_t sign_extend(uint32_t value, int width)` — reads the lowest
  `width` bits of `value` as a two's-complement number and returns it as a
  signed 32-bit integer. `sign_extend(0xF8, 8)` returns `-8`.

### Part 3: `status.h` / `status.c`

A thermostat reports its state in one 16-bit status word:

| Bits | Field | Meaning |
| ---- | ----- | ------- |
| 0 | HEAT | 1 = heater on |
| 1 | COOL | 1 = compressor on |
| 2 | FAN | 1 = fan on |
| 3 | FAULT | 1 = fault detected |
| 6–4 | MODE | 0 = OFF, 1 = HEAT, 2 = COOL, 3 = AUTO, 4 = FAN_ONLY; 5 to 7 are invalid |
| 7 | reserved | must be 0 |
| 15–8 | SETPOINT | set point in degrees C, 8-bit two's complement (−128 to 127) |

`status_t status_unpack(uint16_t word)` takes such a word apart into a
`status_t` struct with one member per field. It is built entirely on top of
`get_field` and `sign_extend` from Part 2, and every field position and
width is a named constant in `status.h` (`STATUS_POS_*`, `STATUS_WIDTH_*`,
`STATUS_MODE_*`) — no magic numbers.

## Building and running the tests

Everything is built and run under WSL (Ubuntu), never with a native Windows
gcc. From PowerShell:

```sh
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make test"
wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make clean"
```

Or from a WSL shell already inside `hw01/`, just `make`, `make test`,
`make clean`.

- `make` compiles each `.c` file to its own `.o` file with
  `gcc -std=c11 -Wall -Wextra` and must produce **no warnings**.
- `make test` builds `tests/test_bits` from those objects and runs it. The
  program prints one `PASS` or `FAIL` line per check plus a summary, and
  **exits with a nonzero code if any check fails**, which makes `make test`
  fail too.
- `make clean` removes everything the build produced.

## Valid ranges of the inputs

| Function | Argument | Valid range |
| -------- | -------- | ----------- |
| `print_binary` | `width` | 1 to 32 |
| `get_field`, `set_field` | `width` | 1 to 32 |
| `get_field`, `set_field` | `pos` | 0 to 31 |
| `get_field`, `set_field` | `pos + width` | at most 32 |
| `sign_extend` | `width` | 1 to 32 |
| `status_unpack` | `word` | every 16-bit value is accepted |

After unpacking: `setpoint` is in −128 to 127, and `mode` is 0 to 7 (0 to 4
are the valid modes, 5 to 7 are invalid — see below).

## Behavior at the boundaries

- **`width` 1** — the smallest legal field: a single bit.
- **`width` 32** — the whole word, and the trickiest case in C: shifting a
  32-bit value by 32 is undefined behaviour, so `get_field`, `set_field` and
  `sign_extend` all special-case it with a full mask of `0xFFFFFFFF`
  instead of shifting. A width-32 field is only legal at `pos` 0.
- **`pos` 31** — the highest legal start of a field:
  `set_field(0, 31, 1, 1)` returns `0x80000000`.
- **A value too wide for its field** — `set_field` keeps only the lowest
  `width` bits of `value` and drops the rest:
  `set_field(0, 4, 4, 0x12345678)` returns `0x80`.
- **Most negative `sign_extend` value** —
  `sign_extend(0x80000000, 32)` returns `INT32_MIN` (−2147483648), and
  `sign_extend(0x80, 8)` returns −128.
- **`print_binary` grouping** — groups are counted from the most
  significant side, so a width that is not a multiple of four starts with a
  short group: width 6 prints `10 1111`.
- In every function the argument checks run **before** any shift or mask is
  evaluated, so a rejected call never reaches undefined behaviour.

## Out-of-range behavior (our Part 2 choice)

The spec asks us to decide what happens outside the valid ranges and to
defend it — "it is undefined" is not an answer. We **reject
deterministically**: the arguments are validated first, and the function
returns one fixed, documented result.

| Function | Result when the arguments are out of range |
| -------- | ------------------------------------------ |
| `get_field` | returns 0 |
| `set_field` | returns `word` unchanged (a true no-op) |
| `sign_extend` | returns 0 |
| `print_binary` | prints nothing at all |

Why this choice: it is simple to state, simple to test, and it never lets a
bad argument reach a shift or mask that C leaves undefined. Returning 0
means "no bits were read or produced"; returning `word` unchanged means
"nothing was written". All of these are covered by the tests (widths 0, −1
and 33; positions −1 and 32; and `pos + width` reaching past 32).

## How `status_unpack` reports an invalid mode

MODE values 5, 6 and 7 are invalid. `status_t` reports that in two steps:

1. `mode` keeps the **raw** 3-bit value exactly as it was read (5, 6 or 7),
   so no information from the word is lost.
2. an extra member, `bool mode_valid`, is **false** for modes 5 to 7 and
   **true** for modes 0 to 4.

Example: `status_unpack(0x0051)` has mode bits `101` (= 5) and the heater
bit set, so it returns `mode == 5`, `mode_valid == false`, `heat == true`.

`status_unpack` never rejects a word: a word that breaks the rules (invalid
mode, or the reserved bit set) is still reported field by field, and the
caller decides what to do about it.

The spec's own example checks out: `status_unpack(0x1631)` gives set point
22, mode 3 (AUTO), heater on, compressor off, fan off, no fault, reserved
clear, and `mode_valid == true`.

## The tests

`tests/test_bits.c` runs **44 automated checks**: every Part 2 function at
the boundaries listed above, plus `status_unpack` on four words (`0x1631`,
`0x8000`, `0x0051`, `0xFFFF`). Each check prints `PASS` or `FAIL`, the
program prints a summary, and it returns a nonzero exit code if anything
failed.

`print_binary` writes to the screen itself, so its seven cases are checked
by eye: each line prints the expected text first, then the real output
after the arrow. The out-of-range cases are wrapped in `[ ]` so you can see
that they print nothing between the brackets.
