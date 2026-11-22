/*
 * test_bst.c - tests for the bst ADT.
 *
 * Build and run:  make test
 *
 * Every check prints PASS or FAIL, then a summary line, and the
 * program exits nonzero if anything failed, so make can tell the
 * difference.
 *
 * The tests cover, at least (the list the spec asks for):
 *   - the input that crashed the faithful port: a search on an
 *     empty tree (run FIRST, so the crash is the first thing seen)
 *   - the original's seven keys (27 14 35 10 19 31 42): size 7,
 *     height 3, 31 found in 3 visits, 15 not found in 3 visits
 *   - a duplicate key is kept (size 2)
 *   - keys inserted in increasing order give height == number of keys
 *   - destroying a full tree
 * plus:
 *   - visits may be NULL
 *   - NULL tree arguments
 *   - two trees are independent (nothing is global any more)
 */
#include <stdio.h>

#include "bst.h"

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
 * test_search_empty - THE INPUT THAT CRASHED the faithful port.
 *
 * Part 2: the original's search reads current->data in the loop
 * condition before anything checks current for NULL, so on an empty
 * tree it dereferences NULL. Run under make memcheck, the faithful
 * port of bst_contains died right here (the first line of the report
 * is in README.md). The fixed bst_contains must return false and
 * report 0 visits instead of crashing.
 */
static void test_search_empty(void)
{
    bst_t *t;
    size_t visits;
    int found;

    t = bst_create();
    if (t == NULL) {
        check(0, "searching an empty tree (create failed)");
        return;
    }

    visits = 999;
    found = bst_contains(t, 42, &visits);

    check(!found && visits == 0 && bst_size(t) == 0
          && bst_height(t) == 0,
          "searching an empty tree: false, 0 visits (the crash input)");

    bst_destroy(t);
}

/*
 * test_seven_keys - the original's tree: 27 14 35 10 19 31 42.
 *
 * Inserted in that order, the tree is
 *              27
 *             /  \
 *           14    35
 *          /  \   / \
 *        10   19 31  42
 * so the size is 7 and the height is 3; 31 is reached 27 -> 35 -> 31
 * (3 visits), and 15 walks 27 -> 14 -> 19 and falls off (3 visits,
 * not found).
 */
static void test_seven_keys(void)
{
    int keys[7] = { 27, 14, 35, 10, 19, 31, 42 };
    bst_t *t;
    size_t visits;
    int i;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "the original's seven keys (create failed)");
        return;
    }

    ok = 1;
    i = 0;
    while (i < 7) {
        ok = ok && bst_insert(t, keys[i]) == 0;
        i = i + 1;
    }
    check(ok && bst_size(t) == 7 && bst_height(t) == 3,
          "the original's seven keys: size 7, height 3");

    visits = 0;
    ok = bst_contains(t, 31, &visits);
    check(ok && visits == 3, "31 found in 3 visits");

    visits = 0;
    ok = bst_contains(t, 15, &visits);
    check(!ok && visits == 3, "15 not found in 3 visits");

    /* Every key that went in must be found; visits may be NULL when
     * the caller does not care how long the search was. */
    ok = 1;
    i = 0;
    while (i < 7) {
        ok = ok && bst_contains(t, keys[i], NULL);
        i = i + 1;
    }
    check(ok, "every inserted key is found (visits may be NULL)");

    bst_destroy(t);
}

/*
 * test_duplicate - an equal key goes RIGHT and is kept.
 *
 * Inserting 20 twice keeps both copies: size 2. The second 20 became
 * the right child of the first, so the height is 2 as well.
 */
static void test_duplicate(void)
{
    bst_t *t;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "a duplicate key is kept (create failed)");
        return;
    }

    ok = 1;
    ok = ok && bst_insert(t, 20) == 0;
    ok = ok && bst_insert(t, 20) == 0;

    check(ok && bst_size(t) == 2 && bst_height(t) == 2
          && bst_contains(t, 20, NULL),
          "a duplicate key is kept (size 2)");

    bst_destroy(t);
}

/*
 * test_sorted_height - increasing order degenerates the tree.
 *
 * Every key is greater than the one before, so every key becomes the
 * right child of the last: the tree is a single right-going chain,
 * and the height equals the number of keys.
 */
static void test_sorted_height(void)
{
    bst_t *t;
    int i;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "increasing order height (create failed)");
        return;
    }

    ok = 1;
    i = 1;
    while (i <= 10) {
        ok = ok && bst_insert(t, i) == 0;
        i = i + 1;
    }

    check(ok && bst_size(t) == 10 && bst_height(t) == 10,
          "increasing order: height equals the number of keys");

    bst_destroy(t);
}

/*
 * test_destroy_full - destroying a full tree.
 *
 * There is no visible answer to check, so this test mainly gives the
 * sanitizer something to catch: a leaked or double-freed node would
 * show up under make memcheck, and a crash would fail the run.
 */
static void test_destroy_full(void)
{
    bst_t *t;
    int i;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "destroying a full tree (create failed)");
        return;
    }

    /* Increasing keys make the deepest tree this size can make. */
    ok = 1;
    i = 0;
    while (i < 1000) {
        ok = ok && bst_insert(t, i) == 0;
        i = i + 1;
    }

    /* The destroy itself is the test: it must not crash or leak. */
    bst_destroy(t);

    check(ok, "destroying a full tree");
}

/*
 * test_null_arguments - NULL is handled by every function.
 *
 * Nothing here may crash; the results are the ones bst.h promises.
 */
static void test_null_arguments(void)
{
    size_t visits;
    int ok;

    visits = 999;
    ok = 1;
    ok = ok && bst_insert(NULL, 1) == -1;
    ok = ok && !bst_contains(NULL, 1, &visits) && visits == 0;
    ok = ok && bst_size(NULL) == 0;
    ok = ok && bst_height(NULL) == 0;

    /* destroy(NULL) must do nothing rather than crash. */
    bst_destroy(NULL);

    check(ok, "NULL tree is safe everywhere");
}

/*
 * test_two_trees - two trees side by side are independent.
 *
 * This is what "nothing is global" buys: each tree keeps its own
 * root and size, so filling one must not touch the other.
 */
static void test_two_trees(void)
{
    int keys[7] = { 27, 14, 35, 10, 19, 31, 42 };
    bst_t *a;
    bst_t *b;
    int i;
    int ok;

    a = bst_create();
    b = bst_create();
    if (a == NULL || b == NULL) {
        check(0, "two trees are independent (create failed)");
        bst_destroy(a);
        bst_destroy(b);
        return;
    }

    /* Give each tree a different life. */
    ok = 1;
    i = 0;
    while (i < 7) {
        ok = ok && bst_insert(a, keys[i]) == 0;
        i = i + 1;
    }
    ok = ok && bst_insert(b, 99) == 0;

    /* a has the seven keys, b still has exactly its one. */
    ok = ok && bst_size(a) == 7 && bst_height(a) == 3;
    ok = ok && bst_size(b) == 1 && bst_height(b) == 1;
    ok = ok && !bst_contains(a, 99, NULL) && bst_contains(b, 99, NULL);

    check(ok, "two trees are independent");

    bst_destroy(a);
    bst_destroy(b);
}

int main(void)
{
    /* The crash input runs FIRST: on the faithful (buggy) port this
     * is where the program dies, which is what Part 2 documents. */
    test_search_empty();
    test_seven_keys();
    test_duplicate();
    test_sorted_height();
    test_destroy_full();
    test_null_arguments();
    test_two_trees();

    printf("%d checks, %d failed\n", checks, failures);
    if (failures > 0) {
        return 1;
    }
    return 0;
}
