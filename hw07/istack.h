/*
 * istack.h - a stack of int that grows on the heap.
 *
 * The stack is opaque: only istack.c knows what is inside struct istack.
 * Callers keep a pointer and pass it to every function.
 *
 * The name is istack_t and not stack_t because macOS already defines
 * stack_t in <pthread.h>, and that would not compile there.
 */
#ifndef ISTACK_H
#define ISTACK_H

#include <stdbool.h>
#include <stddef.h>

/* Room for this many values at first; the room doubles when it is full. */
#define ISTACK_START 8                /* room at first; doubles when full */

typedef struct istack istack_t;       /* opaque */

/*
 * istack_create - make a new, empty stack.
 * Takes: nothing.
 * Returns: a stack with room for ISTACK_START values, or NULL if memory
 *          ran out (nothing is leaked in that case).
 * Assumes: nothing; any caller may call it.
 */
istack_t *istack_create(void);

/*
 * istack_destroy - throw a stack away.
 * Takes: s, the stack to free, or NULL.
 * Returns: nothing.
 * Assumes: s was made by istack_create and is not used again afterwards.
 *          NULL does nothing.
 */
void istack_destroy(istack_t *s);

/*
 * istack_push - put one value on top of the stack.
 * Takes: s, the stack; v, the value to push.
 * Returns: 0 on success, or -1 if s is NULL or memory ran out.
 * Assumes: if the stack is full it first doubles its room with realloc,
 *          into a second pointer, so a failure leaves the stack unchanged.
 */
int istack_push(istack_t *s, int v);

/*
 * istack_pop - take the top value off the stack.
 * Takes: s, the stack; v, where to store the top value (v may be NULL).
 * Returns: 0 on success, or -1 if s is NULL or the stack is empty;
 *          in that case *v is left unchanged.
 * Assumes: nothing about the caller; the value is removed only on success.
 */
int istack_pop(istack_t *s, int *v);

/*
 * istack_peek - look at the top value without removing it.
 * Takes: s, the stack; v, where to store the top value (v may be NULL).
 * Returns: 0 on success, or -1 if s is NULL or the stack is empty;
 *          in that case *v is left unchanged.
 * Assumes: the stack is not changed in any way.
 */
int istack_peek(const istack_t *s, int *v);

/*
 * istack_count - how many values the stack holds right now.
 * Takes: s, the stack (may be NULL).
 * Returns: the number of values; 0 for NULL.
 * Assumes: nothing.
 */
size_t istack_count(const istack_t *s);

/*
 * istack_is_empty - whether the stack holds no values.
 * Takes: s, the stack (may be NULL).
 * Returns: true if empty, and also true for NULL.
 * Assumes: nothing.
 */
bool istack_is_empty(const istack_t *s);

/*
 * istack_capacity - how many values fit right now (the room, never less
 *                   than the count).
 * Takes: s, the stack (may be NULL).
 * Returns: the room in values; 0 for NULL.
 * Assumes: the room never shrinks, so it only grows by doubling.
 */
size_t istack_capacity(const istack_t *s);

/*
 * istack_to_array - copy the values out, bottom first.
 * Takes: s, the stack; n, where to store how many values there are.
 * Returns: a new array the caller owns and must free, with the bottom
 *          value first; NULL and *n = 0 if s is NULL or the stack is
 *          empty, or if memory ran out.
 * Assumes: the array is a copy, so changing it does not touch the stack.
 *          If n is NULL, nothing can be stored, so NULL is returned.
 */
int *istack_to_array(const istack_t *s, size_t *n);

#endif /* ISTACK_H */
