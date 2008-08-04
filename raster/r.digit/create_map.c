
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
#include <stdlib.h>


int create_map(char *name, char *polyfile)
{
    char buf[1024];

    sprintf(buf, "r.in.poly rows=512 i=\"%s\" o=\"%s\"", polyfile, name);
    fprintf(stdout, "Creating raster map %s\n", name);
    return system(buf);
}
