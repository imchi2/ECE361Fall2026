/*
 * store.c: ECE 361, Fall 2026, project - the record store, week 4 form.
 *
 * The inside of store_t, and the only place that inside exists:
 *
 *      struct store {
 *          record_t items[STORE_RECORDS];  kept sorted by record_compare
 *          int      n;                     how many are in use
 *      }
 *
 * "Sorted array" means exactly what HW4's tally does with names: the
 * records sit in order, and a new record is slipped in at the position
 * where it belongs, with everything after it moved up by one slot.
 * Searching walks the records in order; inserting shifts a tail.
 *
 * No malloc, and no globals beyond the pool: like tally.c, the stores
 * live in a fixed static array, each slot marked in use or free. Week
 * 8 replaces the sorted array with a binary search tree - the records
 * get linked by pointers instead of shifted in an array - but every
 * function keeps its signature and its meaning, so nothing outside
 * this file will notice.
 */

#include "store.h"

#include <stddef.h> /* NULL  */
#include <string.h> /* memmove */

/*
 * The completed type. store.h says only "typedef struct store
 * store_t;" - the members are named here and nowhere else, which is
 * what "opaque" means: no caller can touch items[] or n directly.
 *
 * This must come before g_pool below, because an array of store_t
 * needs the full size of a store, not just its name.
 */
struct store {
    record_t items[STORE_RECORDS]; /* the records, kept in key order */
    int      n;                    /* how many slots of items[] are used */
};

/*
 * The pool. STORE_MAX stores exist at the same time, no more; a store
 * is one slot of g_pool, and g_in_use[i] says whether slot i has been
 * handed out by store_create and not yet given back.
 */
static store_t g_pool[STORE_MAX];
static int     g_in_use[STORE_MAX]; /* 0 = free, 1 = handed out */

/*
 * record_compare - order two records: state first, timestamp second.
 *
 * Each field gets its own check, in the same shape: if the field
 * differs, that decides and we return; if it ties, fall through to the
 * next field. Both fields tie only when the two records really are
 * equal on the key, and 0 says so.
 */
int record_compare(const record_t *a, const record_t *b)
{
    /* The states are enum values 0..3, so plain < and > order them
       the way the states were declared: LOCKED before UNLOCKED
       before PASSING before VIOLATION. */
    if (a->state < b->state) return -1;
    if (a->state > b->state) return 1;

    /* Same state: the timestamps decide. time_t compares with < and >
       like any integer, and a negative difference would be wrong to
       write, so both directions are checked separately. */
    if (a->reading.stamp < b->reading.stamp) return -1;
    if (a->reading.stamp > b->reading.stamp) return 1;

    return 0; /* same state, same timestamp: equal on the key */
}

/*
 * store_create - hand out the first free slot of the pool.
 *
 * The loop scans for a slot nobody is using, marks it in use, empties
 * it (n = 0 - the leftover records from the last owner are not
 * anybody's business), and returns its address. If no slot is free,
 * NULL is the honest answer.
 */
store_t *store_create(void)
{
    int i;

    for (i = 0; i < STORE_MAX; i++) {
        if (!g_in_use[i]) {
            g_in_use[i] = 1;
            g_pool[i].n = 0;
            return &g_pool[i];
        }
    }

    return NULL; /* every slot is spoken for */
}

/*
 * store_destroy - mark the slot free again.
 *
 * The pointer is compared against the pool slots to find which one is
 * being given back. A pointer that matches no slot (NULL, or a caller
 * bug) is simply ignored - there is nothing to un-mark.
 */
void store_destroy(store_t *s)
{
    int i;

    if (s == NULL) return; /* the spec says: NULL does nothing */

    for (i = 0; i < STORE_MAX; i++) {
        if (s == &g_pool[i]) {
            g_in_use[i] = 0;
            s->n = 0; /* empty it, so a later create starts clean */
            return;
        }
    }
}

/*
 * store_insert - copy rec into the store, keeping the array sorted.
 *
 * First the checks, all of them before anything is moved, because a
 * refused insert must change nothing: NULL arguments, a full store,
 * and a record whose key is already present (find by key stays
 * unambiguous only while keys are unique).
 *
 * Then the position: records are walked until one compares greater
 * than the new record - that slot is where the new record belongs.
 * Reaching the end without finding a greater record means the new
 * record sorts last. memmove shifts the tail up one slot to open the
 * hole, and memmove (unlike memcpy) is safe when source and
 * destination overlap, which they always do here.
 */
int store_insert(store_t *s, const record_t *rec)
{
    int i; /* the insertion position, found by the scan below */

    if (s == NULL || rec == NULL) return 1;
    if (s->n >= STORE_RECORDS) return 1;                 /* full */
    if (store_find(s, rec->state, rec->reading.stamp) != NULL) return 1; /* duplicate key */

    /* Find where the record belongs: the first slot whose record
       comes after it in key order. */
    for (i = 0; i < s->n; i++) {
        if (record_compare(&s->items[i], rec) > 0) break;
    }

    /* Open one slot at position i by moving items[i..n-1] up to
       items[i+1..n]. When i == n (new record sorts last) the count is
       0 and memmove does nothing, which is exactly right. */
    memmove(&s->items[i + 1], &s->items[i],
            (size_t)(s->n - i) * sizeof(s->items[0]));

    s->items[i] = *rec; /* the hole is where the record goes */
    s->n++;

    return 0;
}

/*
 * store_find - linear scan for the record with this exact key.
 *
 * The array is sorted by state then timestamp, but for the week 4
 * form a plain scan from the front is plenty: STORE_RECORDS is small,
 * and a scan reads the same way as every other loop in the project.
 * (A binary search is the upgrade for a sorted array; the tree in
 * week 8 is the real answer.)
 */
const record_t *store_find(const store_t *s, turnstile_state_t state, time_t stamp)
{
    int i;

    if (s == NULL) return NULL;

    for (i = 0; i < s->n; i++) {
        /* Compare only the two key fields - the same test
           record_compare would run, spelled out here so the loop
           stays obvious. */
        if (s->items[i].state == state && s->items[i].reading.stamp == stamp) {
            return &s->items[i];
        }
    }

    return NULL; /* not in the store */
}

/*
 * store_walk - call fn once per record, in key order.
 *
 * The array already sits in key order (insert kept it that way), so
 * "in-order" is just walking slots 0..n-1. The cast to size_t keeps
 * the comparison honest for compilers that watch signedness.
 */
void store_walk(const store_t *s, store_visit_fn fn, void *arg)
{
    int i;

    if (s == NULL || fn == NULL) return;

    for (i = 0; i < s->n; i++) {
        fn(&s->items[i], arg);
    }
}

/*
 * store_count - how many records are held. 0 for NULL or empty.
 */
int store_count(const store_t *s)
{
    if (s == NULL) return 0;

    return s->n;
}

/*
 * store_height - how deep the store's structure is.
 *
 * The sorted array is one flat row: every record is at the same depth,
 * so a non-empty store has height 1 and an empty one 0. When week 8
 * swaps in the tree this function returns the tree's real height
 * instead - callers only ever ask "how deep", never "which shape".
 */
int store_height(const store_t *s)
{
    if (s == NULL) return 0;
    if (s->n == 0) return 0;

    return 1;
}
