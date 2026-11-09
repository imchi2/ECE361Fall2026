/**
 * @file test_hw06.c
 * @brief Homework 6, Part 4: unit tests for the sensor, the panel,
 *        and the scenario loader, in the order the spec lists them.
 *
 * What is covered, in order:
 *   1. sensor: a reading of 91.0 C reads back as 91.0 (the register
 *      can only hold 90.99998, so this tests the rounding), the
 *      timestamp is stamped, and a failed read changes nothing;
 *   2. panel: pressed and released for a press, a release, and a held
 *      switch; panel_led and panel_toggle keep the other LEDs and
 *      reject n outside 0 to 31;
 *   3. scenario: a load with comments and blank lines, the parsed
 *      field values, values holding between lines, and the wrong
 *      files the spec names - no start line, a missing field, ticks
 *      that do not increase - plus a repeated tick.
 *
 * The scenario tests each write a tiny fixture file, load it, and
 * delete it again (load_text below), because scenario_load reads
 * paths, not strings.
 *
 * How to read the output: each check prints PASS, or FAIL with what
 * was expected and what actually came back. A FAIL does not stop the
 * run: every check reports on its own, so one broken case cannot hide
 * the others. The summary counts them all; the program returns 0 when
 * everything passed and 1 otherwise, so `make test` fails loudly.
 *
 * Build and run, from hw06/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make test"
 */

#include "iom361_r4.h"
#include "panel.h"
#include "scenario.h"
#include "sensor.h"

#include <stdio.h> /* printf, fopen, fputs, fclose, remove */

/**
 * load_text - run one scenario test from a string.
 *
 * Writes content to path, loads it with scenario_load, deletes the
 * file again, and hands back scenario_load's answer - or -1 if the
 * fixture file could not be written at all (which the caller reports
 * as a failure of the test itself).
 *
 * @param path    where to write the fixture (the current directory)
 * @param content the whole file, newlines included
 * @param s       the scenario to fill in
 * @return 0, the first wrong line, or -1
 */
static int load_text(const char *path, const char *content, scenario_t *s)
{
    FILE *fp;
    int rc;

    fp = fopen(path, "w");
    if (fp == NULL)
        return -1;

    fputs(content, fp);
    fclose(fp);

    rc = scenario_load(path, s);
    remove(path);

    return rc;
}

int main(void)
{
    /* How many checks have run, and how many of them printed FAIL.
       The summary and the exit code are built from these two numbers. */
    int tests = 0;
    int failures = 0;

    /* One variable per result type, as in hw01 and hw05: the result
       is stored first, then compared, then printed. */
    int rc;
    uint32_t *base;
    reading_t r;
    reading_t kept;
    uint32_t actual_u32;
    scenario_t sc;
    const scenario_line_t *line;

    printf("hw06 test program\n");

    /* ---------------------------------------------------------------- *
     * The board: 4 switches, 4 LEDs. The display goes off first so    *
     * the model's own LED printing never lands in the test output.     *
     * ---------------------------------------------------------------- */
    rc = 0;
    base = iom361_initialize(4, 4, &rc);
    if (base == NULL || rc != 0)
    {
        printf("FAIL  iom361_initialize: rc=%d base=%p\n", rc, (void *) base);
        return 1;
    }
    iom361_set_display(false);

    /* ---------------------------------------------------------------- *
     * 1. The sensor.                                                    *
     * ---------------------------------------------------------------- */
    printf("\n---- sensor ----\n");

    /* The spec's headline test: the world sets 91.0 C, the register
       can only hold about 90.99998, and the reading must come back
       as 91.0. The humidity (40.0) rides along in the same call, and
       the timestamp must be the one the caller passed in. */
    _iom361_set_sensor1(91.0f, 40.0f);

    tests = tests + 1;
    r.t = (time_t) -1;
    r.temp_c = -1.0f;
    r.rh_pct = -1.0f;
    rc = sensor_read(base, (time_t) 12345, &r);
    if (rc != 0)
    {
        printf("FAIL  sensor_read(91.0): expected 0, got %d\n", rc);
        failures = failures + 1;
    }
    else if (r.t != (time_t) 12345)
    {
        printf("FAIL  sensor_read(91.0).t: expected 12345, got %ld\n",
               (long) r.t);
        failures = failures + 1;
    }
    else if (r.temp_c < 90.95f || r.temp_c > 91.05f)
    {
        printf("FAIL  sensor_read(91.0).temp_c: expected 91.0, got %.5f\n",
               r.temp_c);
        failures = failures + 1;
    }
    else if (r.rh_pct < 39.95f || r.rh_pct > 40.05f)
    {
        printf("FAIL  sensor_read(91.0).rh_pct: expected 40.0, got %.5f\n",
               r.rh_pct);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  a reading of 91.0 C reads back as 91.0\n");
    }

    /* A failed read changes nothing: NULL is never the base that
       iom361_initialize returned, so the read fails (return code 1)
       and the struct must come back exactly as the caller left it -
       all three fields, not just the ones "after" the failure. */
    kept.t = (time_t) 777;
    kept.temp_c = 1.5f;
    kept.rh_pct = 2.5f;

    tests = tests + 1;
    rc = sensor_read(NULL, (time_t) 999, &kept);
    if (rc == 0)
    {
        printf("FAIL  sensor_read(NULL): expected nonzero, got 0\n");
        failures = failures + 1;
    }
    else if (kept.t != (time_t) 777 || kept.temp_c != 1.5f
             || kept.rh_pct != 2.5f)
    {
        printf("FAIL  sensor_read(NULL) changed the reading: "
               "t=%ld temp=%.1f rh=%.1f\n",
               (long) kept.t, kept.temp_c, kept.rh_pct);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  a failed read changes nothing (rc=%d)\n", rc);
    }

    /* ---------------------------------------------------------------- *
     * 2. The panel.                                                     *
     * ---------------------------------------------------------------- */
    printf("\n---- panel ----\n");

    /* A press: switch 0 goes off -> on, so pressed reports it and
       released stays quiet. */
    tests = tests + 1;
    actual_u32 = panel_pressed(0x1u, 0x0u);
    if (actual_u32 != 0x1u)
    {
        printf("FAIL  pressed(0x1, 0x0): expected 0x1, got 0x%X\n",
               actual_u32);
        failures = failures + 1;
    }
    else if (panel_released(0x1u, 0x0u) != 0x0u)
    {
        printf("FAIL  released(0x1, 0x0): expected 0x0, got 0x%X\n",
               panel_released(0x1u, 0x0u));
        failures = failures + 1;
    }
    else
    {
        printf("PASS  a press: pressed reports it, released does not\n");
    }

    /* A release: switch 0 goes on -> off, the other way around. */
    tests = tests + 1;
    actual_u32 = panel_released(0x0u, 0x1u);
    if (actual_u32 != 0x1u)
    {
        printf("FAIL  released(0x0, 0x1): expected 0x1, got 0x%X\n",
               actual_u32);
        failures = failures + 1;
    }
    else if (panel_pressed(0x0u, 0x1u) != 0x0u)
    {
        printf("FAIL  pressed(0x0, 0x1): expected 0x0, got 0x%X\n",
               panel_pressed(0x0u, 0x1u));
        failures = failures + 1;
    }
    else
    {
        printf("PASS  a release: released reports it, pressed does not\n");
    }

    /* A held switch: on in both snapshots, so neither edge fires.
       This is what keeps a held button from toggling every tick. */
    tests = tests + 1;
    if (panel_pressed(0x1u, 0x1u) != 0x0u)
    {
        printf("FAIL  pressed(0x1, 0x1): expected 0x0, got 0x%X\n",
               panel_pressed(0x1u, 0x1u));
        failures = failures + 1;
    }
    else if (panel_released(0x1u, 0x1u) != 0x0u)
    {
        printf("FAIL  released(0x1, 0x1): expected 0x0, got 0x%X\n",
               panel_released(0x1u, 0x1u));
        failures = failures + 1;
    }
    else
    {
        printf("PASS  a held switch: neither pressed nor released\n");
    }

    /* Only the newly pressed bit counts: from 0x1 to 0x3, switch 1
       is the new one, switch 0 was already down. */
    tests = tests + 1;
    actual_u32 = panel_pressed(0x3u, 0x1u);
    if (actual_u32 != 0x2u)
    {
        printf("FAIL  pressed(0x3, 0x1): expected 0x2, got 0x%X\n",
               actual_u32);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  only the newly pressed bit counts\n");
    }

    /* panel_led keeps the other LEDs: start dark, light LED 0, then
       light LED 2. Both must be on afterwards - a write that rebuilt
       the word from scratch would have lost bit 0. */
    rc = 0;
    iom361_writeReg(base, LEDS_REG, 0x0u, &rc);

    tests = tests + 1;
    if (panel_led(base, 0, 1) != 0)
    {
        printf("FAIL  panel_led(base, 0, 1): expected 0, got %d\n",
               panel_led(base, 0, 1));
        failures = failures + 1;
    }
    else if (panel_led(base, 2, 1) != 0)
    {
        printf("FAIL  panel_led(base, 2, 1): expected 0, got %d\n",
               panel_led(base, 2, 1));
        failures = failures + 1;
    }
    else
    {
        actual_u32 = iom361_readReg(base, LEDS_REG, &rc);
        if (rc != 0 || actual_u32 != 0x5u)
        {
            printf("FAIL  panel_led lights 0 and 2: expected 0x5, "
                   "got 0x%X (rc=%d)\n", actual_u32, rc);
            failures = failures + 1;
        }
        else
        {
            printf("PASS  panel_led lights one LED and keeps the others\n");
        }
    }

    /* ...and darkening LED 0 must leave LED 2 lit: 0x5 -> 0x4. */
    tests = tests + 1;
    if (panel_led(base, 0, 0) != 0)
    {
        printf("FAIL  panel_led(base, 0, 0): expected 0, got %d\n",
               panel_led(base, 0, 0));
        failures = failures + 1;
    }
    else
    {
        actual_u32 = iom361_readReg(base, LEDS_REG, &rc);
        if (rc != 0 || actual_u32 != 0x4u)
        {
            printf("FAIL  panel_led darkens 0, keeps 2: expected 0x4, "
                   "got 0x%X (rc=%d)\n", actual_u32, rc);
            failures = failures + 1;
        }
        else
        {
            printf("PASS  panel_led darkens one LED and keeps the others\n");
        }
    }

    /* An LED number outside 0 to 31 is rejected before the register
       is touched: the word stays exactly as it was (0x4). */
    tests = tests + 1;
    if (panel_led(base, -1, 1) != -1)
    {
        printf("FAIL  panel_led(base, -1, 1): expected -1, got %d\n",
               panel_led(base, -1, 1));
        failures = failures + 1;
    }
    else if (panel_led(base, 32, 1) != -1)
    {
        printf("FAIL  panel_led(base, 32, 1): expected -1, got %d\n",
               panel_led(base, 32, 1));
        failures = failures + 1;
    }
    else
    {
        actual_u32 = iom361_readReg(base, LEDS_REG, &rc);
        if (rc != 0 || actual_u32 != 0x4u)
        {
            printf("FAIL  panel_led with bad n changed the LEDs: "
                   "expected 0x4, got 0x%X (rc=%d)\n", actual_u32, rc);
            failures = failures + 1;
        }
        else
        {
            printf("PASS  panel_led rejects n outside 0 to 31\n");
        }
    }

    /* panel_toggle keeps the other LEDs: bit 0 is dark (0x4), so
       toggling it gives 0x5 - bit 2 must survive both toggles. */
    tests = tests + 1;
    if (panel_toggle(base, 0) != 0)
    {
        printf("FAIL  panel_toggle(base, 0): expected 0, got %d\n",
               panel_toggle(base, 0));
        failures = failures + 1;
    }
    else
    {
        actual_u32 = iom361_readReg(base, LEDS_REG, &rc);
        if (rc != 0 || actual_u32 != 0x5u)
        {
            printf("FAIL  panel_toggle 0 on: expected 0x5, got 0%X "
                   "(rc=%d)\n", actual_u32, rc);
            failures = failures + 1;
        }
        else if (panel_toggle(base, 0) != 0)
        {
            printf("FAIL  panel_toggle(base, 0) again: expected 0, "
                   "got %d\n", panel_toggle(base, 0));
            failures = failures + 1;
        }
        else
        {
            actual_u32 = iom361_readReg(base, LEDS_REG, &rc);
            if (rc != 0 || actual_u32 != 0x4u)
            {
                printf("FAIL  panel_toggle 0 off: expected 0x4, "
                       "got 0x%X (rc=%d)\n", actual_u32, rc);
                failures = failures + 1;
            }
            else
            {
                printf("PASS  panel_toggle flips one LED and keeps "
                       "the others\n");
            }
        }
    }

    /* Same rejection as panel_led, checked on its own. */
    tests = tests + 1;
    if (panel_toggle(base, -1) != -1)
    {
        printf("FAIL  panel_toggle(base, -1): expected -1, got %d\n",
               panel_toggle(base, -1));
        failures = failures + 1;
    }
    else if (panel_toggle(base, 32) != -1)
    {
        printf("FAIL  panel_toggle(base, 32): expected -1, got %d\n",
               panel_toggle(base, 32));
        failures = failures + 1;
    }
    else
    {
        printf("PASS  panel_toggle rejects n outside 0 to 31\n");
    }

    /* ---------------------------------------------------------------- *
     * 3. The scenario loader.                                           *
     * ---------------------------------------------------------------- */
    printf("\n---- scenario ----\n");

    /* A load with comments and blank lines. Line by line:
           1  # a full-line comment
           2  (blank)
           3  start 1791273600   # trailing comment
           4  (blank)
           5  # tick temp rh switches
           6  0 20.0 50 0x0
           7  (blank)
           8  5 21.0 40 0x1   # pressed
       None of the comments or blanks carry content, but the line
       numbers still count them - that is what makes a wrong-line
       report match the file an editor shows. */
    tests = tests + 1;
    rc = load_text("tmp_scenario_good.txt",
                   "# a full-line comment\n"
                   "\n"
                   "start 1791273600   # trailing comment\n"
                   "\n"
                   "# tick temp rh switches\n"
                   "0 20.0 50 0x0\n"
                   "\n"
                   "5 21.0 40 0x1   # pressed\n",
                   &sc);
    if (rc == -1)
    {
        printf("FAIL  good scenario: could not write the fixture file\n");
        failures = failures + 1;
    }
    else if (rc != 0)
    {
        printf("FAIL  good scenario: expected 0, got line %d\n", rc);
        failures = failures + 1;
    }
    else if (sc.num_lines != 2)
    {
        printf("FAIL  good scenario: expected 2 lines, got %d\n",
               sc.num_lines);
        failures = failures + 1;
    }
    else if (sc.start != (time_t) 1791273600)
    {
        printf("FAIL  good scenario.start: expected 1791273600, got %ld\n",
               (long) sc.start);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  a load with comments and blank lines\n");
    }

    /* The parsed values land in the right cells of both lines. */
    tests = tests + 1;
    if (sc.num_lines != 2)
    {
        printf("FAIL  field values: expected 2 lines, got %d\n",
               sc.num_lines);
        failures = failures + 1;
    }
    else if (sc.lines[0].tick != 0 || sc.lines[0].temp_c != 20.0f
             || sc.lines[0].rh_pct != 50.0f
             || sc.lines[0].switches != 0x0u)
    {
        printf("FAIL  line 0: expected 0 / 20.0 / 50.0 / 0x0, got "
               "%d / %.1f / %.1f / 0x%X\n",
               sc.lines[0].tick, sc.lines[0].temp_c,
               sc.lines[0].rh_pct, sc.lines[0].switches);
        failures = failures + 1;
    }
    else if (sc.lines[1].tick != 5 || sc.lines[1].temp_c != 21.0f
             || sc.lines[1].rh_pct != 40.0f
             || sc.lines[1].switches != 0x1u)
    {
        printf("FAIL  line 1: expected 5 / 21.0 / 40.0 / 0x1, got "
               "%d / %.1f / %.1f / 0x%X\n",
               sc.lines[1].tick, sc.lines[1].temp_c,
               sc.lines[1].rh_pct, sc.lines[1].switches);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  the fields parse into the right cells\n");
    }

    /* Values hold between lines: at tick 4 the tick-0 line is still
       in force, at tick 5 the tick-5 line takes over, and long after
       the last line the last line still holds. */
    tests = tests + 1;
    line = scenario_at(&sc, 4);
    if (line == NULL)
    {
        printf("FAIL  scenario_at(4): expected line 0, got NULL\n");
        failures = failures + 1;
    }
    else if (line->tick != 0 || line->temp_c != 20.0f)
    {
        printf("FAIL  scenario_at(4): expected tick 0 at 20.0, got "
               "tick %d at %.1f\n", line->tick, line->temp_c);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  between lines the earlier values hold\n");
    }

    tests = tests + 1;
    line = scenario_at(&sc, 5);
    if (line == NULL)
    {
        printf("FAIL  scenario_at(5): expected line 1, got NULL\n");
        failures = failures + 1;
    }
    else if (line->tick != 5 || line->switches != 0x1u)
    {
        printf("FAIL  scenario_at(5): expected tick 5 at 0x1, got "
               "tick %d at 0x%X\n", line->tick, line->switches);
        failures = failures + 1;
    }
    else
    {
        line = scenario_at(&sc, 100);
        if (line == NULL || line->tick != 5)
        {
            printf("FAIL  scenario_at(100): expected the tick-5 line "
                   "to keep holding\n");
            failures = failures + 1;
        }
        else
        {
            printf("PASS  the line takes over at its tick and keeps "
                   "holding\n");
        }
    }

    /* Before the first line nothing is in force yet: scenario_at
       returns NULL rather than pretending tick 0 already started. */
    tests = tests + 1;
    line = scenario_at(&sc, -1);
    if (line != NULL)
    {
        printf("FAIL  scenario_at(-1): expected NULL, got tick %d\n",
               line->tick);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  before the first line nothing is in force\n");
    }

    /* A wrong file: a tick line with no start line before it. The
       first wrong line is line 1 - the file starts out wrong. */
    tests = tests + 1;
    rc = load_text("tmp_scenario_nostart.txt",
                   "0 20.0 50 0x0\n",
                   &sc);
    if (rc == -1)
    {
        printf("FAIL  no start line: could not write the fixture\n");
        failures = failures + 1;
    }
    else if (rc != 1)
    {
        printf("FAIL  no start line: expected line 1, got line %d\n", rc);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  a scenario without a start line is line 1\n");
    }

    /* A wrong file: line 3 drops the switches field. The start line
       is line 1 and the first tick line is line 2, both fine. */
    tests = tests + 1;
    rc = load_text("tmp_scenario_short.txt",
                   "start 1791273600\n"
                   "0 20.0 50 0x0\n"
                   "5 20.0 50\n",
                   &sc);
    if (rc == -1)
    {
        printf("FAIL  missing field: could not write the fixture\n");
        failures = failures + 1;
    }
    else if (rc != 3)
    {
        printf("FAIL  missing field: expected line 3, got line %d\n", rc);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  a missing field is reported at its line\n");
    }

    /* A wrong file: ticks must increase, and 3 after 5 does not. The
       bad line is line 4 (start, 0, 5, 3). */
    tests = tests + 1;
    rc = load_text("tmp_scenario_backwards.txt",
                   "start 1791273600\n"
                   "0 20.0 50 0x0\n"
                   "5 20.0 50 0x0\n"
                   "3 20.0 50 0x0\n",
                   &sc);
    if (rc == -1)
    {
        printf("FAIL  decreasing ticks: could not write the fixture\n");
        failures = failures + 1;
    }
    else if (rc != 4)
    {
        printf("FAIL  decreasing ticks: expected line 4, got line %d\n",
               rc);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  ticks that do not increase are reported\n");
    }

    /* The same rule catches a repeated tick: two lines at tick 5
       would fight over which one is in force, so line 3 is wrong. */
    tests = tests + 1;
    rc = load_text("tmp_scenario_repeated.txt",
                   "start 1791273600\n"
                   "5 20.0 50 0x0\n"
                   "5 21.0 40 0x1\n",
                   &sc);
    if (rc == -1)
    {
        printf("FAIL  repeated tick: could not write the fixture\n");
        failures = failures + 1;
    }
    else if (rc != 3)
    {
        printf("FAIL  repeated tick: expected line 3, got line %d\n", rc);
        failures = failures + 1;
    }
    else
    {
        printf("PASS  a repeated tick is reported\n");
    }

    /* ---------------------------------------------------------------- *
     * Summary                                                          *
     *                                                                     *
     * The exit code is what make looks at: 0 says every check passed,    *
     * 1 makes `make test` stop with an error.                            *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- summary ----\n");
    printf("%d automated test(s)\n", tests);

    if (failures == 0)
    {
        printf("ALL TESTS PASSED\n");
        return 0;
    }

    printf("%d FAILURE(S)\n", failures);
    printf("SOME TESTS FAILED\n");
    return 1;
}
