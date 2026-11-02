/*
 * events.c: ECE 361, Fall 2026, HW4 - count the names on standard input.
 *
 * Reads standard input line by line with fgets, removes the newline,
 * skips blank lines and lines that start with '#', and adds every
 * other line to one tally. At the end it prints the tally (one line
 * per name, sorted) and one last line:
 *
 *     names: N, lines skipped: S
 *
 * where N is how many different names the tally holds and S counts
 * the lines tally_add REFUSED - too long, or new while the tally was
 * already full. Blank and comment lines are not part of S: they were
 * never offered to the tally at all.
 *
 * Run it as:  ./events < data/door.txt
 *
 * Note that events.c never looks inside a tally_t. It only calls the
 * six functions of tally.h - that is the whole point of the opaque
 * type (see README.md, Part 3).
 */

#include "tally.h"

#include <stdio.h>  /* fgets, getchar, printf, stdout */
#include <string.h> /* strlen, strchr */

/*
 * Big enough for any name tally_add will accept (TALLY_LEN = 15) with
 * room to spare, so ordinary lines arrive whole. A line longer than
 * the buffer is not a disaster: fgets returns the first piece, that
 * piece is refused by tally_add (it is far longer than TALLY_LEN) and
 * counted, and the drain below throws the rest of the line away so
 * the next fgets starts at a real line boundary.
 */
#define LINE_MAX 128

int main(void)
{
    tally_t *t;           /* the one tally this program uses */
    char     line[LINE_MAX];
    int      skipped = 0; /* lines tally_add refused */

    t = tally_create();
    if (t == NULL) {
        /* Only possible if all TALLY_MAX tallies are already in use,
           which a fresh program never manages - but the header says
           create can return NULL, so the answer is checked anyway. */
        fprintf(stderr, "events: no tally available\n");
        return 1;
    }

    while (fgets(line, (int)sizeof(line), stdin) != NULL) {
        char *nl = strchr(line, '\n'); /* where the newline is, or NULL */

        if (nl 09!= NULL) {
            /* Drop the newline fgets keeps. It is the last character
               before the '\0', so one assignment removes it and the
               string simply ends one place earlier. */
            *nl = '\0';
        } else {
            /* No newline in the piece we got: the line was longer
               than the buffer. Throw away the rest of it (up to the
               newline or the end of the input) so the next fgets
               begins at the next real line, not mid-line. */
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
                /* discard */
            }
        }

        /* Blank line: skipped, and not counted in S. */
        if (line[0] == '\0') continue;

        /* Comment line: starts with '#', also skipped and not in S. */
        if (line[0] == '#') continue;

        /* Everything else is a name. A nonzero return means tally_add
           refused it (empty is impossible here, but too long or a new
           name in a full tally is not), and refusals are what S
           counts. */
        if (tally_add(t, line) != 0) {
            skipped = skipped + 1;
        }
    }

    /* The tally prints itself, in sorted order; the summary line is
       this program's own idea, built from the two answers it wants. */
    tally_print(t, stdout);
    printf("names: %d, lines skipped: %d\n", tally_size(t), skipped);

    tally_destroy(t);

    return 0;
}
