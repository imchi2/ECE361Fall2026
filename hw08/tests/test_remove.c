/*
 * test_remove.c - tests for the bonus bst_remove.
 *
 * Build and run:  make bonus   (rebuilds with the address and
 * undefined-behaviour sanitizers, then runs this program).
 *
 * PASS/FAIL per check, a summary, and a nonzero exit status on
 * failure. "Frees exactly one node" is checked by the sanitizers: a
 * leak or a double free would be reported and fail the run.
 *
 * The tests cover:
 *   - removing from an empty tree and a NULL tree: -1, nothing changes
 *   - removing a key that is not in the tree: -1, nothing changes
 *   - the three removals BONUS.md draws, on the original's seven
 *     keys: 10 (a leaf), then 14 (one child), then 27 (two children)
 *   - removing a duplicate takes one copy at a time
 *   - removing the root of a one-node tree
 *   - removing every key until the tree is empty
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
 * test_remove_empty_and_null - removing from nothing must fail
 * politely: -1, and nothing changes.
 */
static void test_remove_empty_and_null(void)
{
    bst_t *t;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "remove from an empty or NULL tree (create failed)");
        return;
    }

    ok = 1;
    ok = ok && bst_remove(t, 42) == -1;    /* empty tree */
    ok = ok && bst_size(t) == 0;           /* unchanged */
    ok = ok && bst_remove(NULL, 42) == -1; /* NULL tree */

    check(ok, "remove from an empty or NULL tree fails, changes nothing");

    bst_destroy(t);
}

/*
 * test_remove_missing - a key that is not in the tree: -1, and the
 * tree is exactly as it was.
 */
static void test_remove_missing(void)
{
    bst_t *t;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "removing a missing key (create failed)");
        return;
    }

    ok = 1;
    ok = ok && bst_insert(t, 10) == 0;
    ok = ok && bst_insert(t, 20) == 0;
    ok = ok && bst_remove(t, 15) == -1;    /* 15 was never inserted */
    ok = ok && bst_size(t) == 2;           /* unchanged */
    ok = ok && bst_contains(t, 10, NULL);
    ok = ok && bst_contains(t, 20, NULL);

    check(ok, "removing a missing key fails, changes nothing");

    bst_destroy(t);
}

/*
 * test_bonus_walk - the three removals BONUS.md draws, on the
 * original's seven keys:
 *
 *        27              27                27               31
 *       /  \            /  \              /  \             /  \
 *     14    35   ->   14    35    ->    19    35    ->   19    35
 *    /  \   / \        \    / \              /  \               \
 *  10   19 31  42      19  31  42          31   42              42
 *
 *   remove 10: a leaf        (just detach it)
 *   remove 14: one child     (19 moves up into 14's place)
 *   remove 27: two children  (31, the smallest key of the right
 *                             subtree, moves up; 31's old node is
 *                             removed instead)
 */
static void test_bonus_walk(void)
{
    int keys[7] = { 27, 14, 35, 10, 19, 31, 42 };
    bst_t *t;
    int i;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "the BONUS.md walk (create failed)");
        return;
    }

    ok = 1;
    i = 0;
    while (i < 7) {
        ok = ok && bst_insert(t, keys[i]) == 0;
        i = i + 1;
    }

    /* remove 10: a leaf; the rest of the shape is kept. */
    ok = ok && bst_remove(t, 10) == 0;
    ok = ok && bst_size(t) == 6 && !bst_contains(t, 10, NULL);
    ok = ok && bst_height(t) == 3;
    check(ok, "remove 10, a leaf (size 6, height still 3)");

    /* remove 14: one child (19), which must move up into its place. */
    ok = 1;
    ok = ok && bst_remove(t, 14) == 0;
    ok = ok && bst_size(t) == 5 && !bst_contains(t, 14, NULL);
    ok = ok && bst_contains(t, 19, NULL);
    check(ok, "remove 14, one child (19 takes its place)");

    /* remove 27: two children; 31 must move up to the root. */
    ok = 1;
    ok = ok && bst_remove(t, 27) == 0;
    ok = ok && bst_size(t) == 4 && !bst_contains(t, 27, NULL);
    ok = ok && bst_contains(t, 31, NULL);
    {
        size_t visits;

        /* If 31 really is the root now, it is found in ONE visit. */
        visits = 0;
        ok = ok && bst_contains(t, 31, &visits) && visits == 1;
    }
    check(ok, "remove 27, two children (31 is the new root)");

    /* The four remaining keys must all still be there. */
    ok = 1;
    ok = ok && bst_contains(t, 19, NULL) && bst_contains(t, 31, NULL);
    ok = ok && bst_contains(t, 35, NULL) && bst_contains(t, 42, NULL);
    check(ok, "the remaining keys 19 31 35 42 are all still found");

    bst_destroy(t);
}

/*
 * test_remove_duplicate - one removal takes ONE copy.
 *
 * Inserting 20, 10, 20 keeps the duplicate to the right:
 *        20
 *       /  \
 *     10    20
 * Removing 20 once leaves one copy findable; removing it again takes
 * the last one.
 */
static void test_remove_duplicate(void)
{
    bst_t *t;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "removing a duplicate (create failed)");
        return;
    }

    ok = 1;
    ok = ok && bst_insert(t, 20) == 0;
    ok = ok && bst_insert(t, 10) == 0;
    ok = ok && bst_insert(t, 20) == 0;    /* the duplicate */

    /* One removal: one copy left, and it is still found. */
    ok = ok && bst_remove(t, 20) == 0;
    ok = ok && bst_size(t) == 2 && bst_contains(t, 20, NULL);

    /* Second removal: no copy left. */
    ok = ok && bst_remove(t, 20) == 0;
    ok = ok && bst_size(t) == 1 && !bst_contains(t, 20, NULL);

    check(ok, "removing a duplicate takes one copy at a time");

    bst_destroy(t);
}

/*
 * test_remove_single_root - removing the only node empties the tree,
 * and the tree is a valid empty tree afterwards.
 */
static void test_remove_single_root(void)
{
    bst_t *t;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "removing the root of a one-node tree (create failed)");
        return;
    }

    ok = 1;
    ok = ok && bst_insert(t, 7) == 0;
    ok = ok && bst_remove(t, 7) == 0;
    ok = ok && bst_size(t) == 0 && bst_height(t) == 0;
    ok = ok && !bst_contains(t, 7, NULL);
    ok = ok && bst_remove(t, 7) == -1;    /* now empty: must fail */

    check(ok, "removing the root of a one-node tree empties it");

    bst_destroy(t);
}

/*
 * test_remove_everything - removing every key, in insertion order,
 * must succeed each time and leave a truly empty tree. Along the way
 * this hits every case, including removing the root with two children
 * (27 goes first) and the root with one child.
 */
static void test_remove_everything(void)
{
    int keys[7] = { 27, 14, 35, 10, 19, 31, 42 };
    bst_t *t;
    int i;
    int ok;

    t = bst_create();
    if (t == NULL) {
        check(0, "removing every key (create failed)");
        return;
    }

    ok = 1;
    i = 0;
    while (i < 7) {
        ok = ok && bst_insert(t, keys[i]) == 0;
        i = i + 1;
    }

    i = 0;
    while (i < 7) {
        ok = ok && bst_remove(t, keys[i]) == 0;
        ok = ok && bst_size(t) == (size_t)(7 - i - 1);
        i = i + 1;
    }
    ok = ok && bst_size(t) == 0 && bst_height(t) == 0;

    check(ok, "removing every key leaves a truly empty tree");

    bst_destroy(t);
}

int main(void)
{
    test_remove_empty_and_null();
    test_remove_missing();
    test_bonus_walk();
    test_remove_duplicate();
    test_remove_single_root();
    test_remove_everything();

    printf("%d checks, %d failed\n", checks, failures);
    if (failures > 0) {
        return 1;
    }
    return 0;
}
