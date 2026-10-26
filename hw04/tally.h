/*
 * tally.h: ECE 361, Fall 2026, HW4 - the tally ADT's interface.
 *
 * A tally counts how often each name occurs, such as the events your
 * project device sees. Callers use it only through this header; only
 * tally.c knows what is inside a tally_t. That opacity is the point of
 * the homework: README.md, Part 3, shows what it prevents.
 *
 * Three numbers bound the ADT, from the spec:
 *
 *   TALLY_MAX     4   tallies that can exist at the same time
 *   TALLY_NAMES  16   different names one tally can hold
 *   TALLY_LEN    15   longest name, without the '\0'
 *
 * There is no malloc: the tallies live in a fixed pool inside
 * tally.c, so tally_create hands out a slot and tally_destroy gives
 * it back for the next create.
 *
 * Every function here has a comment saying what it takes, what it
 * returns, and what it assumes - the same three questions the HW3
 * header asked.
 */
#ifndef TALLY_H
#define TALLY_H

#include <stdio.h> /* FILE - tally_print writes to one */

#define TALLY_MAX     4    /* tallies that can exist at the same time */
#define TALLY_NAMES  16    /* different names one tally can hold */
#define TALLY_LEN    15    /* longest name, without the '\0' */

typedef struct tally tally_t; /* opaque: only tally.c completes it */

/*
 * tally_create - a new, empty tally.
 *
 * Takes:   nothing.
 * Returns: a pointer to a fresh tally, size 0, or NULL when all
 *          TALLY_MAX tallies are already in use.
 * Assumes: the caller will hand it back with tally_destroy; a tally
 *          that is never destroyed stays in use until the program
 *          ends.
 */
tally_t *tally_create(void);

/*
 * tally_destroy - give the tally back to the pool.
 *
 * Takes:   t, a tally that came from tally_create; NULL does nothing.
 * Returns: nothing.
 * Assumes: after this call the pointer must not be used again; the
 *          next tally_create may hand the same memory to somebody
 *          else, emptied.
 */
void tally_destroy(tally_t *t);

/*
 * tally_add - count one more occurrence of name.
 *
 * Takes:   t, the tally; name, a '\0'-terminated string.
 * Returns: 0 when the occurrence was counted; nonzero when nothing
 *          was done - t or name is NULL, name is empty, name is
 *          longer than TALLY_LEN, or name is new and the tally
 *          already holds TALLY_NAMES different names.
 * Assumes: a name already in the tally is still counted even when the
 *          tally is full: full only blocks *new* names. The name is
 *          copied into the tally, so the caller may change its own
 *          buffer right after the call.
 */
int tally_add(tally_t *t, const char *name);

/*
 * tally_count - how often name was added to this tally.
 *
 * Takes:   t, the tally; name, the name to look for.
 * Returns: the number of times name was added: 0 when it was never
 *          added, and 0 for a NULL tally or a NULL name.
 * Assumes: comparison is exact (strcmp): "Alice" and "alice" are two
 *          different names.
 */
int tally_count(const tally_t *t, const char *name);

/*
 * tally_size - how many different names the tally holds.
 *
 * Takes:   t, the tally.
 * Returns: the number of distinct names, 0 for a NULL tally.
 * Assumes: a full tally still has size TALLY_NAMES, no matter how
 *          many additions that took.
 */
int tally_size(const tally_t *t);

/*
 * tally_print - write one line per name, "name count", sorted.
 *
 * Takes:   t, the tally; out, the stream to write to (stdout, a file,
 *          anything stdio offers).
 * Returns: nothing.
 * Assumes: the lines come out in alphabetical order, by strcmp - the
 *          tally keeps its names sorted, so printing is just a walk.
 *          A NULL tally or a NULL out writes nothing at all.
 */
void tally_print(const tally_t *t, FILE *out);

#endif /* TALLY_H */
