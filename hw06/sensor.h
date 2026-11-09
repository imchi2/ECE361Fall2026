/**
 * @file sensor.h
 * @brief Homework 6, Part 1: the sensor module's interface.
 *
 * One job: turn the two read-only sensor registers of the iom361
 * (TEMP_REG and HUMID_REG) into one reading_t. The conversion formulas
 * come from the register table in iom361_r4.h:
 *
 *      temperature in C  = raw / 2^20 * 200 - 50
 *      humidity in %RH   = raw / 2^20 * 100
 *
 * The registers hold 20-bit integers, so a reading comes back close to
 * but never exactly what the "world" set: set 91.0 C and the register
 * gives back about 90.99998. sensor.c therefore rounds every reading
 * to 0.1, so a scenario which sets 91.0 reads back as 91.0.
 *
 * Rules for the module (from the spec):
 *   - no printf in sensor.c: printing is the caller's job;
 *   - no calls to the test functions _iom361_set_...: on a real board
 *     they would not exist.
 *
 * Build and run, from hw06/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make test"
 */
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h> /* uint32_t */
#include <time.h>   /* time_t */

/**
 * reading_t - one sensor reading, taken at one tick of the clock.
 *
 * The three fields the spec names for this homework: the timestamp,
 * the temperature in degrees Celsius, and the humidity in percent.
 */
typedef struct reading {
    time_t t;       /* the clock when the reading was taken */
    float  temp_c;  /* degrees Celsius, rounded to 0.1 */
    float  rh_pct;  /* relative humidity in percent, rounded to 0.1 */
} reading_t;

/**
 * sensor_read - read both sensor registers, convert, round, fill *r.
 *
 * @param base the base address from iom361_initialize()
 * @param now  the current clock value (scenario start + tick); the
 *             sensor has no clock of its own, so the caller passes it
 * @param r    the reading to fill in; left completely unchanged if
 *             either register read fails
 * @return 0 on success, or the iom361 return code of the failed read
 */
int sensor_read(uint32_t *base, time_t now, reading_t *r);

#endif /* SENSOR_H */
