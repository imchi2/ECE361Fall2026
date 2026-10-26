/*
 * test_tally.c: ECE 361, Fall 2026, HW4 - unit tests for the tally ADT.
 *
 * Prints PASS or FAIL per check, a summary at the end, and returns
 * nonzero if any check failed (so `make test` fails loudly). The
 * checks follow the spec's list:
 *
 *   - counting up; a name never added; an empty name
 *   - a name of exactly TALLY_LEN characters and one of TALLY_LEN + 1
 *   - NULL arguments
 *   - a full tally (a new name is refused, an existing one still counts)
 *   - TALLY_MAX tallies, then one more (NULL)
 *   - create after destroy; two tallies are independent; print order
 *
 * Build and run, from hw04/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw04 && make test"
 */

#include "tally.h"

#include <stdio.h>  /* printf, tmpfile, fgets */
#include <string.h> /* strcmp */

/*
 * Exactly TALLY_LEN (15) characters - the longest name tally_add must
 * accept - and 16 characters, the shortest it must refuse. The tests
 * count these lengths themselves rather than trusting the literals to
 * have stayed in step with the #define.
 */
static const char name_at_limit[] = "abcdefghijklmno"; /* 15 chars */
static const char name_too_long[] = "abcdefghijklmnop"; /* 16 chars */

int main(void)
{
    /* How many checks ran, and how many printed FAIL. Both feed the
       summary and the exit status. */
    int tests = 0;
    int failures = 0;

    tally_t *t;          /* the tally most checks use */
    tally_t *t2;         /* a second tally for the independence check */
    tally_t *pool[5];    /* TALLY_MAX of them, plus the one that must fail */
    int      n;          /* scratch counts */
    int      i;
    int      first_failed; /* remembers a failure inside a loop */

    printf("hw04 test program\n\n");

    /* ------------------------------------------------------------------ *
     * Creating, and the empty tally                                      *
     * ------------------------------------------------------------------ */
    printf("---- create ----\n");

    t = tally_create();

    tests = tests + 1;
    if (t != NULL)
    {
        printf("PASS  tally_create returns a tally\n");
    }
    else
    {
        printf("FAIL  tally_create returned NULL with no tallies in use\n");
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_size(t) == 0)
    {
        printf("PASS  a new tally has size 0\n");
    }
    else
    {
        printf("FAIL  a new tally has size %d, expected 0\n", tally_size(t));
        failures = failures + 1;
    }

    /* ------------------------------------------------------------------ *
     * Counting up, and a name that was never added                       *
     * ------------------------------------------------------------------ */
    printf("\n---- tally_add / tally_count ----\n");

    tests = tests + 1;
    if (tally_add(t, "alice") == 0)
    {
        printf("PASS  first add of alice succeeds\n");
    }
    else
    {
        printf("FAIL  first add of alice refused\n");
        failures = failures + 1;
    }

    /* Count the same name up: three more occurrences. */
    tally_add(t, "alice");
    tally_add(t, "alice");
    tally_add(t, "alice");

    tests = tests + 1;
    n = tally_count(t, "alice");
    if (n == 4)
    {
        printf("PASS  alice counted up to 4\n");
    }
    else
    {
        printf("FAIL  alice counted up to %d, expected 4\n", n);
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_size(t) == 1)
    {
        printf("PASS  four additions of one name give size 1\n");
    }
    else
    {
        printf("FAIL  size is %d after four alice, expected 1\n", tally_size(t));
        failures = failures + 1;
    }

    tests = tests + 1;
    n = tally_count(t, "nobody");
    if (n == 0)
    {
        printf("PASS  a name never added counts 0\n");
    }
    else
    {
        printf("FAIL  a name never added counts %d, expected 0\n", n);
        failures = failures + 1;
    }

    /* ------------------------------------------------------------------ *
     * Empty name, and the length boundary                                *
     * ------------------------------------------------------------------ */
    printf("\n---- name lengths ----\n");

    tests = tests + 1;
    if (strlen(name_at_limit) == TALLY_LEN)
    {
        printf("PASS  the at-limit name really is TALLY_LEN characters\n");
    }
    else
    {
        printf("FAIL  at-limit name is %d chars, expected TALLY_LEN=%d\n",
               (int)strlen(name_at_limit), TALLY_LEN);
        failures = failures + 1;
    }

    tests = tests + 1;
    if (strlen(name_too_long) == TALLY_LEN + 1)
    {
        printf("PASS  the long name really is TALLY_LEN + 1 characters\n");
    }
    else
    {
        printf("FAIL  long name is %d chars, expected %d\n",
               (int)strlen(name_too_long), TALLY_LEN + 1);
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_add(t, "") != 0)
    {
        printf("PASS  an empty name is refused\n");
    }
    else
    {
        printf("FAIL  an empty name was accepted\n");
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_add(t, name_at_limit) == 0)
    {
        printf("PASS  a name of exactly TALLY_LEN characters is accepted\n");
    }
    else
    {
        printf("FAIL  a name of exactly TALLY_LEN characters was refused\n");
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_add(t, name_too_long) != 0)
    {
        printf("PASS  a name of TALLY_LEN + 1 characters is refused\n");
    }
    else
    {
        printf("FAIL  a name of TALLY_LEN + 1 characters was accepted\n");
        failures = failures + 1;
    }

    /* The refusals above must not have changed anything. */
    tests = tests + 1;
    n = tally_count(t, name_at_limit);
    if (n == 1 && tally_size(t) == 2)
    {
        printf("PASS  refusals changed nothing: at-limit counted 1, size 2\n");
    }
    else
    {
        printf("FAIL  after refusals: at-limit=%d, size=%d, expected 1 and 2\n",
               n, tally_size(t));
        failures = failures + 1;
    }

    /* ------------------------------------------------------------------ *
     * NULL arguments                                                     *
     * ------------------------------------------------------------------ */
    printf("\n---- NULL arguments ----\n");

    tests = tests + 1;
    if (tally_add(NULL, "alice") != 0 && tally_add(t, NULL) != 0)
    {
        printf("PASS  tally_add refuses a NULL tally and a NULL name\n");
    }
    else
    {
        printf("FAIL  tally_add accepted a NULL argument\n");
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_count(NULL, "alice") == 0 && tally_count(t, NULL) == 0)
    {
        printf("PASS  tally_count returns 0 for NULL arguments\n");
    }
    else
    {
        printf("FAIL  tally_count did not return 0 for NULL arguments\n");
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_size(NULL) == 0)
    {
        printf("PASS  tally_size(NULL) is 0\n");
    }
    else
    {
        printf("FAIL  tally_size(NULL) is not 0\n");
        failures = failures + 1;
    }

    tests = tests + 1;
    tally_print(NULL, stdout);     /* neither call may crash */
    tally_print(t, NULL);
    printf("PASS  tally_print(NULL, stdout) and tally_print(t, NULL) return quietly\n");

    /* ------------------------------------------------------------------ *
     * A full tally: a new name is refused, an existing one still counts  *
     * ------------------------------------------------------------------ */
    printf("\n---- a full tally ----\n");

    /* Fill the tally with TALLY_NAMES different names: alice and the
       at-limit name are already in, so TALLY_NAMES - 2 more. */
    for (i = 0; i < TALLY_NAMES - 2; i++)
    {
        char nm[TALLY_LEN + 1];
        snprintf(nm, sizeof nm, "name%02d", i);
        tally_add(t, nm);
    }

    tests = tests + 1;
    if (tally_size(t) == TALLY_NAMES)
    {
        printf("PASS  tally filled to TALLY_NAMES (%d)\n", TALLY_NAMES);
    }
    else
    {
        printf("FAIL  tally size is %d after filling, expected %d\n",
               tally_size(t), TALLY_NAMES);
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_add(t, "newname") != 0)
    {
        printf("PASS  a new name into a full tally is refused\n");
    }
    else
    {
        printf("FAIL  a new name into a full tally was accepted\n");
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_add(t, "alice") == 0 && tally_count(t, "alice") == 5)
    {
        printf("PASS  an existing name still counts in a full tally (5)\n");
    }
    else
    {
        printf("FAIL  existing name in full tally: add or count wrong, count=%d\n",
               tally_count(t, "alice"));
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_size(t) == TALLY_NAMES)
    {
        printf("PASS  the refused new name did not grow the tally\n");
    }
    else
    {
        printf("FAIL  size is %d after refusal, expected %d\n",
               tally_size(t), TALLY_NAMES);
        failures = failures + 1;
    }

    tally_destroy(t);

    /* ------------------------------------------------------------------ *
     * TALLY_MAX tallies, then one more                                   *
     * ------------------------------------------------------------------ */
    printf("\n---- the pool ----\n");

    first_failed = 0;
    for (i = 0; i < TALLY_MAX; i++)
    {
        pool[i] = tally_create();
        if (pool[i] == NULL) first_failed = 1;
    }

    tests = tests + 1;
    if (!first_failed)
    {
        printf("PASS  %d tallies can exist at the same time\n", TALLY_MAX);
    }
    else
    {
        printf("FAIL  one of %d tally_create calls returned NULL\n", TALLY_MAX);
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_create() == NULL)
    {
        printf("PASS  the %dth tally_create returns NULL\n", TALLY_MAX + 1);
    }
    else
    {
        printf("FAIL  the %dth tally_create did not return NULL\n", TALLY_MAX + 1);
        failures = failures + 1;
    }

    /* Give one slot back, then ask for it again: it must come back
       empty, not with the previous owner's names. */
    tally_destroy(pool[0]);

    tests = tests + 1;
    t = tally_create();
    if (t != NULL && tally_size(t) == 0)
    {
        printf("PASS  create after destroy hands out an empty tally\n");
    }
    else
    {
        printf("FAIL  create after destroy failed or came back non-empty\n");
        failures = failures + 1;
    }
    tally_destroy(t);

    /* All four slots are back in the pool now: pool[0] was destroyed
       above, the empty tally t just went back too, and pool[1..3] go
       back here so the next section starts from an empty pool. */
    tally_destroy(pool[1]);
    tally_destroy(pool[2]);
    tally_destroy(pool[3]);

    /* ------------------------------------------------------------------ *
     * Two tallies are independent                                        *
     * ------------------------------------------------------------------ */
    printf("\n---- independence ----\n");

    /* The pool is empty, so these two creates are the only ones in
       flight: whatever is added to one must not appear in the other. */
    t  = tally_create();
    t2 = tally_create();

    tally_add(t, "alice");
    tally_add(t, "alice");
    tally_add(t2, "bob");

    tests = tests + 1;
    if (t != NULL && t2 != NULL && t != t2)
    {
        printf("PASS  two tallies exist at once\n");
    }
    else
    {
        printf("FAIL  could not get two separate tallies\n");
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_count(t, "alice") == 2 && tally_count(t2, "alice") == 0)
    {
        printf("PASS  adding to one tally leaves the other at 0\n");
    }
    else
    {
        printf("FAIL  tallies are not independent (alice: %d and %d)\n",
               tally_count(t, "alice"), tally_count(t2, "alice"));
        failures = failures + 1;
    }

    tests = tests + 1;
    if (tally_size(t) == 1 && tally_size(t2) == 1)
    {
        printf("PASS  each tally holds only its own name\n");
    }
    else
    {
        printf("FAIL  sizes are %d and %d, expected 1 and 1\n",
               tally_size(t), tally_size(t2));
        failures = failures + 1;
    }

    tally_destroy(t);
    tally_destroy(t2);
    /* pool[1..3] were already given back before this section. */

    /* ------------------------------------------------------------------ *
     * Print order: alphabetical, one line per name                       *
     * ------------------------------------------------------------------ */
    printf("\n---- tally_print ----\n");

    t = tally_create();
    /* Added deliberately out of order: the print must sort them. */
    tally_add(t, "carol");
    tally_add(t, "alice");
    tally_add(t, "dave");
    tally_add(t, "bob");
    tally_add(t, "alice");

    /* Print into a temporary file, then read it back line by line.
       tmpfile() needs no name and no cleanup, so the test leaves
       nothing behind. */
    {
        FILE *f = tmpfile();
        char  line[64];
        int   line_ok;
        int   eof_ok;

        tests = tests + 1;
        if (f == NULL)
        {
            printf("FAIL  tmpfile() could not create a stream\n");
            failures = failures + 1;
            return failures == 0 ? 0 : 1; /* nothing further can run */
        }

        tally_print(t, f);
        rewind(f);

        /* Expected lines, in alphabetical order with counts. */
        line_ok = 1;
        if (fgets(line, (int)sizeof line, f) == NULL || strcmp(line, "alice 2\n") != 0) line_ok = 0;
        if (fgets(line, (int)sizeof line, f) == NULL || strcmp(line, "bob 1\n") != 0)   line_ok = 0;
        if (fgets(line, (int)sizeof line, f) == NULL || strcmp(line, "carol 1\n") != 0) line_ok = 0;
        if (fgets(line, (int)sizeof line, f) == NULL || strcmp(line, "dave 1\n") != 0)  line_ok = 0;

        /* And nothing after the fourth line. */
        eof_ok = (fgets(line, (int)sizeof line, f) == NULL);

        tests = tests + 1;
        if (line_ok)
        {
            printf("PASS  print order is alphabetical with counts\n");
        }
        else
        {
            printf("FAIL  print order wrong: wanted alice 2, bob 1, carol 1, dave 1\n");
            failures = failures + 1;
        }

        tests = tests + 1;
        if (eof_ok)
        {
            printf("PASS  print stops after one line per name\n");
        }
        else
        {
            printf("FAIL  print wrote more lines than there are names\n");
            failures = failures + 1;
        }

        fclose(f);
    }

    tally_destroy(t);

    /* ------------------------------------------------------------------ *
     * Summary                                                            *
     * ------------------------------------------------------------------ */
    printf("\n%d test(s), %d failure(s)\n", tests, failures);

    if (failures == 0)
    {
        printf("ALL TESTS PASSED\n");
        return 0;
    }

    return 1;
}
