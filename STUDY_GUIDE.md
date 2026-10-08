# Study Guide: C, the Machine, and Bits

A concept-first guide. Every entry answers four questions in order:

1. **What is it?** — a plain-language definition
2. **Why does it exist?** — the problem it solves
3. **What does it look like in memory?** — the picture you should hold in your head
4. **The trap** — the beginner mistake that follows from misunderstanding it

The indented bullet under a concept is always its explanation. Deeper
indentation means "this is a detail of the thing above it."

---

## 1. What happens when you run `gcc`

- **A C program does not run. It is *translated*.**
    - You write text (`hello.c`). The machine executes ones and zeros.
    - Something in between has to turn one into the other.
    - `gcc` is not one program — it is a **pipeline of four programs** that
      gcc invokes for you in order.
        - Stages must run in this order because each stage's *output format*
          is the next stage's *input format*.

```
hello.c  --preprocessor-->  hello.i   gcc -E hello.c   (text after pasting headers)
hello.i  --compiler------>  hello.s   gcc -S hello.c   (assembly: human-readable CPU ops)
hello.s  --assembler----->  hello.o   gcc -c hello.c   (object file: raw machine code)
hello.o + other.o --link--> hello     gcc hello.o other.o -o hello
```

- **Stage 1 — the preprocessor: pure text editing, no understanding at all**
    - It does exactly three kinds of things:
        - `#include <stdio.h>` — *paste the whole file right here*
        - `#define UPPER 300` — *find the word `UPPER` and replace it with `300`*
        - `#ifdef / #ifndef / #if / #else / #endif` — *delete the lines that
          are turned off*
    - It never knows what a variable or a function is. It only manipulates text.
        - This is why `#define` has no type, no scope, and no semicolon —
          it is not a C construct at all, it is a *find-and-replace* rule.
    - After this stage the file contains **zero** `#include` and **zero**
      `#define` names — everything has been expanded in place.
    - See the result yourself: `gcc -E hello.c`
    - **Trap:** because it is text substitution, `#define SQUARE(x) x * x`
      followed by `SQUARE(1 + 2)` becomes `1 + 2 * 1 + 2` = **5**, not 9.
      The preprocessor does not know about multiplication precedence rules
      because it does not know about multiplication.

- **Stage 2 — the compiler: where the actual C happens**
    - Reads C, checks it against the standard, and emits **assembly** —
      a text listing of CPU instructions (`mov`, `add`, `jmp`, `call`).
    - This is where `-std=c11`, `-Wall`, `-Wextra` matter: they change *which*
      rules the compiler enforces and *how loudly* it complains.
    - If you want to see how expensive a C construct is, look at the `.s` file.

- **Stage 3 — the assembler: text instructions become bytes**
    - Turns each assembly line into the numeric opcode the CPU executes.
    - Output is an **object file** (`.o`, or `.obj` on Windows).
    - An object file is *almost* a program. It is machine code, but any
      function it calls that lives elsewhere still just says **"I need this
      name; someone else resolve it later."**

- **Stage 4 — the linker: stitching object files together**
    - Takes several `.o` files and produces one executable.
    - Its whole job is **name resolution across files**:
        - your `main.o` says "I need `get_field`"
        - `bits.o` says "here I am, `get_field` lives at this offset"
        - the linker wires them together
    - See it: `gcc hello.o bits.o status.o -o hello`

- **The two error types map exactly onto two stages — this is diagnostic gold**
    - **Compiler error** (stage 2): the C is not valid.
        - "expected `;`", "undeclared identifier", "type mismatch"
        - Fix: fix the source file.
    - **Linker error** (stage 4): the C was valid, but a *name* could not be
      resolved.
        - **`undefined reference to 'get_field'`** — someone *declared* the
          function but no `.o` file *defines* it. Either the `.c` file is
          missing from the build, or you never wrote the function.
        - **`multiple definition of 'counter'`** — two `.o` files both *define*
          the same global. Usually caused by putting a definition (not
          `extern`) in a header that many files include.
    - **Trap:** seeing "undefined reference" and editing the *header*. The
      header is not the problem. The problem is that no object file contains
      the definition, or it was not passed to the linker.

- **Why headers exist at all**
    - A header is *not* compiled. It is **text pasted into `.c` files** by the
      preprocessor.
    - Its only job is to **declare**: "this name exists, with this signature."
      Declarations are what lets two `.c` files agree on how to talk.
    - The **definition** (the actual body, the storage) lives in exactly one
      `.c` file.
        - Declaration = "there will be a function called `get_field`."
        - Definition = "here is `get_field`, actually implemented."
    - A program: **declared many times, defined exactly once.**
        - Declared many times: every file that calls it includes the header.
        - Defined once: the linker would reject a second definition.
    - **Include guards** (`#ifndef BITS_H / #define BITS_H / #endif`) exist
      because the preprocessor *will* happily paste the same file twice if
      one header includes another. The guard makes the second paste empty.
    - **Trap:** a header with `int counter = 0;` in it. Every `.c` that
      includes it gets its own *definition*. Fix: `extern int counter;` in
      the header (declaration only), `int counter = 0;` in one `.c`.

- **Separate compilation is the whole point**
    - Change `bits.c` → only `bits.o` is rebuilt → only the link step reruns.
    - Change `bits.h` → every `.c` that includes it is rebuilt (they all
      contain a copy of its text, so they all might have changed).
    - This is *why* `make` exists: tracking "what changed, and what depends
      on it" by hand does not scale past three files.

- **Compiler flags worth internalizing**
    - `-std=c11` — enforce the C11 standard instead of a GNU dialect.
    - `-Wall` — all *standard* warnings.
    - `-Wextra` — extra warnings beyond `-Wall`, notably **comparing signed
      with unsigned**.
    - `-g` — embed debugging info (file/line/symbol table) for `gdb`/`valgrind`.
    - `-c` — stop after the assembler; produce `.o`, don't link.
    - `-o NAME` — name the output.
    - `-fsanitize=address` — insert run-time checks that catch out-of-bounds
      and use-after-free *as they happen*.
    - **A warning is a bug report from a program that reads your code more
      carefully than you do. Zero warnings is the goal, not "no errors."**

---

## 2. Memory: the one model that explains everything else

If you understand this section, scope, lifetime, pointers, arrays, strings,
and `static` all stop being separate topics.

- **Memory is a very long array of bytes.**
    - Each byte has a unique **address**: a number identifying where it is.
    - Analogy: a huge hallway of lockers, each locker numbered 0, 1, 2, …
        - A byte is one locker.
        - An address is the locker number.
    - On a 64-bit machine addresses are 64 bits wide, so they can name up to
      2^64 locations.

- **A variable is a name for some bytes at some address.**
    - `int x = 42;` does three things:
        1. find 4 consecutive free bytes
        2. remember "those bytes are called `x` and hold an `int`"
        3. write the bit pattern for 42 into them
    - The *name* exists only in your source code. The compiler replaces it
      with an address (or an offset from one) by the time it emits assembly.
        - **This is why the same name can be reused in two functions without
          conflict: the names are gone by then.**

- **The type decides how to *read* the bytes, not what the bytes are.**
    - The same 4 bytes `00 00 00 2A` are `42` as `int`.
    - Flip one bit and they might be a `float`, or part of an address.
    - **Bits have no meaning. Types are the labels you attach.**

- **Two places bytes can live — this is the key distinction**

    - **Automatic storage** (ordinary locals): lives in the current
      **stack frame**, created when the block is entered, destroyed when it
      is left.
        - Picture: a desk. You spread papers out when you sit down; when you
          stand up, the papers are gone (well — the *desk space* is reused,
          and the old papers are still lying there until someone else
          overwrites them). That leftover is **garbage**.
        - Reading a local you never assigned reads whatever the previous
          function left on that desk → **undefined behavior**.

    - **Static storage** (globals, `static` locals): allocated once when the
      program starts, lives for the whole run, **zero-initialized**.
        - Picture: a filing cabinet. Everything in it starts empty/zero and
          is never automatically cleared.
        - Never garbage, because the program loader fills it before `main`
          runs.

    - A third place, **heap** (`malloc`), exists but is out of scope here —
      it is memory you request and must release yourself.

- **A stack frame is one function call's workspace**
    - When `main` calls `foo(3)`, the machine:
        1. copies the argument `3` into a slot belonging to *this call*
        2. records where to come back to
        3. jumps to `foo`
        4. gives `foo` its own slots for its locals
    - When `foo` returns, that frame is done — its locals' lifetime ends.
    - **Two calls to the same function are two different frames**, so each
      call has its own private locals. That's why a local can't be shared
      between callers without a pointer.

- **This model already predicts the scope/lifetime table in section 10.**
    - Visible *where* → lexical position in the source.
    - Lives *how long* → which kind of storage it's in.
    - Never memorize the table; derive it.

---

## 3. Types: how many bytes, and how to read them

- **A type answers two questions: how many bytes, and what does the bit
  pattern mean.**
    - `int x;` → 4 bytes, read as a whole number.
    - `float f;` → 4 bytes, read as IEEE 754 (sign / exponent / fraction).
    - Same size, completely different meaning.

- **Sizes are not fixed by the standard — only *minimums* are.**
    - The standard says "at least this much," because C runs on 16-bit
      microcontrollers and 64-bit workstations alike.

    | Type | Linux/macOS 64-bit | Windows 64-bit | Guaranteed at least |
    |---|---|---|---|
    | `char` | 1 byte | 1 byte | 8 bits |
    | `short` | 2 bytes | 2 bytes | 16 bits |
    | `int` | 4 bytes | 4 bytes | 16 bits |
    | `long` | **8 bytes** | **4 bytes** | 32 bits |
    | `long long` | 8 bytes | 8 bytes | 64 bits |
    | pointer | 8 bytes | 8 bytes | — |

    - **`long` is the one that differs between Linux and Windows.**
        - Linux/macOS: pointers and `long` are both 64-bit (**LP64**).
        - Windows: `long` stays 32-bit while pointers are 64-bit (**LLP64**).
        - Consequence: any code that assumes "`long` = pointer-sized" breaks
          when moved between them.

    - **Trap:** `printf("%ld", ...)` on a value that isn't a `long`, or a
      hand-typed `%lu` for a `uint32_t`. The mismatch is undefined behavior
      and often silently prints garbage.

- **Ask the machine instead of guessing**
    - `printf("%zu\n", sizeof(long));`
    - `%zu` is the format for `size_t`, which is what `sizeof` returns.
    - Browse `<limits.h>` for `INT_MAX`, `INT_MIN`, `CHAR_BIT`, `LONG_MAX`.

- **`signed char` vs `unsigned char` vs plain `char` — three different types**
    - Plain `char` may be signed or unsigned — **the compiler picks**.
        - On x86 Linux it is usually signed; on ARM and PowerPC it is often
          unsigned.
        - If you store a value above 127 in a plain `char` and then compare
          it as a number, the result depends on the platform.
    - Use `signed char` / `unsigned char` whenever the *value* matters.
    - Use plain `char` for **characters and bytes of text**.

- **Fixed-width types: use them whenever the bit count is part of the meaning**
    - `<stdint.h>` gives you exactly what it says:
        - `int8_t, int16_t, int32_t, int64_t` — exactly N bits, two's complement
        - `uint8_t … uint64_t` — exactly N bits, unsigned
    - Why they exist: `int` is 16 bits on some targets and 32 bits on others.
      When you are packing a hardware register, a sensor reading, or a file
      format, "about 32 bits" is not good enough — you need *exactly* 32.
    - Related helpers:
        - `int_least16_t` — smallest type with **at least** 16 bits
        - `int_fast16_t` — fastest type with **at least** 16 bits (may be wider)
        - `uintptr_t` — unsigned integer big enough to hold a pointer
        - `size_t` — type of `sizeof` and array sizes; **unsigned**, because a
          size cannot be negative

- **Printing fixed-width types needs `<inttypes.h>`**
    - ```c
      printf("%" PRIu32 "  0x%08" PRIX32 "\n", x, x);
      ```
    - Why the weird split string: the macro expands to two string *tokens*,
      `"%"` and `"u"`, and **adjacent string literals are concatenated** by
      the compiler, producing `"%u"`.
    - On some systems the same macro expands to `"%" "lu"` because
      `uint32_t` *is* `unsigned long` there. Your hand-typed `"%u"` would
      then be wrong; the macro follows the platform automatically.
    - `PRId8`, `PRIx32`, `PRIX64`, `PRIu64` … the letters are mnemonic:
      **P**rint **R**eserved **i**nteger, **d**ecimal / **x** hex / **X**
      uppercase hex / **u** unsigned.
    - Peeking behind the curtain:
      `gcc -dM -E -include inttypes.h - </dev/null | grep PRI`

- **Floating point is a separate encoding, not a decimal**
    - `float` = 4 bytes ≈ 7 significant digits, max ≈ 3.4e38
    - `double` = 8 bytes ≈ 16 significant digits, max ≈ 1.8e308
    - `long double` = 8–16 bytes, platform-dependent
    - They use **IEEE 754**: a sign bit, an exponent, and a fraction.
        - Because the fraction is *binary*, most decimal fractions are not
          exactly representable.
        - `0.1` in binary is a repeating fraction — just like `1/3` is
          `0.3333…` in decimal.
        - Therefore `0.1 + 0.2 != 0.3` **exactly**, and comparing floats with
          `==` is almost always wrong.
    - The fix: compare with a tolerance — `fabs(a - b) < 1e-9`.
    - Small floats are promoted to `double` during arithmetic, so in practice
      **use `double` unless memory pressure says otherwise.**

---

## 4. Numbers in the machine

### 4.1 Positional notation — one idea, three bases

- **Every positional system says the same thing:**
    - `value = Σ digit(i) × base^i`
    - The *position* determines the weight; the *base* determines how fast
      weights grow.
    - ```text
      decimal  44        = 4×10¹ + 4×10⁰        = 44
      binary   0010 1100  = 2⁵ + 2³ + 2² + 2⁰   = 44
      hex      0x2C       = 2×16¹ + 12×16⁰       = 44
      ```
- **Hex exists purely as shorthand for binary.**
    - One hex digit = **exactly four bits**, because 2⁴ = 16.
    - So `0xB7` is *literally* `1011 0111` — read it off directly.
    - This is why registers, masks, and addresses are written in hex: it is
      binary you can read without counting.

- **The hex↔binary table to memorize (or be able to rebuild):**

    | | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
    |---|---|---|---|---|---|---|---|---|
    | | 0000 | 0001 | 0010 | 0011 | 0100 | 0101 | 0110 | 0111 |

    | | 8 | 9 | A | B | C | D | E | F |
    |---|---|---|---|---|---|---|---|---|
    | | 1000 | 1001 | 1010 | 1011 | 1100 | 1101 | 1110 | 1111 |

- **Group binary in fours *from the right*.**
    - `101100` → pad to `0010 1100`, **not** `1011 00`.
    - Trap: grouping from the left gives the wrong hex digits for any length
      that isn't a multiple of four.

- **Decimal → binary: divide by 2, read remainders upward.**
    - 44 ÷ 2 = 22 r **0**
    - 22 ÷ 2 = 11 r **0**
    - 11 ÷ 2 = 5 r **1**
    - 5 ÷ 2 = 2 r **1**
    - 2 ÷ 2 = 1 r **0**
    - 1 ÷ 2 = 0 r **1**
    - Read the remainders bottom→top: **101100** ✓
    - Or subtract powers of two: 44 = 32 + 8 + 4 = 2⁵ + 2³ + 2² → `101100`.

### 4.2 Unsigned integers and wrapping

- **An `n`-bit unsigned value holds `0 … 2ⁿ − 1`.**
    - `uint8_t`: 0–255 (2⁸ − 1)
    - `uint16_t`: 0–65,535
    - `uint32_t`: 0–4,294,967,295

- **Overflow wraps around — and in C that is *defined* behavior.**
    - `uint8_t x = 255; x++;` → `x` becomes **0**.
    - Counting down: `uint8_t x = 0; x--;` → `x` becomes **255**.
    - It is modular arithmetic: the value is effectively taken **mod 2ⁿ**.
        - Hardware-wise: the adder simply produces a carry out of the top bit
          that nobody stores, and the low bits are correct regardless.

- **Trap — the unsigned loop that never ends:**
    - ```c
      for (uint8_t i = 10; i >= 0; i--)   /* infinite */
      ```
    - `i >= 0` is **always true** for an unsigned type: every unsigned value
      is ≥ 0. When `i` is 0 and you subtract 1 it becomes 255, and the loop
      keeps going forever.
    - Fix: compare against a nonzero floor (`i > 0`), or use a signed type.

### 4.3 Negative numbers: three encodings, one winner

- **The problem:** how do you store a minus sign in bits? There is no sign
  column in hardware — you must encode it in the bit pattern.

- **Three historical answers, on 3 bits:**

    | Bits | Unsigned | Sign–magnitude | One's complement | **Two's complement** |
    |---|---|---|---|---|
    | 000 | 0 | +0 | +0 | **0** |
    | 001 | 1 | 1 | 1 | **1** |
    | 010 | 2 | 2 | 2 | **2** |
    | 011 | 3 | 3 | 3 | **3** |
    | 100 | 4 | −0 | −3 | **−4** |
    | 101 | 5 | −1 | −2 | **−3** |
    | 110 | 6 | −2 | −1 | **−2** |
    | 111 | 7 | −3 | −0 | **−1** |

    - **Sign–magnitude:** high bit is the sign, rest is magnitude. Human
      intuition, but it has **two zeros** (+0 and −0) and the adder needs
      special cases for negatives.
    - **One's complement:** invert all bits to negate. Still **two zeros**,
      and `a + b` needs an "add the end-around carry" correction.
    - **Two's complement:** invert and add 1. **One zero**, and — the crucial
      part — **the exact same unsigned adder works unchanged.**

- **Why two's complement won**
    - The CPU has one adder. If negative numbers were encoded such that
      ordinary unsigned addition produces the right answer, you don't need a
      second adder or sign special-casing.
    - Two's complement does exactly that. Hardware cost, not elegance, decided
      this.

- **How to negate: invert every bit, then add 1**
    - ```text
      +5 in 8 bits:  0000 0101
      invert:        1111 1010
      add one:       1111 1011   =  −5
      ```
    - **Verify it:** `0000 0101 + 1111 1011 = 1 0000 0000`. The carry out of
      bit 7 is discarded (there's nowhere to store it), leaving **0**. ✓
    - This is also why `-x` is computed as `~x + 1` in real compilers.

- **The top bit changes weight — this is the mental model that sticks**
    - Unsigned: bit *k* weighs **+2ᵏ**. Total for n bits: `2ⁿ − 1`.
    - Two's complement: bit *k* for k < n−1 weighs **+2ᵏ**, but the top bit
      weighs **−2ⁿ⁻¹**.
        - ```text
          1111 1011 = −128 + 64 + 32 + 16 + 8 + 2 + 1 = −5
          ```
    - Range of n bits: **−2ⁿ⁻¹ … 2ⁿ⁻¹ − 1**
        - 8 bits: **−128 … +127**
        - 32 bits: about **−2.1 billion … +2.1 billion**

- **The asymmetry: there is a most negative number with no positive twin**
    - +128 does not exist in 8 bits; the positive side stops at +127.
    - **Negate −128:** invert `1000 0000` → `0111 1111`, add 1 →
      `1000 0000` = **−128 again**. It maps to itself.
    - Why: `+128` needs 8 bits of magnitude plus a sign — it doesn't fit.
    - Practical danger: `abs(INT_MIN)` overflows, because the result can't be
      represented. Any code that negates without checking has this hole.

### 4.4 Sign extension — same bits, different answers

- **Widening a value requires *filling* the new bits. The type decides with what.**
    - **Signed:** copy the sign bit (the top bit) into every new bit — this
      preserves the *value*.
        - ```text
          int8_t  −8 = 1111 1000                 (0xF8)
          int16_t −8 = 1111 1111 1111 1000       (0xFFF8)
          ```
    - **Unsigned:** fill with zeros — this also preserves the value.
        - ```text
          uint8_t  248 = 1111 1000               (0xF8)
          uint16_t 248 = 0000 0000 1111 1000     (0x00F8)
          ```

- **The deep point: `0xF8` alone has no value. Only the type gives it one.**
    - As `int8_t` → −8. As `uint8_t` → 248. The bits are identical.
    - This is the single most common source of "wrong number" bugs in
      bit-manipulation code: you extracted 8 bits correctly, but then read
      them with the wrong type.
    - Your own `sign_extend()` is the general form: given a value of *width*
      bits, produce the correct n-bit signed interpretation for any width
      1…32.

- **How sign extension works mechanically:** copy bit n−1 (the sign bit of
  the source) into bits n…31.
    - `0xF8` in 8 bits, sign bit = 1 → fill bits 8..31 with 1 → `0xFFFFFFF8`
      = −8 in 32 bits. ✓
    - `0x08` in 8 bits, sign bit = 0 → fill with 0 → `0x00000008` = 8. ✓

### 4.5 Overflow

- **Unsigned overflow: defined, wraps mod 2ⁿ.**
    - Sometimes exactly what you want (hashing, checksums, ring buffers).

- **Signed overflow: undefined behavior.**
    - The standard imposes **no** requirement. The program may crash, may
      wrap, may be *optimized on the assumption it never happens*.
    - Real consequence: the compiler may delete a check that "can't" be
      reached, because proving signed overflow impossible lets it assume the
      overflowing path doesn't exist.
    - **The rule: check before you add, not after.**
        - ```c
          if ((b > 0 && a > INT32_MAX - b) ||
              (b < 0 && a < INT32_MIN - b)) {
              /* a + b would overflow */
          }
          ```
        - Why `INT32_MAX - b`: rearranging `a + b > INT32_MAX` to
          `a > INT32_MAX - b` is safe — the subtraction can't overflow,
          because `b > 0` guarantees `INT32_MAX - b` is in range.
        - Why the two branches: when `b` is negative, subtracting it *increases*
          the limit, and the test is against `INT32_MIN` instead.
        - The `&&` short-circuits, so each comparison only runs when it's safe.
    - **On paper:** two numbers of the same sign produce a result of the
      *other* sign — that's the visual red flag.

- **Casting does not fix overflow; it just reinterprets the low bits.**
    - `(int8_t)(200)` is not 200 — it is −56, because 200 = `0xC8` and the
      top bit reads as sign.

---

## 5. Integer promotion — the hidden rule that explains weird outputs

- **What it is:** before *any* arithmetic or comparison happens, C quietly
  converts the operands to a common type. Two rules, in order.

    - **Rule 1 — promotion:** any type *narrower than `int`* is widened to
      `int` first.
        - `char`, `signed char`, `unsigned char`, `short`, `unsigned short`,
          `_Bool`, and `uint8_t` / `uint16_t` all become `int`.
        - Why: CPUs do arithmetic in a register-sized unit anyway. `int` is
          the natural working size, and on a 16-bit machine `int` might be
          the smallest thing that fits.
        - **Consequence: `uint8_t` arithmetic is actually `int` arithmetic.**

    - **Rule 2 — usual arithmetic conversions:** if after promotion the two
      operands still differ, C picks a common type:
        - different types but the signed type can hold every value of the
          unsigned one → both become the signed type.
        - otherwise, if both have the *same width*, the signed one is
          converted to **unsigned** — which is why `int` vs `unsigned int`
          always becomes `unsigned int`.
        - otherwise the wider type wins, keeping it unsigned if it already was.

- **Worked example 1: why `200 + 100` prints 300, then 44**
    - ```c
      uint8_t a = 200, b = 100;
      printf("%d\n", a + b);      /* 300 */
      ```
        - Promotion: `a` and `b` become `int`.
        - Arithmetic: 200 + 100 = **300**, computed in `int`. 300 fits.
        - Printed with `%d` as an `int` → 300.
    - ```c
      uint8_t c = a + b;
      printf("%d\n", c);          /* 44 */
      ```
        - Storing back into `uint8_t` truncates to the low 8 bits:
          300 = 0b1 0010 1100 → keep low 8 → 0b0010 1100 = **44**.
        - Equivalently 300 mod 256 = 44.
    - **The same expression gives two different numbers depending on where
      the result goes.** The type of the *destination* matters as much as
      the type of the operands.

- **Worked example 2: why `-1 < 0u` is false**
    - ```c
      printf("%d\n", -1 < 0u);    /* prints 0 */
      ```
        - `-1` is `int`, `0u` is `unsigned int`.
        - Rule 2: the `unsigned int` wins → `-1` is converted to
          `unsigned int`.
        - Two's complement conversion: −1 becomes `0xFFFFFFFF` =
          **4,294,967,295**.
        - `4294967295 < 0` → **false** → prints 0.
    - **This is a silent logic bug**, not a crash. `-Wall -Wextra` warns
      (`-Wsign-compare`) precisely because it's so common.
    - Fix: make both sides the same signedness, explicitly.

- **How to keep this from biting you**
    - Don't mix signed and unsigned in a comparison.
    - Don't assume a narrow type stays narrow through arithmetic — assign the
      result back explicitly if truncation is what you want.
    - Read warnings. `-Wextra` exists to catch exactly these.

---

## 6. Characters are small numbers

- **A `char` is a 1-byte integer.** That's the whole idea.
    - `putchar('A')` sends the *number* 65.
    - `printf("%c", 65)` prints the *character* whose number is 65 → `A`.
    - The same byte, read through two different lenses.

- **ASCII is the standard number↔character mapping.**
    - A handful of values worth knowing cold:

        | Value | Character | | Value | Character |
        |---|---|---|---|---|
        | 32 | space | | 48 | `'0'` |
        | 10 | `'\n'` (newline) | | 57 | `'9'` |
        | 9 | `'\t'` (tab) | | 65 | `'A'` |
        | 0 | `'\0'` (end of string) | | 90 | `'Z'` |
        | 13 | `'\r'` (carriage return) | | 97 | `'a'` |
        | 127 | DEL | | 122 | `'z'` |

    - **The digits `'0'`…`'9'` are consecutive** (48…57).
    - **Letters are consecutive within each case** (65…90, 97…122).
    - Lowercase and uppercase differ by exactly 32.

- **Why those gaps matter — the two idioms that depend on them**
    - ```c
      c - '0'                 /* '7' → 7 */
      ```
      works *only* because `'0'`…`'9'` are consecutive. The difference of
      their codes is the digit's value. The standard **guarantees** this.
    - ```c
      c >= 'A' && c <= 'Z'     /* is uppercase letter */
      ```
      works because letters are contiguous. What is *between* `'Z'` (90) and
      `'a'` (97)? The six punctuation characters `[\]^_` + backtick — so the
      range check is correct, not sloppy.
    - **Trap:** `'10'` is not ten. It's a *multi-character constant* — a
      compiler-specific value. Numbers without quotes are values; numbers
      with quotes are characters.

- **Escape sequences** — the backslash introduces a non-printable or special
  character:
    - `'\n'` newline, `'\t'` tab, `'\r'` return, `'\0'` **nul**, `'\\'`
      backslash, `'\''` apostrophe, `'\"'` double quote, `'\x41'` hex → `A`
    - **`'\0'` (nul) is the important one:** it is the value **0**, and it
      marks the end of every C string (section 9).

- **Why `getchar()` returns `int`, not `char`**
    - It must return both *characters* and an *end-of-file marker*.
    - `EOF` is a macro defined as **−1** (or some negative value).
    - If `getchar()` returned `char`:
        - on a **signed** `char` platform, byte `0xFF` (255) would be read as
          −1 and mistaken for EOF;
        - on an **unsigned** `char` platform, EOF couldn't be represented at all.
    - So it returns `int`: values 0…255 for real bytes, **−1 for EOF**.
      Two disjoint ranges, no ambiguity.
    - **Always store it in an `int`.**
        - ```c
          int c;
          while ((c = getchar()) != EOF)
              putchar(c);
          ```

- **Why the parentheses in that loop**
    - `=` binds *looser* than `!=`. Written without parentheses,
      `while (c = getchar() != EOF)` parses as
      `while (c = (getchar() != EOF))` — which stores 1 or 0 into `c`, not
      the character.
    - The compiler warns (`-Wparentheses`). **Treat that warning as an error.**

- **What `EOF` actually is at the machine level**
    - `getchar()` reads from `stdin`, which is *buffered*. The read may block
      waiting for you to press Enter. When the stream is closed (Ctrl-D on
      Linux/macOS, Ctrl-Z + Enter on Windows) and the buffer is drained, it
      returns `EOF` forever.
    - `EOF` means "the stream is finished," **not** "you pressed Ctrl-D."

---

## 7. Functions

- **What a function is:** a named block of code with parameters and a return
  value — a way to give a procedure a name so it can be reused and reasoned
  about separately.
    - ```c
      return_type name(parameter declarations)
      {
          declarations and statements
      }
      ```

- **Every function has a return type.** Use `void` if it returns nothing.
    - Omitting the return type is K&R C and has been invalid since C99.

- **`(void)` vs `()` — a real, testable difference**
    - `int foo(void)` — takes **no** arguments; the compiler will reject
      `foo(1)`.
    - `int foo()` — *unspecified* arguments (pre-C23); the compiler **cannot
      check your calls**, so `foo(1, 2, 3)` compiles.
    - **Trap:** trusting a `()` prototype to catch your own mistakes. It
      won't. Always write `(void)`.

- **C passes everything by value — this is the central fact**
    - "By value" = *a copy is made*. The callee receives a private duplicate.
    - ```c
      void swap_wrong(int a, int b) { int t = a; a = b; b = t; }
      void swap(int *a, int *b)     { int t = *a; *a = *b; *b = t; }

      int x = 1, y = 2;
      swap_wrong(x, y);   /* x, y unchanged */
      swap(&x, &y);       /* x = 2, y = 1 */
      ```
    - Why `swap_wrong` fails: `a` and `b` are copies in `swap_wrong`'s own
      stack frame. Swapping them swaps the copies; when the function returns,
      the copies are gone. The caller's `x` and `y` were never touched.
    - Why `swap` works: you passed **addresses** (still by value — the
      address itself is copied). The callee now has a copy of *where* `x`
      lives, and `*a` writes through it.
    - **There is no call-by-reference in C.** The only way to modify a
      caller's variable is to pass its address.

- **A picture of a call**
    - ```text
      main's frame              foo's frame (a copy)
      ┌──────────┐             ┌──────────┐
      │ x = 1    │   foo(x)    │ a = 1    │  ← a *copy* of x's value
      │ y = 2    │  ─────────► │ b = 2    │
      └──────────┘             └──────────┘
        untouched                 writing to `a` cannot reach x
      ```

- **Prototypes and separate compilation**
    - A **prototype** declares the signature before use:
      `long power(int base, unsigned n);`
    - The **definition** comes later, or in a different file.
    - Without a prototype, C assumes the function returns `int` and accepts
      whatever arguments you pass — no checking at all.
    - **Prototypes go in headers** so every `.c` that calls the function
      gets the same, checked declaration.

- **The return value is a value**
    - `long p = 1; ... return p;` — the returned `p` is copied back into the
      caller's expression. The callee's `p` dies with its frame.
    - Ignoring a return value is legal but often a mistake — that's what
      `warn_unused_result` attributes are for.

- **What happens on the stack, step by step, for `power(2, 5)`**
    1. Evaluate the arguments (order unspecified — don't depend on it).
    2. Copy `2` and `5` into the new frame's parameter slots.
    3. Push the return address; jump.
    4. Allocate slots for locals (`p`, `i`).
    5. Run the body.
    6. Copy the return value to a waiting slot; pop the frame; jump back.
    - Consequence: **locals vanish on return.** Returning the address of a
      local is the classic dangling-pointer bug.

---

## 8. Arrays

- **What an array is:** a fixed number of elements of one type, stored
  **next to each other, with no gaps and no metadata**.
    - No length is stored in the array. No bounds. No type tag.
    - Just contiguous bytes plus the compiler's knowledge of the declaration.

- **Address math — the one formula to know**
    - ```text
      address of a[i] = base address of a  +  i × sizeof(element)
      ```
    - Example: `uint32_t a[256];` (each element 4 bytes)
        - `sizeof a` = 4 × 256 = **1024 bytes**
        - `a[0]` starts at offset **0**
        - `a[32]` starts at 4 × 32 = **128 bytes in**
        - `a[255]` starts at 4 × 255 = 1020, occupies bytes 1020…1023 — the
          last valid byte of the array
    - **This is why array indexing is `base + i*size` and why it's fast:**
      one multiply and one add, no searching.

- **C does not check indexes — ever**
    - `a[256]` on the array above compiles, runs, and reads or writes
      whatever bytes live at offset 1024 — which belong to **something else**.
        - Maybe a neighboring local variable.
        - Maybe a saved return address.
        - Maybe unmapped memory → segfault.
    - This is **undefined behavior**, not an error message.
    - The compiler *may* diagnose a constant out-of-range index
      (`-Warray-bounds`), but at run time with a variable index there is
      nothing checking.
    - **The picture to hold:** the array is a row of lockers; `a[300]` isn't
      "rejected," it's *the next locker down the hall, which someone else owns*.

- **Initialization rules**
    - ```c
      int a[4] = {1, 2, 4, 8};      /* all four set */
      int b[]  = {1, 2, 4, 8};      /* size inferred = 4 */
      int c[4] = {1, 2};            /* {1, 2, 0, 0} — rest zero-filled */
      int d[8] = {0};               /* all zero */
      int e[8] = {[3] = 7, [7] = 1};/* designated (C99): rest zero */
      int f[4] = {1, 2, 3, 4, 5};   /* ERROR: too many initializers */
      ```
    - Partial initialization **zero-fills the remainder** — this is a
      guarantee, not an accident.
    - **Designated initializers** `[index] = value` are C99. Useful for
      sparse arrays and for making the index self-documenting.

- **Zero vs. garbage depends on *storage duration*, not on the syntax**
    - Local array with **no** initializer → **garbage** (stack frame reuse).
    - Local array **with** `= {0}` → zeros.
    - Global / `static` array → **always zeros**, initialized before `main`.
    - **When in doubt, write `= {0}`.** It costs nothing at run time for
      static storage and it makes intent explicit.

- **Array arguments decay to pointers — this breaks `sizeof`**
    - When you pass an array to a function, what the callee receives is a
      **pointer to the first element**, not the array.
        - ```c
          void f(int arr[]) {
              /* sizeof arr is sizeof(int*), i.e. 8 — NOT the array size */
          }
          ```
        - Why: copying a whole array on every call would be expensive and
          would defeat pass-by-value's cheapness. C chose address-passing.
        - The declaration `int arr[]` in a parameter list *means*
          `int *arr`. They are the same thing to the compiler.
    - **Consequence: the length must be passed separately.**
        - ```c
          void f(int arr[], int n);
          ```
    - **`sizeof arr / sizeof arr[0]` only works where the array itself is in
      scope** — i.e. in the function that declared it, never on a parameter.

- **Variable-length arrays (`int v[n]`) are optional in C11**
    - Most compilers support them, but they are not required, they can't be
      statically initialized, and they put the size on the stack (risking
      overflow for big `n`).
    - Use a fixed size, or `malloc` if the size truly isn't known until run
      time.

- **Multidimensional arrays are arrays of arrays**
    - `int m[3][4]` = 3 rows, each row an `int[4]`, laid out contiguously.
      Row-major order: all of row 0, then row 1, then row 2.
    - `m[i][j]` → `base + (i × 4 + j) × sizeof(int)`.

---

## 9. Strings

- **C has no string type. This is the single most important fact about C
  strings.**
    - A string is just **a `char` array that happens to end with `'\0'`**.
    - Nothing marks where it *started*, and nothing but the `'\0'` marks
      where it *ends*. There is no length field, no capacity, no type.

- **The picture**
    - ```text
      char s[] = "hello\n";

      index   0     1     2     3     4     5      6
            ┌────┬────┬────┬────┬────┬─────┬─────┐
            │ 'h'│ 'e'│ 'l'│ 'l'│ 'o'│ '\n'│ '\0'│   = 7 bytes
            └────┴────┴────┴────┴────┴─────┴─────┘
      6 visible characters, 7 bytes — every string function reads up to \0
      ```
    - `"ECE 361"` → 7 characters + `'\0'` = **8 bytes**.
    - The array's declared size includes the `'\0'`. `"hi"` needs 3 bytes.

- **Why `'\0'` is load-bearing**
    - `strlen`, `printf("%s", …)`, `strcpy`, `strcmp` all scan forward until
      they hit a zero byte.
    - **If you forget it, they don't stop.** They run off the end of your
      array into whatever memory follows, until by chance they find a 0.
        - That's a buffer over-read: wrong output, a crash, or a security hole.
    - **Trap:** `char buf[5]; buf[0]='a'; buf[1]='b'; … buf[4]='e';` — no
      room left for `'\0'`. The buffer is full and *still not a valid string*.

- **Array you own vs. pointer to a literal**
    - ```c
      char s[] = "hi";        /* an array of 3 bytes, in writable memory */
      s[0] = 'H';             /* fine */

      const char *p = "hi";   /* points at a read-only literal */
      p[0] = 'H';             /* UNDEFINED BEHAVIOR */
      ```
    - String literals usually live in read-only pages. Writing to them
      crashes (or silently corrupts) depending on the platform.
    - The `const` documents the intent and lets the compiler enforce it.
    - **Rule of thumb:** `char s[] = "..."` when you need to modify it;
      `const char *p = "..."` when you only read it.

- **Every string function needs the buffer's size — but none of them can see it**
    - A `char *` carries an address and nothing else. `strcpy` has no way to
      know how big `dst` is.
    - Maxim: **"Always carry the size of a buffer with the buffer."**

- **The risk table**
    - | Function | Behavior | Risk |
      |---|---|---|
      | `strcpy(dst, src)` | copies until `'\0'` | writes past `dst` if `src` is longer |
      | `strcat(dst, src)` | appends at `dst`'s `'\0'` | same, at the end of `dst` |
      | `strncpy(dst, src, n)` | copies at most `n` bytes | **does not write `'\0'`** if `src` is ≥ n |
      | `snprintf(dst, size, "%s", src)` | bounded, formatted | **safe** — never writes more than `size`, always terminates |
      | `fgets(buf, size, stdin)` | reads at most `size-1` | **safe** line input |
      | `gets()` | unbounded | **never use — removed from C11** |
    - `strncpy` deserves special suspicion: its name sounds safe, and its
      most common use leaves an unterminated string.

- **Manually copying a string — the idiom**
    - ```c
      void copy_line(char to[], const char from[])
      {
          int i = 0;
          while ((to[i] = from[i]) != '\0')
              ++i;
      }
      ```
    - Read it slowly: *assign, then test the assigned value.* It copies the
      `'\0'` too (the loop stops only after copying it), so the result is a
      valid string.
    - The extra parentheses are required for the same reason as in
      `while ((c = getchar()) != EOF)`.

- **Reading a line safely — and the boundary bug**
    - ```c
      int get_line(char s[], int lim)
      {
          int c = EOF, i = 0;
          while (i < lim - 1 && (c = getchar()) != EOF && c != '\n')
              s[i++] = (char)c;
          if (c == '\n')
              s[i++] = '\n';
          s[i] = '\0';
          return i;
      }
      ```
    - Three deliberate boundary decisions:
        - `lim - 1` reserves one byte for `'\0'`.
        - `c` is initialized to `EOF` so that if the loop body never runs,
          `c` is not some leftover value and no bogus `'\n'` gets appended.
        - The cast `(char)c` is required because `c` is an `int`.
    - The loop's three conditions test three independent ways to stop:
      buffer full, input ended, newline seen. **Each must be checked.**
    - **The classic bug this fixes:** an older version returned 0 at EOF
      *before* storing what it had already read — so a final line with no
      trailing newline **vanished**. Test the boundaries: empty input, input
      exactly `lim` long, input longer than `lim`, input with no newline.

---

## 10. Scope, lifetime, and linkage — derived, not memorized

Three *separate* questions that beginners fuse into one:

- **Scope — *where in the source* is the name visible?**
    - Lexical: from its declaration to the end of the enclosing `{ }`.
    - Block scope: locals, parameters, `enum` constants.
    - File scope: anything declared outside every function — visible from
      that point to the end of the file.

- **Lifetime — *how long do the bytes exist*?**
    - Automatic: created on block entry, destroyed on block exit.
    - Static: created before `main`, destroyed after `main`.
    - (Dynamic: created by `malloc`, destroyed by `free`.)

- **Linkage — *can other files see this name*?**
    - External: other translation units can link to it (the default for
      globals and functions).
    - Internal: hidden — only this `.c` file. **This is what `static` means
      at file scope.**
    - None: a local has no linkage at all; the name exists only here.

- **The table, with the reasoning attached**

    | Declared | Visible in | Lives | Starts as | Why |
    |---|---|---|---|---|
    | inside a block | that block | while the block runs | **garbage** | automatic storage; stack frame reused |
    | `static`, inside a block | that block | **whole program** | **zero** | static storage; zero-filled before `main` |
    | outside functions | every file that declares it | whole program | zero | static storage, external linkage |
    | `static`, outside functions | **this file only** | whole program | zero | static storage, internal linkage |

- **`static` really does have two unrelated meanings**
    - On a **local**: changes *lifetime* (and initial value). Nothing to do
      with hiding.
        - ```c
          void counter(void) {
              static int n = 0;   /* one copy, shared by all calls */
              ++n;
              printf("%d\n", n);
          }
          ```
        - Each call sees the previous call's value — because the bytes
          persist. Local variables without `static` do not.
    - On a **global or function**: changes *linkage* — hides it from other
      files.
        - ```c
          static void helper(void) { ... }   /* only this .c can call it */
          ```
    - Same keyword, two independent axes. Keeping them separate in your head
      prevents most `static` confusion.

- **Why you should make things `static` by default**
    - A helper function or file-level variable that no other file needs
      should be `static`: it keeps the namespace small, prevents accidental
      name collisions across files, and lets the compiler optimize harder
      (it knows nobody else can see it).
    - Only *promote* a name to external linkage when another file genuinely
      needs it.

- **Globals: the practical guidance**
    - Define a global in **exactly one** `.c`; declare it `extern` in the
      header.
        - ```c
          /* counter.h */
          #ifndef COUNTER_H
          #define COUNTER_H
          extern int counter;       /* declaration — no storage */
          void count_up(void);
          #endif

          /* counter.c */
          #include "counter.h"
          int counter = 0;          /* the one definition */
          void count_up(void) { ++counter; }
          ```
    - Omit `extern` in the header and every includer gets a *definition* →
      **multiple definition** link error (gcc ≥ 10 enforces this strictly).
    - Better still: **avoid globals.** Pass what a function needs as
      parameters — you'll see every input, and testing becomes possible.

---

## 11. Bits in C

### 11.1 Everything is bits; the type decides what they mean

- The same byte `1111 1011` is:
    - **251** as `uint8_t`
    - **−5** as `int8_t`
    - a **status word** with seven flags set and one clear
    - a **non-printable ASCII character** (251 is in the extended range)
- One pattern, four readings. Bits don't know what they mean.

### 11.2 Bit numbering

- **Bit 0 is the least significant (rightmost) bit.**
    - ```text
      bit index:  7   6   5   4   3   2   1   0
      value:      1   0   1   1   0   1   1   0     = 0xB6
      weights:  128  64  32  16   8   4   2   1
      ```
- Saying "bits 15..8" means positions counted **from the right**, inclusive.
- Hex notation reads naturally from this: `0xB6C5`, each digit = 4 bits,
  the rightmost digit = bits 3..0.

### 11.3 Bitwise vs. logical — different operators, different meanings

- **Bitwise operators work on individual bits:**

    | Operator | Meaning | With `x = 1011 0110` |
    |---|---|---|
    | `x & m` | keep bits where `m` has 1 | `x & 0x0F` = `0000 0110` |
    | `x \| m` | turn on bits where `m` has 1 | `x \| 0x01` = `1011 0111` |
    | `x ^ m` | flip bits where `m` has 1 | `x ^ 0xFF` = `0100 1001` |
    | `~x` | flip every bit | `~x` = `0100 1001` (8-bit view) |
    | `x << n` | shift left, fill with 0 | `x << 1` = `0110 1100` |
    | `x >> n` | shift right | `x >> 4` = `0000 1011` |

- **Logical operators work on truth values and short-circuit:**
    - `&&` (and), `||` (or), `!` (not)
    - `a && b` does not evaluate `b` if `a` is false — this is why guards
      like `p != NULL && p->x > 0` are safe.
    - **Any nonzero value is true; only 0 is false.**
- **The confusion:** `&` and `&&` look similar and behave differently.
    - `x & 1` computes a *bit* (0 or 1).
    - `x && 1` computes a *truth value* (0 or 1).
    - They often agree — which is exactly why the mistake survives.

### 11.4 Masks and single-bit operations

- **A mask is a value with only the bits you care about set to 1.**
    - `0x0F` = `0000 1111` → the low 4 bits.
    - `1u << k` → a single 1 at position `k`.
    - The name: think of it as a stencil you hold over the number, letting
      only certain bits show through (`&`) or letting you paint certain bits
      on (`|`).

- **Four operations you should be able to write without looking:**
    - ```c
      x |=  (1u << 3);      /* set   bit 3 to 1     */
      x &= ~(1u << 3);      /* clear bit 3 to 0     */
      x ^=  (1u << 3);      /* toggle bit 3         */
      if (x & (1u << 3))    /* test  bit 3: 0 or 1? */
      ```
    - Walk through `clear`:
        - `1u << 3` = `0000 1000`
        - `~` = `1111 0111` — all bits *except* bit 3
        - `&=` keeps everything else and forces bit 3 to zero. ✓
    - Walk through `test`:
        - `x & (1u << 3)` isolates bit 3: the result is `0000 1000` if set,
          `0000 0000` if not.
        - Both are truth-tested correctly (`8` is true, `0` is false), so the
          `!= 0` is optional — but writing it makes the intent explicit.

- **Always write `1u`, not `1`.**
    - `1` is a signed `int`. On a 32-bit `int`, **`1 << 31` is undefined
      behavior** — it would set the sign bit, which signed left-shift rules
      forbid.
    - **`1u << 31` is perfectly fine** — unsigned shift, no sign bit issue.
    - Using `1u` is a habit that costs nothing and removes an entire class
      of bugs.

- **Shift rules — know the three cases**
    - **Left shift `x << n`:** bits move left, zeros fill in on the right.
      Defined for unsigned as long as `n < width`.
    - **Right shift `x >> n`:**
        - For **unsigned**: zeros fill in on the left. Always defined.
        - For **signed** (negative values): **implementation-defined** —
          the hardware may fill with 1s (arithmetic shift, preserving sign)
          or 0s (logical shift). x86 does arithmetic; some targets don't.
        - **Practically: shift unsigned values.** Do the sign handling
          yourself so the code doesn't depend on the platform.
    - **Shift count ≥ the type's width: undefined behavior.**
        - `1u << 32` on a 32-bit `unsigned` is UB — not 0, not 1. *Undefined.*
        - This is the trap behind `mask = (1u << width) - 1u` when
          `width == 32`. **More on this in 11.6.**

### 11.5 Reading a field out of a packed word

- **A field is a run of consecutive bits with a meaning.**
    - A hardware status register packs many small values into one integer,
      because reading one 32-bit word is cheaper than reading eight 8-bit
      registers.

- **The three operations, drawn:**

    - **Get** — shift the field down to bit 0, then mask it:
        - ```c
          mask  = (1u << width) - 1u;    /* `width` low bits set  */
          field = (word >> pos) & mask;  /* shift then keep       */
          ```
        - Why shift *first*: the mask only knows about low bits, so the field
          must be moved to the bottom before masking.
        - Why mask *after*: `word >> pos` may still have higher junk above
          the field.

    - **Clear** — build a mask with *holes* where the field is:
        - ```c
          word &= ~(mask << pos);
          ```
        - `mask << pos` = the field's position, all ones.
        - `~` flips it → ones everywhere *except* the field.
        - `&=` → the field becomes 0, everything else survives.

    - **Insert** — shift the value into place, OR it in:
        - ```c
          word |= ((value & mask) << pos);
          ```
        - `value & mask` **truncates** the value to the field's width — so
          passing a too-large value silently keeps only the low bits.
          (Whether that's desired is a design decision worth documenting.)

- **A worked picture: extract bits 7..4 of `0xB6C5`, then insert 3**

    ```text
    GET  pos = 4, width = 4, mask = 0xF

    word                 1 0 1 1  0 1 1 0  1 1 0 0  0 1 0 1    0xB6C5
                            └──────┬──────┘
    word >> 4     =   0 0 0 0  0 0 0 0  0 0 0 0  1 0 1 1       0x0B6C
    (word >> 4) & 0xF = 0 0 0 0  0 0 0 0  0 0 0 0  1 1 0 0       0x000C
                                                    ^^^^
                                          field value = 12 = 0xC

    INSERT 3 into the same field

    mask << 4       =   0 0 0 0  0 0 0 0  1 1 1 1  0 0 0 0       0x00F0
    ~(mask << 4)    =   1 1 1 1  1 1 1 1  0 0 0 0  1 1 1 1       0xFF0F
    word & ~mask<<4 =   1 0 1 1  0 1 1 0  0 0 0 0  0 1 0 1       0xB605
                                          └──field cleared──┘
    3 << 4          =   0 0 0 0  0 0 0 0  0 0 1 1  0 0 0 0       0x0030
    OR result       =   1 0 1 1  0 1 1 0  0 0 1 1  0 1 0 1       0xB635
                                          └──field = 3──┘
    ```

    - Note what survived: bits 15..8, bits 7..0 and the high byte are
      **byte-for-byte unchanged**. Only bits 7..4 moved.
      That is the entire contract of `set_field`.

- **The habit: draw it.** Every one of these lines is a picture of bits
  moving. Writing them without a picture is how you get `pos` and `width`
  swapped.

### 11.6 The `width == 32` cliff

- **The general mask fails at the boundary:**
    - ```c
      mask = (1u << width) - 1u;
      ```
    - For `width` = 1…31: `1u << width` puts a 1 just above the field,
      subtracting 1 turns on everything below → correct mask.
    - For `width` = 32: **`1u << 32` is undefined behavior** — the shift
      count equals the width of `unsigned int`.
        - It is *not* `0`. Compilers are allowed to assume shift counts are
          in range, and they will use that assumption to transform your code.
    - Why the boundary exists at all: a shift is defined only while bits
      remain to shift. At `n == width` there's nothing left.

- **Ways to handle it — and the reasoning for each**
    - Special-case it explicitly (clearest):
        - ```c
          if (width >= 32)
              mask = 0xFFFFFFFFu;      /* or ~0u */
          else
              mask = (1u << width) - 1u;
          ```
    - Use `~0u` as "all ones" — no shift involved, works for width 32.
    - **Never** rely on `1u << width` for width 32 and hope.
    - Note also `width == 0`: `(1u << 0) - 1` = 0, which *is* the right mask,
      but whether width 0 is valid at all is a separate design decision.

- **Other boundary cases to think about while you're here**
    - `pos + width > 32` — the field runs off the end of the word.
    - `pos == 31` with `width == 1` — the top bit; works, but only if you
      avoided signed shifts.
    - Negative `pos` or `width` — a shift by a negative amount is UB.
    - **Test: 0, largest value, most negative value, width 1, width 32.**

### 11.7 A realistic packed register

- **A status word, decoded:**

    ```text
    w = 0xF819 = 1111 1000 0001 1001

    bit     field
    0       HEAT      HEAT  =  w       & 0x1   = 1
    1       COOL      COOL  = (w >> 1) & 0x1   = 0
    2       FAN       FAN   = (w >> 2) & 0x1   = 0
    3       FAULT     FAULT = (w >> 3) & 0x1   = 1
    6..4    MODE      MODE  = (w >> 4) & 0x7   = 1
    7       reserved
    15..8   SETPOINT  SP    = (w >> 8) & 0xFF  = 0xF8  -> sign-extend -> -8
    ```

    - `MODE` is a **3-bit** field: mask `0x7` = `111`, because 2³ − 1 = 7.
    - `SETPOINT` is **signed**, so `0xF8` = 248 as unsigned must be
      sign-extended from bit 7 to become **−8**. Getting this wrong turns
      −8 °C into 248 °C — a classic decoding bug.
    - **Fields are described by three numbers: position, width, signedness.**
      Miss any one and the decode is wrong.

- **A larger register layout, same idea:**

    ```text
    RGB LED register
    bit  31       enable
    bits 30..24   reserved (write 0, read as 0)
    bits 23..16   red duty cycle    (8 bits)
    bits 15..8    green duty cycle  (8 bits)
    bits  7..0    blue duty cycle   (8 bits)
    ```
    - Building this is `set_field` **four times** — one call per field.
    - This is exactly what packed-register programming looks like in
      embedded systems: read the whole word, mask/shift the field you want,
      write back the whole word.

### 11.8 Precedence: the bug that looks like it works

- **`==` binds tighter than `&`.**
    - ```c
      if (status & 0x08 == 0)    /* WRONG */
      ```
      parses as
      ```c
      if (status & (0x08 == 0))  /* 0x08 == 0 is 0 */
      ```
      which is `status & 0` → always **0** → the branch **never runs**.
    - Correct:
      ```c
      if ((status & 0x08) == 0)
      ```
    - **gcc warns about this** (`-Wparentheses`). Heed it.

- **The precedence chain worth knowing (loosely, high to low):**
    - unary (`~ ! - & * sizeof`) → `* / %` → `+ -` → `<< >>` →
      `< <= > >=` → `== !=` → `&` → `^` → `|` → `&&` → `||` →
      `?:` → `=` → `,`
    - Notice: `==` **above** `&`, and `&` **above** `|` — which is *not* the
      order most people guess from the symbols' visual grouping.
- **The rule that removes the whole class of bugs: parenthesize bit
  operations whenever you mix them with anything else.** It costs nothing
  and reads clearly.

---

## 12. Undefined behavior, warnings, and reading what the compiler says

- **Undefined behavior (UB) is the most misunderstood term in C.**
    - It does **not** mean "random." It does **not** mean "probably fine."
    - It means: **the standard imposes no requirement whatsoever.** The
      program may do anything, including appear to work.
    - Worse: because the compiler is allowed to assume UB never happens, the
      observable wrongness may occur **somewhere else entirely**, or only on
      a different optimization level, or only on a different machine.

- **A concrete illustration**
    - ```c
      int x = some_value;
      if (x > 0) { ... }
      if (x + 100 < 0) { /* "impossible" — compiler may delete this */ }
      ```
    - If `x + 100` *could* overflow (signed overflow = UB), the compiler may
      reason "signed overflow can't happen, so this branch is unreachable"
      and remove it — before you ever hit the overflow.

- **The UB list you should be able to recite from this material**
    - reading an uninitialized local
    - array index out of bounds
    - signed integer overflow
    - `1 << 31` (left shift that sets a signed sign bit)
    - shift count ≥ type width (`1u << 32`)
    - writing past the end of a string / buffer (`strcpy`, `gets`)
    - writing to a string literal
    - returning the address of a local
    - `null` pointer dereference
    - Right-shifting a negative signed value: **implementation-defined**,
      not UB — a milder category ("the compiler must document what it does").

- **The three categories, side by side**
    - | Category | Meaning | Example |
      |---|---|---|
      | **Defined** | the standard says exactly what happens | unsigned wraparound |
      | **Implementation-defined** | the standard *requires a documented choice* | `>>` on a negative value |
      | **Undefined** | no requirement at all | signed overflow, OOB index |
    - Knowing which one you're in tells you whether to worry about
      portability (implementation-defined) or correctness (UB).

- **Warnings are free bug reports**
    - `-Wall -Wextra` catches, among others:
        - signed/unsigned comparison (`-1 < 0u`)
        - `=` used where `==` was meant
        - `status & 0x08 == 0` precedence
        - missing parentheses around an assignment in a condition
        - unused parameters/variables
        - implicit conversions that lose information
    - **Zero warnings is the standard to hold.** A build with warnings is a
      build where the compiler told you something and you chose not to look.

- **Defensive habits that make UB unlikely**
    - Initialize every local at declaration.
    - Pass the buffer size alongside the buffer.
    - Validate every index and every shift count *before* you shift.
    - Use unsigned types for bit manipulation.
    - Test boundaries first: `0`, max, most negative, width 1, width 32.
    - Turn on `-fsanitize=address` when you suspect memory trouble — it
      converts silent corruption into an immediate, located error.

---

## 13. Glossary — one sentence each

- **Address** — a number naming a byte in memory.
- **Array** — contiguous elements of one type, no stored length, no bounds
  checks.
- **Bit** — a single binary digit, 0 or 1; the smallest unit the machine
  stores.
- **Block** — a `{ ... }` region; the unit of scope for locals.
- **Buffer** — a chunk of memory you write into; its size must be tracked
  separately.
- **Byte** — 8 bits; the unit `char` and `sizeof` are defined against.
- **Compiler** — turns C source into assembly; enforces the language rules.
- **Preprocessor** — text-only stage: pastes headers, expands macros.
- **Assembler** — turns assembly into object-file machine code.
- **Linker** — resolves names across object files and produces an executable.
- **Declaration** — "this name exists, with this signature." No storage.
- **Definition** — a declaration that also reserves storage. Exactly once.
- **Header** — text included by the preprocessor, carrying declarations.
- **Include guard** — `#ifndef/#define/#endif` that stops double pasting.
- **Scope** — where in the source a name is visible.
- **Lifetime** — how long the underlying bytes exist.
- **Linkage** — whether other translation units can refer to the name.
- **Automatic storage** — locals: frame-scoped, garbage until assigned.
- **Static storage** — globals and `static` locals: program-long, zero-init.
- **Stack frame** — one function call's private workspace for args and locals.
- **Pass by value** — the callee receives a copy; C has nothing else.
- **Pointer** — a value that holds an address; `&` gets one, `*` follows one.
- **Decay** — an array argument becoming a pointer to its first element.
- **Two's complement** — negative encoding: invert and add 1; one zero, one adder.
- **Sign extension** — copying the sign bit into new higher bits when widening.
- **Integer promotion** — narrow types widened to `int` before any arithmetic.
- **Usual arithmetic conversions** — the operands coerced to a common type.
- **Mask** — a value with only the bits of interest set to 1.
- **Field** — a run of consecutive bits inside a larger word, with a meaning.
- **Shift** — moving bits left/right; undefined if the count ≥ type width.
- **Overflow** — exceeding a type's range: wraps if unsigned, UB if signed.
- **Undefined behavior** — the standard requires nothing; anything may happen.
- **Implementation-defined** — the compiler must choose and document a behavior.
- **EOF** — a negative marker from stream functions meaning "no more input."
- **`'\0'` (nul)** — the zero byte that terminates every C string.

---

## Quick self-check — answer out loud, then verify

1. What four stages does `gcc` run, and which one produces an
   "undefined reference" error?
2. Why is an uninitialized local garbage while a `static` local is zero?
3. How big is a `long` on Linux vs. Windows, and why does that matter?
4. Why does `getchar()` return `int`?
5. Why is `-1 < 0u` false?
6. What are the ranges of 8-bit unsigned and 8-bit two's complement?
7. Why does negating −128 in 8 bits give −128 back?
8. What's the difference between `0xF8` as `int8_t` and as `uint8_t`, and
   what converts between them?
9. What does `sizeof` return for an array *parameter*, and why?
10. How many bytes does `"hello\n"` occupy, and which one marks the end?
11. What's wrong with `1 << 31`, and what's the fix?
12. Why does `mask = (1u << width) - 1u` break at `width == 32`?
13. What does `status & 0x08 == 0` actually test?
14. Give the four-row scope/lifetime/linkage table from memory.
15. Which is safer, `strncpy` or `snprintf`, and why does the "safe-sounding"
    one bite?
