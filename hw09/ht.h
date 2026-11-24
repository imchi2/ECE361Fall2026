/*
 * ht.h - a hash table that counts how often each key was added.
 *
 * The table is opaque: only ht.c knows what is inside struct ht and
 * what a chain node looks like. Callers keep an ht_t pointer and pass
 * it to every function, so any number of tables can exist at once.
 *
 * The table is a separate-chaining hash table. The bucket array has
 * nbuckets slots; each slot holds the first node of a linked list of
 * the keys that hash to that slot. The hash of a key is
 *
 *     key % nbuckets
 *
 * Keys are never negative, so the remainder is never negative either
 * and always names a real bucket.
 *
 * Each chain node remembers one key and how many times that key was
 * added. A key is stored once, no matter how often it is added; only
 * its count goes up. A key seen for the first time gets a new node at
 * the front of its bucket's chain, with count 1.
 */
#ifndef HT_H
#define HT_H

#include <stddef.h>

typedef struct ht ht_t;            /* opaque */

/*
 * ht_create - make a new, empty table.
 * Takes: nbuckets, how many buckets the bucket array should have.
 * Returns: a new table whose nbuckets chains are all empty, or NULL if
 *          nbuckets is 0 or memory runs out. Nothing is leaked when it
 *          returns NULL.
 * Assumes: nothing; any caller may call it.
 */
ht_t   *ht_create(size_t nbuckets);

/*
 * ht_destroy - free every chain node, the bucket array, and the table.
 * Takes: t, the table to free, or NULL.
 * Returns: nothing.
 * Assumes: t was made by ht_create and is not used again afterwards.
 *          NULL does nothing.
 */
void    ht_destroy(ht_t *t);

/*
 * ht_add - count one more of key.
 * Takes: t, the table, or NULL; key, the key to add (never negative).
 * Returns: 0 on success; -1 if t is NULL or a new node cannot be
 *          allocated. In both failure cases the table is unchanged.
 * Assumes: key is never negative. If key is already in its bucket's
 *          chain its count goes up by 1; otherwise a new node with
 *          count 1 is put at the FRONT of that chain.
 */
int     ht_add(ht_t *t, int key);

/*
 * ht_count - how many times key was added.
 * Takes: t, the table, or NULL; key, the key to look for.
 * Returns: the count stored for key, or 0 if key was never added or t
 *          is NULL.
 * Assumes: key is never negative, and the same hash (key % nbuckets)
 *          is used here as in ht_add.
 */
size_t  ht_count(const ht_t *t, int key);

/*
 * ht_size - how many different keys the table holds.
 * Takes: t, the table, or NULL.
 * Returns: the number of distinct keys; 0 if the table is empty or t
 *          is NULL.
 * Assumes: nothing.
 */
size_t  ht_size(const ht_t *t);

/*
 * ht_longest_chain - the length of the longest bucket chain.
 * Takes: t, the table, or NULL.
 * Returns: the number of nodes in the longest chain; 0 if the table is
 *          empty or t is NULL.
 * Assumes: nothing; each node of each chain is visited once.
 */
size_t  ht_longest_chain(const ht_t *t);

#endif /* HT_H */
