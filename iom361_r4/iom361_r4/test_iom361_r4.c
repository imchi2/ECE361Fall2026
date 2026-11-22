/**
 * @file    test_iom361_r4.c
 * @brief   Self-checking tests for the ECE 361 I/O module emulator, version 4.0
 * @author  Christof Teuscher (teuscher@pdx.edu)
 * @date    05-Oct-2026
 * @version 4.0
 *
 * @details
 * Every check prints PASS or FAIL. The program ends with the number of checks
 * that passed and failed, and returns 1 if any failed, so that make test fails.
 * Run it with -fsanitize=address,undefined (make test does) to also catch
 * undefined behavior inside the module.
 */

#define _POSIX_C_SOURCE 200809L     // for dup(), dup2(), fileno() with -std=c11

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "iom361_r4.h"

static int passed = 0, failed = 0;

static void check(int ok, const char* what) {
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (ok)
        passed++;
    else
        failed++;
}

static double temp_of(uint32_t raw) {
    return raw / 1048576.0 * 200.0 - 50.0;
}

static double humid_of(uint32_t raw) {
    return raw / 1048576.0 * 100.0;
}

/* runs f() with standard output sent to a temporary file; returns the number of bytes printed */
static long output_of(void (*f)(uint32_t*), uint32_t* base) {
    FILE* tmp = tmpfile();
    if (tmp == NULL)
        return -1;
    fflush(stdout);
    int saved = dup(fileno(stdout));
    dup2(fileno(tmp), fileno(stdout));
    f(base);
    fflush(stdout);
    dup2(saved, fileno(stdout));
    close(saved);
    long n = ftell(tmp);
    fseek(tmp, 0, SEEK_END);
    n = ftell(tmp);
    fclose(tmp);
    return n;
}

static void write_leds_and_rgb(uint32_t* base) {
    iom361_writeReg(base, LEDS_REG, 0x5, NULL);
    iom361_writeReg(base, RGB_LED_REG, build_rgb_reg(1, 10, 20, 30), NULL);
}

static void init_quietly(uint32_t* base) {
    (void) base;
    iom361_initialize(4, 4, NULL);
}

int main(void) {
    int rc = -1;
    uint32_t v;

    printf("ECE 361 - iom361 version 4.0 self-checking tests\n\n");

    // ---- before initialization
    _iom361_set_switches(0x1);                  // version 3.0 crashed here
    check(1, "test functions before iom361_initialize() do not crash");
    v = iom361_readReg(NULL, SWITCHES_REG, &rc);
    check(rc == 5 && v == 0xDEADBEEFu, "readReg before iom361_initialize() returns code 5");

    // ---- initialization
    check(iom361_initialize(33, 4, &rc) == NULL && rc == 1, "33 switches is rejected with code 1");
    check(iom361_initialize(4, -1, &rc) == NULL && rc == 1, "-1 LEDs is rejected with code 1");
    check(output_of(init_quietly, NULL) == 0, "iom361_initialize() prints nothing");
    uint32_t* base = iom361_initialize(4, 8, &rc);
    check(base != NULL && rc == 0, "iom361_initialize(4, 8) succeeds");

    check(iom361_readReg(base, SWITCHES_REG, &rc) == 0 && rc == 0, "switches start at 0");
    check(iom361_readReg(base, LEDS_REG, NULL) == 0, "LEDs start at 0");
    check(iom361_readReg(base, RGB_LED_REG, NULL) == 0, "RGB LED starts at 0");
    check(iom361_readReg(base, RSVD1_REG, NULL) == 0x11111111u, "reserved 1 starts at 0x11111111");
    check(iom361_readReg(base, RSVD3_REG, NULL) == 0x33333333u, "reserved 3 starts at 0x33333333");
    check(fabs(temp_of(iom361_readReg(base, TEMP_REG, NULL)) - 23.5) < 1e-4, "sensor starts at 23.5 C");
    check(fabs(humid_of(iom361_readReg(base, HUMID_REG, NULL)) - 75.0) < 1e-4, "sensor starts at 75 %RH");

    // ---- argument checks
    v = iom361_readReg(base + 1, SWITCHES_REG, &rc);
    check(rc == 1 && v == 0xDEADBEEFu, "a wrong base returns code 1");
    v = iom361_readReg(base, 0x20, &rc);
    check(rc == 2 && v == 0xDEADBEEFu, "readReg at offset 0x20 returns code 2");
    v = iom361_readReg(base, 0x01, &rc);
    check(rc == 3 && v == 0xDEADBEEFu, "readReg at offset 1 returns code 3");
    v = iom361_writeReg(base, 0x22, 0, &rc);
    check(rc == 2, "writeReg at offset 0x22 returns code 2");
    v = iom361_writeReg(base, 0x06, 0, &rc);
    check(rc == 3, "writeReg at offset 6 returns code 3");

    // ---- switches
    _iom361_set_switches(0xA5);
    check(iom361_readReg(base, SWITCHES_REG, NULL) == 0x5, "with 4 switches, 0xA5 reads as 0x5");
    v = iom361_writeReg(base, SWITCHES_REG, 0xF, &rc);
    check(rc == 0 && v == 0x5 && iom361_readReg(base, SWITCHES_REG, NULL) == 0x5,
          "writing the switch register is ignored and returns its contents");
    iom361_initialize(32, 32, NULL);
    _iom361_set_switches(0xFFFFFFFFu);
    check(iom361_readReg(base, SWITCHES_REG, NULL) == 0xFFFFFFFFu, "with 32 switches, all 32 bits read back");

    // ---- LEDs and RGB LED
    iom361_set_display(false);
    v = iom361_writeReg(base, LEDS_REG, 0x80000001u, &rc);
    check(rc == 0 && v == 0x80000001u && iom361_readReg(base, LEDS_REG, NULL) == 0x80000001u,
          "LED register: write, return value, and read back agree");
    check(build_rgb_reg(1, 255, 0, 0) == 0x80FF0000u, "build_rgb_reg(1, 255, 0, 0) == 0x80FF0000");
    check(build_rgb_reg(0, 23, 1, 47) == 0x0017012Fu, "build_rgb_reg(0, 23, 1, 47) == 0x0017012F");
    check(build_rgb_reg(3, 0, 0, 0) == 0x80000000u, "build_rgb_reg uses only bit 0 of enable");
    iom361_writeReg(base, RSVD2_REG, 0xCAFEF00Du, NULL);
    check(iom361_readReg(base, RSVD2_REG, NULL) == 0xCAFEF00Du, "a reserved register can be written and read");

    // ---- display on and off
    check(output_of(write_leds_and_rgb, base) == 0, "with the display off, LED and RGB writes print nothing");
    iom361_set_display(true);
    check(output_of(write_leds_and_rgb, base) > 0, "with the display on, LED and RGB writes print");
    iom361_set_display(false);

    // ---- sensor round trip: every value comes back within 0.0001
    const float temps[] = { -40.0f, 0.0f, 0.1f, 2.0f, 8.0f, 23.5f, 37.5f, 91.0f, 92.0f, 98.0f, 100.0f, 125.0f };
    int ok = 1;
    for (size_t i = 0; i < sizeof temps / sizeof temps[0]; i++) {
        _iom361_set_sensor1(temps[i], 50.0f);
        ok = ok && fabs(temp_of(iom361_readReg(base, TEMP_REG, NULL)) - temps[i]) < 1e-4;
    }
    check(ok, "temperature from -40 to 125 C reads back within 0.0001 C");
    const float humids[] = { 0.0f, 0.5f, 45.0f, 68.0f, 85.0f, 99.0f };
    ok = 1;
    for (size_t i = 0; i < sizeof humids / sizeof humids[0]; i++) {
        _iom361_set_sensor1(20.0f, humids[i]);
        ok = ok && fabs(humid_of(iom361_readReg(base, HUMID_REG, NULL)) - humids[i]) < 1e-4;
    }
    check(ok, "humidity from 0 to 99 %RH reads back within 0.0001 %RH");

    // ---- sensor clamping
    _iom361_set_sensor1(-60.0f, -5.0f);         // version 3.0: undefined behavior
    check(iom361_readReg(base, TEMP_REG, NULL) == 0 && iom361_readReg(base, HUMID_REG, NULL) == 0,
          "values below the range clamp to 0");
    _iom361_set_sensor1(200.0f, 120.0f);        // version 3.0: humidity overflowed 20 bits
    check(iom361_readReg(base, TEMP_REG, NULL) == 0xFFFFFu && iom361_readReg(base, HUMID_REG, NULL) == 0xFFFFFu,
          "values above the range clamp to 0xFFFFF");
    _iom361_set_sensor1(NAN, NAN);
    check(iom361_readReg(base, TEMP_REG, NULL) == 0 && iom361_readReg(base, HUMID_REG, NULL) == 0,
          "NaN reads as 0");

    // ---- random readings
    iom361_seed(361);
    _iom361_set_sensor1_rndm(42.0f, 52.0f, 87.3f, 72.6f);
    uint32_t t1 = iom361_readReg(base, TEMP_REG, NULL), h1 = iom361_readReg(base, HUMID_REG, NULL);
    check(temp_of(t1) >= 42.0 - 1e-4 && temp_of(t1) <= 52.0 + 1e-4 &&
          humid_of(h1) >= 72.6 - 1e-4 && humid_of(h1) <= 87.3 + 1e-4,
          "random readings fall in range, with the bounds in either order");
    iom361_seed(361);
    _iom361_set_sensor1_rndm(42.0f, 52.0f, 87.3f, 72.6f);
    check(iom361_readReg(base, TEMP_REG, NULL) == t1 && iom361_readReg(base, HUMID_REG, NULL) == h1,
          "the same seed gives the same random readings");

    // ---- re-initialization resets everything
    iom361_initialize(4, 4, NULL);
    check(iom361_readReg(base, LEDS_REG, NULL) == 0 && iom361_readReg(base, RSVD2_REG, NULL) == 0x22222222u,
          "iom361_initialize() again resets the registers");

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
