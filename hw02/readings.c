/*
 * readings.c: ECE 361, Fall 2026, HW2 driver.
 *
 * The program reads sensor readings from standard input and prints a
 * summary: the number of readings, the minimum, maximum, and mean
 * temperature and humidity, the longest run of readings above a
 * temperature threshold, and a histogram of the temperatures.
 *
 * Input: one reading per line, "tick temperature humidity", for example
 *     12  23.5  41.0
 * Blank lines and lines starting with '#' are ignored. A line that does
 * not hold three numbers is counted as skipped.
 *
 * Usage: ./readings [threshold] < data/normal.txt
 *        threshold is in degrees C, default 30.0
 *
 * This file used to contain the whole program. It now only does three
 * things: read the command line, call the modules in order, and print
 * the lines that make up the summary. Every computation lives in its own
 * file (see DIAGRAM.md for the diagram):
 *
 *     input.c       read_dataset()   stdin  -> struct dataset
 *     stats.c       stats_of()       min, max, mean of an array
 *     run.c         longest_run()    longest run above the threshold
 *     histogram.c   hist_print()     the 5 C bins and the bars
 *
 * The printed output is unchanged; regress.sh compares it byte for byte
 * against the files in expected/.
 */
#include <stdio.h>
#include <stdlib.h>

#include "readings.h"     /* struct dataset, MAX_READINGS */
#include "input.h"        /* read_dataset */
#include "stats.h"        /* stats_of */
#include "run.h"          /* longest_run */
#include "histogram.h"    /* hist_print */

int main(int argc, char *argv[]) {
    struct dataset ds;
    float threshold = 30.0f;

    /* The struct lives on the stack now, so it starts with whatever was
     * there before: only the two counters need a defined starting value,
     * because read_dataset() adds to them. The three arrays are written
     * before they are read, so leaving them alone is fine. */
    ds.count = 0;
    ds.skipped = 0;

    /* --- command line ------------------------------------------------ */
    if (argc > 2) {
        fprintf(stderr, "usage: %s [threshold] < readings.txt\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        char *end;
        threshold = strtof(argv[1], &end);
        if (*end != '\0') {
            fprintf(stderr, "error: threshold '%s' is not a number\n", argv[1]);
            return 1;
        }
    }

    /* --- input ------------------------------------------------------- */
    read_dataset(stdin, &ds);

    /* --- summary ----------------------------------------------------- */
    printf("readings: %d\n", ds.count);
    printf("skipped:  %d\n", ds.skipped);
    if (ds.count == 0) {
        /* This check has to stay here, before anything divides by the
         * number of readings: stats_of() would compute 0 / 0 on an
         * empty input, and there is nothing to summarise anyway. */
        printf("no readings, no summary\n");
        return 0;
    }

    /* One call each for temperature and humidity. Both used to be their
     * own copy-pasted loop in main(); now they are the same function
     * pointed at a different array. */
    struct summary t = stats_of(ds.temps, ds.count);
    printf("temperature: min %6.1f  max %6.1f  mean %6.2f C\n",
           t.min, t.max, t.mean);

    struct summary h = stats_of(ds.hums, ds.count);
    printf("humidity:    min %6.1f  max %6.1f  mean %6.2f %%RH\n",
           h.min, h.max, h.mean);

    /* Longest run of consecutive readings above the threshold. The run
     * module returns indexes, so the tick numbers are looked up here,
     * where the dataset is to hand. */
    struct run r = longest_run(ds.temps, ds.count, threshold);
    if (r.length == 0)
        printf("above %.1f C: never\n", threshold);
    else
        printf("above %.1f C: longest run %d readings, from tick %d to tick %d\n",
               threshold, r.length,
               ds.ticks[r.start],
               ds.ticks[r.start + r.length - 1]);

    /* Histogram last; it prints its own section. */
    hist_print(&ds);

    return 0;
}
