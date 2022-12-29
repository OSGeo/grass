
/*-
 * G_clicker()
 * 
 * Print a clock hand (one of '|', '/', '-', '\') to stderr.
 * Used in place of G_percent for unknown number of iterations
 * 
 */
#include <stdio.h>
#include <grass/gis.h>

static struct state {
    int prev;
} state;

static struct state *st = &state;

void G_clicker(void)
{
    static const char clicks[] = "|/-\\";
    int format = G_info_format();

    if (format == G_INFO_FORMAT_SILENT || G_verbose() < 1)
	return;

    st->prev++;
    st->prev %= 4;

    fprintf(stderr, "%1c\b", clicks[st->prev]);
    fflush(stderr);
}
