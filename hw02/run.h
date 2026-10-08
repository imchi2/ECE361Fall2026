/*
 * run.h: ECE 361, Fall 2026, HW2 threshold module.
 *
 * Finds the longest stretch of consecutive readings whose temperature is
 * above a threshold. This is the "above 30.0 C: longest run ..." line.
 */
#ifndef RUN_H
#define RUN_H

/*
 * The answer, as an index and a length rather than as two tick numbers.
 *
 * start is an index into the dataset (not a tick), so the driver can
 * look up the tick values itself. length == 0 means there was no run at
 * all, which is how the driver decides to print "never".
 */
struct run {
    int length;   /* how many readings were in the run; 0 if none */
    int start;    /* index of the first reading of the run */
};

/*
 * Scan values[0..n-1] for the longest run of entries strictly greater
 * than threshold, and return where it starts and how long it is.
 *
 * When two runs tie, the earlier one wins, because `run > best` (not
 * >=) only replaces the best answer on a strictly longer run. That is
 * the original program's rule, and it decides which tick numbers the
 * summary prints, so it is kept.
 *
 * The caller must pass n >= 1 (an empty input never reaches here), but n
 * may be any value and a run may be absent: then length is 0.
 */
struct run longest_run(const float *values, int n, float threshold);

#endif /* RUN_H */
