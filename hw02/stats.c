/*
 * stats.c: ECE 361, Fall 2026, HW2 statistics module.
 */
#include "readings.h"
#include "stats.h"

/*
 * One pass over the array, accumulating min, max, and the running total
 * at the same time. Summing in index order (0, 1, 2, ...) is the same
 * order the original program used, and floating-point addition is not
 * associative, so keeping the order is what guarantees the printed mean
 * comes out bit-for-bit identical to before.
 *
 * n must be at least 1: starting min and max at values[0] reads the
 * array, and dividing by n needs a non-zero n. The driver checks
 * count == 0 and returns before calling this.
 */
struct summary stats_of(const float *values, int n) {
    struct summary s;
    float sum = 0.0f;
    int i;

    s.min = values[0];
    s.max = values[0];

    for (i = 0; i < n; i++) {
        if (values[i] < s.min)
            s.min = values[i];
        if (values[i] > s.max)
            s.max = values[i];
        sum += values[i];
    }

    s.mean = sum / n;
    return s;
}
