/*
 * run.c: ECE 361, Fall 2026, HW2 threshold module.
 */
#include "readings.h"
#include "run.h"

/*
 * Walk the array once, keeping two things: how long the current run of
 * above-threshold readings is (`run`), and the best run seen so far.
 *
 * `start` records where the current run began. It is only meaningful
 * while `run > 0`, which is why it is set inside `if (run == 0)`: that
 * is the moment a run goes from zero to one.
 *
 * Everything here is the loop that used to sit inline in main(), kept in
 * the same order and with the same comparisons so it produces the same
 * numbers.
 */
struct run longest_run(const float *values, int n, float threshold) {
    struct run best;
    int run = 0;
    int start = 0;
    int i;

    /* length 0 means "no run found". start is never read while length
     * is 0, but it is initialised anyway so the struct has no garbage
     * in it when it is handed back. */
    best.length = 0;
    best.start = -1;

    for (i = 0; i < n; i++) {
        if (values[i] > threshold) {
            if (run == 0)
                start = i;
            run++;
            if (run > best.length) {
                best.length = run;
                best.start = start;
            }
        } else {
            run = 0;
        }
    }

    return best;
}
