/*
 * ptr.h: ECE 361, Fall 2026, HW3 pointer library.
 *
 * Four small functions, all of them written around one idea: instead of
 * telling the callee a number (an index), you hand over a *pointer* to
 * the variable you care about. The callee can then read that variable
 * (find_above) or even write straight into it (swap_float, minmax).
 *
 * This header is the contract only - no code lives here. Every function
 * gets three things spelled out: what it takes, what it returns, and what
 * it assumes about its arguments.
 */
#ifndef PTR_H
#define PTR_H

/*
 * swap_float - exchange the values of the two floats being pointed at.
 *
 * Takes:   a and b, two pointers to float, i.e. the addresses of two
 *          float variables that already exist in the caller.
 * Returns: nothing; the two caller variables have swapped values by the
 *          time the call comes back.
 * Assumes: both pointers are valid (never NULL) and point at real floats.
 *          The callee has no way to report an error, so a bad pointer is
 *          simply out of contract.
 */
void swap_float(float *a, float *b);

/*
 * minmax - find the smallest and the largest value of a[0..n-1].
 *
 * Takes:   a, the array to look through (read-only, hence const);
 *          n, how many values it holds;
 *          min and max, the addresses of two float variables to fill in.
 * Returns: 0 when the two answers were stored, -1 when nothing was done.
 * Assumes: -1 is returned when n < 1 or when any of a, min or max is NULL,
 *          and then *min and *max are left exactly as they were. On a
 *          successful call both outputs are copies of a real element, so
 *          comparing them against the same constant with == is exact.
 */
int minmax(const float *a, int n, float *min, float *max);

/*
 * find_above - the first value of a[0..n-1] that is greater than threshold.
 *
 * Takes:   a, the array to look through (read-only);
 *          n, how many values it holds;
 *          threshold, the value that has to be beaten.
 * Returns: a pointer to the first element whose value is strictly greater
 *          than threshold, scanning left to right; NULL when there is none.
 * Assumes: NULL is also returned when a is NULL or n < 1. "Greater than"
 *          is strict, so a value equal to threshold does not count - the
 *          scan keeps going and may still find a later element.
 */
const float *find_above(const float *a, int n, float threshold);

/*
 * reverse - turn a[0..n-1] around, inside the caller's own array.
 *
 * Takes:   a, the array to reverse in place;
 *          n, how many values it holds.
 * Returns: nothing; the caller's array really is changed.
 * Assumes: nothing happens when a is NULL or n < 2 (an empty array, a
 *          single element and a missing array are all "already reversed").
 *          Otherwise a holds at least n floats, and the two ends are
 *          exchanged with swap_float.
 */
void reverse(float *a, int n);

#endif /* PTR_H */
