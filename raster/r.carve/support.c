
/****************************************************************************
 *
 * MODULE:       r.carve
 *
 * AUTHOR(S):    Original author Bill Brown, UIUC GIS Laboratory
 *               Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:      Takes vector stream data, converts it to 3D raster and
 *               subtracts a specified depth
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "enforce.h"


/*
 * update_rast_history - Update a history file.  Some of the digit file 
 * information is placed in the history file.
 * returns  0  -  successful creation of history file
 *         -1  -  error
 */
int update_rast_history(struct parms *parm)
{
    struct History hist;

    /* write command line to history */
    G_short_history(parm->outrast->answer, "raster", &hist);
    sprintf(hist.edhist[0], "%s version %.2f", G_program_name(), APP_VERSION);
    sprintf(hist.edhist[1], "stream width: %.2f", parm->swidth * 2);
    sprintf(hist.datsrc_1, "raster elevation file: %s", parm->inrast->answer);
    sprintf(hist.datsrc_2, "vector stream file: %s", parm->invect->answer);
    hist.edlinecnt = 2;
    G_command_history(&hist);

    return G_write_history(parm->outrast->answer, &hist);
}
