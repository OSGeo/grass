
/**
 * \file wind_2_box.c
 *
 * \brief GIS Library - Window box functions.
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
 * \return always returns 0
 */

int G_adjust_window_to_box(const struct Cell_head *src,
			   struct Cell_head *dst, int rows, int cols)
{
    double ew, ns;

    G_copy((char *)dst, (char *)src, sizeof(*dst));

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

    return 0;
}
