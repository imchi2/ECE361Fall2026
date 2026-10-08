/*
 * histogram.c: ECE 361, Fall 2026, HW2 histogram module.
 */
#include <stdio.h>
#include "readings.h"
#include "histogram.h"

/* 0 to 100 C in 20 bins of 5 C each. These two constants only ever
 * appear here: they describe the chart, so they belong to the module
 * that draws it rather than to the shared header. */
#define NUM_BINS  20
#define BIN_WIDTH 5.0f

/*
 * Fill the bins and print the chart, for the first ds->count readings.
 *
 * Counting:
 *   - divide the temperature by the bin width and truncate to get a bin
 *     index, so 27.6 C lands in bin 5 (25 to 30 C);
 *   - a temperature below 0 C cannot have a negative bin, so it is
 *     pushed into bin 0;
 *   - anything at or above 100 C is pushed into the last bin.
 * That clamping is what keeps every reading inside the 20 printed
 * ranges, and it is the original program's behaviour.
 *
 * Printing:
 *   - empty bins print nothing at all, so the chart only shows ranges
 *     that actually occurred;
 *   - each bar is one '*' per reading, followed by the count;
 *   - the header line "histogram:" is printed here too, so the whole
 *     section is produced by one function.
 */
void hist_print(const struct dataset *ds) {
    int bins[NUM_BINS];
    int b;
    int i;

    for (b = 0; b < NUM_BINS; b++)
        bins[b] = 0;

    for (i = 0; i < ds->count; i++) {
        int bin = (int) (ds->temps[i] / BIN_WIDTH);
        if (ds->temps[i] < 0.0f)
            bin = 0;
        if (bin >= NUM_BINS)
            bin = NUM_BINS - 1;
        bins[bin]++;
    }

    printf("histogram:\n");
    for (b = 0; b < NUM_BINS; b++) {
        if (bins[b] == 0)
            continue;
        printf("  %5.1f to %5.1f C | ",
               b * BIN_WIDTH, (b + 1) * BIN_WIDTH);
        for (i = 0; i < bins[b]; i++)
            putchar('*');
        printf(" %d\n", bins[b]);
    }
}
