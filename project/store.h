/*
 * store.h: ECE 361, Fall 2026, project - the record store's interface.
 *
 * The store keeps one record per tick of the device's life, ordered by
 * key K3: by state first, then by timestamp. Two things hide behind
 * this header:
 *
 *  - record_t is visible on purpose. report.c has to print "the tick,
 *    the reading, and the change" while reading the store through this
 *    header only, so the record's fields cannot be a secret.
 *
 *  - store_t is opaque. store.h says only "typedef struct store
 *    store_t;" and only store.c completes the struct. Right now the
 *    inside is a sorted array, the way tally.c in HW4 keeps its names.
 *    In week 8 the inside becomes a binary search tree, and because no
 *    caller has ever looked inside, nothing outside store.c changes.
 *
 * Ordering rule, written once in record_compare and obeyed everywhere:
 * state first, timestamp second, exactly like sorting a stack of
 * records by state and then by time within each state.
 */
#ifndef STORE_H
#define STORE_H

#include <stdint.h>
#include <time.h>

#include "fsm.h"    /* turnstile_state_t: the state field of a record */
#include "sensor.h" /* reading_t: the reading field of a record */

/*
 * STORE_MAX:      how many stores can exist at once (a fixed pool,
 *                 because malloc is week 7).
 * STORE_RECORDS:  how many records one store can hold. The tests
 *                 insert 100 records, so this must be comfortably
 *                 above that; 256 covers every scenario on the sheet.
 */
#define STORE_MAX     4
#define STORE_RECORDS 256

/*
 * record_t: one tick of the device's life, as the store keeps it.
 *
 * The three things the project sheet names, and nothing more:
 * what the sensor saw, what the machine was doing, and the packed
 * status word for that tick.
 */
typedef struct record {
    reading_t         reading; /* the timestamp, temperature, humidity */
    turnstile_state_t state;   /* which of the four states this tick reports */
    uint32_t          status;  /* packed: state, UNLOCK, ALARM, credit */
} record_t;

/*
 * record_compare - the ordering function, on the key K3.
 *
 * Takes:   a and b, two records to order (never NULL).
 * Returns: a negative number when a comes before b, 0 when they are
 *          equal, a positive number when b comes before a - like
 *          strcmp, so the same "< 0 means first" habit carries over.
 * Assumes: only two fields decide the order: state first, timestamp
 *          second. The temperature, humidity, and status are ignored,
 *          so two records with the same state and timestamp compare
 *          equal even if their readings differ.
 */
int record_compare(const record_t *a, const record_t *b);

/*
 * store_t: the opaque container. The inside lives in store.c only.
 */
typedef struct store store_t;

/*
 * store_visit_fn: the callback an in-order walk calls, once per record.
 *
 * Takes:   rec, the record being visited (read-only); arg, whatever the
 *          caller passed to store_walk, forwarded untouched - this is
 *          how a visitor carries its own variables without globals.
 */
typedef void (*store_visit_fn)(const record_t *rec, void *arg);

/*
 * store_create - a new, empty store.
 *
 * Returns: a pointer to a fresh store, or NULL when STORE_MAX stores
 *          are already in use.
 * Assumes: the caller will hand it back with store_destroy.
 */
store_t *store_create(void);

/*
 * store_destroy - give the store back to the pool.
 *
 * Takes:   s, a store from store_create; NULL does nothing.
 * Returns: nothing.
 * Assumes: after this call the pointer must not be used again.
 */
void store_destroy(store_t *s);

/*
 * store_insert - put one record into the store, in key order.
 *
 * Takes:   s, the store; rec, the record to copy in.
 * Returns: 0 when the record was stored; nonzero when nothing was
 *          done: s or rec was NULL, the store already holds
 *          STORE_RECORDS records, or a record with the same key
 *          (same state, same timestamp) is already in the store -
 *          keys stay unique so that store_find has one answer.
 * Assumes: the record is copied; the caller may reuse or discard its
 *          own copy afterwards.
 */
int store_insert(store_t *s, const record_t *rec);

/*
 * store_find - look up a record by its key, state plus timestamp.
 *
 * Takes:   s, the store to search; state and stamp, the two halves of
 *          key K3.
 * Returns: a pointer to the stored record, or NULL when there is no
 *          such record (or s is NULL).
 * Assumes: the pointer belongs to the store - it stays valid until
 *          store_destroy, and the caller must not write through it.
 */
const record_t *store_find(const store_t *s, turnstile_state_t state, time_t stamp);

/*
 * store_walk - visit every record in key order.
 *
 * Takes:   s, the store; fn, the callback; arg, passed on to fn
 *          unchanged.
 * Returns: nothing.
 * Assumes: fn is called once per record, in key order - state first,
 *          then timestamp. With the sorted array (and later the tree)
 *          that order is automatic; the walk never sorts anything
 *          itself. fn must not insert into or destroy the store while
 *          the walk is running. A NULL s or NULL fn does nothing.
 */
void store_walk(const store_t *s, store_visit_fn fn, void *arg);

/*
 * store_count - how many records the store holds.
 *
 * Returns: the number of records, 0 when the store is empty or s is
 *          NULL.
 */
int store_count(const store_t *s);

/*
 * store_height - the height of the store's inside structure.
 *
 * Returns: 0 for an empty store or NULL; 1 for a non-empty store while
 *          the inside is the sorted array (every record sits in one
 *          flat row). After week 8 this returns the real height of the
 *          binary search tree, so callers should treat it as "how deep
 *          did the walk have to dig", not as a count.
 */
int store_height(const store_t *s);

#endif /* STORE_H */
