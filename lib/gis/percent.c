
/**
 * \file percent.c
 *
 * \brief GIS Library - percentage progress functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <stdio.h>
#include <grass/gis.h>

static struct state {
    int prev;
    int first;
} state = {-1, 1};

static struct state *st = &state;

void G_percent(long n, long d, int s)
{
    int x, format;

    format = G_info_format();

    x = (d <= 0 || s <= 0)
	? 100 : (int)(100 * n / d);

    /* be verbose only 1> */
    if (format == G_INFO_FORMAT_SILENT || G_verbose() < 1)
	return;

    if (n <= 0 || n >= d || x > st->prev + s) {
	st->prev = x;

	if (format == G_INFO_FORMAT_STANDARD) {
	    fprintf(stderr, "%4d%%\b\b\b\b\b", x);
	}
	else {
	    if (format == G_INFO_FORMAT_PLAIN) {
		if (x == 100)
		    fprintf(stderr, "%d\n", x);
		else
		    fprintf(stderr, "%d..", x);
	    }
	    else {		/* GUI */
		if (st->first) {
		    fprintf(stderr, "\n");
		}
		fprintf(stderr, "GRASS_INFO_PERCENT: %d\n", x);
		fflush(stderr);
		st->first = 0;
	    }
	}
    }

    if (x >= 100) {
	if (format == G_INFO_FORMAT_STANDARD) {
	    fprintf(stderr, "\n");
	}
	st->prev = -1;
	st->first = 1;
    }

    return;
}


/**
 * \brief Reset G_percent() to 0%; do not add newline.
 *
 * \return always returns 0
 */

void G_percent_reset(void)
{
    st->prev = -1;
    st->first = 1;
}
