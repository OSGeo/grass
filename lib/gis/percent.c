
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


static int prev = -1;
static int first = 1;


int G_percent(long n, long d, int s)
{
    int x, format;

    format = G_info_format();

    x = (d <= 0 || s <= 0)
	? 100 : (int)(100 * n / d);

    /* be verbose only 1> */
    if (format == G_INFO_FORMAT_SILENT || G_verbose() < 1)
	return 0;

    if (n <= 0 || n >= d || x > prev + s) {
	prev = x;

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
		if (first) {
		    fprintf(stderr, "\n");
		}
		fprintf(stderr, "GRASS_INFO_PERCENT: %d\n", x);
		fflush(stderr);
		first = 0;
	    }
	}
    }

    if (x >= 100) {
	if (format == G_INFO_FORMAT_STANDARD) {
	    fprintf(stderr, "\n");
	}
	prev = -1;
	first = 1;
    }

    return 0;
}


/**
 * \brief Reset G_percent() to 0%; do not add newline.
 *
 * \return always returns 0
 */

int G_percent_reset(void)
{
    prev = -1;
    first = 1;

    return 0;
}
