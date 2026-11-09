/**
 * @file sensor.c
 * @brief Homework 6, Part 1: turn the iom361 temperature and humidity
 *        registers into one reading_t.
 *
 * The two registers hold 20-bit raw values. This module reads them,
 * converts each with the formulas printed in iom361_r4.h, rounds the
 * result to 0.1, and stores everything with the caller's timestamp.
 *
 * Two rules from the spec, visible in the code below:
 *   - a failed read returns the iom361 return code and changes *r
 *     nothing at all, so the caller never sees a half-updated reading;
 *   - no printf and no _iom361_set_... calls: on a real board the
 *     test hooks would not exist, and printing is the caller's job.
 *
 * Build and run, from hw06/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make test"
 */

#include "sensor.h"
#include "iom361_r4.h"

/* The registers are 20 bits wide (0..1048575). The conversion formulas
   divide by 2^20 = 1048576, the value that maps full scale to 1.0. */
#define SENSOR_FULL_SCALE 1048576.0

/* Mask for bits 19..0: keeps the raw reading and drops whatever the
   top 12 bits of the 32-bit register happen to hold. */
#define SENSOR_RAW_MASK   0x000FFFFFu

/**
 * round_tenth - round a converted sensor value to the nearest 0.1.
 *
 * Why the +0.5 / -0.5 dance: casting a double to int truncates toward
 * zero, so adding 0.5 first lands positive values on the nearest
 * integer, and subtracting 0.5 does the same for negative ones (the
 * temperature formula can go down to -50). Scaling by 10 first means
 * "the nearest integer" is "the nearest tenth".
 *
 * @param v the converted value (degrees C or %RH)
 * @return the value rounded to one decimal place
 */
static float round_tenth(double v)
{
    double scaled = v * 10.0;

    if (scaled >= 0.0)
        scaled = scaled + 0.5;
    else
        scaled = scaled - 0.5;

    /* A 20-bit raw value can never push this past about 1500 in
       either direction, so int is plenty wide for the cast. */
    return ((float) ((int) scaled)) / 10.0f;
}

int sensor_read(uint32_t *base, time_t now, reading_t *r)
{
    int rc;
    uint32_t raw_temp;
    uint32_t raw_humid;
    double temp_c;
    double rh_pct;

    /* Read both registers first. If either read fails, return its
       code right away: *r has not been touched yet, which is exactly
       the "then it changes nothing" rule from the spec. */
    rc = 0;
    raw_temp = iom361_readReg(base, TEMP_REG, &rc);
    if (rc != 0)
        return rc;

    rc = 0;
    raw_humid = iom361_readReg(base, HUMID_REG, &rc);
    if (rc != 0)
        return rc;

    /* Keep bits 19..0 of each reading: that is where the 20-bit raw
       value lives, as the register table in iom361_r4.h says. */
    raw_temp = raw_temp & SENSOR_RAW_MASK;
    raw_humid = raw_humid & SENSOR_RAW_MASK;

    /* The formulas from iom361_r4.h, computed in double so the
       rounding below sees every digit the register can express. */
    temp_c = ((double) raw_temp) / SENSOR_FULL_SCALE * 200.0 - 50.0;
    rh_pct = ((double) raw_humid) / SENSOR_FULL_SCALE * 100.0;

    /* Only now, with both values safely in hand, update the caller's
       struct - all three fields at once. */
    r->t = now;
    r->temp_c = round_tenth(temp_c);
    r->rh_pct = round_tenth(rh_pct);

    return 0;
}
