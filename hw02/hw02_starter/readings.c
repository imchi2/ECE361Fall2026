/*
 * readings.c: ECE 361, Fall 2026, HW2 starter
 *
 * Reads sensor readings from standard input and prints a summary: the number
 * of readings, the minimum, maximum, and mean temperature and humidity, the
 * longest run of readings above a temperature threshold, and a histogram of
 * the temperatures.
 *
 * Input: one reading per line, "tick temperature humidity", for example
 *     12  23.5  41.0
 * Blank lines and lines starting with '#' are ignored. A line that does not
 * hold three numbers is counted as skipped.
 *
 * Usage: ./readings [threshold] < data/normal.txt
 *        threshold is in degrees C, default 30.0
 *
 * The program works. It is also one file, with global variables, repeated
 * code, and a main() that does everything. HW2 asks you to split it into
 * modules without changing what it prints.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_READINGS 1000
#define LINE_LEN     128
#define NUM_BINS     20          /* histogram: 0 to 100 C in 5 C bins */
#define BIN_WIDTH    5.0f

int   ticks[MAX_READINGS];
float temps[MAX_READINGS];
float hums[MAX_READINGS];
int   count = 0;
int   skipped = 0;
float threshold = 30.0f;

/* largest temperature from index i to the end, recursively */
float max_temp(int i) {
    if (i == count - 1)
        return temps[i];
    float rest = max_temp(i + 1);
    return temps[i] > rest ? temps[i] : rest;
}

int main(int argc, char *argv[]) {
    char line[LINE_LEN];

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

    while (fgets(line, sizeof line, stdin) != NULL) {
        int i = 0;
        while (line[i] == ' ' || line[i] == '\t')
            i++;
        if (line[i] == '\n' || line[i] == '\0' || line[i] == '#')
            continue;
        if (count == MAX_READINGS) {
            fprintf(stderr, "warning: more than %d readings, the rest are ignored\n", MAX_READINGS);
            break;
        }
        if (sscanf(line, "%d %f %f", &ticks[count], &temps[count], &hums[count]) == 3)
            count++;
        else
            skipped++;
    }

    printf("readings: %d\n", count);
    printf("skipped:  %d\n", skipped);
    if (count == 0) {
        printf("no readings, no summary\n");
        return 0;
    }

    /* temperature: min and mean */
    float tmin = temps[0];
    float tsum = 0.0f;
    for (int i = 0; i < count; i++) {
        if (temps[i] < tmin)
            tmin = temps[i];
        tsum += temps[i];
    }
    printf("temperature: min %6.1f  max %6.1f  mean %6.2f C\n", tmin, max_temp(0), tsum / count);

    /* humidity: min, max, and mean, the same loop again */
    float hmin = hums[0];
    float hmax = hums[0];
    float hsum = 0.0f;
    for (int i = 0; i < count; i++) {
        if (hums[i] < hmin)
            hmin = hums[i];
        if (hums[i] > hmax)
            hmax = hums[i];
        hsum += hums[i];
    }
    printf("humidity:    min %6.1f  max %6.1f  mean %6.2f %%RH\n", hmin, hmax, hsum / count);

    /* longest run of consecutive readings above the threshold */
    int run = 0, best = 0, best_start = -1, start = 0;
    for (int i = 0; i < count; i++) {
        if (temps[i] > threshold) {
            if (run == 0)
                start = i;
            run++;
            if (run > best) {
                best = run;
                best_start = start;
            }
        } else {
            run = 0;
        }
    }
    if (best == 0)
        printf("above %.1f C: never\n", threshold);
    else
        printf("above %.1f C: longest run %d readings, from tick %d to tick %d\n",
               threshold, best, ticks[best_start], ticks[best_start + best - 1]);

    /* histogram of temperatures, 5 C bins; values outside 0..100 go to the end bins */
    int bins[NUM_BINS] = {0};
    for (int i = 0; i < count; i++) {
        int b = (int) (temps[i] / BIN_WIDTH);
        if (temps[i] < 0.0f)
            b = 0;
        if (b >= NUM_BINS)
            b = NUM_BINS - 1;
        bins[b]++;
    }
    printf("histogram:\n");
    for (int b = 0; b < NUM_BINS; b++) {
        if (bins[b] == 0)
            continue;
        printf("  %5.1f to %5.1f C | ", b * BIN_WIDTH, (b + 1) * BIN_WIDTH);
        for (int k = 0; k < bins[b]; k++)
            putchar('*');
        printf(" %d\n", bins[b]);
    }
    return 0;
}
