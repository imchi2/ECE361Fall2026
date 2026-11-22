/**
 * @file    iom361_r4.h
 * @brief   Header file for the ECE 361 I/O module emulator
 * @author  Roy Kravitz (roy.kravitz@pdx.edu), versions 1.0 to 3.0
 * @author  Christof Teuscher (teuscher@pdx.edu), version 4.0
 * @date    05-Oct-2026
 * @version 4.0
 *
 * @details
 * The ECE 361 I/O module (iom361) emulates a memory-mapped I/O system with a
 * number of "typical" peripheral registers: switches, LEDs, an RGB LED, and an
 * AHT20 temperature and humidity sensor. The registers are a block of eight
 * 32-bit words in memory. A program reads and writes them the way it would on
 * a microcontroller, but it runs on a laptop, and test code can set what the
 * "hardware" sees.
 *
 * Roy Kravitz wrote the original module (versions 1.0 to 3.0) for ECE 361.
 * Version 4.0 keeps his API: every call that worked with version 3.0 works
 * with version 4.0.
 *
 * Revision History:
 * -----------------
 * 1.0  Oct-2023  Roy Kravitz        Created the module
 * 2.0  Oct-2024  Roy Kravitz        Added a few functions, used array for I/O registers
 * 3.0  Oct-2025  Roy Kravitz        Changed function and variable names to put them more
 *                                   in line w/ C coding standards
 * 4.0  Oct-2026  Christof Teuscher  Compiles on macOS (clang) and Linux (gcc) with
 *                                   -std=c11 -Wall -Wextra and no warnings; no undefined
 *                                   behavior under -fsanitize=address,undefined; sensor
 *                                   values rounded instead of truncated and clamped to the
 *                                   sensor range; arguments checked; LED display can be
 *                                   turned off; reproducible random readings; random
 *                                   helper folded in (float_rndm.c is gone). See README.md.
 *
 * Register formats:
 * -----------------
 *
 *  o switches[31:0]:   One bit per switch starting w/ bit[0] (rightmost, LSB). The number
 *                      of switches is specified in iom361_initialize(), max of 32.
 *                      A switch is on for every bit that is 1. Bits above the number of
 *                      switches read as 0.
 *
 *  o leds[31:0]:       One bit per LED starting w/ bit[0] (rightmost, LSB). The number
 *                      of LEDs is specified in iom361_initialize(), max of 32.
 *                      An LED is on (lit) for every bit that is 1. While the display is
 *                      on (the default), every write to the register prints it: 'o' for
 *                      every lit LED, '_' for every dark LED.
 *
 *  o rgb_led[31:0]:    Control register for the RGB LED:
 * <pre>
 *      - bits[31:31]:  Enable - 1 if the RGB outputs are enabled
 *      - bits[30:24]:  *reserved*
 *      - bits[23:16]:  8-bit duty cycle for the red segment
 *      - bits[15:8]:   8-bit duty cycle for the green segment
 *      - bits[7:0]:    8-bit duty cycle for the blue segment
 * </pre>
 *                      While the display is on, every write to the register prints it.
 *
 *  o temperature[31:0]: iom361 emulates an AHT20 temperature/humidity sensor. The
 *                      temperature is a 20-bit unsigned number ST in bits[19:0]:
 *                          Temp(degrees C) = (ST / 2**20) * 200 - 50
 *
 *  o humidity[31:0]:   The relative humidity is a 20-bit unsigned number SRH in bits[19:0]:
 *                          Rel Humidity(%) = (SRH / 2**20) * 100
 *
 *  o reserved_1[31:0]: Reserved for future use. Can be written and read.
 *  o reserved_2[31:0]: Reserved for future use. Can be written and read.
 *  o reserved_3[31:0]: Reserved for future use. Can be written and read.
 *
 * Return codes (rtn_code):
 * ------------------------
 *  0  success
 *  1  base is not the address returned by iom361_initialize(), or bad arguments
 *     to iom361_initialize()
 *  2  offset is out of range
 *  3  offset is not a multiple of 4 (not the start of a register)
 *  5  iom361_initialize() has not been called
 */

#ifndef IOM361_R4_H
#define IOM361_R4_H

#include <stdint.h>
#include <stdbool.h>

// define the I/O register map
typedef struct {
    uint32_t    switches;
    uint32_t    leds;
    uint32_t    rgbled;
    uint32_t    temperature;
    uint32_t    humidity;
    uint32_t    reserved_1;
    uint32_t    reserved_2;
    uint32_t    reserved_3;
} ioreg_t, *ioreg_ptr_t;

// register offsets, in bytes from the base of the register block
enum {
    SWITCHES_REG    = 0x00,
    LEDS_REG        = 0x04,
    RGB_LED_REG     = 0x08,
    TEMP_REG        = 0x0C,
    HUMID_REG       = 0x10,
    RSVD1_REG       = 0x14,
    RSVD2_REG       = 0x18,
    RSVD3_REG       = 0x1C
};

// define constants
#define NUM_IO_REGS     8       // There are 8 I/O registers in the I/O map

/*
 * API functions. These are low level functions that read/write the I/O
 * registers directly. You can use them to build higher level functionality
 * in your own code, but it doesn't get much more basic than this.
 */

/**
 * iom361_initialize() - initializes the ECE 361 I/O module
 *
 * Initializes the I/O module emulator and returns a pointer to the base of the
 * I/O register block. After initialization the switches and LEDs are 0, the
 * RGB LED is off, and the sensor reads 23.5 degrees C and 75% RH. Calling it
 * again resets the module.
 *
 * @param   num_switches: the number of switches, 0 to 32
 * @param   num_leds: the number of LEDs, 0 to 32
 * @param   *rtn_code: a pointer to the return code: 0 for success, 1 if num_switches
 *          or num_leds is out of range. May be NULL.
 *
 * @return  a pointer to the base of the I/O register block. NULL if the function fails
 */
uint32_t* iom361_initialize(int num_switches, int num_leds, int* rtn_code);

/**
 * iom361_readReg() - returns the value of an I/O register
 *
 * Reads and returns the value of the I/O register at base + offset.
 *
 * @param   base: address of the base of the I/O register block
 * @param   offset: offset into the I/O register block, a multiple of 4 (use the enum)
 * @param   *rtn_code: a pointer to the return code (see the table above). May be NULL.
 *
 * @return  the contents of the register, or 0xDEADBEEF on error
 */
uint32_t iom361_readReg(uint32_t* base, uint32_t offset, int* rtn_code);

/**
 * iom361_writeReg() - writes a 32-bit value to an I/O register
 *
 * Writes a new value into the I/O register at base + offset. Writes to the
 * read-only registers (switches, temperature, humidity) are ignored, and the
 * return code is still 0, as in version 3.0.
 *
 * @param   base: address of the base of the I/O register block
 * @param   offset: offset into the I/O register block, a multiple of 4 (use the enum)
 * @param   value: the value to write
 * @param   *rtn_code: a pointer to the return code (see the table above). May be NULL.
 *
 * @return  the contents of the register after the write (a read-only register is
 *          unchanged), or 0xDEADBEEF on error
 */
uint32_t iom361_writeReg(uint32_t* base, uint32_t offset, uint32_t value, int* rtn_code);

/**
 * build_rgb_reg() - builds a value for the RGB LED register from duty cycles
 *
 * @param   enable: 1 to enable the PWM outputs to the RGB LED, 0 to disable
 * @param   red_dc, green_dc, blue_dc: red, green and blue duty cycles, 0 to 255
 *
 * @return  a uint32_t that can be written to the RGB LED register
 */
uint32_t build_rgb_reg(uint8_t enable, uint8_t red_dc, uint8_t green_dc, uint8_t blue_dc);

/**
 * iom361_set_display() - turns the printing of the LED and RGB LED registers on or off
 *
 * New in version 4.0. The display is on by default, as in version 3.0. Turn it
 * off when your program's output must not be mixed with the LED display, for
 * example in tests that compare output with a file.
 *
 * @param   on: true to print every write to LEDS_REG and RGB_LED_REG, false to stay quiet
 */
void iom361_set_display(bool on);

/**
 * iom361_seed() - seeds the random number generator used by _iom361_set_sensor1_rndm()
 *
 * New in version 4.0. Version 3.0 seeded rand() from the clock in iom361_initialize(),
 * so random readings could not be reproduced. Version 4.0 does not touch rand() unless
 * you call this function. Pass a fixed seed for reproducible tests, or
 * (unsigned) time(NULL) for different values on every run.
 *
 * @param   seed: the seed passed to srand()
 */
void iom361_seed(unsigned int seed);

/*
 * These functions are used for testing. They set a read-only register to a value.
 * For example, there is a function to write a new value to the switch register, and
 * one for the temperature/humidity sensor. They exist because we are emulating
 * memory-mapped I/O: there is no "real" hardware at the other end.
 *
 * The function names start w/ a _ to differentiate them from what would normally be
 * the API. Call iom361_initialize() first; it resets every register.
 */

/**
 * _iom361_set_switches() - sets the value of the switch register
 *
 * @param   value: value for the switch register. Bits above the number of switches
 *          given to iom361_initialize() are cleared.
 */
void _iom361_set_switches(uint32_t value);

/**
 * _iom361_set_sensor1() - sets the temperature and humidity of the emulated AHT20
 *
 * Converts the temperature and humidity to the 20-bit register values and writes
 * them to the sensor registers. The conversion rounds to the nearest register
 * value, so reading the register back gives the value you set to within 0.0001
 * degrees C or %RH, slightly above or slightly below. Compare readings with a
 * tolerance, or round them, before you compare them with a threshold.
 *
 * Values outside what a 20-bit register can hold are clamped: temperature to
 * -50 to just under 150 degrees C, humidity to 0 to just under 100% RH.
 *
 * @param   new_temp: new temperature in degrees C
 * @param   new_humid: new relative humidity in %
 */
void _iom361_set_sensor1(float new_temp, float new_humid);

/**
 * _iom361_set_sensor1_rndm() - sets a random temperature and humidity
 *
 * Sets the temperature and humidity to random values within the given ranges.
 * The order of the bounds does not matter. Uses rand(); see iom361_seed().
 *
 * @param   temp_low, temp_hi: temperature range in degrees C
 * @param   humid_low, humid_hi: relative humidity range in %
 */
void _iom361_set_sensor1_rndm(float temp_low, float temp_hi,
    float humid_low, float humid_hi);

#endif
