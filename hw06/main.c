/**
 * @file main.c
 * @brief Homework 6, Part 3b: ./replay - replay a scenario file into
 *        a trace, one line per tick.
 *
 * Usage:  ./replay scenario.txt
 *
 * The program loads the scenario, initializes the iom361 with 4
 * switches and 4 LEDs, turns the model's own LED display off, and then
 * walks every tick from 0 to the last one in the file. Each tick:
 *
 *   1. the line in force sets the "world" (_iom361_set_sensor1 and
 *      _iom361_set_switches), so the registers hold the scenario's
 *      values for this tick;
 *   2. the clock is start + tick, one second per tick;
 *   3. the sensor and the switches are read back through the
 *      registers, the way a program on a real board would;
 *   4. every switch that just went down toggles its LED;
 *   5. one trace line is printed, in UTC so the trace reads the same
 *      on every computer.
 *
 * Build and run, from hw06/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make"
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && ./replay scenarios/press.txt"
 */

#include "iom361_r4.h"
#include "panel.h"
#include "scenario.h"
#include "sensor.h"

#include <stdbool.h> /* false */
#include <stdio.h>   /* printf, fprintf, stderr */
#include <time.h>    /* time_t, gmtime, strftime */

int main(int argc, char *argv[])
{
    scenario_t sc;      /* the loaded scenario (about 16 KB) */
    int bad;            /* scenario_load's answer */
    int rc;             /* iom361 return code */
    uint32_t *base;     /* the register block, from iom361 */
    uint32_t before;    /* the switch register at the previous tick */
    int last_tick;      /* the last line's tick: the loop's bound */
    int tick;

    /* Exactly one argument: the scenario file. Anything else gets
       the usage line on stderr (never on stdout - the trace lives
       there and the regression test diffs it). */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s scenario.txt\n", argv[0]);
        return 1;
    }

    bad = scenario_load(argv[1], &sc);
    if (bad != 0)
    {
        fprintf(stderr, "%s: wrong scenario, first wrong line %d\n",
                argv[1], bad);
        return 1;
    }

    /* A file with only a start line loads fine but has nothing to
       replay: the "last tick" would not exist. */
    if (sc.num_lines == 0)
    {
        fprintf(stderr, "%s: no tick lines to replay\n", argv[1]);
        return 1;
    }

    /* The board: 4 switches and 4 LEDs, as the spec says. */
    rc = 0;
    base = iom361_initialize(4, 4, &rc);
    if (base == NULL || rc != 0)
    {
        fprintf(stderr, "iom361_initialize failed: %d\n", rc);
        return 1;
    }

    /* The model prints every write to the LED register - turn that
       off first thing so the trace stays one clean line per tick. */
    iom361_set_display(false);

    /* Everything starts off, so a switch that is already on at
       tick 0 counts as pressed on the very first tick. */
    before = 0;

    /* The last line's tick is the last tick of the replay: the
       values hold after it, so there is nothing new to show. */
    last_tick = sc.lines[sc.num_lines - 1].tick;

    for (tick = 0; tick <= last_tick; tick++)
    {
        const scenario_line_t *line;
        time_t now;
        reading_t r;
        int rc_sensor;
        int rc_sw;
        uint32_t switches;
        uint32_t pressed;
        uint32_t leds;
        int rc_led;
        struct tm *utc;
        char when[32];

        /* Step 1: the line in force sets the world. Before the first
           line nothing is in force yet (scenario_at returns NULL),
           so the board keeps the state iom361_initialize gave it. */
        line = scenario_at(&sc, tick);
        if (line != NULL)
        {
            _iom361_set_sensor1(line->temp_c, line->rh_pct);
            _iom361_set_switches(line->switches);
        }

        /* Step 2: the clock is the start time plus the tick. */
        now = sc.start + (time_t) tick;

        /* Step 3: read the sensor and the switches through the
           registers. Both reads must work before anything is
           printed, so a failure ends the replay loudly instead of
           quietly skipping a trace line. */
        rc_sensor = sensor_read(base, now, &r);
        if (rc_sensor != 0)
        {
            fprintf(stderr, "sensor_read failed at tick %d: %d\n",
                    tick, rc_sensor);
            return 1;
        }

        rc_sw = 0;
        switches = iom361_readReg(base, SWITCHES_REG, &rc_sw);
        if (rc_sw != 0)
        {
            fprintf(stderr, "switch read failed at tick %d: %d\n",
                    tick, rc_sw);
            return 1;
        }

        /* Step 4: which switches went down since the previous tick?
           Those are the presses, and each one toggles its LED. A
           release changes nothing - only presses toggle, exactly as
           the spec describes the simple device. */
        pressed = panel_pressed(switches, before);
        before = switches;

        /* Only bits 0..3 can ever be set here: _iom361_set_switches
           masks the value to the 4 switches we initialized, so this
           loop never reaches LED 4 and above. */
        for (int k = 0; k < 32; k++)
        {
            if (((pressed >> k) & 1u) != 0)
                panel_toggle(base, k);
        }

        /* The LED register after the toggles: that is what this
           tick's trace line reports as leds=. */
        rc_led = 0;
        leds = iom361_readReg(base, LEDS_REG, &rc_led);
        if (rc_led != 0)
        {
            fprintf(stderr, "LED read failed at tick %d: %d\n",
                    tick, rc_led);
            return 1;
        }

        /* Step 5: the time in UTC, so a trace is the same on every
           computer. gmtime converts the clock, strftime formats it. */
        utc = gmtime(&now);
        if (utc == NULL)
        {
            fprintf(stderr, "gmtime failed at tick %d\n", tick);
            return 1;
        }
        if (strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", utc) == 0)
        {
            fprintf(stderr, "strftime failed at tick %d\n", tick);
            return 1;
        }

        /* One line per tick - the trace the regression test compares
           with expected/. temp and rh are already rounded to 0.1 by
           sensor_read, so "%.1f" shows them exactly as stored. */
        printf("%s temp=%.1f rh=%.1f switches=0x%X pressed=0x%X leds=0x%X\n",
               when, r.temp_c, r.rh_pct, switches, pressed, leds);
    }

    return 0;
}
