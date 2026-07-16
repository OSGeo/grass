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
 * SPDX-FileCopyrightText: 2006 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "enforce.h"

/*
 * update_rast_history - Update a history file.  Some of the digit file
 * information is placed in the history file.
 */
void update_rast_history(struct parms *parm)
{
    struct History hist;

    /* write command line to history */
    Rast_short_history(parm->outrast->answer, "raster", &hist);
    Rast_append_format_history(&hist, "%s version %.2f", G_program_name(),
                               APP_VERSION);
    Rast_append_format_history(&hist, "stream width: %.2f", parm->swidth * 2);
    Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file: %s",
                        parm->inrast->answer);
    Rast_format_history(&hist, HIST_DATSRC_2, "vector stream file: %s",
                        parm->invect->answer);
    Rast_command_history(&hist);
    Rast_write_history(parm->outrast->answer, &hist);
}

