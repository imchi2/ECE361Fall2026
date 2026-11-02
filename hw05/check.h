/**
 * @file check.h
 * @brief The bonus: a static checker that looks at a transition table
 *        and reports the four ways it can be broken.
 *
 * fsm_check never runs the machine. It reads the table the way you
 * would on paper, which is the point: the behaviour of this lock lives
 * entirely in data, so the data can be inspected without executing it.
 *
 * The four problems it reports, one line each:
 *   1. an entry left out  - its action is A_UNDEFINED (the value a
 *                           cell keeps when nobody wrote it),
 *   2. a next state out of range - not one of LOCKED..OPEN,
 *   3. a state that cannot be reached from start - found by searching
 *                          breadth first over the table,
 *   4. a state with no way out - every event leaves you right there.
 *
 * @note Built with <tt>gcc -std=c11 -Wall -Wextra -g3 -O0</tt>, no warnings.
 */

#ifndef CHECK_H
#define CHECK_H

#include <stdio.h> /* FILE */
#include "lock.h"

/**
 * @brief Check a transition table for the four problems above.
 *
 * Example: <tt>fsm_check(lock_table, LOCKED, stdout)</tt> prints
 * nothing and returns 0 for the correct table, because every entry is
 * filled in, every next state is a real state, every state is reachable
 * from LOCKED, and every state has an event that leaves it.
 *
 * @param table The table to check. It is const because checking never
 *        changes it - fsm_check only reads.
 * @param start The state to measure reachability from: a state that
 *        cannot be reached from here is reported as problem 3.
 * @param out Where the problem lines go. Tests pass stdout; anything
 *        that is a valid FILE works (stderr, a file opened by the
 *        caller). Must not be NULL.
 * @return How many problems were found: 0 means the table is clean.
 */
int fsm_check(const transition_t table[N_STATES][N_EVENTS], state_t start, FILE *out);

#endif /* CHECK_H */
