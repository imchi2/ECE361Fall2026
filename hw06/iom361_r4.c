/**
 * @file    iom361_r4.c
 * @brief   Source file for the ECE 361 I/O module emulator
 * @author  Roy Kravitz (roy.kravitz@pdx.edu), versions 1.0 to 3.0
 * @author  Christof Teuscher (teuscher@pdx.edu), version 4.0
 * @date    05-Oct-2026
 * @version 4.0
 *
 * @details
 * This is the source code for the ECE 361 I/O module emulation. The I/O module
 * emulates a memory-mapped I/O system with a number of "typical" peripheral
 * registers. The registers are an array of uint32_t, which models memory-mapped
 * I/O registers more accurately than a struct.
 *
 * Roy Kravitz wrote the original module (versions 1.0 to 3.0) for ECE 361.
 * Version 4.0 keeps his API; see iom361_r4.h for the revision history and
 * README.md for the changes.
 */

#include <stdlib.h>
#include <stdio.h>

#include "iom361_r4.h"

// constants
//#define _DEBUG_ 1

#define REG_INDEX(offset)   ((offset) / sizeof(uint32_t))
#define SENSOR_FULL_SCALE   1048576.0           // 2**20, the AHT20 full scale
#define SENSOR_MAX_RAW      0x000FFFFFu         // largest 20-bit value
#define ERROR_VALUE         0xDEADBEEFu         // value returned on error

// global variables
static uint32_t io_space[sizeof(ioreg_t) / sizeof(uint32_t)];
static uint32_t* const io_space_ptr = io_space;  // valid even before initialization
static int nsw;                         // number of switches
static int nleds;                       // number of LEDs
static bool is_initialized = false;     // true once iom361_initialize() has succeeded
static bool display_on = true;          // print writes to the LED and RGB LED registers

// Helper function prototypes
static int check_access(const uint32_t* base, uint32_t offset);
static uint32_t switch_mask(void);
static uint32_t sensor_raw(double value, double offset, double span);
static double random_in_range(double a, double b);
static void display_leds(uint32_t value, int num_leds);
static void display_rgb_leds(uint32_t value);

// API functions

/* iom361_initialize() */
uint32_t* iom361_initialize(int num_switches, int num_leds, int* rtn_code) {
    if (num_switches < 0 || num_switches > 32 || num_leds < 0 || num_leds > 32) {
        if (rtn_code != NULL)
            *rtn_code = 1;
        return NULL;
    }

    // initialize global variables
    nsw = num_switches;
    nleds = num_leds;

    #ifdef _DEBUG_
        printf("INFO [iom361_initialize()]: io_space_ptr=%p, io_space length=%zu\n",
            (void*) io_space_ptr, sizeof(io_space));
    #endif

    // initialize the I/O registers directly: initialization prints nothing
    io_space[REG_INDEX(LEDS_REG)] = 0x00000000;
    io_space[REG_INDEX(RGB_LED_REG)] = 0x00000000;
    io_space[REG_INDEX(RSVD1_REG)] = 0x11111111;
    io_space[REG_INDEX(RSVD2_REG)] = 0x22222222;
    io_space[REG_INDEX(RSVD3_REG)] = 0x33333333;

    is_initialized = true;
    _iom361_set_switches(0x00000000);
    _iom361_set_sensor1(23.5f, 75.0f);

    if (rtn_code != NULL)
        *rtn_code = 0;
    return io_space_ptr;
}

/* iom361_readReg() */
uint32_t iom361_readReg(uint32_t* base, uint32_t offset, int* rtn_code) {
    int rc = check_access(base, offset);

    if (rtn_code != NULL)
        *rtn_code = rc;
    if (rc != 0)
        return ERROR_VALUE;

    uint32_t value = io_space[REG_INDEX(offset)];

    #ifdef _DEBUG_
        printf("INFO [iom361_readReg()]: offset=0x%02X, value=%08X\n", (unsigned) offset, (unsigned) value);
    #endif

    return value;
}

/* iom361_writeReg() */
uint32_t iom361_writeReg(uint32_t* base, uint32_t offset, uint32_t value, int* rtn_code) {
    int rc = check_access(base, offset);

    if (rtn_code != NULL)
        *rtn_code = rc;
    if (rc != 0)
        return ERROR_VALUE;

    #ifdef _DEBUG_
        printf("INFO [iom361_writeReg()]: offset=0x%02X, value=%08X\n", (unsigned) offset, (unsigned) value);
    #endif

    // check_access() guarantees that offset is one of the eight registers
    switch (offset) {
        case SWITCHES_REG:  break;      // switches are a read-only input
        case TEMP_REG:      break;      // temperature is a read-only input
        case HUMID_REG:     break;      // humidity is a read-only input

        case LEDS_REG:      io_space[REG_INDEX(offset)] = value;
                            if (display_on)
                                display_leds(value, nleds);
                            break;

        case RGB_LED_REG:   io_space[REG_INDEX(offset)] = value;
                            if (display_on)
                                display_rgb_leds(value);
                            break;

        default:            io_space[REG_INDEX(offset)] = value;    // reserved registers
                            break;
    }
    return io_space[REG_INDEX(offset)];
}

/* build_rgb_reg() */
uint32_t build_rgb_reg(uint8_t enable, uint8_t red_dc, uint8_t green_dc, uint8_t blue_dc) {
    // shift unsigned values: (uint32_t) 1 << 31 is defined, 1 << 31 on an int is not
    return ((uint32_t) (enable & 0x01u) << 31)
         | ((uint32_t) red_dc << 16)
         | ((uint32_t) green_dc << 8)
         | ((uint32_t) blue_dc << 0);
}

/* iom361_set_display() */
void iom361_set_display(bool on) {
    display_on = on;
}

/* iom361_seed() */
void iom361_seed(unsigned int seed) {
    srand(seed);
}


// Functions used for testing - set register values for read-only registers

/* _iom361_set_switches() */
void _iom361_set_switches(uint32_t value) {
    io_space[REG_INDEX(SWITCHES_REG)] = value & switch_mask();
}

/* _iom361_set_sensor1() */
void _iom361_set_sensor1(float new_temp, float new_humid) {
    // per AHT20 data sheet, Temp(C) = (ST/2**20) * 200 - 50,
    // so ST = (2**20/200) * (Temp(C) + 50)
    io_space[REG_INDEX(TEMP_REG)] = sensor_raw(new_temp, 50.0, 200.0);

    // per AHT20 data sheet, RH(%) = (SRH/2**20) * 100,
    // so SRH = (2**20/100) * RH(%)
    io_space[REG_INDEX(HUMID_REG)] = sensor_raw(new_humid, 0.0, 100.0);
}

/* _iom361_set_sensor1_rndm() */
void _iom361_set_sensor1_rndm(float temp_low, float temp_hi,
    float humid_low, float humid_hi) {
    float new_temp = (float) random_in_range(temp_low, temp_hi);
    float new_humid = (float) random_in_range(humid_low, humid_hi);
    _iom361_set_sensor1(new_temp, new_humid);
}


// Helper Functions

/**
 * check_access() - checks the arguments of a register read or write
 *
 * @return  0 if base and offset name a register, otherwise the return code
 */
static int check_access(const uint32_t* base, uint32_t offset) {
    if (!is_initialized)
        return 5;
    if (base != io_space_ptr)
        return 1;
    if (offset > sizeof(io_space) - sizeof(uint32_t))
        return 2;
    if (offset % sizeof(uint32_t) != 0)
        return 3;
    return 0;
}

/**
 * switch_mask() - the bits of the switch register that are switches
 */
static uint32_t switch_mask(void) {
    return (nsw >= 32) ? 0xFFFFFFFFu : (((uint32_t) 1 << nsw) - 1u);
}

/**
 * sensor_raw() - converts a sensor value to its 20-bit register value
 *
 * raw = (value + offset) / span * 2**20, rounded to the nearest integer and
 * clamped to 0 .. 2**20 - 1. Version 3.0 truncated instead of rounding, so a
 * value read back up to one step (0.0002 degrees C) low, and it did not clamp,
 * so a value below the range was undefined behavior.
 */
static uint32_t sensor_raw(double value, double offset, double span) {
    double raw = (value + offset) / span * SENSOR_FULL_SCALE + 0.5;

    if (!(raw >= 0.0))                  // also catches NaN
        return 0;
    if (raw >= (double) SENSOR_MAX_RAW)
        return SENSOR_MAX_RAW;
    return (uint32_t) raw;
}

/**
 * random_in_range() - a random number between a and b, in either order
 *
 * Replaces float_rndm.c from version 3.0.
 */
static double random_in_range(double a, double b) {
    return a + (b - a) * ((double) rand() / RAND_MAX);
}

/**
 * display_leds() - displays the LED register
 *
 * Displays the LED values in a readable format: 'o' for a lit LED, '_' for a
 * dark one, most significant LED first, in groups of four.
 *
 * @param   value is the leds to display
 * @param   num_leds is the number of LEDs to display, 0 to 32
 */
static void display_leds(uint32_t value, int num_leds) {
    for (int i = num_leds - 1; i >= 0; i--) {
        if ((num_leds - 1 - i) % 4 == 0) {
            putchar(' ');
            putchar(' ');
        }
        putchar((value >> i) & 1u ? 'o' : '_');
    }
    putchar('\n');
}

/**
 * display_rgb_leds() - displays the duty cycles for an RGB LED
 *
 * Displays the red, green, and blue duty cycles and the enable bit from the
 * RGB LED I/O register (see the register format in iom361_r4.h).
 *
 * @param   value is the RGB control register
 */
static void display_rgb_leds(uint32_t value) {
    unsigned blue_dc  = (value >> 0) & 0xFFu;
    unsigned green_dc = (value >> 8) & 0xFFu;
    unsigned red_dc   = (value >> 16) & 0xFFu;
    unsigned enable   = (value >> 31) & 0x01u;

    printf("RedDC=%2u%% (%3u), GrnDC=%2u%% (%3u), BluDC=%2u%% (%3u)\tEnable=%s\n",
        red_dc * 100u / 255u, red_dc,
        green_dc * 100u / 255u, green_dc,
        blue_dc * 100u / 255u, blue_dc,
        enable ? "ON" : "OFF");
}
