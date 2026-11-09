/**
 * @file panel.h
 * @brief Homework 6, Part 2: switch edges and LED writes.
 *
 * The panel is the part of the device that touches the switch and LED
 * registers. Two functions turn two snapshots of the switch register
 * into events (a press, a release); two others change exactly one LED
 * without disturbing the rest.
 *
 * Build and run, from hw06/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make test"
 */
#ifndef PANEL_H
#define PANEL_H

#include <stdint.h> /* uint32_t */

/**
 * panel_pressed - the switches that went from off to on.
 *
 * @param now   the switch register now
 * @param before the switch register at the previous tick
 * @return one bit per switch that is newly on
 */
uint32_t panel_pressed(uint32_t now, uint32_t before);

/**
 * panel_released - the switches that went from on to off.
 *
 * @param now   the switch register now
 * @param before the switch register at the previous tick
 * @return one bit per switch that is newly off
 */
uint32_t panel_released(uint32_t now, uint32_t before);

/**
 * panel_led - turn LED n on or off, leaving every other LED alone.
 *
 * Does the job by read-modify-write: it reads LEDS_REG, changes only
 * bit n, and writes the word back.
 *
 * @param base the base address from iom361_initialize()
 * @param n    the LED number, 0 to 31
 * @param on   nonzero to light LED n, 0 to darken it
 * @return 0 on success, -1 if n is not 0 to 31, or the iom361 return
 *         code of the failed read or write
 */
int panel_led(uint32_t *base, int n, int on);

/**
 * panel_toggle - flip LED n, leaving every other LED alone.
 *
 * @param base the base address from iom361_initialize()
 * @param n    the LED number, 0 to 31
 * @return 0 on success, -1 if n is not 0 to 31, or the iom361 return
 *         code of the failed read or write
 */
int panel_toggle(uint32_t *base, int n);

#endif /* PANEL_H */
