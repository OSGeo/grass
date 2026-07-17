/*****************************************************************************
 *
 * MODULE:       Vector library
 *
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * SPDX-FileCopyrightText: 2001 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#include <grass/vector.h>

double dig_x_intersect(double beg_x, double end_x, double beg_y, double end_y,
                       double Y)
{
    double b;

    /* assumes beg_y != end_y */

    /* sort for numerical stability
     * ray along X for given Y -> sort by Y */
    if (end_y < beg_y || (end_y == beg_y && end_x < beg_x)) {
        b = end_x;
        end_x = beg_x;
        beg_x = b;

        b = end_y;
        end_y = beg_y;
        beg_y = b;
    }

    /* solve simple linear equation to get X = a + b * Y
     * with
     * b = (end_x - beg_x) / (end_y - beg_y)
     * a = beg_x - b * beg_y
     *
     * simplify a + b * Y:
     * a + b * Y = beg_x - b * beg_y + b * Y
     * a + b * Y = beg_x + b * (Y - beg_y)
     * a + b * Y = beg_x + (end_x - beg_x) * (Y - beg_y) / (end_y - beg_y) */

    b = (Y - beg_y) / (end_y - beg_y); /* always within [0, 1] */

    return beg_x + b * (end_x - beg_x);
}
