/*
 * istack.c - the stack of int that grows as it fills.
 *
 * The whole struct lives here and nowhere else: istack.h only says
 * "typedef struct istack istack_t", so callers can hold a pointer but
 * cannot look inside.
 *
 * The values live in ONE array on the heap. The room never shrinks: it
 * starts at ISTACK_START and doubles whenever it is full. Every malloc
 * has its free, and realloc goes into a second pointer first, so a
 * failure loses nothing.
 */
#include <stdlib.h>

#include "istack.h"

struct istack {
    int   *data;      /* the values, bottom first; one array on the heap */
    size_t count;     /* how many values are held right now */
    size_t capacity;  /* how many fit right now; only ever grows */
};

/*
 * istack_create - make a new, empty stack (see istack.h).
 *
 * Two allocations, so they are checked one at a time: if the array fails
 * the stack struct is freed first, because a NULL from malloc means
 * nothing was allocated and nothing may be leaked.
 */
istack_t *istack_create(void)
{
    istack_t *s;

    s = malloc(sizeof *s);
    if (s == NULL) {
        /* Memory ran out: return NULL and leak nothing. */
        return NULL;
    }

    s->data = malloc(ISTACK_START * sizeof *s->data);
    if (s->data == NULL) {
        /* The array failed: free the struct too, or it would leak. */
        free(s);
        return NULL;
    }

    /* A new stack holds nothing yet, with room for ISTACK_START values. */
    s->count = 0;
    s->capacity = ISTACK_START;

    return s;
}

/*
 * istack_destroy - free the stack and everything it holds (see istack.h).
 *
 * NULL does nothing: free(NULL) is legal in C, and calling this with
 * NULL must not crash.
 */
void istack_destroy(istack_t *s)
{
    if (s == NULL) {
        return;
    }

    /* Free the values array first, then the struct that points at it. */
    free(s->data);
    free(s);
}

/*
 * istack_push - put v on top (see istack.h).
 *
 * The validation runs BEFORE anything else, so a NULL s never reaches
 * the field accesses below it.
 */
int istack_push(istack_t *s, int v)
{
    if (s == NULL) {
        /* Nothing to push onto: report failure, change nothing. */
        return -1;
    }

    if (s->count == s->capacity) {
        /* Full: double the room. realloc goes into a SECOND pointer, so
         * that if it fails, s->data still points at the old array and
         * every value is still there. */
        size_t bigger;
        int *grown;

        bigger = s->capacity * 2;
        grown = realloc(s->data, bigger * sizeof *s->data);
        if (grown == NULL) {
            /* Memory ran out: the stack is unchanged, values and all. */
            return -1;
        }

        /* Success: only now hand the new array to the stack. */
        s->data = grown;
        s->capacity = bigger;
    }

    /* There is room now: store v just past the last value. */
    s->data[s->count] = v;
    s->count = s->count + 1;

    return 0;
}

/*
 * istack_pop - take the top value off (see istack.h).
 *
 * Everything is checked before *v is touched, so an error leaves *v
 * unchanged exactly as the interface promises.
 */
int istack_pop(istack_t *s, int *v)
{
    if (s == NULL) {
        /* No stack: fail before writing to v. */
        return -1;
    }

    if (s->count == 0) {
        /* Empty: nothing to give, so *v stays as it was. */
        return -1;
    }

    /* One less value: the top was at count - 1. */
    s->count = s->count - 1;

    /* v may be NULL: the caller may only care that a value was removed. */
    if (v != NULL) {
        *v = s->data[s->count];
    }

    return 0;
}

/*
 * istack_peek - look at the top value (see istack.h).
 *
 * Same checks as pop, but count is never changed: peeking leaves the
 * stack exactly as it was.
 */
int istack_peek(const istack_t *s, int *v)
{
    if (s == NULL) {
        /* No stack: fail before writing to v. */
        return -1;
    }

    if (s->count == 0) {
        /* Empty: *v is left unchanged. */
        return -1;
    }

    /* v may be NULL: just checking the top is enough for some callers. */
    if (v != NULL) {
        *v = s->data[s->count - 1];
    }

    return 0;
}

/*
 * istack_count - how many values are held (see istack.h).
 *
 * The NULL check comes first, because ->count would crash on NULL.
 */
size_t istack_count(const istack_t *s)
{
    if (s == NULL) {
        /* The interface says NULL counts as nothing held. */
        return 0;
    }

    return s->count;
}

/*
 * istack_is_empty - whether nothing is held (see istack.h).
 *
 * NULL counts as empty: the answer "no values in it" is true for a
 * stack that does not exist, and it keeps callers from crashing.
 */
bool istack_is_empty(const istack_t *s)
{
    if (s == NULL) {
        return true;
    }

    return s->count == 0;
}

/*
 * istack_capacity - the room right now (see istack.h).
 *
 * Room never shrinks, so this is only ever ISTACK_START, then twice
 * that, and so on.
 */
size_t istack_capacity(const istack_t *s)
{
    if (s == NULL) {
        /* No stack means no room. */
        return 0;
    }

    return s->capacity;
}

/*
 * istack_to_array - copy the values out, bottom first (see istack.h).
 *
 * The caller owns the returned array and must free it. A copy is made
 * so that changing the array cannot damage the stack.
 */
int *istack_to_array(const istack_t *s, size_t *n)
{
    int *copy;
    size_t i;

    /* The number goes into *n, so n must exist before anything else. */
    if (n == NULL) {
        return NULL;
    }

    /* Default answer: no array and no values. */
    *n = 0;

    if (s == NULL) {
        /* No stack: NULL and *n = 0, as the interface says. */
        return NULL;
    }

    if (s->count == 0) {
        /* Empty: also NULL and *n = 0; nothing to copy. */
        return NULL;
    }

    copy = malloc(s->count * sizeof *copy);
    if (copy == NULL) {
        /* Memory ran out: NULL and *n = 0, and nothing leaks. */
        return NULL;
    }

    /* Copy bottom first: data[0] is the bottom, so index i stays i. */
    i = 0;
    while (i < s->count) {
        copy[i] = s->data[i];
        i = i + 1;
    }

    *n = s->count;

    return copy;
}
