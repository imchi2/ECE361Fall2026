/**
 * @file lock.c
 * @brief The lock's transition table and the four functions that read it.
 *
 * The table is the program: 4 states x 4 events = 16 cells, each one
 * naming the next state and the action. Everything else here is either
 * a lookup (lock_step), a rule for turning a character into an event
 * (lock_event), or a name to print (state_name, event_name, action_name).
 *
 * Written for people learning C: plain control flow, no feature newer
 * than C99, long expressions split across lines, and a comment on every
 * decision.
 */

#include "lock.h"

/* ------------------------------------------------------------------ *
 * The table.                                                          *
 *                                                                     *
 * Written with designated initializers ([LOCKED] = { ... }), so the   *
 * order of the rows and columns does not have to be remembered: each  *
 * row names its state and each cell names its event. If a cell were   *
 * left out, C would give it {0, 0} = {LOCKED, A_UNDEFINED}, which is  *
 * exactly what fsm_check looks for - so every cell is written here.   *
 *                                                                     *
 * The state diagram this was drawn from (paper first, as asked):      *
 *                                                                     *
 *   3/A_NONE      1/A_NONE      4/A_UNLOCK                            *
 * LOCKED ---> GOT1 ---> GOT2 ---> OPEN                                *
 *   ^  |          |           |        |                              *
 *   |  +-- wrong digit: beep, back to LOCKED (from every typing state)*
 *   +----- timeout t: back to LOCKED, silently                        *
 *   +----- door c or timeout t from OPEN: back to LOCKED, A_LOCK      *
 *                                                                     *
 * The cells that stay in place (door c while locked or typing, any    *
 * digit while open) are the A_NONE rows below.                        *
 * ------------------------------------------------------------------ */

const transition_t lock_table[N_STATES][N_EVENTS] =
{
    /* LOCKED: waiting for the first digit of the code, which is '3'. */
    [LOCKED] =
    {
        [E_RIGHT]   = { GOT1,   A_NONE }, /* '3': first digit accepted */
        [E_WRONG]   = { LOCKED, A_BEEP }, /* other digit: beep, restart here */
        [E_CLOSE]   = { LOCKED, A_NONE }, /* door closed while locked: nothing */
        [E_TIMEOUT] = { LOCKED, A_NONE }  /* no code in progress: nothing to reset */
    },

    /* GOT1: '3' seen, waiting for '1'. */
    [GOT1] =
    {
        [E_RIGHT]   = { GOT2,   A_NONE }, /* '1': second digit accepted */
        [E_WRONG]   = { LOCKED, A_BEEP }, /* wrong digit: beep, back to the start */
        [E_CLOSE]   = { GOT1,   A_NONE }, /* door close mid-code: changes nothing */
        [E_TIMEOUT] = { LOCKED, A_NONE }  /* timer ran out: restart, silently */
    },

    /* GOT2: '3' '1' seen, waiting for the last digit, '4'. */
    [GOT2] =
    {
        [E_RIGHT]   = { OPEN,   A_UNLOCK }, /* '4': the door opens */
        [E_WRONG]   = { LOCKED,   A_BEEP }, /* wrong digit: beep, back to the start */
        [E_CLOSE]   = { GOT2,     A_NONE }, /* door close mid-code: changes nothing */
        [E_TIMEOUT] = { LOCKED,   A_NONE }  /* timer ran out: restart, silently */
    },

    /* OPEN: digits do nothing; closing the door or timing out locks
       the machine again. Both digit cells are written even though
       lock_event only ever reports E_WRONG while open - the spec asks
       for all 16 entries. */
    [OPEN] =
    {
        [E_RIGHT]   = { OPEN,   A_NONE }, /* no next digit while open */
        [E_WRONG]   = { OPEN,   A_NONE }, /* digits do nothing */
        [E_CLOSE]   = { LOCKED, A_LOCK }, /* door closed: lock again */
        [E_TIMEOUT] = { LOCKED, A_LOCK }  /* timer ran out: lock again */
    }
};

/* ------------------------------------------------------------------ *
 * The unlock code, one digit per state that is waiting for it.        *
 * LOCKED waits for '3', GOT1 waits for '1', GOT2 waits for '4'.       *
 * OPEN is deliberately absent: the code is complete there, so no      *
 * digit is the "next" one.                                            *
 * ------------------------------------------------------------------ */
static const char code[] = { '3', '1', '4' };

/* The names the spec asks for, indexed by the enum value. The order
   matches the enum order in lock.h: state_names[LOCKED] is "LOCKED",
   action_names[A_BEEP] is "BEEP", and so on. The N_* members say where
   each array stops, which is what the range checks compare against. */
static const char *state_names[N_STATES] =
{
    "LOCKED", "GOT1", "GOT2", "OPEN"
};

/* No "E_" prefix: main prints "GOT2 4 RIGHT -> OPEN UNLOCK", exactly
   the format in the spec. */
static const char *event_names[N_EVENTS] =
{
    "RIGHT", "WRONG", "CLOSE", "TIMEOUT"
};

static const char *action_names[N_ACTIONS] =
{
    "UNDEFINED", "NONE", "UNLOCK", "LOCK", "BEEP"
};

state_t lock_step(state_t s, event_t e, action_t *action)
{
    /* One table lookup, copied into a local so it happens only once.
       The spec requires "one table lookup, no if or switch on the
       state" - the entry itself says where to go and what to do. */
    transition_t entry = lock_table[s][e];

    /* action is an out-parameter: the caller passes a pointer because a
       C function can only return one value, and lock_step returns the
       next state. Writing through the pointer hands the action back. */
    *action = entry.action;

    return entry.next;
}

int lock_event(state_t s, char c)
{
    /* An invalid state has no row in the table and no "next digit" to
       ask about, so there is no event to report. Casting to int first
       makes the comparison a plain signed one. */
    int state_index = (int)s;

    if (state_index < 0)
    {
        return -1;
    }
    if (state_index >= N_STATES)
    {
        return -1;
    }

    /* The door closing is an event in every state, including while a
       code is being typed (the table decides that it changes nothing). */
    if (c == 'c')
    {
        return E_CLOSE;
    }

    /* The timer running out, again in every state. There is no real
       5-second timer in this program: typing 't' stands in for it, so
       the input file can trigger a timeout on demand. */
    if (c == 't')
    {
        return E_TIMEOUT;
    }

    /* Digits are the only characters left that can be an event. The
       range check is split over two lines to keep each comparison
       short enough to read at a glance. */
    if (c >= '0' &&
        c <= '9')
    {
        /* Which digit moves this state forward? code[] is indexed by
           the state: code[LOCKED] is '3', code[GOT1] is '1', and so
           on. '\0' is the default for a state that is not waiting for
           a digit - that is OPEN, where the code is already complete. */
        char expected = '\0';

        if (s == LOCKED)
        {
            expected = code[LOCKED];
        }
        else if (s == GOT1)
        {
            expected = code[GOT1];
        }
        else if (s == GOT2)
        {
            expected = code[GOT2];
        }
        /* OPEN keeps expected as '\0': no digit is the next one. */

        if (expected == '\0')
        {
            /* While open, a digit is never "the next digit of the
               code", so it reports E_WRONG. The table turns that into
               {OPEN, A_NONE} - digits do nothing, as the spec asks. */
            return E_WRONG;
        }

        if (c == expected)
        {
            return E_RIGHT;
        }

        return E_WRONG;
    }

    /* Letters, spaces, the newline at the end of each line: not events.
       main.c skips these without printing anything, which is why a data
       file can look like ordinary text. */
    return -1;
}

const char *state_name(state_t s)
{
    /* Out of range gets "?" instead of reading past the array. The
       cast to int keeps the comparison signed even if the enum is
       unsigned under the hood. */
    int index = (int)s;

    if (index < 0)
    {
        return "?";
    }
    if (index >= N_STATES)
    {
        return "?";
    }

    return state_names[index];
}

const char *event_name(event_t e)
{
    /* Same shape as state_name: check first, index second. The check
       is repeated on purpose - three short copies are easier to read
       than one shared helper with an array passed in. */
    int index = (int)e;

    if (index < 0)
    {
        return "?";
    }
    if (index >= N_EVENTS)
    {
        return "?";
    }

    return event_names[index];
}

const char *action_name(action_t a)
{
    /* A_UNDEFINED (0) is a real, nameable value here: it prints as
       "UNDEFINED". Only an index outside 0..N_ACTIONS-1 prints "?". */
    int index = (int)a;

    if (index < 0)
    {
        return "?";
    }
    if (index >= N_ACTIONS)
    {
        return "?";
    }

    return action_names[index];
}
