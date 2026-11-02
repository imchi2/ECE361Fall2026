/**
 * @file test_lock.c
 * @brief Test program for homework 5: every one of the 16 table
 *        entries, lock_event in every state, the four spec sequences,
 *        and the name functions.
 *
 * What is covered, in order:
 *   1. all 16 entries of lock_table, checked against a second copy of
 *      the table written out in this file,
 *   2. lock_event: a right and a wrong digit in each state, digits
 *      while open, 'c' and 't' in every state, and other characters,
 *   3. lock_event on an out-of-range state,
 *   4. state_name / event_name / action_name, including the "?" cases,
 *   5. the four sequences the spec names: 3 1 4 opens, 3 3 1 4 does
 *      not, 3 1 t 4 does not, and 3 1 4 c locks again.
 *
 * How to read the output: each check prints PASS, or FAIL with what was
 * expected and what actually came back. The summary counts them all.
 * The program returns 0 when everything passed and 1 otherwise, so
 * `make test` fails loudly if any check failed. A FAIL does not stop
 * the run: each check reports on its own, so one broken entry cannot
 * hide the other fifteen.
 *
 * Build and run, from hw05/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make test"
 */

#include "lock.h"

#include <stdio.h>  /* printf */
#include <string.h> /* strcmp - plain standard C, used for the names */

/* ------------------------------------------------------------------ *
 * The expected table, written out a second time on purpose.           *
 *                                                                     *
 * If the test compared lock_table against itself, a wrong entry would *
 * agree with itself and pass. These arrays are the spec's answer key, *
 * transcribed from the state diagram, so a change to lock.c that      *
 * breaks the lock fails here.                                         *
 *                                                                     *
 * Designated initializers ([LOCKED] = { ... }) mean the cells cannot  *
 * land in the wrong column: each one names its own event.            *
 * ------------------------------------------------------------------ */

/** @brief Where each (state, event) pair must go. */
static const state_t expected_next[N_STATES][N_EVENTS] =
{
    [LOCKED] = { [E_RIGHT] = GOT1,   [E_WRONG] = LOCKED,
                 [E_CLOSE] = LOCKED, [E_TIMEOUT] = LOCKED },
    [GOT1]   = { [E_RIGHT] = GOT2,   [E_WRONG] = LOCKED,
                 [E_CLOSE] = GOT1,   [E_TIMEOUT] = LOCKED },
    [GOT2]   = { [E_RIGHT] = OPEN,   [E_WRONG] = LOCKED,
                 [E_CLOSE] = GOT2,   [E_TIMEOUT] = LOCKED },
    [OPEN]   = { [E_RIGHT] = OPEN,   [E_WRONG] = OPEN,
                 [E_CLOSE] = LOCKED, [E_TIMEOUT] = LOCKED }
};

/** @brief What each (state, event) pair must do. Never A_UNDEFINED. */
static const action_t expected_action[N_STATES][N_EVENTS] =
{
    [LOCKED] = { [E_RIGHT] = A_NONE,   [E_WRONG] = A_BEEP,
                 [E_CLOSE] = A_NONE,   [E_TIMEOUT] = A_NONE },
    [GOT1]   = { [E_RIGHT] = A_NONE,   [E_WRONG] = A_BEEP,
                 [E_CLOSE] = A_NONE,   [E_TIMEOUT] = A_NONE },
    [GOT2]   = { [E_RIGHT] = A_UNLOCK, [E_WRONG] = A_BEEP,
                 [E_CLOSE] = A_NONE,   [E_TIMEOUT] = A_NONE },
    [OPEN]   = { [E_RIGHT] = A_NONE,   [E_WRONG] = A_NONE,
                 [E_CLOSE] = A_LOCK,   [E_TIMEOUT] = A_LOCK }
};

/* The same names the program prints, kept here so the FAIL messages
   read like the program's output instead of like raw numbers. */
static const char *expected_state_names[N_STATES] =
{
    "LOCKED", "GOT1", "GOT2", "OPEN"
};

static const char *expected_event_names[N_EVENTS] =
{
    "RIGHT", "WRONG", "CLOSE", "TIMEOUT"
};

static const char *expected_action_names[N_ACTIONS] =
{
    "UNDEFINED", "NONE", "UNLOCK", "LOCK", "BEEP"
};

/* The three states that are waiting for a digit, with the digit each
   one wants and a digit each one must reject. Note '3' in wrong_digits
   for GOT1 and GOT2: typing 3 3 1 4 never opens because the second 3
   is wrong in GOT1 - the exact case the Shine question asks about. */
static const state_t typing_states[3] = { LOCKED, GOT1, GOT2 };

static const char right_digits[3] = { '3', '1', '4' };

static const char wrong_digits[3] = { '1', '3', '3' };

/**
 * @brief Feed a string of characters to the lock, starting in LOCKED.
 *
 * This is the same loop main.c runs, shrunk down: characters that are
 * not events are skipped, every event steps the machine, and the state
 * carries from one character to the next.
 *
 * @param chars The characters to type, e.g. "314". Not a real keypad
 *        sequence of numbers - just characters in order.
 * @param last_action Where to write the action of the last step. It
 *        stays A_UNDEFINED if the string produced no event at all.
 * @return The state the lock ended in.
 */
static state_t run_sequence(const char *chars, action_t *last_action)
{
    state_t state = LOCKED;
    action_t action = A_UNDEFINED;
    int i;

    for (i = 0; chars[i] != '\0'; i++)
    {
        int event_index = lock_event(state, chars[i]);

        /* Skipped exactly like main.c skips them: not an event. */
        if (event_index >= 0)
        {
            state = lock_step(state, (event_t)event_index, &action);
        }
    }

    *last_action = action;

    return state;
}

int main(void)
{
    /* How many checks have run, and how many of them printed FAIL.
       The summary and the exit code are built from these two numbers. */
    int tests = 0;
    int failures = 0;

    /* Loop counters for the table and the state-wise checks. */
    int s;
    int e;
    int i;

    /* One variable per result type, as in hw01: the result is stored
       first, then compared, then printed. */
    int actual_event;
    state_t final_state;
    action_t last_action;

    printf("hw05 test program\n");

    /* ---------------------------------------------------------------- *
     * 1. All 16 entries of lock_table.                                 *
     *                                                                    *
     * lock_step does the reading, so this checks the table AND the      *
     * "one lookup" function at the same time: if lock_step ever grew an *
     * if or a switch that changed the answer, these checks would fail. *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- all 16 entries of lock_table ----\n");

    for (s = 0; s < N_STATES; s++)
    {
        for (e = 0; e < N_EVENTS; e++)
        {
            /* Start from A_UNDEFINED so a lock_step that forgets to
               write *action cannot pass by accident. */
            action_t action = A_UNDEFINED;
            state_t next;

            tests = tests + 1;
            next = lock_step((state_t)s, (event_t)e, &action);

            if (next != expected_next[s][e])
            {
                printf("FAIL  [%s][%s].next: expected %s, got %s\n",
                       expected_state_names[s],
                       expected_event_names[e],
                       expected_state_names[expected_next[s][e]],
                       state_name(next));
                failures = failures + 1;
            }
            else if (action != expected_action[s][e])
            {
                printf("FAIL  [%s][%s].action: expected %s, got %s\n",
                       expected_state_names[s],
                       expected_event_names[e],
                       expected_action_names[expected_action[s][e]],
                       action_name(action));
                failures = failures + 1;
            }
            else
            {
                printf("PASS  [%s][%s] -> %s, %s\n",
                       expected_state_names[s],
                       expected_event_names[e],
                       expected_state_names[next],
                       expected_action_names[action]);
            }
        }
    }

    /* ---------------------------------------------------------------- *
     * 2. lock_event: digits.                                           *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- lock_event: digits ----\n");

    /* A right digit in each state that is waiting for one. */
    for (i = 0; i < 3; i++)
    {
        tests = tests + 1;
        actual_event = lock_event(typing_states[i], right_digits[i]);

        if (actual_event == E_RIGHT)
        {
            printf("PASS  lock_event(%s, '%c') is E_RIGHT\n",
                   expected_state_names[typing_states[i]],
                   right_digits[i]);
        }
        else
        {
            printf("FAIL  lock_event(%s, '%c'): expected E_RIGHT (%d), "
                   "got %d\n",
                   expected_state_names[typing_states[i]],
                   right_digits[i],
                   E_RIGHT,
                   actual_event);
            failures = failures + 1;
        }
    }

    /* A wrong digit in each of those states: any digit that is not the
       one that state is waiting for. */
    for (i = 0; i < 3; i++)
    {
        tests = tests + 1;
        actual_event = lock_event(typing_states[i], wrong_digits[i]);

        if (actual_event == E_WRONG)
        {
            printf("PASS  lock_event(%s, '%c') is E_WRONG\n",
                   expected_state_names[typing_states[i]],
                   wrong_digits[i]);
        }
        else
        {
            printf("FAIL  lock_event(%s, '%c'): expected E_WRONG (%d), "
                   "got %d\n",
                   expected_state_names[typing_states[i]],
                   wrong_digits[i],
                   E_WRONG,
                   actual_event);
            failures = failures + 1;
        }
    }

    /* '0' appears nowhere in the code 3-1-4, so it is wrong in every
       state that is waiting for a digit. */
    tests = tests + 1;
    actual_event = lock_event(LOCKED, '0');

    if (actual_event == E_WRONG)
    {
        printf("PASS  lock_event(LOCKED, '0') is E_WRONG\n");
    }
    else
    {
        printf("FAIL  lock_event(LOCKED, '0'): expected E_WRONG (%d), "
               "got %d\n",
               E_WRONG,
               actual_event);
        failures = failures + 1;
    }

    /* While open, the code is complete: no digit is the "next" one, so
       lock_event reports E_WRONG for every digit, and the table turns
       that into {OPEN, A_NONE} - digits do nothing, as the spec says.
       Both a code digit ('3') and a digit from nowhere ('9') are
       checked, because the answer is the same for both. */
    tests = tests + 1;
    actual_event = lock_event(OPEN, '3');

    if (actual_event == E_WRONG)
    {
        printf("PASS  lock_event(OPEN, '3') is E_WRONG "
               "(digits do nothing while open)\n");
    }
    else
    {
        printf("FAIL  lock_event(OPEN, '3'): expected E_WRONG (%d), "
               "got %d\n",
               E_WRONG,
               actual_event);
        failures = failures + 1;
    }

    tests = tests + 1;
    actual_event = lock_event(OPEN, '9');

    if (actual_event == E_WRONG)
    {
        printf("PASS  lock_event(OPEN, '9') is E_WRONG\n");
    }
    else
    {
        printf("FAIL  lock_event(OPEN, '9'): expected E_WRONG (%d), "
               "got %d\n",
               E_WRONG,
               actual_event);
        failures = failures + 1;
    }

    /* ---------------------------------------------------------------- *
     * 3. lock_event: the door and the timer, in every state.           *
     *                                                                    *
     * 'c' and 't' are events no matter where the lock is; the table    *
     * decides what they do (nothing while locked or typing, lock again *
     * while open).                                                     *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- lock_event: door c and timer t in every state ----\n");

    for (i = 0; i < N_STATES; i++)
    {
        tests = tests + 1;
        actual_event = lock_event((state_t)i, 'c');

        if (actual_event == E_CLOSE)
        {
            printf("PASS  lock_event(%s, 'c') is E_CLOSE\n",
                   expected_state_names[i]);
        }
        else
        {
            printf("FAIL  lock_event(%s, 'c'): expected E_CLOSE (%d), "
                   "got %d\n",
                   expected_state_names[i],
                   E_CLOSE,
                   actual_event);
            failures = failures + 1;
        }
    }

    for (i = 0; i < N_STATES; i++)
    {
        tests = tests + 1;
        actual_event = lock_event((state_t)i, 't');

        if (actual_event == E_TIMEOUT)
        {
            printf("PASS  lock_event(%s, 't') is E_TIMEOUT\n",
                   expected_state_names[i]);
        }
        else
        {
            printf("FAIL  lock_event(%s, 't'): expected E_TIMEOUT (%d), "
                   "got %d\n",
                   expected_state_names[i],
                   E_TIMEOUT,
                   actual_event);
            failures = failures + 1;
        }
    }

    /* ---------------------------------------------------------------- *
     * 4. lock_event: characters that are not events.                   *
     *                                                                    *
     * These all return -1. The newline case matters most: every line   *
     * of data/codes.txt ends with one, and main.c must skip it        *
     * without printing a line of its own.                              *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- lock_event: characters that are not events ----\n");

    tests = tests + 1;
    actual_event = lock_event(LOCKED, 'x');

    if (actual_event == -1)
    {
        printf("PASS  lock_event(LOCKED, 'x') is -1\n");
    }
    else
    {
        printf("FAIL  lock_event(LOCKED, 'x'): expected -1, got %d\n",
               actual_event);
        failures = failures + 1;
    }

    tests = tests + 1;
    actual_event = lock_event(GOT2, 'A');

    if (actual_event == -1)
    {
        printf("PASS  lock_event(GOT2, 'A') is -1\n");
    }
    else
    {
        printf("FAIL  lock_event(GOT2, 'A'): expected -1, got %d\n",
               actual_event);
        failures = failures + 1;
    }

    tests = tests + 1;
    actual_event = lock_event(OPEN, ' ');

    if (actual_event == -1)
    {
        printf("PASS  lock_event(OPEN, ' ') is -1\n");
    }
    else
    {
        printf("FAIL  lock_event(OPEN, ' '): expected -1, got %d\n",
               actual_event);
        failures = failures + 1;
    }

    tests = tests + 1;
    actual_event = lock_event(LOCKED, '\n');

    if (actual_event == -1)
    {
        printf("PASS  lock_event(LOCKED, '\\n') is -1\n");
    }
    else
    {
        printf("FAIL  lock_event(LOCKED, '\\n'): expected -1, got %d\n",
               actual_event);
        failures = failures + 1;
    }

    tests = tests + 1;
    actual_event = lock_event(GOT1, '?');

    if (actual_event == -1)
    {
        printf("PASS  lock_event(GOT1, '?') is -1\n");
    }
    else
    {
        printf("FAIL  lock_event(GOT1, '?'): expected -1, got %d\n",
               actual_event);
        failures = failures + 1;
    }

    /* ---------------------------------------------------------------- *
     * 5. lock_event: an out-of-range state.                            *
     *                                                                    *
     * Not in the spec's list, but the function documents it: there is  *
     * no "next digit" to ask about in a state that does not exist, so  *
     * the answer is -1, the same as for a character that is not an     *
     * event.                                                           *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- lock_event: out-of-range state ----\n");

    tests = tests + 1;
    actual_event = lock_event((state_t)99, '3');

    if (actual_event == -1)
    {
        printf("PASS  lock_event((state_t)99, '3') is -1\n");
    }
    else
    {
        printf("FAIL  lock_event((state_t)99, '3'): expected -1, got %d\n",
               actual_event);
        failures = failures + 1;
    }

    tests = tests + 1;
    actual_event = lock_event((state_t)-1, 'c');

    if (actual_event == -1)
    {
        printf("PASS  lock_event((state_t)-1, 'c') is -1\n");
    }
    else
    {
        printf("FAIL  lock_event((state_t)-1, 'c'): expected -1, got %d\n",
               actual_event);
        failures = failures + 1;
    }

    /* ---------------------------------------------------------------- *
     * 6. The name functions.                                           *
     *                                                                    *
     * strcmp compares the characters of two strings and returns 0 when *
     * they are equal. It is the ordinary way to compare text in C -    *
     * the == sign would compare addresses, not letters.                *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- name functions ----\n");

    for (i = 0; i < N_STATES; i++)
    {
        tests = tests + 1;

        if (strcmp(state_name((state_t)i), expected_state_names[i]) == 0)
        {
            printf("PASS  state_name(%s) == \"%s\"\n",
                   expected_state_names[i],
                   expected_state_names[i]);
        }
        else
        {
            printf("FAIL  state_name(%s): expected \"%s\", got \"%s\"\n",
                   expected_state_names[i],
                   expected_state_names[i],
                   state_name((state_t)i));
            failures = failures + 1;
        }
    }

    for (i = 0; i < N_EVENTS; i++)
    {
        tests = tests + 1;

        if (strcmp(event_name((event_t)i), expected_event_names[i]) == 0)
        {
            printf("PASS  event_name(%s) == \"%s\"\n",
                   expected_event_names[i],
                   expected_event_names[i]);
        }
        else
        {
            printf("FAIL  event_name(%s): expected \"%s\", got \"%s\"\n",
                   expected_event_names[i],
                   expected_event_names[i],
                   event_name((event_t)i));
            failures = failures + 1;
        }
    }

    for (i = 0; i < N_ACTIONS; i++)
    {
        tests = tests + 1;

        if (strcmp(action_name((action_t)i), expected_action_names[i]) == 0)
        {
            printf("PASS  action_name(%s) == \"%s\"\n",
                   expected_action_names[i],
                   expected_action_names[i]);
        }
        else
        {
            printf("FAIL  action_name(%s): expected \"%s\", got \"%s\"\n",
                   expected_action_names[i],
                   expected_action_names[i],
                   action_name((action_t)i));
            failures = failures + 1;
        }
    }

    /* Out of range prints "?" rather than reading past the array. */
    tests = tests + 1;

    if (strcmp(state_name((state_t)99), "?") == 0)
    {
        printf("PASS  state_name((state_t)99) == \"?\"\n");
    }
    else
    {
        printf("FAIL  state_name((state_t)99): expected \"?\", got \"%s\"\n",
               state_name((state_t)99));
        failures = failures + 1;
    }

    tests = tests + 1;

    if (strcmp(event_name((event_t)99), "?") == 0)
    {
        printf("PASS  event_name((event_t)99) == \"?\"\n");
    }
    else
    {
        printf("FAIL  event_name((event_t)99): expected \"?\", got \"%s\"\n",
               event_name((event_t)99));
        failures = failures + 1;
    }

    tests = tests + 1;

    if (strcmp(action_name((action_t)99), "?") == 0)
    {
        printf("PASS  action_name((action_t)99) == \"?\"\n");
    }
    else
    {
        printf("FAIL  action_name((action_t)99): expected \"?\", got \"%s\"\n",
               action_name((action_t)99));
        failures = failures + 1;
    }

    /* ---------------------------------------------------------------- *
     * 7. The four sequences the spec names.                            *
     *                                                                    *
     * Each one starts from LOCKED inside run_sequence, types its       *
     * characters, and reports where the lock ended up - plus the last  *
     * action, which tells us HOW it ended there (unlock, lock, beep).  *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- sequences ----\n");

    /* 3 1 4 opens: ends OPEN, and the last thing it did was unlock. */
    tests = tests + 1;
    final_state = run_sequence("314", &last_action);

    if (final_state == OPEN &&
        last_action == A_UNLOCK)
    {
        printf("PASS  sequence 3 1 4 ends OPEN with A_UNLOCK\n");
    }
    else
    {
        printf("FAIL  sequence 3 1 4: expected OPEN and A_UNLOCK, "
               "got %s and %s\n",
               state_name(final_state),
               action_name(last_action));
        failures = failures + 1;
    }

    /* 3 3 1 4 does not open: the second 3 is wrong in GOT1, so the
       lock beeps and restarts, and the rest of the code arrives from
       LOCKED, where 1 and 4 are both wrong. */
    tests = tests + 1;
    final_state = run_sequence("3314", &last_action);

    if (final_state != OPEN &&
        last_action == A_BEEP)
    {
        printf("PASS  sequence 3 3 1 4 does not open (ends %s, "
               "last action A_BEEP)\n",
               state_name(final_state));
    }
    else
    {
        printf("FAIL  sequence 3 3 1 4: expected not OPEN and A_BEEP, "
               "got %s and %s\n",
               state_name(final_state),
               action_name(last_action));
        failures = failures + 1;
    }

    /* 3 1 t 4 does not open: the timeout after 3 1 restarts the lock
       silently, so the 4 arrives in LOCKED and is wrong. */
    tests = tests + 1;
    final_state = run_sequence("31t4", &last_action);

    if (final_state != OPEN &&
        last_action == A_BEEP)
    {
        printf("PASS  sequence 3 1 t 4 does not open (ends %s, "
               "last action A_BEEP)\n",
               state_name(final_state));
    }
    else
    {
        printf("FAIL  sequence 3 1 t 4: expected not OPEN and A_BEEP, "
               "got %s and %s\n",
               state_name(final_state),
               action_name(last_action));
        failures = failures + 1;
    }

    /* 3 1 4 c locks again: it opens first (A_UNLOCK), then the door
       closing locks it (A_LOCK), so it ends LOCKED. */
    tests = tests + 1;
    final_state = run_sequence("314c", &last_action);

    if (final_state == LOCKED &&
        last_action == A_LOCK)
    {
        printf("PASS  sequence 3 1 4 c locks again (ends LOCKED, "
               "last action A_LOCK)\n");
    }
    else
    {
        printf("FAIL  sequence 3 1 4 c: expected LOCKED and A_LOCK, "
               "got %s and %s\n",
               state_name(final_state),
               action_name(last_action));
        failures = failures + 1;
    }

    /* ---------------------------------------------------------------- *
     * Summary.                                                        *
     *                                                                     *
     * The exit code is what make looks at: 0 says every check passed,  *
     * 1 makes `make test` stop with an error.                          *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- summary ----\n");
    printf("%d automated tests, %d passed, %d failed\n",
           tests,
           tests - failures,
           failures);

    if (failures == 0)
    {
        printf("ALL TESTS PASSED\n");
        return 0;
    }

    printf("SOME TESTS FAILED\n");
    return 1;
}
