/**
 * @file bits.h
 * @brief Bit-manipulation library: binary printing, field extraction and
 *        insertion, and two's-complement sign extension on a 32-bit word.
 *
 * Bits are numbered from 0, the least significant bit. A field is described
 * by a position @c pos and a width @c width and is valid when
 * <tt>width</tt> is in <tt>[1, 32]</tt>, <tt>pos</tt> is in
 * <tt>[0, 31]</tt>, and <tt>pos + width <= 32</tt>.
 *
 * Invalid arguments are rejected deterministically and always before any
 * shift or mask expression is evaluated, so a rejected call never reaches
 * undefined behaviour. Each function documents its own result on rejection.
 *
 * @note Built with <tt>gcc -std=c11 -Wall -Wextra -g3 -O0</tt>, no warnings.
 */

#ifndef BITS_H
#define BITS_H

#include <stdint.h>

/**
 * @brief Print the lowest @p width bits of @p x, most significant bit first,
 *        in groups of four separated by a single space.
 *
 * Example: <tt>print_binary(0x2C, 8)</tt> writes <tt>0010 1100</tt>.
 * Groups are counted from the most significant bit side, so a width that is
 * not a multiple of four produces a short first group: <tt>print_binary(0x2F, 6)</tt>
 * writes <tt>10 1111</tt>. Bits of @p x above @p width are ignored.
 *
 * @param x     Word to print.
 * @param width Number of bits to print, 1..32.
 * @return Nothing. No trailing newline is emitted; the caller prints its own.
 * @note If @p width is outside <tt>[1, 32]</tt> (zero, negative, or > 32),
 *       nothing is printed at all.
 */
void print_binary(uint32_t x, int width);

/**
 * @brief Extract bits <tt>pos</tt>..<tt>pos + width - 1</tt> of @p word,
 *        shifted down so the field starts at bit 0.
 *
 * @param word  Source word.
 * @param pos   Lowest bit of the field, 0..31.
 * @param width Number of bits in the field, 1..32.
 * @return The field as an unsigned value in <tt>[0, 2^width - 1]</tt>,
 *         or 0 when the arguments are rejected.
 * @note Rejection (returns 0): <tt>width</tt> outside <tt>[1, 32]</tt>,
 *       <tt>pos</tt> outside <tt>[0, 31]</tt>, or <tt>pos + width > 32</tt>.
 * @note <tt>width == 32</tt> is only reachable with <tt>pos == 0</tt> and
 *       takes the mask special case, because shifting a 32-bit value by 32
 *       is undefined in C.
 */
uint32_t get_field(uint32_t word, int pos, int width);

/**
 * @brief Replace bits <tt>pos</tt>..<tt>pos + width - 1</tt> of @p word with
 *        the lowest @p width bits of @p value; all other bits are unchanged.
 *
 * @param word  Source word.
 * @param pos   Lowest bit of the field, 0..31.
 * @param width Number of bits in the field, 1..32.
 * @param value Replacement bits; only its lowest @p width bits are used, so
 *              a value too wide for the field has its extra bits dropped.
 * @return @p word with the field replaced, or @p word unchanged when the
 *         arguments are rejected.
 * @note Rejection (returns @p word untouched, a true no-op):
 *       <tt>width</tt> outside <tt>[1, 32]</tt>, <tt>pos</tt> outside
 *       <tt>[0, 31]</tt>, or <tt>pos + width > 32</tt>.
 * @note Validation runs before the mask is built, so the shift
 *       <tt>fmask << pos</tt> always satisfies <tt>pos + width <= 32</tt>.
 */
uint32_t set_field(uint32_t word, int pos, int width, uint32_t value);

/**
 * @brief Interpret the lowest @p width bits of @p value as a two's-complement
 *        number and return it as a signed 32-bit integer.
 *
 * Example: <tt>sign_extend(0xF8, 8)</tt> returns <tt>-8</tt>.
 *
 * @param value Source word; only its lowest @p width bits are considered.
 * @param width Number of bits to interpret, 1..32.
 * @return The sign-extended value as int32_t, or 0 when @p width is outside
 *         <tt>[1, 32]</tt>.
 * @note Uses unsigned arithmetic only: the negative case subtracts
 *       <tt>2^width</tt> from the masked value, which wraps correctly, so
 *       the result never depends on the implementation-defined right shift
 *       of a signed negative number.
 * @note <tt>sign_extend(0x80000000, 32)</tt> returns <tt>INT32_MIN</tt>,
 *       the most negative int32_t value.
 */
int32_t sign_extend(uint32_t value, int width);

#endif /* BITS_H */
