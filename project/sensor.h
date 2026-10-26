/*
 * sensor.h: ECE 361, Fall 2026, project - the sensor module's interface.
 *
 * One job: turn the two read-only registers of the iom361 sensor
 * (TEMP_REG and HUMID_REG) into a reading_t. The conversion formulas
 * come from the register table on the project sheet:
 *
 *      temperature in C  = raw / 2^20 * 200 - 50
 *      humidity in %RH   = raw / 2^20 * 100
 *
 * The registers hold 20-bit integers, so a reading comes back close to
 * but never exactly what the "world" set: set 91.0 and the register
 * gives 90.99998. Every reading is therefore rounded to 0.1, so that a
 * scenario which sets 91.0 reads 91.0 and thresholds behave sensibly.
 *
 * The implementation (sensor.c) arrives in week 6, together with the
 * iom361 library from Canvas. This header is the contract only.
 *
 * Rule for the module: no printf in here. Printing is the caller's job.
 */
#ifndef SENSOR_H
#define SENSOR_H

#include <time.h>

/*
 * reading_t: one sensor reading, taken at one tick of the clock.
 *
 * The project sheet requires exactly these three things; the record in
 * the store wraps this whole struct, so keep it small and complete.
 */
typedef struct reading {
    time_t stamp;  /* the clock when this reading was taken: the scenario
                      start time plus the current tick, in seconds since
                      the Unix epoch (1 January 1970, 00:00:00 UTC) */
    float  temp_c; /* temperature in degrees Celsius, rounded to 0.1 */
    float  rh;     /* relative humidity in percent, rounded to 0.1 */
} reading_t;

/*
 * sensor_read - read the sensor registers, convert, round, fill *out.
 *
 * Takes:   now, the current clock value (scenario start + tick). It is
 *          stamped into the reading, because the sensor itself has no
 *          clock - time is the caller's business.
 *          out, the address of a reading_t in the caller, to fill in.
 * Returns: 0 when *out holds a fresh reading; nonzero when the register
 *          read failed, in which case *out is left unchanged.
 * Assumes: iom361_initialize has already been called (otherwise the
 *          register reads fail and this function reports it), and out
 *          is never NULL. A NULL out is a bug in the caller.
 */
int sensor_read(time_t now, reading_t *out);

#endif /* SENSOR_H */
