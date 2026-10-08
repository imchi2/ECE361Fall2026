/*
 * histogram.h: ECE 361, Fall 2026, HW2 histogram module.
 *
 * Builds and prints the temperature histogram. It prints its own output
 * (both the "histogram:" header and the bar lines) because the exact
 * wording of those lines is tied to the bin edges, and keeping the
 * format next to the data that produces it is what stops the printing
 * and the counting from drifting apart.
 */
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "readings.h"

/*
 * Count the temperatures in ds into 5 C bins from 0 to 100 C and print
 * the histogram for the first ds->count readings.
 *
 * Prints nothing at all when ds->count is 0, so an empty input produces
 * the same output as before (the driver returns before calling this,
 * but the guard is cheap and makes the module safe on its own).
 */
void hist_print(const struct dataset *ds);

#endif /* HISTOGRAM_H */
