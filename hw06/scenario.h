/**
 * @file scenario.h
 * @brief Homework 6, Part 3a: loading a scenario file and asking
 *        which line is in force at a tick.
 *
 * A scenario is a plain text file (the format comes from the project
 * sheet):
 *
 *      # comments start with '#'
 *      start 1791273600                # 2026-10-06 08:00:00 UTC
 *
 *      # tick  temp   rh    switches
 *      0       20.0   50    0x0
 *      5       20.0   50    0x1      # COIN pressed
 *      6       20.0   50    0x0      # and released
 *
 * The first content line gives the start time as a Unix timestamp.
 * Every line after it gives a tick (seconds since the start), a
 * temperature, a humidity, and the switch register in hexadecimal.
 * The ticks must increase from line to line; between two lines the
 * values hold.
 *
 * scenario_t holds at most 1000 lines and never calls malloc: the
 * array is part of the struct, so the caller decides where the
 * scenario lives (on the stack in main, in the tests).
 *
 * Build and run, from hw06/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make test"
 */
#ifndef SCENARIO_H
#define SCENARIO_H

#include <stdint.h> /* uint32_t */
#include <time.h>   /* time_t */

/* The most tick lines a scenario_t can hold. A file with more lines
   than this is rejected as wrong rather than silently truncated. */
#define SCENARIO_MAX_LINES 1000

/**
 * scenario_line_t - one "tick temp rh switches" line of the file.
 */
typedef struct scenario_line {
    int      tick;     /* seconds since the start */
    float    temp_c;   /* temperature in degrees C */
    float    rh_pct;   /* humidity in percent */
    uint32_t switches; /* the switch register, one bit per switch */
} scenario_line_t;

/**
 * scenario_t - a whole loaded scenario: the start time plus the lines.
 */
typedef struct scenario {
    time_t          start;     /* from the "start" line */
    int             num_lines; /* how many tick lines were loaded */
    scenario_line_t lines[SCENARIO_MAX_LINES];
} scenario_t;

/**
 * scenario_load - read a scenario file into *s.
 *
 * Everything *s held is discarded first. Comments ('# to end of
 * line), blank lines and whitespace are skipped; line numbers in the
 * return value count every physical line of the file, comments and
 * blanks included, so the number always matches what an editor shows.
 *
 * A line is wrong when it is a tick line before the start line, a
 * second start line, a line with a missing or extra field, a negative
 * tick, a tick that does not increase, a switch value that is not
 * hexadecimal, or the 1001st line. A file that cannot be opened is
 * reported as line 1.
 *
 * @param path the file to read
 * @param s    the scenario to fill in
 * @return 0 on success, or the number of the first wrong line
 */
int scenario_load(const char *path, scenario_t *s);

/**
 * scenario_at - the line in force at a tick.
 *
 * The values hold between lines, so the line in force at tick t is
 * the last line whose tick has already happened. Before the first
 * line (or on an empty scenario) no line is in force yet and the
 * function returns NULL; after the last line, the last line stays in
 * force forever.
 *
 * @param s     a loaded scenario
 * @param tick  the tick to look up
 * @return the line in force, or NULL if none has started yet
 */
const scenario_line_t *scenario_at(const scenario_t *s, int tick);

#endif /* SCENARIO_H */
