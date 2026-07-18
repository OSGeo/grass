/*
 * SPDX-FileCopyrightText: 1994-1995 James Darrell McCauley
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include "quaddefs.h"

void count_sites(COOR *quads, int nquads, int *counts, double radius,
                 struct Map_info *Map, int field)
/*
 * counts the number of sites in the Map that fall within nquads quads of a
 * certain radius
 */
{

    int i, line, nlines;
    struct line_pnts *Points;
    struct line_cats *Cats;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(Map);

    for (line = 1; line <= nlines; line++) {
        int type;

        type = Vect_read_line(Map, Points, Cats, line);

        if (field != -1 && !Vect_cat_get(Cats, field, NULL))
            continue;

        if (!(type & GV_POINT))
            continue;

        for (i = 0; i < nquads; ++i) {
            if (hypot(Points->x[0] - quads[i].x, Points->y[0] - quads[i].y) <=
                radius) {
                counts[i]++;
                break; /* next point, quads don't overlap */
            }
        }
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
}
