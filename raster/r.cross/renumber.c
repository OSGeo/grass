/****************************************************************************
 *
 * MODULE:       r.cross
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Creates a cross product of the category values from
 *               multiple raster map layers.
 *
 * SPDX-FileCopyrightText: 2006 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 ***************************************************************************/

#include <stdlib.h>
#include "glob.h"
#include "local_proto.h"
#include <grass/raster.h>
#include <grass/glocale.h>

int renumber(int in, int out)
{
    CELL *cell, *c;
    int row, col;

    cell = Rast_allocate_c_buf();

    G_message(_("%s: STEP 3 ... "), G_program_name());
    for (row = 0; row < nrows; row++) {
        G_percent(row, nrows, 5);
        Rast_get_c_row(in, c = cell, row);
        col = ncols;
        while (col-- > 0) {
            if (!Rast_is_c_null_value(c))
                *c = table[*c];
            c++;
        }
        Rast_put_row(out, cell, CELL_TYPE);
    }
    G_percent(row, nrows, 10);
    G_free(cell);

    return 0;
}
