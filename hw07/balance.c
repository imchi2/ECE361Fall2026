/*
 * balance.c - check that brackets balance, one character at a time.
 *
 * Build:  make
 * Run:    ./balance < file
 *
 * It reads standard input char by char, pushes every '(', '[' and '{',
 * and pops for every ')', ']' and '}'. Every bracket counts, also the
 * ones inside comments and strings: this program does not know what a
 * comment is, and does not need to.
 *
 * It stops at the FIRST error and prints exactly one line. Lines and
 * columns count from 1. On success it prints how many pairs matched and
 * how deep the nesting got. Exit status is 0 when balanced, 1 otherwise.
 */
#include <stdio.h>
#include <stdlib.h>

#include "istack.h"

/*
 * closer_for - the character that closes an opener.
 * Takes: open, one of '(', '[' or '{'.
 * Returns: ')', ']' or '}' to match it, or 0 if it is not an opener.
 *
 * Kept as a small function because the same three-way choice is needed
 * twice: once to report what was expected, once to check a match.
 */
static char closer_for(int open)
{
    if (open == '(') {
        return ')';
    }
    if (open == '[') {
        return ']';
    }
    if (open == '{') {
        return '}';
    }

    /* Not an opener at all; the callers never ask for this case. */
    return 0;
}

int main(void)
{
    istack_t *s;
    int c;              /* getchar returns int, so EOF fits in it */
    int line = 1;       /* lines count from 1 */
    int column = 1;     /* columns count from 1 */
    size_t pairs = 0;   /* how many brackets matched */
    size_t deepest = 0; /* the most brackets open at once */

    s = istack_create();
    if (s == NULL) {
        /* Out of memory before the first character; nothing to check. */
        fprintf(stderr, "balance: out of memory\n");
        return 1;
    }

    while ((c = getchar()) != EOF) {
        /*
         * The bracket, if this character is one, is handled at the
         * position it was read from: (line, column) still points at it.
         * The counters move on afterwards.
         */
        if (c == '(' || c == '[' || c == '{') {
            /* An opener: put it on the stack. */
            if (istack_push(s, c) != 0) {
                /* Memory ran out; the stack is unchanged. Say so on
                 * stderr, so the one-line rule for bracket errors holds. */
                fprintf(stderr, "balance: out of memory\n");
                istack_destroy(s);
                return 1;
            }

            /* Track the most brackets ever open at once. */
            if (istack_count(s) > deepest) {
                deepest = istack_count(s);
            }
        } else if (c == ')' || c == ']' || c == '}') {
            int top;

            if (istack_is_empty(s)) {
                /* Nothing open: this ')' has no partner to join. */
                printf("line %d, column %d: found '%c', nothing open\n",
                       line, column, c);
                istack_destroy(s);
                return 1;
            }

            /* Look at the top without removing it yet: the top decides
             * what was expected here. peek cannot fail, the stack is
             * not empty, but the return value is still checked so the
             * compiler and the reader both see it. */
            if (istack_peek(s, &top) != 0) {
                fprintf(stderr, "balance: peek failed\n");
                istack_destroy(s);
                return 1;
            }

            if (c != closer_for(top)) {
                /* Wrong partner: for example '[' open and ')' found. */
                printf("line %d, column %d: found '%c', expected '%c'\n",
                       line, column, c, closer_for(top));
                istack_destroy(s);
                return 1;
            }

            /* The right partner: take the opener off and count a pair. */
            if (istack_pop(s, NULL) != 0) {
                fprintf(stderr, "balance: pop failed\n");
                istack_destroy(s);
                return 1;
            }
            pairs = pairs + 1;
        }

        /*
         * Move the position past this character. A newline starts the
         * next line at column 1; anything else steps one column right.
         */
        if (c == '\n') {
            line = line + 1;
            column = 1;
        } else {
            column = column + 1;
        }
    }

    /* End of input: anything still open never got closed. */
    if (!istack_is_empty(s)) {
        int top;

        /* The stack is not empty, so peek must find the innermost
         * opener: the one pushed last. */
        if (istack_peek(s, &top) != 0) {
            fprintf(stderr, "balance: peek failed\n");
            istack_destroy(s);
            return 1;
        }

        printf("end of input: %zu still open, innermost '%c'\n",
               istack_count(s), (char)top);
        istack_destroy(s);
        return 1;
    }

    printf("balanced: %zu pairs, deepest %zu\n", pairs, deepest);
    istack_destroy(s);
    return 0;
}
