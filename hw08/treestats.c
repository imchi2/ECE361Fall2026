/*
 * treestats.c - Part 3: what insertion order does to a tree.
 *
 *     ./treestats < file
 *
 * Reads integers from standard input until the end of the input and
 * inserts them into one bst in the order read, then prints exactly
 * one line:
 *
 *     n 7, height 3                  exit status 0
 *     empty tree                     exit status 0
 *     not a number after 4 values    exit status 1
 *
 * The tree is destroyed on every path, so make memcheck stays clean.
 */
#include <stdio.h>

#include "bst.h"

int main(void)
{
    bst_t *t;
    int key;
    size_t n;
    int rc;

    t = bst_create();
    if (t == NULL) {
        /* Out of memory: not one of the specified outputs, so fail
         * quietly with a nonzero status. */
        return 1;
    }

    /* scanf returns 1 per integer read, 0 when the next input is not
     * a number, and EOF at the end of the input. */
    n = 0;
    rc = scanf("%d", &key);
    while (rc == 1) {
        if (bst_insert(t, key) != 0) {
            /* Out of memory: same quiet failure as above. */
            bst_destroy(t);
            return 1;
        }
        n = n + 1;
        rc = scanf("%d", &key);
    }

    if (rc != EOF) {
        /* The loop ended on input that is not a number: n integers
         * were read before it. */
        printf("not a number after %zu values\n", n);
        bst_destroy(t);
        return 1;
    }

    if (n == 0) {
        printf("empty tree\n");
    } else {
        printf("n %zu, height %zu\n", n, bst_height(t));
    }

    bst_destroy(t);
    return 0;
}
