/*
 * stats.h: ECE 361, Fall 2026, HW2 statistics module.
 *
 * Computes the three numbers the summary needs (min, max, mean) over a
 * plain array of floats. It knows nothing about sensors, ticks, or
 * humidity: it is a small numeric helper that happens to be used twice.
 */
#ifndef STATS_H
#define STATS_H

/*
 * The three answers for one array, returned together so the caller does
 * not have to walk the data three times.
 *
 * mean is only meaningful when the array is non-empty; the caller must
 * check count before asking, which is what the original program did by
 * returning early on "no readings, no summary".
 */
struct summary {
    float min;
    float max;
    float mean;
};

/*
 * One pass over values[0..n-1]: track the smallest, the largest, and the
 * running total, then divide the total by n.
 *
 * This replaces three separate pieces of the original program: the
 * temperature loop, the copy-pasted humidity loop, and the recursive
 * max_temp() function. Min and max start at values[0] rather than at
 * some sentinel like 0, because the readings can legitimately be below
 * zero and a sentinel would then be wrong.
 */
struct summary stats_of(const float *values, int n);

#endif /* STATS_H */
