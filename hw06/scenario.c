/**
 * @file scenario.c
 * @brief Homework 6, Part 3a: read a scenario file, then answer
 *        "which line is in force at this tick?".
 *
 * The loader reads the file line by line with fgets (no malloc: the
 * file is streamed straight into the struct's fixed array). Each line
 * is cleaned in a fixed order - cut the comment off, skip leading
 * whitespace, trim trailing whitespace, skip the line if nothing is
 * left - and only then parsed. The first content line must be the
 * start line; every later one must be a tick line with strictly
 * increasing ticks.
 *
 * Build and run, from hw06/ under WSL:
 *     wsl -d Ubuntu -- sh -c "cd /mnt/c/ECE361_C/hw06 && make test"
 */

#include "scenario.h"

#include <ctype.h>  /* isspace */
#include <stdio.h>  /* fopen, fgets, fclose, sscanf */
#include <stdlib.h> /* strtol, strtoul */
#include <string.h> /* strchr, strlen */

/* The longest line the loader accepts. A real scenario line is a few
   dozen characters; anything longer than this (a comment gone wild)
   is reported as wrong instead of being parsed in two half-lines. */
#define SCENARIO_LINE_MAX 512

int scenario_load(const char *path, scenario_t *s)
{
    FILE *fp;
    char buf[SCENARIO_LINE_MAX];
    int line_no;     /* physical lines read so far, for error reports */
    int have_start;  /* has the start line been seen? */
    int prev_tick;   /* the tick of the last line stored */

    /* Throw away whatever the struct held: a failed load must not
       leave a half-old, half-new scenario behind. */
    s->start = 0;
    s->num_lines = 0;

    fp = fopen(path, "r");
    if (fp == NULL)
    {
        /* Not a "wrong line" in the file - the file could not be
           opened at all. Reported as line 1 so the caller still gets
           a nonzero number it can print. */
        return 1;
    }

    line_no = 0;
    have_start = 0;
    prev_tick = 0;

    while (fgets(buf, (int) sizeof buf, fp) != NULL)
    {
        char *p;         /* the current line, comment cut off */
        char *hash;      /* where a '#' comment starts, if any */
        size_t len;      /* the length of the trimmed line */
        int is_start;    /* does this line look like "start ..."? */

        line_no = line_no + 1;

        /* fgets stops at the buffer edge: if there is no newline and
           the file is not over, the line was cut in half. Refuse it
           rather than parse a fragment. */
        if (strchr(buf, '\n') == NULL && !feof(fp))
        {
            fclose(fp);
            return line_no;
        }

        /* Cut the comment off first, so '#' works anywhere on the
           line - full-line comments and trailing ones alike. */
        hash = strchr(buf, '#');
        if (hash != NULL)
            *hash = '\0';

        /* Skip leading whitespace... */
        p = buf;
        while (isspace((unsigned char) *p))
            p = p + 1;

        /* ...then trim trailing whitespace, so the line is exactly
           its content with nothing around it. */
        len = strlen(p);
        while (len > 0 && isspace((unsigned char) p[len - 1]))
        {
            len = len - 1;
            p[len] = '\0';
        }

        /* Nothing left: a blank line or a comment-only line. It
           counts as a physical line (line_no moved on) but carries
           no content, so the loader just moves on. */
        if (len == 0)
            continue;

        /* Does the line start with the word "start" followed by a
           space? A line like "startx" or "started" must not match,
           so the character after "start" has to be whitespace. */
        is_start = 0;
        if (len > 5 && strncmp(p, "start", 5) == 0
            && isspace((unsigned char) p[5]))
        {
            is_start = 1;
        }

        if (is_start && !have_start)
        {
            long stamp;   /* the parsed Unix timestamp */
            char extra;   /* anything left after the timestamp */

            /* Exactly "start <number>" and nothing else: a second
               number (or any junk) makes sscanf report 2, and a
               missing number makes it report 0 - both wrong. */
            if (sscanf(p, "start %ld %c", &stamp, &extra) != 1)
            {
                fclose(fp);
                return line_no;
            }

            s->start = (time_t) stamp;
            have_start = 1;
            continue;
        }

        /* Everything else must be a tick line, and it can only come
           after the start line: "0 20.0 50 0x0" as the first content
           line means the start line never happened. */
        if (!have_start)
        {
            fclose(fp);
            return line_no;
        }

        {
            int tick;         /* the parsed tick */
            float temp;       /* the parsed temperature */
            float rh;         /* the parsed humidity */
            char sw_text[64]; /* the switch field, still text */
            char extra;       /* anything after the four fields */
            int fields;       /* how many fields sscanf found */
            char *sw_end;     /* where strtoul stopped */
            unsigned long sw; /* the parsed switch register */

            /* Four fields and nothing more: fewer means a missing
               field, more means junk the format does not allow. */
            fields = sscanf(p, "%d %f %f %63s %c",
                            &tick, &temp, &rh, sw_text, &extra);
            if (fields != 4)
            {
                fclose(fp);
                return line_no;
            }

            /* Seconds since the start cannot be negative. */
            if (tick < 0)
            {
                fclose(fp);
                return line_no;
            }

            /* Ticks must increase: a repeat or a step backwards
               would leave two lines fighting over the same tick. */
            if (s->num_lines > 0 && tick <= prev_tick)
            {
                fclose(fp);
                return line_no;
            }

            /* The fixed array holds SCENARIO_MAX_LINES entries; the
               next one would run past the end of the struct. */
            if (s->num_lines >= SCENARIO_MAX_LINES)
            {
                fclose(fp);
                return line_no;
            }

            /* The switch field is hexadecimal. strtoul with base 16
               accepts the "0x" prefix the format uses and bare hex
               alike; reject a sign, empty text, leftover characters,
               and anything wider than 32 bits. */
            if (sw_text[0] == '-' || sw_text[0] == '+')
            {
                fclose(fp);
                return line_no;
            }
            sw = strtoul(sw_text, &sw_end, 16);
            if (sw_end == sw_text || *sw_end != '\0' || sw > 0xFFFFFFFFul)
            {
                fclose(fp);
                return line_no;
            }

            /* Everything checks out: store the line. */
            s->lines[s->num_lines].tick = tick;
            s->lines[s->num_lines].temp_c = temp;
            s->lines[s->num_lines].rh_pct = rh;
            s->lines[s->num_lines].switches = (uint32_t) sw;
            s->num_lines = s->num_lines + 1;
            prev_tick = tick;
        }
    }

    fclose(fp);

    /* The file ended without a start line: wrong from its very first
       line (an empty file lands here too, and is wrong the same
       way - there is nothing to replay). */
    if (!have_start)
        return 1;

    return 0;
}

const scenario_line_t *scenario_at(const scenario_t *s, int tick)
{
    const scenario_line_t *in_force;
    int i;

    /* Walk the lines in order and remember the last one whose tick
       has arrived; that is the line whose values still hold. The
       lines are increasing, so the first line past `tick` ends the
       search - no need to look further. */
    in_force = NULL;
    for (i = 0; i < s->num_lines; i++)
    {
        if (s->lines[i].tick <= tick)
            in_force = &s->lines[i];
        else
            break;
    }

    /* NULL before the first line (or on an empty scenario): no line
       has started yet, so nothing is in force. */
    return in_force;
}
