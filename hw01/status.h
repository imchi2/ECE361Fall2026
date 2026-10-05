/**
 * @file status.h
 * @brief Thermostat status word: the 16-bit field layout as named
 *        constants, a status_t struct with one member per field, and
 *        status_unpack(), which takes a raw word apart.
 *
 * A thermostat reports its state in one 16-bit status word. Bits are
 * numbered from 0, the least significant bit, exactly as in bits.h:
 *
 * <pre>
 *   bit 0          HEAT      1 = heater on
 *   bit 1          COOL      1 = compressor on
 *   bit 2          FAN       1 = fan on
 *   bit 3          FAULT     1 = fault detected
 *   bits 6 to 4    MODE      0 = OFF, 1 = HEAT, 2 = COOL, 3 = AUTO,
 *                            4 = FAN_ONLY; 5 to 7 are invalid
 *   bit 7          reserved  must be 0
 *   bits 15 to 8   SETPOINT  set point in degrees C, 8-bit two's
 *                            complement (-128 to 127)
 * </pre>
 *
 * Every field position and width appears below as a named constant, so
 * status.c contains no bare bit numbers. The homework asks for exactly
 * that: "named constants for every position and width. No magic numbers."
 *
 * @note Built with <tt>gcc -std=c11 -Wall -Wextra -g3 -O0</tt>, no warnings.
 */

#ifndef STATUS_H
#define STATUS_H

#include <stdbool.h> /* bool - a C99 feature, used by the struct below */
#include <stdint.h>

/* ------------------------------------------------------------------ *
 * Named constants for the word layout.                               *
 * Every position (which bit a field starts at) and every width (how  *
 * many bits it spans) used by status_unpack is listed here.          *
 * ------------------------------------------------------------------ */

/** @brief Position of the HEAT field (a single flag bit). */
#define STATUS_POS_HEAT 0

/** @brief Position of the COOL field (a single flag bit). */
#define STATUS_POS_COOL 1

/** @brief Position of the FAN field (a single flag bit). */
#define STATUS_POS_FAN 2

/** @brief Position of the FAULT field (a single flag bit). */
#define STATUS_POS_FAULT 3

/** @brief Position of the MODE field (its lowest bit). */
#define STATUS_POS_MODE 4

/** @brief Position of the reserved field (a single bit that must be 0). */
#define STATUS_POS_RESERVED 7

/** @brief Position of the SETPOINT field (its lowest bit). */
#define STATUS_POS_SETPOINT 8

/** @brief Width of every flag field: HEAT, COOL, FAN, FAULT, reserved. */
#define STATUS_WIDTH_FLAG 1

/** @brief Width of the MODE field, in bits: 0 to 7 fits in three bits. */
#define STATUS_WIDTH_MODE 3

/** @brief Width of the SETPOINT field: 8-bit two's complement. */
#define STATUS_WIDTH_SETPOINT 8

/* ------------------------------------------------------------------ *
 * Values the MODE field can hold.                                    *
 * The spec defines 0 to 4 and calls 5 to 7 invalid.                  *
 * ------------------------------------------------------------------ */

/** @brief MODE value: everything off. */
#define STATUS_MODE_OFF 0

/** @brief MODE value: heating. */
#define STATUS_MODE_HEAT 1

/** @brief MODE value: cooling. */
#define STATUS_MODE_COOL 2

/** @brief MODE value: heating and cooling as needed. */
#define STATUS_MODE_AUTO 3

/** @brief MODE value: fan only. */
#define STATUS_MODE_FAN_ONLY 4

/** @brief Lowest invalid MODE value, per the homework spec. */
#define STATUS_MODE_INVALID_FIRST 5

/** @brief Highest invalid MODE value, per the homework spec. */
#define STATUS_MODE_INVALID_LAST 7

/**
 * @brief One status word, unpacked into one member per field.
 *
 * The members follow the bit order of the word, lowest bit first.
 * <tt>mode_valid</tt> sits next to <tt>mode</tt> because it is derived
 * from <tt>mode</tt>: the spec says modes 5 to 7 are invalid, and this
 * is how status_t reports that. The raw mode value is kept either way,
 * so no bit of the original word is lost.
 */
typedef struct status
{
    bool heat;       /**< Bit 0: 1 = heater on. */
    bool cool;       /**< Bit 1: 1 = compressor on. */
    bool fan;        /**< Bit 2: 1 = fan on. */
    bool fault;      /**< Bit 3: 1 = fault detected. */
    int mode;        /**< Bits 6 to 4: raw value 0..7, see STATUS_MODE_*. */
    bool mode_valid; /**< false when mode is 5..7, true when it is 0..4. */
    bool reserved;   /**< Bit 7: must be 0; reported exactly as read. */
    int setpoint;    /**< Bits 15 to 8: degrees C, -128..127, sign extended. */
} status_t;

/**
 * @brief Unpack one 16-bit status word into its fields.
 *
 * Example: <tt>status_unpack(0x1631)</tt> returns a status_t with set
 * point 22, mode 3 (<tt>STATUS_MODE_AUTO</tt>), heater on, compressor
 * off, fan off, no fault, reserved clear, and <tt>mode_valid</tt> true.
 *
 * Every field is read with <tt>get_field()</tt> from Part 2, using the
 * named position and width constants above, and the SETPOINT byte is
 * turned into a signed number with <tt>sign_extend()</tt> from Part 2.
 *
 * @param word The raw status word.
 * @return The word's fields, as a status_t.
 * @note No bit pattern is rejected. A word that breaks the rules
 *       (MODE 5 to 7, or the reserved bit set) is still reported field
 *       by field: <tt>mode_valid</tt> turns false for MODE 5 to 7, and
 *       <tt>reserved</tt> shows the reserved bit as it was found. The
 *       caller decides what to do about it.
 */
status_t status_unpack(uint16_t word);

#endif /* STATUS_H */
