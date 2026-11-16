/*
 * test_istack.c - tests for the istack ADT.
 *
 * Build and run:  make test
 *
 * Every check prints PASS or FAIL, then a summary line, and the program
 * exits nonzero if anything failed, so make can tell the difference.
 *
 * The tests cover, at least:
 *   - pop and peek on an empty stack leave *v alone
 *   - last in, first out
 *   - full at 8, and the 9th push doubles the room to 16
 *   - 1000 pushes and pops
 *   - istack_to_array is bottom first and a copy
 *   - NULL arguments
 *   - two stacks are independent
 *   - destroying a stack that is not empty
 */
#include <stdio.h>
#include <stdlib.h>

#include "istack.h"

/* Counts, so the summary and the exit status come from one place. */
static int checks = 0;
static int failures = 0;

/*
 * check - report one test result.
 * Takes: ok, nonzero when the test passed; name, what was tested.
 * Returns: nothing, but remembers failures for the summary.
 */
static void check(int ok, const char *name)
{
    checks = checks + 1;
    if (ok) {
        printf("PASS: %s\n", name);
    } else {
        failures = failures + 1;
        printf("FAIL: %s\n", name);
    }
}

/*
 * test_empty_pop_peek - pop and peek on an empty stack leave *v alone.
 *
 * v starts with a value that is not on any stack; a correct pop or peek
 * fails with -1 and must not write to v.
 */
static void test_empty_pop_peek(void)
{
    istack_t *s;
    int v;
    int pop_result;
    int peek_result;

    s = istack_create();
    if (s == NULL) {
        check(0, "empty pop and peek leave *v alone (create failed)");
        return;
    }

    v = 12345;
    pop_result = istack_pop(s, &v);
    peek_result = istack_peek(s, &v);

    check(pop_result == -1
          && peek_result == -1
          && v == 12345
          && istack_is_empty(s),
          "pop and peek on an empty stack leave *v alone");

    istack_destroy(s);
}

/*
 * test_lifo - last in, first out.
 *
 * Push 1, 2, 3 and take them out again: they must come back 3, 2, 1.
 */
static void test_lifo(void)
{
    istack_t *s;
    int a;
    int b;
    int c;
    int ok;

    s = istack_create();
    if (s == NULL) {
        check(0, "lifo (create failed)");
        return;
    }

    ok = 1;
    ok = ok && istack_push(s, 1) == 0;
    ok = ok && istack_push(s, 2) == 0;
    ok = ok && istack_push(s, 3) == 0;

    a = 0;
    b = 0;
    c = 0;
    ok = ok && istack_pop(s, &a) == 0;
    ok = ok && istack_pop(s, &b) == 0;
    ok = ok && istack_pop(s, &c) == 0;

    /* 3 came out first, then 2, then 1: reverse order is the point. */
    check(ok && a == 3 && b == 2 && c == 1 && istack_is_empty(s),
          "last in, first out");

    istack_destroy(s);
}

/*
 * test_grow_to_16 - full at 8, the 9th push doubles the room to 16.
 *
 * The capacity is watched before and after, and every value is checked
 * afterwards so a bad realloc would show up as a missing value.
 */
static void test_grow_to_16(void)
{
    istack_t *s;
    int i;
    int ok;
    int still_there;

    s = istack_create();
    if (s == NULL) {
        check(0, "grow to 16 (create failed)");
        return;
    }

    /* The room starts at ISTACK_START, which is 8. */
    ok = istack_capacity(s) == ISTACK_START;

    /* Fill it exactly: still 8 values, still room 8. */
    i = 0;
    while (i < 8) {
        ok = ok && istack_push(s, i * 10) == 0;
        i = i + 1;
    }
    ok = ok && istack_capacity(s) == 8 && istack_count(s) == 8;

    /* The 9th push must double the room before it stores anything. */
    ok = ok && istack_push(s, 90) == 0;
    ok = ok && istack_capacity(s) == 16 && istack_count(s) == 9;
    check(ok, "the 9th push doubles the room from 8 to 16");

    /* Every value must have survived the realloc, in order: 90 came
     * out last, so it comes out first, then 70 down to 0. */
    still_there = 1;
    {
        int v;

        v = -1;
        if (istack_pop(s, &v) != 0) {
            still_there = 0;
        } else if (v != 90) {
            still_there = 0;
        }
    }
    i = 7;
    while (i >= 0) {
        int v;

        v = -1;
        if (istack_pop(s, &v) != 0) {
            still_there = 0;
        } else if (v != i * 10) {
            still_there = 0;
        }
        i = i - 1;
    }
    check(still_there && istack_is_empty(s),
          "every value is kept when the room doubles");

    istack_destroy(s);
}

/*
 * test_thousand - 1000 pushes, then 1000 pops.
 *
 * Big enough to double the room several times (8, 16, ... 1024), so it
 * exercises realloc again and again.
 */
static void test_thousand(void)
{
    istack_t *s;
    int i;
    int ok;
    int matched;

    s = istack_create();
    if (s == NULL) {
        check(0, "1000 pushes and pops (create failed)");
        return;
    }

    /* Push 0 up to 999. */
    ok = 1;
    i = 0;
    while (i < 1000) {
        ok = ok && istack_push(s, i) == 0;
        i = i + 1;
    }
    ok = ok && istack_count(s) == 1000;

    /* Pop them: they must come back in reverse, 999 down to 0. */
    matched = 1;
    i = 999;
    while (i >= 0) {
        int v;

        v = -1;
        if (istack_pop(s, &v) != 0) {
            matched = 0;
        } else if (v != i) {
            matched = 0;
        }
        i = i - 1;
    }

    check(ok && matched && istack_is_empty(s),
          "1000 pushes and 1000 pops keep every value");

    istack_destroy(s);
}

/*
 * test_to_array - istack_to_array is bottom first and a copy.
 *
 * After copying, the array is written to; the stack must not notice.
 */
static void test_to_array(void)
{
    istack_t *s;
    int *arr;
    size_t n;
    int ok;
    int after;

    s = istack_create();
    if (s == NULL) {
        check(0, "to_array (create failed)");
        return;
    }

    /* Bottom is 100, then 200, then 300 on top. */
    istack_push(s, 100);
    istack_push(s, 200);
    istack_push(s, 300);

    n = 999;
    arr = istack_to_array(s, &n);

    ok = arr != NULL && n == 3;
    if (arr != NULL && n == 3) {
        /* Bottom first: arr[0] is the first value pushed. */
        ok = ok && arr[0] == 100 && arr[1] == 200 && arr[2] == 300;

        /* It is a copy: changing it leaves the stack alone. */
        arr[0] = -1;
        after = -1;
        ok = ok && istack_pop(s, &after) == 0 && after == 300;
    }
    check(ok, "istack_to_array is bottom first and a copy");

    free(arr);
    istack_destroy(s);

    /* An empty stack gives NULL and *n = 0. */
    s = istack_create();
    if (s == NULL) {
        check(0, "to_array on an empty stack (create failed)");
        return;
    }
    n = 999;
    arr = istack_to_array(s, &n);
    check(arr == NULL && n == 0, "to_array on an empty stack gives NULL and n = 0");
    free(arr);              /* free(NULL) does nothing, and is safe */
    istack_destroy(s);
}

/*
 * test_null_arguments - NULL is handled by every function.
 *
 * Nothing here may crash; the results are the ones istack.h promises.
 */
static void test_null_arguments(void)
{
    int v;
    size_t n;
    int *arr;
    int ok;

    v = 77;
    ok = 1;

    /* push, pop, peek all report -1 and leave v alone. */
    ok = ok && istack_push(NULL, 1) == -1;
    ok = ok && istack_pop(NULL, &v) == -1;
    ok = ok && istack_peek(NULL, &v) == -1;
    ok = ok && v == 77;

    /* count, is_empty, capacity give 0, true, 0. */
    ok = ok && istack_count(NULL) == 0;
    ok = ok && istack_is_empty(NULL);
    ok = ok && istack_capacity(NULL) == 0;

    /* to_array gives NULL and *n = 0. */
    n = 999;
    arr = istack_to_array(NULL, &n);
    ok = ok && arr == NULL && n == 0;
    free(arr);

    /* destroy(NULL) must do nothing rather than crash. */
    istack_destroy(NULL);

    check(ok, "NULL arguments are safe everywhere");
}

/*
 * test_two_stacks - two stacks are independent.
 *
 * Values pushed on one must not appear on the other, and growing one
 * must not touch the other.
 */
static void test_two_stacks(void)
{
    istack_t *a;
    istack_t *b;
    int v;
    int ok;

    a = istack_create();
    b = istack_create();
    if (a == NULL || b == NULL) {
        check(0, "two stacks are independent (create failed)");
        istack_destroy(a);
        istack_destroy(b);
        return;
    }

    ok = 1;

    /* Give each stack a different life: a grows past its first room. */
    ok = ok && istack_push(a, 11) == 0;
    ok = ok && istack_push(a, 12) == 0;
    ok = ok && istack_push(b, 99) == 0;

    /* Drain a completely; b must still hold 99. */
    v = -1;
    ok = ok && istack_pop(a, &v) == 0 && v == 12;
    v = -1;
    ok = ok && istack_pop(a, &v) == 0 && v == 11;
    ok = ok && istack_is_empty(a);
    ok = ok && istack_count(b) == 1;

    /* b still hands out its own value. */
    v = -1;
    ok = ok && istack_pop(b, &v) == 0 && v == 99;
    ok = ok && istack_is_empty(b);

    check(ok, "two stacks are independent");

    istack_destroy(a);
    istack_destroy(b);
}

/*
 * test_destroy_nonempty - destroying a stack that is not empty.
 *
 * There is no visible answer to check, so this test mainly gives the
 * sanitizer something to catch: a leak or a bad free would show up
 * under make memcheck, and a crash would fail the run.
 */
static void test_destroy_nonempty(void)
{
    istack_t *s;
    int ok;

    s = istack_create();
    if (s == NULL) {
        check(0, "destroy a stack that is not empty (create failed)");
        return;
    }

    /* Fill it past its first room so two blocks must be freed. */
    ok = 1;
    ok = ok && istack_push(s, 1) == 0;
    ok = ok && istack_push(s, 2) == 0;
    ok = ok && istack_push(s, 3) == 0;

    /* The destroy itself is the test: it must not crash or leak. */
    istack_destroy(s);

    check(ok, "destroying a stack that is not empty");
}

int main(void)
{
    test_empty_pop_peek();
    test_lifo();
    test_grow_to_16();
    test_thousand();
    test_to_array();
    test_null_arguments();
    test_two_stacks();
    test_destroy_nonempty();

    printf("%d checks, %d failed\n", checks, failures);
    if (failures > 0) {
        return 1;
    }
    return 0;
}
