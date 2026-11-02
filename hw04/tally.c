/*
 * tally.c: ECE 361, Fall 2026, HW4 - the tally ADT, the implementation.
 *
 * The inside of tally_t, and the only place that inside exists:
 *
 *   struct entry { char name[TALLY_LEN + 1]; int count; };
 *   struct tally { struct entry e[TALLY_NAMES]; int n; };
 *
 * "Kept sorted by name" is the heart of it: the n used entries sit in
 * alphabetical order (strcmp order), so printing is a plain walk from
 * 0 to n - 1 and finding a name is a scan that can stop early. A new
 * name is slipped in where it belongs and the entries after it move up
 * by one - the same sorted-array trick as the store in the project.
 *
 * No malloc. The tallies live in a static pool of TALLY_MAX slots,
 * each marked in use or free, so tally_create hands out a free slot
 * and tally_destroy hands it back. Names are copied in with a length
 * check first: no strcpy, no strncpy, nothing that can write past the
 * end of name[TALLY_LEN + 1].
 */

#include "tally.h"

#include <stddef.h> /* NULL  */
#include <string.h> /* strlen, strcmp */

/* One name and its count. The name array holds TALLY_LEN characters
   plus the '\0', which is why the name check in tally_add is
   "longer than TALLY_LEN is refused" and not "TALLY_LEN or more". */
struct entry {
    char name[TALLY_LEN + 1];
    int  count;
};

/* The completed type - tally.h calls it opaque, this is the one place
   its members are named. */
struct tally {
    struct entry e[TALLY_NAMES]; /* the names, sorted by strcmp order */
    int          n;              /* how many slots of e[] are used */
};

/*
 * The pool: TALLY_MAX tallies can exist at the same time. Slot i is
 * handed out when g_in_use[i] is 1 and free when it is 0. There are no
 * globals beyond this pool - the tally never keeps state of its own
 * between calls.
 */
static tally_t g_pool[TALLY_MAX];
static int     g_in_use[TALLY_MAX]; /* 0 = free, 1 = handed out */

/*
 * Find name in t's sorted entries.
 *
 * Returns the slot index, or -1 when the name is not there. The scan
 * compares against every used entry; because the entries are sorted,
 * the loop can stop as soon as it meets an entry whose name sorts
 * after the one being looked for - a name that is not present is
 * proven absent the same way, by reaching a "greater" entry or the
 * end. A helper only tally.c uses, which is why it is not in the
 * header.
 */
static int find_slot(const tally_t *t, const char *name)
{
    int i;

    for (i = 0; i < t->n; i++) {
        int cmp = strcmp(t->e[i].name, name);
        if (cmp == 0) return i;           /* exact match, found it */
        if (cmp > 0) return -1;           /* sorted past it: not here */
    }

    return -1; /* ran out of entries: not here */
}

/*
 * tally_create - hand out the first free slot of the pool.
 *
 * The loop looks for a slot nobody is using, marks it in use, empties
 * it (n = 0; whatever the last owner left in the entries is not
 * anybody's business), and returns its address. No free slot means
 * NULL, which is exactly what the header promises.
 */
tally_t *tally_create(void)
{
    int i;

    for (i = 0; i < TALLY_MAX; i++) {
        if (!g_in_use[i]) {
            g_in_use[i] = 1;
            g_pool[i].n = 0;
            return &g_pool[i];
        }
    }

    return NULL; /* every slot is spoken for */
}

/*
 * tally_destroy - mark the slot free again.
 *
 * The pointer is compared against the pool slots to learn which one is
 * being given back. A pointer that matches no slot (NULL, or a caller
 * bug) is ignored: there is nothing to un-mark.
 */
void tally_destroy(tally_t *t)
{
    int i;

    if (t == NULL) return; /* the header says NULL does nothing */

    for (i = 0; i < TALLY_MAX; i++) {
        if (t == &g_pool[i]) {
            g_in_use[i] = 0;
            t->n = 0; /* empty it, so the next create starts clean */
            return;
        }
    }
}

/*
 * tally_add - count one more occurrence of name.
 *
 * Every check runs before anything is written, because a refused call
 * must change nothing:
 *
 *   1. NULL tally or NULL name          - nothing to count into
 *   2. empty name or longer than TALLY_LEN - would not fit the copy
 *   3. the name is already here         - just bump its count
 *   4. the name is new and the tally is full (n == TALLY_NAMES) - no
 *      room for a new entry
 *
 * Case 3 is checked before case 4 on purpose: an existing name still
 * counts even in a full tally, because its entry is already there and
 * only the number changes.
 *
 * A new name is inserted at the first slot whose name sorts after it,
 * and every entry from there on is copied one slot up - the "move up
 * by one" the spec describes, done with a loop so the movement is
 * visible. The name itself is copied character by character, stopping
 * at the '\0'; the length was already checked, so this loop can never
 * reach past the end of the destination.
 */
int tally_add(tally_t *t, const char *name)
{
    int  i;     /* the slot the name lives in, or the slot it goes to */
    int  j;     /* walks the entries when they move up */
    size_t len; /* strlen(name), measured once */

    if (t == NULL || name == NULL) return -1; /* case 1 */

    len = strlen(name);
    if (len == 0 || len > TALLY_LEN) return -1; /* case 2 */

    /* Case 3: already known - one more occurrence. */
    i = find_slot(t, name);
    if (i >= 0) {
        t->e[i].count = t->e[i].count + 1;
        return t->e[i].count;
    }

    /* Case 4: new name, no room. */
    if (t->n >= TALLY_NAMES) return -1;

    /* Find where the new name belongs: the first slot whose name
       sorts after it. find_slot returned -1, so this loop runs to the
       end when the new name sorts last. */
    for (i = 0; i < t->n; i++) {
        if (strcmp(t->e[i].name, name) > 0) break;
    }

    /* Move entries i..n-1 up to i+1..n, opening one free slot at i. */
    for (j = t->n; j > i; j--) {
        t->e[j] = t->e[j - 1];
    }

    /* The safe copy: the length check above guarantees len + 1 bytes
       (the characters plus the '\0') fit in name[TALLY_LEN + 1]. */
    for (j = 0; (size_t)j <= len; j++) {
        t->e[i].name[j] = name[j];
    }
    t->e[i].count = 1; /* the first occurrence */

    t->n = t->n + 1;

    return t->e[i].count; /* counted */
}

/*
 * tally_count - how often name was added.
 *
 * The name is searched with the same find_slot the insert uses, so
 * "present" means the same thing in both. A missing name, a NULL
 * tally, and a NULL name all read "0 times", which is the header's
 * promise - there is no error to report, only a number to give back.
 */
int tally_count(const tally_t *t, const char *name)
{
    int i;

    if (t == NULL || name == NULL) return 0;

    i = find_slot(t, name);
    if (i < 0) return 0; /* never added, or nothing to ask */

    return t->e[i].count;
}

/*
 * tally_size - how many different names the tally holds.
 *
 * n counts entries, and each entry is one *different* name, so n is
 * the answer. 0 for a NULL tally.
 */
int tally_size(const tally_t *t)
{
    if (t == NULL) return 0;

    return t->n;
}

/*
 * tally_print - one line per name, "name count", in sorted order.
 *
 * The entries are already sorted (tally_add kept them that way), so
 * the printing is a straight walk from 0 to n - 1 with nothing to
 * sort. Nothing is written when the tally or the stream is NULL.
 */
void tally_print(const tally_t *t, FILE *out)
{
    int i;

    if (t == NULL || out == NULL) return;

    for (i = 0; i < t->n; i++) {
        fprintf(out, "%s %d\n", t->e[i].name, t->e[i].count);
    }
}
