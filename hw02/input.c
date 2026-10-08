/*
 * input.c: ECE 361, Fall 2026, HW2 input module.
 *
 * Everything about reading the file lives here and nowhere else: the
 * line buffer length, the rules for deciding which lines are data, the
 * parse itself, and the warning when the input is too big. The rest of
 * the program only ever sees a filled-in struct dataset.
 */
#include <stdio.h>
#include "readings.h"
#include "input.h"

/* Longest line we will look at. Anything longer than this is read in
 * pieces by fgets; that is exactly what the original program did, so the
 * behaviour on a pathological input does not change either. */
#define LINE_LEN 128

/*
 * Read every line from `in` and store the valid ones in *ds.
 *
 * The order of the checks matters and matches the original program:
 *
 *   1. skip over leading spaces and tabs, so an indented comment or an
 *      indented blank line is still treated as nothing;
 *   2. drop blank lines and lines whose first real character is '#';
 *   3. only then check the capacity, so a comment arriving after the
 *      arrays are full does not trigger a spurious warning;
 *   4. otherwise try to parse "tick temperature humidity". Three numbers
 *      means a reading and count goes up; anything else means the line
 *      was not data and skipped goes up.
 */
void read_dataset(FILE *in, struct dataset *ds) {
    char line[LINE_LEN];

    while (fgets(line, sizeof line, in) != NULL) {
        int i = 0;

        while (line[i] == ' ' || line[i] == '\t')
            i++;

        if (line[i] == '\n' || line[i] == '\0' || line[i] == '#')
            continue;

        if (ds->count == MAX_READINGS) {
            fprintf(stderr, "warning: more than %d readings, the rest are ignored\n", MAX_READINGS);
            break;
        }

        /* sscanf writes straight into the next free slot of each array,
         * which is why the three writes can share one index: count is
         * only bumped when all three numbers were actually there. */
        if (sscanf(line, "%d %f %f",
                   &ds->ticks[ds->count],
                   &ds->temps[ds->count],
                   &ds->hums[ds->count]) == 3)
            ds->count++;
        else
            ds->skipped++;
    }
}
