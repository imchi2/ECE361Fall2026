# iom361, version 4.0: the ECE 361 I/O module emulator

iom361 is a small C library that imitates an I/O board attached to a
microcontroller: up to 32 switches, up to 32 LEDs, an RGB LED, and an AHT20
temperature and humidity sensor. The board is a block of eight 32-bit
registers in memory. Your program reads and writes them the way it would on
real hardware, but it runs on your laptop, and your tests decide what the
"hardware" sees.

Roy Kravitz (roy.kravitz@pdx.edu) wrote iom361 for ECE 361, versions 1.0 (2023)
to 3.0 (2025). Christof Teuscher (teuscher@pdx.edu) wrote version 4.0 for Fall
2026. It keeps Roy's API: code written for version 3.0 compiles and runs with
version 4.0 after changing `#include "iom361_r3.h"` to `#include "iom361_r4.h"`.

## Files

| File | What it is |
|---|---|
| `iom361_r4.h` | the API: register map, return codes, and every function, documented |
| `iom361_r4.c` | the module. Compile it with your code |
| `test_iom361_r4.c` | 36 self-checking tests; `make test` runs them |
| `demo_iom361_r4.c` | a walk through the module, ported from Roy's `test_iom361_r3.c`; `make` builds it |
| `Makefile` | `make`, `make test`, `make clean` |

To use iom361 in your own program, copy `iom361_r4.h` and `iom361_r4.c` next
to your code, `#include "iom361_r4.h"`, and add `iom361_r4.c` to your build.
No other files and no `-lm` are needed.

## The registers

| Offset | Name | Access | Contents |
|---|---|---|---|
| `0x00` | `SWITCHES_REG` | read | one bit per switch, bit 0 first; 1 is on |
| `0x04` | `LEDS_REG` | read, write | one bit per LED, bit 0 first; 1 is lit |
| `0x08` | `RGB_LED_REG` | read, write | bit 31 enable; bits 23–16 red, 15–8 green, 7–0 blue duty cycle |
| `0x0C` | `TEMP_REG` | read | 20-bit ST: °C = ST / 2^20 × 200 − 50 |
| `0x10` | `HUMID_REG` | read | 20-bit SRH: %RH = SRH / 2^20 × 100 |
| `0x14`–`0x1C` | `RSVD1_REG`–`RSVD3_REG` | read, write | reserved |

## The functions

| Function | What it does |
|---|---|
| `iom361_initialize(num_switches, num_leds, &rc)` | sets up the module, returns the base address of the registers |
| `iom361_readReg(base, offset, &rc)` | reads one register |
| `iom361_writeReg(base, offset, value, &rc)` | writes one register; returns its contents afterward |
| `build_rgb_reg(enable, red, green, blue)` | builds a value for `RGB_LED_REG` |
| `iom361_set_display(on)` | **new:** turns the printing of LED and RGB LED writes on (default) or off |
| `iom361_seed(seed)` | **new:** seeds the random readings, for reproducible tests |
| `_iom361_set_switches(value)` | test function: sets the switches |
| `_iom361_set_sensor1(temp, humidity)` | test function: sets the sensor reading |
| `_iom361_set_sensor1_rndm(t_lo, t_hi, h_lo, h_hi)` | test function: sets a random sensor reading |

Return codes: 0 success; 1 wrong base, or bad arguments to `iom361_initialize()`;
2 offset out of range; 3 offset not a multiple of 4; 5 not initialized.

## Things to know

- **The display prints.** While the display is on, every write to `LEDS_REG` or
  `RGB_LED_REG` prints a line to standard output. Call
  `iom361_set_display(false)` when your output must not be mixed with it, for
  example in tests.
- **Sensor readings are close, not exact.** The registers hold 20-bit integers.
  A temperature you set reads back within 0.0001 °C, slightly above or
  slightly below: 91.0 °C reads back as 90.99998 °C. Round readings (to 0.1 °C,
  say) before you compare them with a threshold, or a test that sets exactly
  91.0 can land on the wrong side.
- **There is no clock.** Your program keeps time itself.
- **Call `iom361_initialize()` first.** It resets every register, including
  values set with the test functions.

## What changed from version 3.0

Version 3.0 had problems that students would hit; version 4.0 fixes them
without changing how the functions are called.

| # | Version 3.0 | Version 4.0 |
|---|---|---|
| 1 | did not compile on macOS: a `static` variable initialized with a call to `powf()`, which only gcc accepts | compiles with gcc (Linux, WSL) and clang (macOS) |
| 2 | `build_rgb_reg()` shifted an `int` by 31 (`enable << 31`): undefined behavior, reported by `-fsanitize=undefined` | shifts unsigned values |
| 3 | a warning under `-Wall -Wextra`: `writeReg` took an `int` offset, `readReg` an unsigned one | both take `uint32_t`; no warnings with `-std=c11 -Wall -Wextra -pedantic` |
| 4 | sensor values truncated, so a value read back up to 0.0002 °C low | rounded to the nearest register value |
| 5 | a temperature below −50 °C was undefined behavior; a humidity above 100 % overflowed 20 bits | clamped to the register range |
| 6 | more than 32 LEDs overflowed a buffer; `iom361_initialize()` checked nothing | 0 to 32 switches and LEDs, otherwise return code 1 and NULL |
| 7 | the test functions crashed if called before `iom361_initialize()` | they do not crash; reads and writes before initialization return code 5 |
| 8 | `readReg` did not check alignment: offset 1 read register 0 and reported success | return code 3, as in `writeReg` |
| 9 | every LED write printed, and `iom361_initialize()` printed two lines | initialization prints nothing; `iom361_set_display()` turns printing off |
| 10 | `iom361_initialize()` reseeded `rand()` from the clock: no reproducible tests, and student code using `rand()` was affected | `rand()` is untouched unless you call `iom361_seed()` |
| 11 | random values needed `float_rndm.c`, third-party code with no license, and `-lm` | one line inside `iom361_r4.c`; neither is needed |
| 12 | `writeReg` returned the value passed in, not the register contents | returns the register contents after the write |
| 13 | bits above the number of switches could be set | they read as 0 |
| 14 | comments: "AHT0", and a "24-bit" temperature | AHT20, 20-bit |

Not changed, on purpose:

- **The registers are not `volatile`.** On real hardware they must be, and week 6
  explains why. Here `iom361_initialize()` returns a plain `uint32_t*`, as in
  version 3.0; making the registers `volatile` behind it would change the API.
  Nothing outside this program changes the registers, so the model is correct
  without it.
- **The names starting with `_`** (`_iom361_set_switches()` and the others). The C
  standard reserves such names, but renaming them would break existing code.

## Testing version 4.0

`make test` builds `test_iom361_r4.c` with `-fsanitize=address,undefined` and
runs 36 checks. On 5-Oct-2026 all 36 passed, with no warnings, on macOS
(Apple clang, arm64) and on Linux (gcc 16.2). Roy's `test_iom361_r3.c` also
builds and runs against version 4.0 with only the `#include` changed.
