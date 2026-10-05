/**
 * @file bits.c
 * @brief Implementation of the bit-manipulation library declared in bits.h.
 *
 * Every function checks its own arguments first and returns a documented
 * neutral result when they are out of range, so no shift count outside
 * [0, 31] and no mask wider than 32 bits is ever evaluated. The validation
 * and the mask building are repeated in each function on purpose: repeating
 * eight straightforward lines is easier to read than shared helpers.
 *
 * Written for people learning C: plain control flow, no feature newer than
 * C99, long expressions split across lines, and a comment on every decision.
 */

#include "bits.h"

#include <stdbool.h> /* bool, true, false - a C99 feature */
#include <stdio.h>

/** @brief Width of the word this library works on, in bits. */
#define WORD_BITS 32

/** @brief Index of the most significant bit of that word. */
#define WORD_MSB 31

void print_binary(uint32_t x, int width)
{
    /* Valid widths are 1 to 32. Anything else prints nothing at all, so a
       bad width never reaches the shift inside the loop. Bits of x above
       the width are ignored. */
    if (width < 1)
    {
        return;
    }
    if (width > WORD_BITS)
    {
        return;
    }

    /* Walk from the highest bit of the field down to bit 0 and print each
       one. Bits are numbered from 0, so bit width-1 is the leftmost bit
       that gets printed. */
    for (int i = width - 1; i >= 0; i--)
    {
        uint32_t bit = (x >> i) & 1u;

        if (bit == 0u)
        {
            putchar('0');
        }
        else
        {
            putchar('1');
        }

        /* Insert a space after every fourth bit, but never after the last
           one. The groups are counted from the high end, which is why a
           width that is not a multiple of four prints a short first group:
           print_binary(0x2F, 6) prints "10 1111". */
        bool group_ends_here = (i % 4) == 0;
        bool not_last_bit = i != 0;

        if (group_ends_here && not_last_bit)
        {
            putchar(' ');
        }
    }
}

uint32_t get_field(uint32_t word, int pos, int width)
{
    /* A valid field has width 1 to 32, starts at bit 0 to 31, and fits in
       the word: pos + width must not exceed 32. Rejecting bad arguments
       before doing any arithmetic keeps the shifts below well defined. */
    if (width < 1)
    {
        return 0;
    }
    if (width > WORD_BITS)
    {
        return 0;
    }
    if (pos < 0)
    {
        return 0;
    }
    if (pos > WORD_MSB)
    {
        return 0;
    }
    if (pos + width > WORD_BITS)
    {
        return 0;
    }

    /* Mask with the lowest `width` bits set to one. A width of 32 is a
       separate case, because 1u << 32 is undefined behaviour in C. A valid
       field of width 32 can only start at pos 0. */
    uint32_t mask;

    if (width == WORD_BITS)
    {
        mask = 0xFFFFFFFFu;
    }
    else
    {
        mask = (1u << width) - 1u;
    }

    /* Shift the field down so it starts at bit 0, then drop everything
       outside it. pos is at most 31 here, so the shift is well defined. */
    uint32_t shifted = word >> pos;
    uint32_t field = shifted & mask;

    return field;
}

uint32_t set_field(uint32_t word, int pos, int width, uint32_t value)
{
    /* Same valid ranges as get_field. Out-of-range arguments leave the word
       untouched, so a rejected call is a no-op and never corrupts word. */
    if (width < 1)
    {
        return word;
    }
    if (width > WORD_BITS)
    {
        return word;
    }
    if (pos < 0)
    {
        return word;
    }
    if (pos > WORD_MSB)
    {
        return word;
    }
    if (pos + width > WORD_BITS)
    {
        return word;
    }

    /* Mask over the field's own bits: the lowest `width` bits set to one.
       The width == 32 case again avoids shifting by 32. */
    uint32_t field_mask;

    if (width == WORD_BITS)
    {
        field_mask = 0xFFFFFFFFu;
    }
    else
    {
        field_mask = (1u << width) - 1u;
    }

    /* Slide that mask up to the field's position, giving a mask with a zero
       in every bit the field covers and a one everywhere else. The check
       above guarantees pos + width <= 32, so this shift cannot overflow. */
    uint32_t mask = field_mask << pos;

    /* Keep every bit outside the field, and write the field from the lowest
       bits of value. Masking value first means a value that is too wide for
       the field simply has its extra bits dropped. */
    uint32_t kept_bits = word & ~mask;
    uint32_t new_bits = (value & field_mask) << pos;

    return kept_bits | new_bits;
}

int32_t sign_extend(uint32_t value, int width)
{
    /* Only widths from 1 to 32 make sense as a two's-complement number.
       A rejected width returns 0, checked before any shift happens. */
    if (width < 1)
    {
        return 0;
    }
    if (width > WORD_BITS)
    {
        return 0;
    }

    /* Mask with the lowest `width` bits set to one, then keep only those
       bits of value. width == 32 avoids shifting by 32, as elsewhere. */
    uint32_t mask;

    if (width == WORD_BITS)
    {
        mask = 0xFFFFFFFFu;
    }
    else
    {
        mask = (1u << width) - 1u;
    }

    uint32_t low_bits = value & mask;

    /* The sign bit of the field is its highest bit, at position width-1.
       width is at least 1 here, so this shift is always in range. */
    uint32_t sign_bit = (low_bits >> (width - 1)) & 1u;

    /* Positive: the value already fits in int32_t unchanged. */
    if (sign_bit == 0u)
    {
        return (int32_t)low_bits;
    }

    /* Negative and full width: 0x80000000 through 0xFFFFFFFF are already
       the int32_t representations of INT32_MIN through -1, so casting is
       enough. */
    if (width == WORD_BITS)
    {
        return (int32_t)low_bits;
    }

    /* Negative: subtracting 2^width from the unsigned value wraps around to
       the correct negative number. Doing it this way avoids shifting a
       negative signed value, which C leaves implementation defined.
       sign_extend(0xF8, 8) computes 0xF8 - 0x100, which is -8. */
    uint32_t magnitude = 1u << width;
    uint32_t wrapped = low_bits - magnitude;

    return (int32_t)wrapped;
}
