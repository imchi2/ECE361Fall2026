/**
 * @file lock.h
 * @brief The combination lock as a finite state machine: the three
 *        enums (states, events, actions), the transition struct, the
 *        4x4 transition table, and the five functions that drive it.
 *
 * The lock is a Mealy machine: the action belongs to the transition
 * (state + event), not to the state on its own. The same state can beep,
 * unlock, lock or do nothing depending on which event arrives, which is
 * why transition_t carries an action next to the next state.
 *
 * The code is 3-1-4, typed on a keypad:
 *   - a right digit moves the machine one step forward,
 *   - a wrong digit beeps and starts over from the beginning,
 *   - a 5-second timer running out starts over silently,
 *   - the door closing (or the timer) while open locks it again.
 *
 * Everything about how the lock behaves lives in lock_table in lock.c.
 * The functions here only read that table; none of them contains a
 * chain of ifs about the state.
 *
 * The types below are exactly the ones the homework spec gives us,
 * wording and member names unchanged.
 *
 * @note Built with <tt>gcc -std=c11 -Wall -Wextra -g3 -O0</tt>, no warnings.
 */

#ifndef LOCK_H
#define LOCK_H

/* ------------------------------------------------------------------ *
 * The three enums, exactly as the spec lists them.                    *
 *                                                                     *
 * Each one ends in an N_* member that is not a real value: it sits    *
 * right after the last real one, so it doubles as the count of the    *
 * real values before it (N_STATES is 4: LOCKED, GOT1, GOT2, OPEN)    *
 * and as the size of the table's rows and columns. Range checks      *
 * compare against it, so adding a state later only means editing      *
 * this line and the table.                                            *
 * ------------------------------------------------------------------ */

/** @brief Where the lock is: waiting for digit 1, digit 2, and so on. */
typedef enum { LOCKED, GOT1, GOT2, OPEN, N_STATES } state_t;

/**
 * @brief What just happened to the lock.
 *
 * E_RIGHT means "the digit typed is the next digit of the code",
 * E_WRONG means "it is not". The door and the timer are events too,
 * so the machine sees the same kind of input from every source.
 */
typedef enum { E_RIGHT, E_WRONG, E_CLOSE, E_TIMEOUT, N_EVENTS } event_t;

/**
 * @brief What the lock does on a step.
 *
 * A_UNDEFINED is 0 on purpose: it is the value a table cell keeps when
 * nobody wrote one, so "action == A_UNDEFINED" is how fsm_check (the
 * bonus) spots a forgotten entry. Nothing in a correct table uses it.
 */
typedef enum { A_UNDEFINED, A_NONE, A_UNLOCK, A_LOCK, A_BEEP, N_ACTIONS } action_t;

/** @brief One cell of the table: where the machine goes, what it does. */
typedef struct { state_t next; action_t action; } transition_t;

/**
 * @brief The whole behaviour of the lock, as data.
 *
 * Rows are states (LOCKED to OPEN), columns are events (E_RIGHT to
 * E_TIMEOUT), so lock_table[GOT2][E_RIGHT] is "in GOT2, a right digit
 * gives this transition". Every one of the 16 cells is written with a
 * designated initializer in lock.c.
 *
 * It is const: nothing in this program ever changes the rules, only the
 * current state.
 */
extern const transition_t lock_table[N_STATES][N_EVENTS];

/**
 * @brief Take one step of the machine: look the transition up and report it.
 *
 * This is the whole point of the homework: one table lookup, no if and
 * no switch on the state. The entry itself says where to go and what
 * to do, so lock_step never needs to know what a state means.
 *
 * Example: <tt>lock_step(GOT2, E_RIGHT, &a)</tt> sets a to A_UNLOCK
 * and returns OPEN.
 *
 * @param s The state the lock is in now.
 * @param e The event that just happened.
 * @param action Where to write what the lock should do. The caller
 *        passes the address of an action_t; it must not be NULL.
 * @return The state the lock moves to.
 * @note No range check on purpose: the spec asks for exactly one
 *       lookup, and every caller in this homework passes real enum
 *       values. The bonus checker exists to catch bad tables instead.
 */
state_t lock_step(state_t s, event_t e, action_t *action);

/**
 * @brief Turn one input character into an event, for the given state.
 *
 * The rules, in order:
 *   - 'c' is the door closing and 't' the timer running out, in any state;
 *   - a digit is E_RIGHT when it is the next digit of the code for this
 *     state ('3' in LOCKED, '1' in GOT1, '4' in GOT2), otherwise E_WRONG.
 *     While open the code is complete, so no digit is "next" and every
 *     digit reports E_WRONG - the table makes that "do nothing";
 *   - anything else (letters, space, newline) returns -1: not an event.
 *
 * @param s The state the lock is in now.
 * @param c The character read from input.
 * @return An event value 0..N_EVENTS-1, or -1 when c is not an event.
 * @note An out-of-range state also returns -1: there is no "next digit"
 *       to ask about, so there is no event to report.
 */
int lock_event(state_t s, char c);

/**
 * @brief The name of a state, for printing: "LOCKED", "GOT1", ...
 * @param s The state to name.
 * @return Its name, or "?" when s is out of range (never read past the
 *         end of the name array).
 */
const char *state_name(state_t s);

/**
 * @brief The name of an event, for printing: "RIGHT", "WRONG", ...
 * @param e The event to name.
 * @return Its name, or "?" when e is out of range.
 */
const char *event_name(event_t e);

/**
 * @brief The name of an action, for printing: "BEEP", "UNLOCK", ...
 * @param a The action to name.
 * @return Its name, or "?" when a is out of range.
 */
const char *action_name(action_t a);

#endif /* LOCK_H */
