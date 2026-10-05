/**
 * @file status.c
 * @brief Implementation of status_unpack(), declared in status.h.
 *
 * The word is taken apart one field at a time: every field is read with
 * get_field() from bits.c, every call uses the named position and width
 * constants from status.h, and the SETPOINT byte is signed with
 * sign_extend() from bits.c. There are no bare bit numbers in here.
 *
 * Written for people learning C: plain control flow, no feature newer
 * than C99, long expressions split across lines, and a comment on every
 * decision.
 */

#include "status.h"

#include "bits.h"
#include <stdbool.h> /* bool, true, false - a C99 feature */

/**
 * @brief Unpack a 16-bit thermostat status word into a status_t.
 *
 * See status.h for the field layout and for the worked example
 * status_unpack(0x1631).
 *
 * @param word The raw status word.
 * @return The word's fields, as a status_t.
 */
status_t status_unpack(uint16_t word)
{
    status_t status;

    /* get_field() takes a 32-bit word. The 16-bit status word widens to
       32 bits automatically on assignment, which is always safe: making
       an unsigned value wider keeps its value. We do that once, into a
       named variable, so every read below clearly works on the same
       word. */
    uint32_t bits = word;

    /* The four single-bit flags: HEAT, COOL, FAN, FAULT. Each get_field
       call returns 0 or 1, because each field is STATUS_WIDTH_FLAG (1)
       bit wide. Comparing against 0 turns that number into the bool
       false or true, which is what the struct members hold.

       About the precedence warning in the spec: == binds tighter than
       &, so a hand-written test such as

           bits & (1u << STATUS_POS_FAULT) == 0

       is actually read as

           bits & ((1u << STATUS_POS_FAULT) == 0)

       and the comparison yields 0, so the whole expression is 0 no
       matter what the word contains: the test passes even when the
       fault bit is set. We never write a bare & here, because
       get_field() does the masking for us. If you ever do write one,
       parenthesize it: (bits & MASK) == 0. */
    status.heat = get_field(bits, STATUS_POS_HEAT, STATUS_WIDTH_FLAG) != 0;
    status.cool = get_field(bits, STATUS_POS_COOL, STATUS_WIDTH_FLAG) != 0;
    status.fan = get_field(bits, STATUS_POS_FAN, STATUS_WIDTH_FLAG) != 0;
    status.fault = get_field(bits, STATUS_POS_FAULT, STATUS_WIDTH_FLAG) != 0;

    /* MODE is STATUS_WIDTH_MODE (3) bits wide, so get_field returns a
       number from 0 to 7. It is stored as a plain int; converting from
       the unsigned 32-bit result is always in range. The raw value is
       kept even when it is invalid, so no information from the word is
       lost. */
    status.mode = (int)get_field(bits, STATUS_POS_MODE, STATUS_WIDTH_MODE);

    /* The spec says MODE 5 to 7 are invalid. status_t reports that with
       mode_valid: false for 5, 6 and 7, true for 0 to 4. The named
       constants make the check read like the spec sentence itself. */
    if (status.mode >= STATUS_MODE_INVALID_FIRST
        && status.mode <= STATUS_MODE_INVALID_LAST)
    {
        status.mode_valid = false;
    }
    else
    {
        status.mode_valid = true;
    }

    /* The reserved bit must be 0 in a well-formed word, but unpack does
       not reject a word that has it set. It reports the bit exactly as
       it was read, and the caller decides what to do about it. */
    status.reserved = get_field(bits, STATUS_POS_RESERVED,
                                STATUS_WIDTH_FLAG) != 0;

    /* SETPOINT is 8 bits of two's complement. get_field first shifts the
       byte down so it sits in bits 0 to 7, then sign_extend reads those
       bits as a signed number: 0x16 becomes 22, 0xF8 becomes -8, and
       0x80 becomes -128, the most negative value. */
    uint32_t setpoint_bits = get_field(bits, STATUS_POS_SETPOINT,
                                       STATUS_WIDTH_SETPOINT);
    status.setpoint = sign_extend(setpoint_bits, STATUS_WIDTH_SETPOINT);

    return status;
}
