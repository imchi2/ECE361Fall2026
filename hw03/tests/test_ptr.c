/*
 * tests/test_ptr.c: ECE 361, Fall 2026, HW3 - the test program (Part 2).
 *
 * How to read the output:
 *   - every check prints one line: PASS, or FAIL with what was expected;
 *   - a summary at the end counts the checks and the failures;
 *   - the program returns 0 only when everything passed, so `make test`
 *     stops with an error the moment one check fails.
 *
 * The one rule this program does NOT follow is the pointer-arithmetic
 * rule: that rule belongs to ptr.c, so the tests are free to write
 * a[0], a[2] ... while they read the answers back.
 *
 * Floats are compared with == on purpose. Every expected number here is
 * a constant the test itself wrote (or a copy of one), never the result
 * of arithmetic, so the bits are identical and == is exact.
 *
 * Build and run, from hw03/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw03 && make test"
 */

#include "ptr.h"

#include <stddef.h> /* NULL */

#include <stdio.h> /* printf */

/* How many checks have run, and how many of them printed FAIL. The
   summary and the exit code are built from these two numbers. */
static int tests = 0;
static int failures = 0;

/* One line per test. `ok` is any integer expression: a real 1 or 0, or
   the result of a comparison such as (mn == -1.0f && r == 0). */
static void check(int ok, const char *what) { tests += 1; failures += ok ? 0 : 1; printf("%s  %s\n", ok ? "PASS" : "FAIL", what); }

int main(void)
{
    /* ---- swap_float ---- */
    float x = 1.5f, y = -2.25f;
    float mn, mx;                       /* outputs for minmax */
    float arr[5] = {3.5f, -1.0f, 7.25f, 0.0f, 2.5f};
    float hits[4] = {0.5f, 9.5f, 9.0f, 1.0f};
    float ties[3] = {2.0f, 2.0f, 5.0f};
    float even[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float odd[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    int r;                              /* the return value of minmax */

    printf("hw03 test program\n\n");

    /* One call, and the two caller variables must have swapped. */
    swap_float(&x, &y);
    check(x == -2.25f && y == 1.5f, "swap_float exchanges *a and *b");

    /* ---- minmax: several values, then one value ---- */
    printf("\n---- minmax ----\n");
    mn = -999.0f; mx = -999.0f;
    r = minmax(arr, 5, &mn, &mx);
    check(r == 0 && mn == -1.0f && mx == 7.25f, "minmax(5 values) returns 0, min -1, max 7.25");

    r = minmax(arr, 1, &mn, &mx);       /* only arr[0] = 3.5 is looked at */
    check(r == 0 && mn == 3.5f && mx == 3.5f, "minmax(one value) returns 0 and stores it twice");

    /* ---- minmax: the rejections. Both outputs start as sentinels, so
       any write at all is visible as a changed sentinel. ---- */
    mn = -999.0f; mx = 999.0f;
    r = minmax(arr, 0, &mn, &mx);
    check(r == -1 && mn == -999.0f && mx == 999.0f, "minmax(n = 0) returns -1, outputs unchanged");

    mn = -999.0f; mx = 999.0f;
    r = minmax(NULL, 5, &mn, &mx);
    check(r == -1 && mn == -999.0f && mx == 999.0f, "minmax(a = NULL) returns -1, outputs unchanged");

    mn = -999.0f; mx = 999.0f;
    r = minmax(arr, 5, NULL, &mx);
    check(r == -1 && mx == 999.0f, "minmax(min = NULL) returns -1, *max unchanged");

    mn = -999.0f; mx = 999.0f;
    r = minmax(arr, 5, &mn, NULL);
    check(r == -1 && mn == -999.0f, "minmax(max = NULL) returns -1, *min unchanged");

    /* ---- find_above ---- */
    printf("\n---- find_above ----\n");
    check(find_above(hits, 4, 5.0f) == &hits[1], "find_above(two matches) returns the first, &hits[1]");

    check(find_above(ties, 3, 2.0f) == &ties[2], "find_above(2.0) skips the values equal to the threshold");

    check(find_above(ties, 3, 5.0f) == NULL, "find_above(nothing greater) returns NULL");

    check(find_above(NULL, 3, 1.0f) == NULL, "find_above(a = NULL) returns NULL");

    check(find_above(ties, 0, 1.0f) == NULL, "find_above(n = 0) returns NULL");

    /* ---- reverse ---- */
    printf("\n---- reverse ----\n");
    reverse(even, 4);
    check(even[0] == 4.0f && even[1] == 3.0f && even[2] == 2.0f && even[3] == 1.0f, "reverse(even, 4) gives 4 3 2 1");

    reverse(odd, 5);
    check(odd[0] == 5.0f && odd[1] == 4.0f && odd[2] == 3.0f && odd[3] == 2.0f && odd[4] == 1.0f, "reverse(odd, 5) gives 5 4 3 2 1");

    /* n < 2 and a missing array: if these did anything, or crashed, the
       lines below would never be printed - so printing them is the check. */
    reverse(even, 1);
    check(even[0] == 4.0f && even[1] == 3.0f, "reverse(n = 1) does nothing");

    reverse(even, 0);
    check(even[0] == 4.0f && even[3] == 1.0f, "reverse(n = 0) does nothing");

    reverse(NULL, 4);
    check(1, "reverse(a = NULL) returns without touching anything");

    /* ---- summary: the exit code is what `make test` looks at ---- */
    printf("\n---- summary ----\n");
    printf("%d test(s), %d failure(s)\n", tests, failures);

    if (failures == 0) { printf("ALL TESTS PASSED\n"); return 0; }

    printf("SOME TESTS FAILED\n");
    return 1;
}
