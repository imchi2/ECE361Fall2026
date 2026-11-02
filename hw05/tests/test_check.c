/**
 * @file test_check.c
 * @brief Tests for the bonus table checker: the real table must come
 *        back clean, and three deliberately broken copies must each be
 *        reported with the right number of problems.
 *
 * The point of these tests is that fsm_check is code too - it can be
 * wrong in both directions (missing a real problem, or inventing one).
 * Feeding it known-good and known-broken tables pins down both.
 *
 * Each test prints the checker's own problem lines first (that is the
 * output BONUS.md quotes), then PASS or FAIL for the count.
 *
 * Build and run, from hw05/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw05 && make bonus"
 */

#include "check.h"
#include "lock.h"

#include <stdio.h> /* printf */

/**
 * @brief Copy the real table into dest so each test can break it.
 *
 * A plain double loop instead of memcpy: same result, no extra header
 * to look up, and it reads like the rest of this homework.
 *
 * @param dest A table the caller owns; every cell is overwritten.
 */
static void copy_table(transition_t dest[N_STATES][N_EVENTS])
{
    int s;
    int e;

    for (s = 0; s < N_STATES; s++)
    {
        for (e = 0; e < N_EVENTS; e++)
        {
            dest[s][e] = lock_table[s][e];
        }
    }
}

int main(void)
{
    /* One table per broken copy, so the three tests cannot interfere
       with each other or with the real lock_table. */
    transition_t broken[N_STATES][N_EVENTS];

    int tests = 0;
    int failures = 0;
    int problems;

    printf("hw05 bonus test program (fsm_check)\n");

    /* ---------------------------------------------------------------- *
     * Test 1: the real table must be clean.                            *
     *                                                                    *
     * fsm_check prints nothing here - no problem, no line - and        *
     * returns 0. If this ever fails, either the table broke or the     *
     * checker started inventing problems.                              *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- the real lock_table (expect no problems) ----\n");

    tests = tests + 1;
    problems = fsm_check(lock_table, LOCKED, stdout);

    if (problems == 0)
    {
        printf("PASS  fsm_check(lock_table) found 0 problems\n");
    }
    else
    {
        printf("FAIL  fsm_check(lock_table): expected 0 problems, "
               "got %d\n",
               problems);
        failures = failures + 1;
    }

    /* ---------------------------------------------------------------- *
     * Test 2: broken copy 1 - the unlock cell was forgotten.           *
     *                                                                    *
     * [GOT2][E_RIGHT] is set to what an unwritten cell contains:      *
     * {LOCKED, A_UNDEFINED} (both fields are 0).                       *
     *                                                                    *
     * Expected: 2 problems - the forgotten cell itself, plus OPEN,     *
     * because that cell was the only way into OPEN, so now no state    *
     * can reach it. A forgotten entry never stays a single problem:    *
     * it also breaks whatever depended on it.                           *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- broken copy 1: forgotten unlock cell ----\n");

    copy_table(broken);
    broken[GOT2][E_RIGHT].next = LOCKED;
    broken[GOT2][E_RIGHT].action = A_UNDEFINED;

    tests = tests + 1;
    problems = fsm_check(broken, LOCKED, stdout);

    if (problems == 2)
    {
        printf("PASS  fsm_check(forgotten cell) found 2 problems\n");
    }
    else
    {
        printf("FAIL  fsm_check(forgotten cell): expected 2 problems, "
               "got %d\n",
               problems);
        failures = failures + 1;
    }

    /* ---------------------------------------------------------------- *
     * Test 3: broken copy 2 - a next state one past the end.           *
     *                                                                    *
     * [GOT2][E_TIMEOUT].next = N_STATES (4), which is not a state.    *
     *                                                                    *
     * Expected: 1 problem - the out-of-range next state. Everything   *
     * is still reachable through the other events, and every state     *
     * still has a way out, so the other three checks stay quiet.       *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- broken copy 2: next state out of range ----\n");

    copy_table(broken);
    broken[GOT2][E_TIMEOUT].next = (state_t)N_STATES;

    tests = tests + 1;
    problems = fsm_check(broken, LOCKED, stdout);

    if (problems == 1)
    {
        printf("PASS  fsm_check(out of range) found 1 problem\n");
    }
    else
    {
        printf("FAIL  fsm_check(out of range): expected 1 problem, "
               "got %d\n",
               problems);
        failures = failures + 1;
    }

    /* ---------------------------------------------------------------- *
     * Test 4: broken copy 3 - OPEN swallows everything.                *
     *                                                                    *
     * Closing the door or timing out while open now stays in OPEN      *
     * instead of locking the machine.                                  *
     *                                                                    *
     * Expected: 1 problem - OPEN has no way out. Everything is still   *
     * reachable, every cell is filled, every next state is real; the   *
     * only thing wrong is that once the door is open it can never be  *
     * locked again.                                                     *
     * ---------------------------------------------------------------- */
    printf("\n");
    printf("---- broken copy 3: OPEN has no way out ----\n");

    copy_table(broken);
    broken[OPEN][E_CLOSE].next = OPEN;
    broken[OPEN][E_CLOSE].action = A_NONE;
    broken[OPEN][E_TIMEOUT].next = OPEN;
    broken[OPEN][E_TIMEOUT].action = A_NONE;

    tests = tests + 1;
    problems = fsm_check(broken, LOCKED, stdout);

    if (problems == 1)
    {
        printf("PASS  fsm_check(no way out) found 1 problem\n");
    }
    else
    {
        printf("FAIL  fsm_check(no way out): expected 1 problem, "
               "got %d\n",
               problems);
        failures = failures + 1;
    }

    /* ---------------------------------------------------------------- *
     * Summary.                                                        *
     *                                                                     *
     * The exit code is what make looks at: 0 says every check passed,  *
     * 1 makes `make bonus` stop with an error.                         *
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
