/*
 * fsm.h: ECE 361, Fall 2026, project - the state machine's interface.
 *
 * Scaffolding step (week 3/4): only the states live here for now,
 * because store.h has to name "the state at that moment" inside a
 * record. The events, the transition table, and fsm_step arrive with
 * week 5's work.
 *
 * The four states are given by the project sheet, in this order on
 * purpose: the enum values 0..3 are exactly the 2-bit state field of
 * the status word, and they also set the order records are grouped in
 * the store (LOCKED records first, then UNLOCKED, and so on).
 */
#ifndef FSM_H
#define FSM_H

typedef enum {
    ST_LOCKED = 0, /* arm locked; the initial state */
    ST_UNLOCKED,   /* paid, arm free for one passage */
    ST_PASSING,    /* someone is going through; lasts 3 ticks */
    ST_VIOLATION   /* pushed while locked; alarm sounding */
} turnstile_state_t;

#endif /* FSM_H */
