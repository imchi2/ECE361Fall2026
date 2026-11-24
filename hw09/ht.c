/*
 * ht.c - a counting hash table with separate chaining.
 *
 * Both structs are defined HERE and nowhere else, so ht.h can keep the
 * table opaque and nothing outside this file can touch a chain node.
 * Nothing is global: every function is given the table it works on.
 *
 * The shape of the table:
 *
 *     struct ht
 *     +-----------+     buckets
 *     | *buckets -+---> [0] -> node -> node -> NULL
 *     | nbuckets  |     [1] -> NULL
 *     | size      |     [2] -> node -> NULL
 *     +-----------+      ...
 *
 * `size` counts the different keys, i.e. the number of nodes in all
 * the chains together. It is kept up to date by ht_add, so ht_size
 * does not have to walk anything.
 */
#include <stdlib.h>

#include "ht.h"

/* One chain node: a key, how often it was added, and the next node. */
struct ht_node {
    int              key;     /* the key this node counts */
    size_t           count;   /* how many times key was added */
    struct ht_node  *next;    /* next node in the same bucket, or NULL */
};

/* The table itself: the bucket array plus its two sizes. */
struct ht {
    struct ht_node **buckets;  /* array of nbuckets chain heads */
    size_t           nbuckets; /* how long that array is (never 0) */
    size_t           size;     /* how many different keys are stored */
};

/*
 * ht_create - make a new, empty table.
 *
 * The table struct is allocated first, then the bucket array. If the
 * array cannot be allocated, the table struct is freed again before
 * returning NULL, so a failed create leaks nothing.
 */
ht_t *ht_create(size_t nbuckets)
{
    ht_t   *t;
    size_t  i;

    /* A table with no buckets could never store a key: reject it. */
    if (nbuckets == 0) {
        return NULL;
    }

    t = malloc(sizeof *t);
    if (t == NULL) {
        return NULL;
    }

    /*
     * One pointer per bucket. malloc does not clear the array, so the
     * loop below sets every slot to NULL ("this chain is empty").
     */
    t->buckets = malloc(nbuckets * sizeof(struct ht_node *));
    if (t->buckets == NULL) {
        free(t);
        return NULL;
    }

    for (i = 0; i < nbuckets; i++) {
        t->buckets[i] = NULL;
    }

    t->nbuckets = nbuckets;
    t->size = 0;

    return t;
}

/*
 * ht_destroy - free every node, the bucket array, and the table.
 *
 * Each bucket is one linked list, so it is walked with the usual
 * "remember next before free" pattern: after free(current) the node's
 * memory may not be read again, so `next` was saved first.
 */
void ht_destroy(ht_t *t)
{
    size_t          i;
    struct ht_node *current;
    struct ht_node *next;

    if (t == NULL) {
        return;
    }

    for (i = 0; i < t->nbuckets; i++) {
        current = t->buckets[i];
        while (current != NULL) {
            next = current->next;
            free(current);
            current = next;
        }
    }

    free(t->buckets);
    free(t);
}

/*
 * ht_add - count one more of key.
 *
 * First the key's bucket is computed, then its chain is searched. An
 * existing node just has its count increased and no memory is touched,
 * so that case can never fail. A key that is not there yet needs one
 * new node; if malloc fails we return -1 with the table untouched.
 */
int ht_add(ht_t *t, int key)
{
    size_t           b;
    struct ht_node  *current;
    struct ht_node  *node;

    if (t == NULL) {
        return -1;
    }

    /*
     * The hash of a key. Keys are never negative, so the remainder is
     * never negative; casting to size_t keeps any future slip from
     * turning into a huge, out-of-range index.
     */
    b = (size_t)key % t->nbuckets;

    /* Look for key in its chain. */
    current = t->buckets[b];
    while (current != NULL) {
        if (current->key == key) {
            current->count = current->count + 1;
            return 0;
        }
        current = current->next;
    }

    /* Not found: a new key needs a new node. */
    node = malloc(sizeof *node);
    if (node == NULL) {
        return -1;
    }

    node->key = key;
    node->count = 1;

    /*
     * Put the new node at the FRONT of the chain. The old head (which
     * may be NULL) becomes the new node's next, and the bucket points
     * at the new node. This is O(1) and needs no tail pointer.
     */
    node->next = t->buckets[b];
    t->buckets[b] = node;

    /*
     * A brand new key makes the table one key bigger. Duplicates never
     * get here, so the size counts different keys, as its name says.
     */
    t->size = t->size + 1;

    return 0;
}

/*
 * ht_count - how many times key was added.
 *
 * Same walk as in ht_add, but there is no memory to allocate: a key
 * that is missing simply has count 0.
 */
size_t ht_count(const ht_t *t, int key)
{
    size_t               b;
    const struct ht_node *current;

    if (t == NULL) {
        return 0;
    }

    b = (size_t)key % t->nbuckets;

    current = t->buckets[b];
    while (current != NULL) {
        if (current->key == key) {
            return current->count;
        }
        current = current->next;
    }

    return 0;
}

/*
 * ht_size - how many different keys the table holds.
 *
 * The count is maintained by ht_add, so this is a single lookup and
 * does not walk the chains.
 */
size_t ht_size(const ht_t *t)
{
    if (t == NULL) {
        return 0;
    }

    return t->size;
}

/*
 * ht_longest_chain - the number of nodes in the longest bucket chain.
 *
 * Every bucket is walked and its length remembered; the largest length
 * seen wins. An empty table has no nodes at all, so the answer starts
 * at 0 and stays 0.
 */
size_t ht_longest_chain(const ht_t *t)
{
    size_t               i;
    size_t               length;
    size_t               longest;
    const struct ht_node *current;

    if (t == NULL) {
        return 0;
    }

    longest = 0;

    for (i = 0; i < t->nbuckets; i++) {
        length = 0;
        current = t->buckets[i];
        while (current != NULL) {
            length = length + 1;
            current = current->next;
        }

        /*
         * Strictly greater: on a tie the earlier bucket's length is
         * kept, which is fine because both lengths are equal.
         */
        if (length > longest) {
            longest = length;
        }
    }

    return longest;
}
