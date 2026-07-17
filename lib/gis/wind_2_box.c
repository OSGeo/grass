/*!
 * \file lib/gis/wind_2_box.c
 *
 * \brief GIS Library - Window box functions.
 *
 * SPDX-FileCopyrightText: 2001-2014 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \author GRASS Development Team
 *
 * \date 1999-2014
 */

#include <grass/gis.h>

/**
 * \brief Adjusts window to a rectangular box.
 *
 * Creates a new window <b>dst</b> from a window <b>src</b> which fits
 * into the rectangular box with dimensions <b>rows</b> by <b>cols</b>.
 *
 * \param[in] src source window
 * \param[in,out] dst destination window
 * \param[in] rows number of rows in box
 * \param[in] cols number of columns in box
 * \return
 */
void G_adjust_window_to_box(const struct Cell_head *src, struct Cell_head *dst,
                            int rows, int cols)
{
    double ew, ns;

    *dst = *src;

    /* calculate the effective resolutions */
    ns = (src->ns_res * src->rows) / rows;
    ew = (src->ew_res * src->cols) / cols;

    /* set both resolutions equal to the larger */
    if (ns > ew)
        ew = ns;
    else
        ns = ew;

    dst->ns_res = ns;
    dst->ew_res = ew;

    /* compute rows and cols */
    dst->rows = (dst->north - dst->south) / dst->ns_res;
    dst->cols = (dst->east - dst->west) / dst->ew_res;
}
