/**
 * @file test_bits.c
 * @brief Test program for homework 1: every function of Part 2 at its
 *        boundaries, plus status_unpack from Part 3, as Part 4 asks.
 *
 * How to read the output:
 *   - Each automated check prints one line: PASS, or FAIL with what was
 *     expected and what actually came back.
 *   - print_binary writes to the screen itself, so nothing can compare
 *     its result for you. Those lines are marked "check by eye": the
 *     expected text comes first, the real output follows the arrow.
 *   - The summary at the end counts the automated tests. The program
 *     returns 0 when everything passed and 1 otherwise, so `make test`
 *     fails loudly if any check failed.
 *
 * Build and run, from hw01/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw01 && make test"
 */

#include "bits.h"
#include "status.h"

#include <stdbool.h> /* true, false - a C99 feature */
#include <stdint.h>  /* uint32_t, int32_t, INT32_MIN */
#include <stdio.h>   /* printf */

int main(void)
{
    /* How many checks have run, and how many of them printed FAIL.
       The summary and the exit code are built from these two numbers. */
    int tests = 0;
    int failures = 0;

    /* Every result is stored here first, then compared against the
       expected value, then printed. One variable per result type. */
    uint32_t actual_u32;
    int32_t actual_i32;
    status_t actual_status;

    printf("hw01 test program\n");

    /* ------------------------------------------------------------------ *
     * print_binary - checked by eye                                      *
     *                                                                     *
     * print_binary prints its answer itself instead of returning it, so   *
     * there is nothing to compare against. Each line prints what the      *
     * output SHOULD look like, then the real output after the arrow.      *
     * The out-of-range cases are wrapped in [ ] so you can see that       *
     * they print nothing at all between the brackets.                     *
     * ------------------------------------------------------------------ */
    printf("\n");
    printf("---- print_binary (check by eye) ----\n");

    printf("print_binary(1, 1) should print 1 -> ");
    print_binary(1u, 1);
    printf("\n");

    printf("print_binary(0x2C, 8) should print 0010 1100 -> ");
    print_binary(0x2Cu, 8);
    printf("\n");

    printf("print_binary(0x2F, 6) should print 10 1111 -> ");
    print_binary(0x2Fu, 6);
    printf("\n");

    printf("print_binary(0x0000FFFF, 32) should print ");
    printf("0000 0000 0000 0000 1111 1111 1111 1111 -> ");
    print_binary(0x0000FFFFu, 32);
    printf("\n");

    printf("print_binary(0, 0) should print nothing -> [");
    print_binary(0u, 0);
    printf("]\n");

    printf("print_binary(0, 33) should print nothing -> [");
    print_binary(0u, 33);
    printf("]\n");

    printf("print_binary(0, -1) should print nothing -> [");
    print_binary(0u, -1);
    printf("]\n");

    /* ------------------------------------------------------------------ *
     * get_field                                                          *
     * ------------------------------------------------------------------ */
    printf("\n");
    printf("---- get_field ----\n");

    /* Width 1, the smallest legal field: the lowest bit of the word. */
    tests = tests + 1;
    actual_u32 = get_field(0xFFFFFFFFu, 0, 1);
    if (actual_u32 == 1u)
    {
        printf("PASS  get_field(0xFFFFFFFF, 0, 1) == 1\n");
    }
    else
    {
        printf("FAIL  get_field(0xFFFFFFFF, 0, 1): expected 1, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* pos 31, the highest legal start: bit 31 of 0x80000000 is set. */
    tests = tests + 1;
    actual_u32 = get_field(0x80000000u, 31, 1);
    if (actual_u32 == 1u)
    {
        printf("PASS  get_field(0x80000000, 31, 1) == 1\n");
    }
    else
    {
        printf("FAIL  get_field(0x80000000, 31, 1): expected 1, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* Width 32, the other boundary: the whole word, only legal at
       pos 0. This is the case where shifting by 32 would be undefined,
       so get_field must take its special path. */
    tests = tests + 1;
    actual_u32 = get_field(0xDEADBEEFu, 0, 32);
    if (actual_u32 == 0xDEADBEEFu)
    {
        printf("PASS  get_field(0xDEADBEEF, 0, 32) == 0xDEADBEEF\n");
    }
    else
    {
        printf("FAIL  get_field(0xDEADBEEF, 0, 32): "
               "expected 0xDEADBEEF, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* An ordinary field in the middle: bits 4 to 11 of 0x1234 are 0x23. */
    tests = tests + 1;
    actual_u32 = get_field(0x00001234u, 4, 8);
    if (actual_u32 == 0x23u)
    {
        printf("PASS  get_field(0x00001234, 4, 8) == 0x23\n");
    }
    else
    {
        printf("FAIL  get_field(0x00001234, 4, 8): expected 0x23, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* Out-of-range arguments: our Part 2 policy is to reject them
       deterministically and return 0. Four ways to be out of range. */
    tests = tests + 1;
    actual_u32 = get_field(0xFFFFFFFFu, 0, 0);
    if (actual_u32 == 0u)
    {
        printf("PASS  get_field(0xFFFFFFFF, 0, 0) is rejected, returns 0\n");
    }
    else
    {
        printf("FAIL  get_field width 0: expected 0 (rejected), got 0x%08X\n",
               actual_u32);
        return 1;
    }

    tests = tests + 1;
    actual_u32 = get_field(0xFFFFFFFFu, 0, -1);
    if (actual_u32 == 0u)
    {
        printf("PASS  get_field(0xFFFFFFFF, 0, -1) is rejected, returns 0\n");
    }
    else
    {
        printf("FAIL  get_field negative width: expected 0, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    tests = tests + 1;
    actual_u32 = get_field(0xFFFFFFFFu, 0, 33);
    if (actual_u32 == 0u)
    {
        printf("PASS  get_field(0xFFFFFFFF, 0, 33) is rejected, returns 0\n");
    }
    else
    {
        printf("FAIL  get_field width 33: expected 0, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    tests = tests + 1;
    actual_u32 = get_field(0xFFFFFFFFu, -1, 1);
    if (actual_u32 == 0u)
    {
        printf("PASS  get_field(0xFFFFFFFF, -1, 1) is rejected, returns 0\n");
    }
    else
    {
        printf("FAIL  get_field negative pos: expected 0, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    tests = tests + 1;
    actual_u32 = get_field(0xFFFFFFFFu, 32, 1);
    if (actual_u32 == 0u)
    {
        printf("PASS  get_field(0xFFFFFFFF, 32, 1) is rejected, returns 0\n");
    }
    else
    {
        printf("FAIL  get_field pos 32: expected 0, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* pos 30 with width 4 would reach bit 33, past the end of the word. */
    tests = tests + 1;
    actual_u32 = get_field(0xFFFFFFFFu, 30, 4);
    if (actual_u32 == 0u)
    {
        printf("PASS  get_field(0xFFFFFFFF, 30, 4) is rejected, returns 0\n");
    }
    else
    {
        printf("FAIL  get_field pos+width past 32: expected 0, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* ------------------------------------------------------------------ *
     * set_field                                                          *
     * ------------------------------------------------------------------ */
    printf("\n");
    printf("---- set_field ----\n");

    /* Width 1 at pos 0: turn the lowest bit on in an empty word. */
    tests = tests + 1;
    actual_u32 = set_field(0u, 0, 1, 1u);
    if (actual_u32 == 1u)
    {
        printf("PASS  set_field(0, 0, 1, 1) == 1\n");
    }
    else
    {
        printf("FAIL  set_field(0, 0, 1, 1): expected 1, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* pos 31: the single bit at the very top of the word. */
    tests = tests + 1;
    actual_u32 = set_field(0u, 31, 1, 1u);
    if (actual_u32 == 0x80000000u)
    {
        printf("PASS  set_field(0, 31, 1, 1) == 0x80000000\n");
    }
    else
    {
        printf("FAIL  set_field(0, 31, 1, 1): "
               "expected 0x80000000, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* Width 32: replaces the whole word, again the special case that
       must not shift anything by 32. */
    tests = tests + 1;
    actual_u32 = set_field(0xAAAAAAAAu, 0, 32, 0x55555555u);
    if (actual_u32 == 0x55555555u)
    {
        printf("PASS  set_field(0xAAAAAAAA, 0, 32, 0x55555555) == 0x55555555\n");
    }
    else
    {
        printf("FAIL  set_field width 32: "
               "expected 0x55555555, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* A value too wide for its field: 0x12345678 has 32 bits but the
       field holds only 4, so the extra bits are dropped. The low 4 bits
       of 0x12345678 are 0x8, placed at pos 4 -> 0x80. */
    tests = tests + 1;
    actual_u32 = set_field(0u, 4, 4, 0x12345678u);
    if (actual_u32 == 0x80u)
    {
        printf("PASS  set_field(0, 4, 4, 0x12345678) == 0x80 "
               "(value too wide, extra bits dropped)\n");
    }
    else
    {
        printf("FAIL  set_field value too wide: expected 0x80, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* Clearing a field must leave every other bit untouched. */
    tests = tests + 1;
    actual_u32 = set_field(0xFFFFFFFFu, 4, 4, 0u);
    if (actual_u32 == 0xFFFFFF0Fu)
    {
        printf("PASS  set_field(0xFFFFFFFF, 4, 4, 0) == 0xFFFFFF0F\n");
    }
    else
    {
        printf("FAIL  set_field clearing a field: "
               "expected 0xFFFFFF0F, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* Out-of-range arguments: our Part 2 policy is to return the word
       unchanged, a true no-op. Four ways to be out of range. */
    tests = tests + 1;
    actual_u32 = set_field(0xCAFEBABEu, 0, 0, 0x11111111u);
    if (actual_u32 == 0xCAFEBABEu)
    {
        printf("PASS  set_field(0xCAFEBABE, 0, 0, ...) is rejected, "
               "word unchanged\n");
    }
    else
    {
        printf("FAIL  set_field width 0: expected 0xCAFEBABE, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    tests = tests + 1;
    actual_u32 = set_field(0xCAFEBABEu, 0, 33, 0x11111111u);
    if (actual_u32 == 0xCAFEBABEu)
    {
        printf("PASS  set_field(0xCAFEBABE, 0, 33, ...) is rejected, "
               "word unchanged\n");
    }
    else
    {
        printf("FAIL  set_field width 33: expected 0xCAFEBABE, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    tests = tests + 1;
    actual_u32 = set_field(0xCAFEBABEu, 32, 1, 0x11111111u);
    if (actual_u32 == 0xCAFEBABEu)
    {
        printf("PASS  set_field(0xCAFEBABE, 32, 1, ...) is rejected, "
               "word unchanged\n");
    }
    else
    {
        printf("FAIL  set_field pos 32: expected 0xCAFEBABE, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* pos 31 with width 2 would reach bit 32, past the end of the word. */
    tests = tests + 1;
    actual_u32 = set_field(0xCAFEBABEu, 31, 2, 0x11111111u);
    if (actual_u32 == 0xCAFEBABEu)
    {
        printf("PASS  set_field(0xCAFEBABE, 31, 2, ...) is rejected, "
               "word unchanged\n");
    }
    else
    {
        printf("FAIL  set_field pos+width past 32: "
               "expected 0xCAFEBABE, got 0x%08X\n",
               actual_u32);
        return 1;
    }

    /* ------------------------------------------------------------------ *
     * sign_extend                                                        *
     * ------------------------------------------------------------------ */
    printf("\n");
    printf("---- sign_extend ----\n");

    /* Width 1, the smallest field: bit 0 is the sign bit, and 0 is
       positive. */
    tests = tests + 1;
    actual_i32 = sign_extend(0u, 1);
    if (actual_i32 == 0)
    {
        printf("PASS  sign_extend(0, 1) == 0\n");
    }
    else
    {
        printf("FAIL  sign_extend(0, 1): expected 0, got %d\n", actual_i32);
        return 1;
    }

    /* Width 1 again: 1 means the sign bit is set, so the value is -1. */
    tests = tests + 1;
    actual_i32 = sign_extend(1u, 1);
    if (actual_i32 == -1)
    {
        printf("PASS  sign_extend(1, 1) == -1\n");
    }
    else
    {
        printf("FAIL  sign_extend(1, 1): expected -1, got %d\n", actual_i32);
        return 1;
    }

    /* The homework's own example: 0xF8 in 8 bits is -8. */
    tests = tests + 1;
    actual_i32 = sign_extend(0xF8u, 8);
    if (actual_i32 == -8)
    {
        printf("PASS  sign_extend(0xF8, 8) == -8\n");
    }
    else
    {
        printf("FAIL  sign_extend(0xF8, 8): expected -8, got %d\n",
               actual_i32);
        return 1;
    }

    /* The most negative byte: 0x80 is -128, not +128. */
    tests = tests + 1;
    actual_i32 = sign_extend(0x80u, 8);
    if (actual_i32 == -128)
    {
        printf("PASS  sign_extend(0x80, 8) == -128 (most negative byte)\n");
    }
    else
    {
        printf("FAIL  sign_extend(0x80, 8): expected -128, got %d\n",
               actual_i32);
        return 1;
    }

    /* The most positive byte: 0x7F is 127, and the sign bit is clear. */
    tests = tests + 1;
    actual_i32 = sign_extend(0x7Fu, 8);
    if (actual_i32 == 127)
    {
        printf("PASS  sign_extend(0x7F, 8) == 127 (most positive byte)\n");
    }
    else
    {
        printf("FAIL  sign_extend(0x7F, 8): expected 127, got %d\n",
               actual_i32);
        return 1;
    }

    /* The most negative value of all: 0x80000000 in 32 bits is
       INT32_MIN, the boundary case Part 4 asks for. */
    tests = tests + 1;
    actual_i32 = sign_extend(0x80000000u, 32);
    if (actual_i32 == INT32_MIN)
    {
        printf("PASS  sign_extend(0x80000000, 32) == INT32_MIN "
               "(most negative value)\n");
    }
    else
    {
        printf("FAIL  sign_extend(0x80000000, 32): expected INT32_MIN, "
               "got %d\n", actual_i32);
        return 1;
    }

    /* Width 32 with every bit set: all ones is -1. */
    tests = tests + 1;
    actual_i32 = sign_extend(0xFFFFFFFFu, 32);
    if (actual_i32 == -1)
    {
        printf("PASS  sign_extend(0xFFFFFFFF, 32) == -1\n");
    }
    else
    {
        printf("FAIL  sign_extend(0xFFFFFFFF, 32): expected -1, got %d\n",
               actual_i32);
        return 1;
    }

    /* Out-of-range widths: our Part 2 policy is to return 0. */
    tests = tests + 1;
    actual_i32 = sign_extend(0xFFFFu, 0);
    if (actual_i32 == 0)
    {
        printf("PASS  sign_extend(0xFFFF, 0) is rejected, returns 0\n");
    }
    else
    {
        printf("FAIL  sign_extend width 0: expected 0, got %d\n", actual_i32);
        return 1;
    }

    tests = tests + 1;
    actual_i32 = sign_extend(0xFFFFu, 33);
    if (actual_i32 == 0)
    {
        printf("PASS  sign_extend(0xFFFF, 33) is rejected, returns 0\n");
    }
    else
    {
        printf("FAIL  sign_extend width 33: expected 0, got %d\n", actual_i32);
        return 1;
    }

    /* ------------------------------------------------------------------ *
     * status_unpack - four words, starting with the spec's example       *
     * ------------------------------------------------------------------ */
    printf("\n");
    printf("---- status_unpack ----\n");

    /* Word 1: 0x1631, the exact example from the homework spec:
       set point 22, mode 3 (AUTO), heater on, everything else off. */
    actual_status = status_unpack(0x1631u);

    tests = tests + 1;
    if (actual_status.setpoint == 22)
    {
        printf("PASS  status_unpack(0x1631): setpoint is 22\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x1631): setpoint: expected 22, got %d\n",
               actual_status.setpoint);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.mode == 3 && actual_status.mode_valid == true)
    {
        printf("PASS  status_unpack(0x1631): mode is 3 (AUTO) and valid\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x1631): mode: expected 3 and valid, "
               "got mode %d, mode_valid %d\n",
               actual_status.mode, actual_status.mode_valid);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.heat == true && actual_status.cool == false
        && actual_status.fan == false && actual_status.fault == false)
    {
        printf("PASS  status_unpack(0x1631): heater on, "
               "compressor/fan/fault off\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x1631): flags: expected heat=1 "
               "cool=0 fan=0 fault=0, got heat=%d cool=%d fan=%d fault=%d\n",
               actual_status.heat, actual_status.cool,
               actual_status.fan, actual_status.fault);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.reserved == false)
    {
        printf("PASS  status_unpack(0x1631): reserved bit is clear\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x1631): reserved: "
               "expected 0, got 1\n");
        return 1;
    }

    /* Word 2: 0x8000 - the most negative set point (-128), mode 0,
       every flag off, reserved clear. */
    actual_status = status_unpack(0x8000u);

    tests = tests + 1;
    if (actual_status.setpoint == -128)
    {
        printf("PASS  status_unpack(0x8000): setpoint is -128\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x8000): setpoint: expected -128, got %d\n",
               actual_status.setpoint);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.mode == 0 && actual_status.mode_valid == true)
    {
        printf("PASS  status_unpack(0x8000): mode is 0 (OFF) and valid\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x8000): mode: expected 0 and valid, "
               "got mode %d, mode_valid %d\n",
               actual_status.mode, actual_status.mode_valid);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.heat == false && actual_status.cool == false
        && actual_status.fan == false && actual_status.fault == false)
    {
        printf("PASS  status_unpack(0x8000): every flag is off\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x8000): flags: expected all 0, "
               "got heat=%d cool=%d fan=%d fault=%d\n",
               actual_status.heat, actual_status.cool,
               actual_status.fan, actual_status.fault);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.reserved == false)
    {
        printf("PASS  status_unpack(0x8000): reserved bit is clear\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x8000): reserved: "
               "expected 0, got 1\n");
        return 1;
    }

    /* Word 3: 0x0051 - an INVALID mode. Low byte 0x51 is 0101 0001:
       mode bits 101 (=5, invalid), heater bit set, set point 0.
       This is the case status_valid reports as false. */
    actual_status = status_unpack(0x0051u);

    tests = tests + 1;
    if (actual_status.setpoint == 0)
    {
        printf("PASS  status_unpack(0x0051): setpoint is 0\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x0051): setpoint: expected 0, got %d\n",
               actual_status.setpoint);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.mode == 5 && actual_status.mode_valid == false)
    {
        printf("PASS  status_unpack(0x0051): mode is 5 and "
               "mode_valid is false\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x0051): mode: expected 5 and "
               "invalid, got mode %d, mode_valid %d\n",
               actual_status.mode, actual_status.mode_valid);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.heat == true && actual_status.cool == false
        && actual_status.fan == false && actual_status.fault == false)
    {
        printf("PASS  status_unpack(0x0051): heater on, "
               "compressor/fan/fault off\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x0051): flags: expected heat=1 "
               "cool=0 fan=0 fault=0, got heat=%d cool=%d fan=%d fault=%d\n",
               actual_status.heat, actual_status.cool,
               actual_status.fan, actual_status.fault);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.reserved == false)
    {
        printf("PASS  status_unpack(0x0051): reserved bit is clear\n");
    }
    else
    {
        printf("FAIL  status_unpack(0x0051): reserved: "
               "expected 0, got 1\n");
        return 1;
    }

    /* Word 4: 0xFFFF - every bit set. Set point 0xFF is -1, mode 7
       (also invalid), all flags on, reserved set. */
    actual_status = status_unpack(0xFFFFu);

    tests = tests + 1;
    if (actual_status.setpoint == -1)
    {
        printf("PASS  status_unpack(0xFFFF): setpoint is -1\n");
    }
    else
    {
        printf("FAIL  status_unpack(0xFFFF): setpoint: expected -1, got %d\n",
               actual_status.setpoint);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.mode == 7 && actual_status.mode_valid == false)
    {
        printf("PASS  status_unpack(0xFFFF): mode is 7 and "
               "mode_valid is false\n");
    }
    else
    {
        printf("FAIL  status_unpack(0xFFFF): mode: expected 7 and "
               "invalid, got mode %d, mode_valid %d\n",
               actual_status.mode, actual_status.mode_valid);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.heat == true && actual_status.cool == true
        && actual_status.fan == true && actual_status.fault == true)
    {
        printf("PASS  status_unpack(0xFFFF): every flag is on\n");
    }
    else
    {
        printf("FAIL  status_unpack(0xFFFF): flags: expected all 1, "
               "got heat=%d cool=%d fan=%d fault=%d\n",
               actual_status.heat, actual_status.cool,
               actual_status.fan, actual_status.fault);
        return 1;
    }

    tests = tests + 1;
    if (actual_status.reserved == true)
    {
        printf("PASS  status_unpack(0xFFFF): reserved bit is set\n");
    }
    else
    {
        printf("FAIL  status_unpack(0xFFFF): reserved: "
               "expected 1, got 0\n");
        return 1;
    }

    /* ------------------------------------------------------------------ *
     * Summary                                                            *
     *                                                                     *
     * The exit code is what make looks at: 0 says every check passed,    *
     * 1 makes `make test` stop with an error.                             *
     * ------------------------------------------------------------------ */
    printf("\n");
    printf("---- summary ----\n");
    printf("%d automated test(s) ", tests);
    printf("print_binary was checked by eye (see the lines above).\n");

    if (failures == 0)
    {
        printf("ALL TESTS PASSED\n");
        return 0;
    }

    printf("SOME TESTS FAILED\n");
    return 1;
}
