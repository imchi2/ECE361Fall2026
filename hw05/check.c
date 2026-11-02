/**
 * @file check.c
 * @brief Implementation of fsm_check: four passes over the table, one
 *        problem line per finding, and a count returned to the caller.
 *
 * The four passes are deliberately separate loops instead of one loop
 * that does everything: each pass answers exactly one question, so you
 * can read (and test) them one at a time. A single cell can appear in
 * more than one pass - a forgotten cell in GOT2 also makes OPEN
 * unreachable - and both facts are worth reporting.
 *
 * Written for people learning C: plain control flow, no feature newer
 * than C99, long expressions split across lines, and a comment on every
 * decision.
 */

#include "check.h"

#include "lock.h"

#include <stdio.h> /* fprintf, FILE */

int fsm_check(const transition_t table[N_STATES][N_EVENTS], state_t start, FILE *out)
{
    /* How many problems we have printed so far. This is the return
       value, so the caller can assert on it without parsing output. */
    int problems = 0;

    /* Loop counters. Plain ints: they count array positions, not
       states, so they never need to be enum values. */
    int s;
    int e;

    /* ---------------------------------------------------------------- *
     * Problem 1: an entry left out.                                    *
     *                                                                    *
     * A cell nobody wrote keeps the value C gives it: {0, 0}, and 0 is  *
     * A_UNDEFINED. So "the action is A_UNDEFINED" is the fingerprint of *
     * a missing entry. The next state of such a cell is LOCKED (also 0),*
     * which looks plausible - that is exactly why the action is what we *
     * test for.                                                        *
     * ---------------------------------------------------------------- */
    for (s = 0; s < N_STATES; s++)
    {
        for (e = 0; e < N_EVENTS; e++)
        {
            if (table[s][e].action == A_UNDEFINED)
            {
                fprintf(out,
                        "left out: [%s][%s] was never written "
                        "(action A_UNDEFINED)\n",
                        state_name((state_t)s),
                        event_name((event_t)e));
                problems = problems + 1;
            }
        }
    }

    /* ---------------------------------------------------------------- *
     * Problem 2: a next state out of range.                            *
     *                                                                    *
     * lock_step would hand this value straight back as the new state,   *
     * and the next call would index lock_table with it - reading past  *
     * the end of the array, which is undefined behaviour in C. So a     *
     * number outside 0..N_STATES-1 is a real bug, not a style issue.   *
     * ---------------------------------------------------------------- */
    for (s = 0; s < N_STATES; s++)
    {
        for (e = 0; e < N_EVENTS; e++)
        {
            /* Cast to int first so the comparison is signed, same as
               in lock.c's name functions. */
            int next = (int)table[s][e].next;

            if (next < 0 ||
                next >= N_STATES)
            {
                fprintf(out,
                        "out of range: [%s][%s].next is %d, "
                        "which is not a state\n",
                        state_name((state_t)s),
                        event_name((event_t)e),
                        next);
                problems = problems + 1;
            }
        }
    }

    /* ---------------------------------------------------------------- *
     * Problem 3: a state that cannot be reached from start.            *
     *                                                                    *
     * Breadth first: start at `start`, follow every next state that is *
     * in range, and mark what you touch. Anything still unmarked at the*
     * end can never happen - the lock can never get there, so a state  *
     * like OPEN would mean the door can never open.                    *
     *                                                                    *
     * A queue of N_STATES entries is enough because a state is only     *
     * enqueued the first time it is visited: at most one visit each.   *
     * ---------------------------------------------------------------- */
    int visited[N_STATES];
    state_t queue[N_STATES];
    int head;
    int tail;
    int start_index;

    for (s = 0; s < N_STATES; s++)
    {
        visited[s] = 0;
    }

    head = 0;
    tail = 0;

    /* Only a real state may be the starting point. If start is out of
       range, nothing gets marked and every state is reported below -
       a noisy but honest answer, and the tests always pass a real one. */
    start_index = (int)start;

    if (start_index >= 0 &&
        start_index < N_STATES)
    {
        visited[start_index] = 1;
        queue[tail] = start;
        tail = tail + 1;
    }

    while (head < tail)
    {
        state_t current = queue[head];

        head = head + 1;

        for (e = 0; e < N_EVENTS; e++)
        {
            state_t next = table[current][e].next;
            int next_index = (int)next;

            /* Follow the edge only when it points at a real state. An
               out-of-range one is problem 2's business; following it
               here would index visited[] out of bounds. */
            if (next_index >= 0 &&
                next_index < N_STATES)
            {
                if (visited[next_index] == 0)
                {
                    visited[next_index] = 1;
                    queue[tail] = next;
                    tail = tail + 1;
                }
            }
        }
    }

    for (s = 0; s < N_STATES; s++)
    {
        if (visited[s] == 0)
        {
            fprintf(out,
                    "unreachable: %s cannot be reached from %s\n",
                    state_name((state_t)s),
                    state_name(start));
            problems = problems + 1;
        }
    }

    /* ---------------------------------------------------------------- *
     * Problem 4: a state with no way out.                              *
     *                                                                    *
     * Every one of its four events points straight back at itself, so   *
     * once the machine enters it, it is stuck forever. For this lock,   *
     * OPEN with no way out would mean the door can never be locked     *
     * again - a security bug, not a cosmetic one.                      *
     *                                                                    *
     * An entry that points somewhere else counts as a way out even if  *
     * that entry is also reported above (left out or out of range):   *
     * each pass tells the truth about its own question only.          *
     * ---------------------------------------------------------------- */
    for (s = 0; s < N_STATES; s++)
    {
        int leaves = 0;

        for (e = 0; e < N_EVENTS; e++)
        {
            if (table[s][e].next != (state_t)s)
            {
                /* One event leaves this state is enough; no need to
                   keep looking. */
                leaves = 1;
                break;
            }
        }

        if (leaves == 0)
        {
            fprintf(out,
                    "no way out: every event from %s stays in %s\n",
                    state_name((state_t)s),
                    state_name((state_t)s));
            problems = problems + 1;
        }
    }

    return problems;
}
