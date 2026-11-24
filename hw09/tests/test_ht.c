/*
 * test_ht.c - tests for the ht counting hash table.
 *
 * Build and run:  make test
 *
 * Every check prints PASS or FAIL, then a summary line, and the
 * program exits nonzero if anything failed, so make can tell the
 * difference.
 *
 * The tests cover, at least (the list the spec asks for):
 *   - 0 buckets gives NULL
 *   - an empty table: size 0, longest chain 0, every count 0
 *   - one key added three times: count 3, size 1
 *   - two keys in the same bucket (5 and 15 with 10 buckets), both
 *     counted right, and 25, never added, counted 0
 *   - 1 to 1000 each added once: size 1000, every count 1
 *   - the four rows of Table 2 (longest chain)
 *   - NULL arguments
 *   - destroying full tables
 * plus:
 *   - the Shine warm-up keys 12 5 22 5 with 10 buckets
 *   - many tables in a row, so make memcheck can look for leaks
 */
#include <stdio.h>

#include "ht.h"

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
 * test_zero_buckets - ht_create(0) must refuse.
 *
 * A table with no bucket array could never hold a key, so the spec
 * makes 0 buckets an error instead of an empty table.
 */
static void test_zero_buckets(void)
{
    ht_t *t;

    t = ht_create(0);
    check(t == NULL, "ht_create(0) returns NULL");

    /* If the call wrongly returned a table, still free it. */
    ht_destroy(t);
}

/*
 * test_empty_table - a new table holds nothing.
 *
 * Every question asked of a brand new table has the "nothing there"
 * answer: size 0, no chains, and a count of 0.
 */
static void test_empty_table(void)
{
    ht_t *t;

    t = ht_create(10);
    if (t == NULL) {
        check(0, "ht_create(10) returns a table (it returned NULL)");
        return;
    }

    check(1, "ht_create(10) returns a table");
    check(ht_size(t) == 0, "empty table: size 0");
    check(ht_longest_chain(t) == 0, "empty table: longest chain 0");
    check(ht_count(t, 1) == 0, "empty table: count of key 1 is 0");

    ht_destroy(t);
}

/*
 * test_one_key_three_times - a duplicate raises the count, not the
 * size.
 *
 * The key is stored once with count 3, so the table has one key and
 * one chain node.
 */
static void test_one_key_three_times(void)
{
    ht_t *t;
    int   ok;

    t = ht_create(10);
    if (t == NULL) {
        check(0, "one key three times (create failed)");
        return;
    }

    ok = 1;
    ok = ok && (ht_add(t, 7) == 0);
    ok = ok && (ht_add(t, 7) == 0);
    ok = ok && (ht_add(t, 7) == 0);

    check(ok, "ht_add of key 7 returns 0 three times");
    check(ht_count(t, 7) == 3, "key 7 added three times: count 3");
    check(ht_size(t) == 1, "key 7 added three times: size 1");
    check(ht_longest_chain(t) == 1,
          "key 7 added three times: longest chain 1");

    ht_destroy(t);
}

/*
 * test_same_bucket - 5 and 15 land in the same chain of 10 buckets.
 *
 * 5 % 10 and 15 % 10 are both 5, so both keys walk the same chain.
 * The chain must still tell them apart: 5 was added twice and 15 once.
 * 25 also hashes to bucket 5 but was never added, so its count is 0
 * and it must not appear in the chain.
 */
static void test_same_bucket(void)
{
    ht_t *t;
    int   ok;

    t = ht_create(10);
    if (t == NULL) {
        check(0, "keys 5 and 15 in one bucket (create failed)");
        return;
    }

    ok = 1;
    ok = ok && (ht_add(t, 5) == 0);
    ok = ok && (ht_add(t, 15) == 0);
    ok = ok && (ht_add(t, 5) == 0);

    check(ok, "adding 5, 15, 5 returns 0 each time");
    check(ht_count(t, 5) == 2, "key 5 added twice: count 2");
    check(ht_count(t, 15) == 1, "key 15 added once: count 1");
    check(ht_count(t, 25) == 0, "key 25 never added: count 0");
    check(ht_size(t) == 2, "keys 5 and 15: size 2");
    check(ht_longest_chain(t) == 2,
          "keys 5 and 15 share bucket 5: longest chain 2");

    ht_destroy(t);
}

/*
 * test_warmup_keys - 12 5 22 5 with 10 buckets.
 *
 * This is the picture the Shine warm-up asks for. 12 % 10 = 2 and
 * 22 % 10 = 2, so those two share bucket 2; 5 % 10 = 5, so 5 is alone
 * in bucket 5. The second 5 is a duplicate, so it raises bucket 5's
 * node count to 2 instead of adding a node. New nodes go at the front,
 * so bucket 2 is 22 -> 12 and bucket 5 is 5, which is why counting 32
 * (bucket 2) compares 22, then 12: two nodes.
 */
static void test_warmup_keys(void)
{
    ht_t *t;
    int   ok;

    t = ht_create(10);
    if (t == NULL) {
        check(0, "warm-up keys 12 5 22 5 (create failed)");
        return;
    }

    ok = 1;
    ok = ok && (ht_add(t, 12) == 0);
    ok = ok && (ht_add(t, 5) == 0);
    ok = ok && (ht_add(t, 22) == 0);
    ok = ok && (ht_add(t, 5) == 0);

    check(ok, "adding 12, 5, 22, 5 returns 0 each time");
    check(ht_count(t, 12) == 1, "warm-up: count of 12 is 1");
    check(ht_count(t, 5) == 2, "warm-up: count of 5 is 2");
    check(ht_count(t, 22) == 1, "warm-up: count of 22 is 1");
    check(ht_count(t, 32) == 0, "warm-up: count of 32 is 0");
    check(ht_size(t) == 3, "warm-up: size 3");
    check(ht_longest_chain(t) == 2, "warm-up: longest chain 2");

    ht_destroy(t);
}

/*
 * test_thousand_keys - 1 to 1000, each added once.
 *
 * One table with 1000 buckets holds them all with one node per bucket.
 * Every key is checked, not just a sample: a broken hash that quietly
 * lost a key would show up as a wrong count for that key.
 */
static void test_thousand_keys(void)
{
    ht_t   *t;
    int     key;
    int     ok;
    int     all_once;

    t = ht_create(1000);
    if (t == NULL) {
        check(0, "keys 1 to 1000 (create failed)");
        return;
    }

    ok = 1;
    for (key = 1; key <= 1000; key++) {
        if (ht_add(t, key) != 0) {
            ok = 0;
        }
    }

    check(ok, "adding keys 1 to 1000 returns 0 each time");
    check(ht_size(t) == 1000, "keys 1 to 1000: size 1000");

    all_once = 1;
    for (key = 1; key <= 1000; key++) {
        if (ht_count(t, key) != 1) {
            all_once = 0;
        }
    }

    check(all_once, "keys 1 to 1000: every count is 1");

    ht_destroy(t);
}

/*
 * test_table_row1 - Table 2, row 1.
 * Keys 1, 2, 3, ..., 1000 with 10 buckets: longest chain 100.
 *
 * key % 10 is the last digit, so each digit 0..9 gets the 100 keys
 * that end in it, and no bucket can be longer than that.
 */
static void test_table_row1(void)
{
    ht_t *t;
    int   key;

    t = ht_create(10);
    if (t == NULL) {
        check(0, "Table 2 row 1 (create failed)");
        return;
    }

    for (key = 1; key <= 1000; key++) {
        if (ht_add(t, key) != 0) {
            check(0, "Table 2 row 1: ht_add failed");
            ht_destroy(t);
            return;
        }
    }

    check(ht_longest_chain(t) == 100,
          "Table 2 row 1: 1..1000 in 10 buckets -> chain 100");

    ht_destroy(t);
}

/*
 * test_table_row2 - Table 2, row 2.
 * Keys 1, 2, 3, ..., 1000 with 1000 buckets: longest chain 1.
 *
 * Every key is below 1000, so key % 1000 is the key itself: every key
 * gets its own bucket.
 */
static void test_table_row2(void)
{
    ht_t *t;
    int   key;

    t = ht_create(1000);
    if (t == NULL) {
        check(0, "Table 2 row 2 (create failed)");
        return;
    }

    for (key = 1; key <= 1000; key++) {
        if (ht_add(t, key) != 0) {
            check(0, "Table 2 row 2: ht_add failed");
            ht_destroy(t);
            return;
        }
    }

    check(ht_longest_chain(t) == 1,
          "Table 2 row 2: 1..1000 in 1000 buckets -> chain 1");

    ht_destroy(t);
}

/*
 * test_table_row3 - Table 2, row 3.
 * Keys 10, 20, 30, ..., 10000 with 1000 buckets: longest chain 10.
 *
 * Every key is a multiple of 10, so its remainder modulo 1000 is also
 * a multiple of 10; only 100 buckets are reachable and the 1000 keys
 * pile up 10 per bucket.
 */
static void test_table_row3(void)
{
    ht_t *t;
    int   key;

    t = ht_create(1000);
    if (t == NULL) {
        check(0, "Table 2 row 3 (create failed)");
        return;
    }

    for (key = 10; key <= 10000; key = key + 10) {
        if (ht_add(t, key) != 0) {
            check(0, "Table 2 row 3: ht_add failed");
            ht_destroy(t);
            return;
        }
    }

    check(ht_size(t) == 1000, "Table 2 row 3: size 1000");
    check(ht_longest_chain(t) == 10,
          "Table 2 row 3: multiples of 10 in 1000 buckets -> chain 10");

    ht_destroy(t);
}

/*
 * test_table_row4 - Table 2, row 4.
 * Keys 10, 20, 30, ..., 10000 with 1009 buckets: longest chain 1.
 *
 * 1009 is prime, so it shares no factor with 10. The 1000 keys get
 * 1000 different remainders and each bucket holds one key.
 */
static void test_table_row4(void)
{
    ht_t *t;
    int   key;

    t = ht_create(1009);
    if (t == NULL) {
        check(0, "Table 2 row 4 (create failed)");
        return;
    }

    for (key = 10; key <= 10000; key = key + 10) {
        if (ht_add(t, key) != 0) {
            check(0, "Table 2 row 4: ht_add failed");
            ht_destroy(t);
            return;
        }
    }

    check(ht_size(t) == 1000, "Table 2 row 4: size 1000");
    check(ht_longest_chain(t) == 1,
          "Table 2 row 4: multiples of 10 in 1009 buckets -> chain 1");

    ht_destroy(t);
}

/*
 * test_null_arguments - every function has a defined answer for NULL.
 *
 * ht_add says -1, the three questions say 0, and ht_destroy simply
 * does nothing.
 */
static void test_null_arguments(void)
{
    check(ht_add(NULL, 1) == -1, "ht_add(NULL, 1) returns -1");
    check(ht_count(NULL, 1) == 0, "ht_count(NULL, 1) returns 0");
    check(ht_size(NULL) == 0, "ht_size(NULL) returns 0");
    check(ht_longest_chain(NULL) == 0,
          "ht_longest_chain(NULL) returns 0");

    ht_destroy(NULL);
    check(1, "ht_destroy(NULL) does nothing");
}

/*
 * test_destroy_full - destroying a table that is full of nodes.
 *
 * 1000 keys in one bucket once, and then 2000 keys in 10 buckets, are
 * all freed. Under make memcheck every node that ht_destroy forgot
 * would be reported as a leak.
 */
static void test_destroy_full(void)
{
    ht_t *t;
    int   key;
    int   ok;

    t = ht_create(1);
    if (t == NULL) {
        check(0, "destroying a full table (create failed)");
        return;
    }

    ok = 1;
    for (key = 1; key <= 1000; key++) {
        if (ht_add(t, key) != 0) {
            ok = 0;
        }
    }

    check(ok && ht_size(t) == 1000,
          "one bucket holds 1000 different keys");
    ht_destroy(t);

    t = ht_create(10);
    if (t == NULL) {
        check(0, "destroying a second full table (create failed)");
        return;
    }

    ok = 1;
    for (key = 1; key <= 1000; key++) {
        if (ht_add(t, key) != 0) {
            ok = 0;
        }
        if (ht_add(t, key) != 0) {
            ok = 0;
        }
    }

    check(ok && ht_size(t) == 1000 && ht_count(t, 500) == 2,
          "10 buckets hold 1000 keys, one of them twice");
    ht_destroy(t);
}

/*
 * test_many_tables - create, use, and destroy many tables in a row.
 *
 * Nothing is global, so each table must start empty and independent
 * of the ones before it. Under make memcheck this also catches a
 * table struct or bucket array that ht_destroy forgets.
 */
static void test_many_tables(void)
{
    int round;
    int all_fresh;

    all_fresh = 1;

    for (round = 0; round < 50; round++) {
        ht_t *t;

        t = ht_create(5);
        if (t == NULL) {
            all_fresh = 0;
            continue;
        }

        /*
         * Round 0 adds key 3; every other round must still see an
         * empty table, and key 3 must count 0 there.
         */
        if (round == 0) {
            if (ht_add(t, 3) != 0) {
                all_fresh = 0;
            }
        } else {
            if (ht_size(t) != 0 || ht_count(t, 3) != 0) {
                all_fresh = 0;
            }
        }

        ht_destroy(t);
    }

    check(all_fresh, "50 tables in a row are fresh and independent");
}

int main(void)
{
    test_zero_buckets();
    test_empty_table();
    test_one_key_three_times();
    test_same_bucket();
    test_warmup_keys();
    test_thousand_keys();
    test_table_row1();
    test_table_row2();
    test_table_row3();
    test_table_row4();
    test_null_arguments();
    test_destroy_full();
    test_many_tables();

    printf("\n%d checks, %d failed\n", checks, failures);

    if (failures != 0) {
        return 1;
    }
    return 0;
}
