/*
 * input.h: ECE 361, Fall 2026, HW2 input module.
 *
 * Owns the job of turning bytes into a struct dataset: reading lines,
 * ignoring the ones that are not data, parsing the rest, and reporting
 * how many lines it had to skip.
 */
#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include "readings.h"

/*
 * Read every line from `in` and store the valid ones in *ds.
 *
 * On return ds->count is how many readings were stored and ds->skipped
 * is how many non-comment lines failed to parse. The caller is expected
 * to have set both to zero first.
 *
 * If the input holds more than MAX_READINGS readings, a warning goes to
 * stderr and reading stops there; the readings already stored stay as
 * they are.
 */
void read_dataset(FILE *in, struct dataset *ds);

#endif /* INPUT_H */
