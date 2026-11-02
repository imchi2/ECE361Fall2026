/**
 * @file main.c
 * @brief The lock program: read characters from standard input, turn
 *        each one into an event, step the machine, print one line per
 *        event, and print the final state at the end.
 *
 * The machine starts in LOCKED. Characters that are not events (letters,
 * spaces, newlines) are consumed silently, so a data file can be plain
 * human-readable text:
 *
 *     314          -> opens
 *     3314         -> does not open: the second 3 resets the code
 *     x3y1z4       -> opens anyway: x, y and z are ignored
 *
 * Try it with:  ./lock < data/codes.txt
 *
 * There is no real 5-second timer here. The character 't' stands in for
 * "the timer ran out", so a timeout can be tested from a file.
 *
 * @note Built with <tt>gcc -std=c11 -Wall -Wextra -g3 -O0</tt>, no warnings.
 */

#include "lock.h"

#include <stdio.h> /* getchar, printf, EOF */

int main(void)
{
    /* Where the lock is right now. The spec says the program starts in
       LOCKED, and the state carries over from one input line to the
       next - a newline is not an event, so it never resets anything. */
    state_t state = LOCKED;
    state_t next_state;
    action_t action;
    event_t event;

    /* getchar returns the character as an int, because it also has to
       return EOF, which does not fit in a char. */
    int character;
    int event_index;

    character = getchar();

    while (character != EOF)
    {
        /* Ask what this character means for the state we are in.
           -1 means "not an event": a letter, a space, the newline at
           the end of the line. Those are skipped without a word, which
           is why the event lines below only ever show digits, c and t. */
        event_index = lock_event(state, (char)character);

        if (event_index >= 0)
        {
            /* lock_event only ever returns -1 or a real event value, so
               this cast cannot produce an out-of-range event_t. */
            event = (event_t)event_index;

            /* One lookup: this is where the state actually changes. */
            next_state = lock_step(state, event, &action);

            /* One line per event, in the spec's exact format, e.g.
               "GOT2 4 RIGHT -> OPEN UNLOCK". The state printed first
               is the one the lock was IN, so the line reads as a
               before -> after. */
            printf("%s %c %s -> %s %s\n",
                   state_name(state),
                   (char)character,
                   event_name(event),
                   state_name(next_state),
                   action_name(action));

            state = next_state;
        }

        /* Read the next character, whether or not it was an event. */
        character = getchar();
    }

    /* The spec's last line: "final: STATE". */
    printf("final: %s\n", state_name(state));

    return 0;
}
