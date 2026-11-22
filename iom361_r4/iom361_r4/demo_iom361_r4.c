/**
 * @file    demo_iom361_r4.c
 * @brief   Shows what the ECE 361 I/O module emulator does
 * @author  Roy Kravitz (roy.kravitz@pdx.edu), test_iom361_r3.c, version 3.0
 * @author  Christof Teuscher (teuscher@pdx.edu), version 4.0
 * @date    05-Oct-2026
 * @version 4.0
 *
 * @details
 * A walk through the I/O module, ported from Roy Kravitz's test_iom361_r3.c:
 * <pre>
 *  - initializes the I/O emulator
 *  - reads all of the registers and displays their initial values
 *  - changes the switches and copies them to the LEDs
 *  - sets the RGB LED
 *  - changes the temperature and humidity and displays the new values
 * </pre>
 * The checks that pass or fail are in test_iom361_r4.c.
 */

#include <stdio.h>
#include <time.h>

#include "iom361_r4.h"

#define TEMP_RANGE_LOW      42.0f
#define TEMP_RANGE_HI       52.0f
#define HUMID_RANGE_LOW     72.6f
#define HUMID_RANGE_HI      87.3f

static void show_sensor(uint32_t* io_base) {
    uint32_t temp_value = iom361_readReg(io_base, TEMP_REG, NULL);
    uint32_t humid_value = iom361_readReg(io_base, HUMID_REG, NULL);
    float temp = (temp_value / 1048576.0f) * 200.0f - 50.0f;
    float humid = (humid_value / 1048576.0f) * 100.0f;

    printf("Temperature: %5.1fC (%08X), Relative Humidity: %5.1f%% (%08X)\n",
        temp, (unsigned) temp_value, humid, (unsigned) humid_value);
}

int main(void) {
    int rtn_code;
    uint32_t reg_value;

    printf("ECE 361 - Demo of the ECE 361 IO Module v4.0\n\n");

    // initialize IO module
    printf("Demo 1: Initialize and read the I/O registers\n");
    uint32_t* io_base = iom361_initialize(16, 16, &rtn_code);
    if (rtn_code != 0) {
        printf("FATAL(main): Could not initialize I/O module\n");
        return 1;
    }
    for (int i = 0; i < NUM_IO_REGS; i++) {
        // The I/O registers are all 32 bits wide, so 4 bytes apart
        reg_value = iom361_readReg(io_base, (uint32_t) i * 4, &rtn_code);
        if (rtn_code == 0)
            printf("Reading I/O register %d, value: %08X\n", i, (unsigned) reg_value);
        else
            printf("ERROR(main): Failed reading I/O register %d\n", i);
    }

    // put some values in the switches and copy them to the LEDs
    printf("\nDemo 2: Copy the switches to the LEDs\n");
    const uint32_t patterns[] = { 0xA5A5, 0x5A5A, 0xFFFF, 0x0000 };
    for (int i = 0; i < 4; i++) {
        printf("\tsetting switches to %04X\n", (unsigned) patterns[i]);
        _iom361_set_switches(patterns[i]);
        reg_value = iom361_readReg(io_base, SWITCHES_REG, NULL);
        iom361_writeReg(io_base, LEDS_REG, reg_value, NULL);
    }

    // set the duty cycles of the RGB LED
    printf("\nDemo 3a: RGB LED enabled, red 67, green 73, blue 99\n");
    iom361_writeReg(io_base, RGB_LED_REG, build_rgb_reg(1, 67, 73, 99), &rtn_code);
    if (rtn_code != 0)
        printf("ERROR(main): Could not write RGB LED register\n");
    printf("\nDemo 3b: RGB LED disabled, red 23, green 1, blue 47\n");
    iom361_writeReg(io_base, RGB_LED_REG, build_rgb_reg(0, 23, 1, 47), NULL);

    // turn the display off: writes still change the registers, but print nothing
    printf("\nDemo 3c: Display off, then write the LEDs (nothing is printed)\n");
    iom361_set_display(false);
    iom361_writeReg(io_base, LEDS_REG, 0xFFFF, NULL);
    printf("\tLED register now holds %04X\n", (unsigned) iom361_readReg(io_base, LEDS_REG, NULL));
    iom361_set_display(true);

    // set a new temperature and humidity and display it
    printf("\nDemo 4: Set the sensor to 10.0 C and 63.0 %%RH\n");
    _iom361_set_sensor1(10.0f, 63.0f);
    show_sensor(io_base);

    printf("\nDemo 5: Two random readings (seeded from the clock)\n");
    iom361_seed((unsigned) time(NULL));
    for (int i = 0; i < 2; i++) {
        _iom361_set_sensor1_rndm(TEMP_RANGE_LOW, TEMP_RANGE_HI, HUMID_RANGE_LOW, HUMID_RANGE_HI);
        show_sensor(io_base);
    }

    printf("\niom361 demo is complete\n");
    return 0;
}
