/*
 * readings.h: ECE 361, Fall 2026, HW2 shared header.
 *
 * This is the only file every module can meet at. It holds the one data
 * type the program passes around and nothing else: no functions, no
 * logic, no globals.
 *
 * Before the split these were four separate globals in readings.c
 * (ticks, temps, hums, count) plus skipped. Globals are hard to reason
 * about because any function can change them at any moment, so instead
 * the driver owns one struct dataset and hands a pointer to whichever
 * module needs it.
 */
#ifndef READINGS_H
#define READINGS_H

/* Upper bound on how many readings one run will hold. Input stops with a
 * warning once this many are stored, which is the same behaviour the
 * original program had. */
#define MAX_READINGS 1000

/*
 * One container for the whole input.
 *
 * The three parallel arrays are kept as arrays rather than one array of
 * structs so that each module can be handed a plain float array: the
 * statistics module only ever wants the temperatures or the humidity,
 * and never needs to know that the other one exists.
 *
 * count   how many readings were stored (the arrays are valid up to here)
 * skipped how many non-comment lines did not parse as three numbers
 */
struct dataset {
    int   ticks[MAX_READINGS];
    float temps[MAX_READINGS];
    float hums[MAX_READINGS];
    int   count;
    int   skipped;
};

#endif /* READINGS_H */
