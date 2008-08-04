
/****************************************************************************
 *
 * MODULE:       r.digit
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Interactive tool used to draw and save vector features
 *               on a graphics monitor using a pointing device (mouse)
 *               and save to a raster map.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdio.h>
#include "local_proto.h"


#define MAXLINES 18
#define TITLE (char *) 0

static char *Title = TITLE;

static int nlines = 0;


int instructions(int n)
{
    if (n == 0)
	nlines = MAXLINES;
    if (nlines >= MAXLINES) {
	if (Title)
	    fprintf(stdout, "%s\n", Title);
	fprintf(stdout, " Buttons:\n");
	fprintf(stdout, "  Left:   where am i\n");
	fprintf(stdout, "  Middle: mark point\n");
	fprintf(stdout, "  Right:  done\n\n");
	nlines = 0;
    }
    nlines += n;

    return 0;
}
