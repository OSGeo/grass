/*!
 * \file lib/raster/get_row_colr.c
 * 
 * \brief Raster Library - Get raster row (colors)
 *
 * (C) 1999-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author USACERL and many others
 */

#include <grass/gis.h>
#include <grass/raster.h>

#include "R.h"

/*!
 * \brief Reads a row of raster data and converts it to RGB.
 *
 * Reads a row of raster data and converts it to red, green and blue
 * components according to the <em>colors</em> parameter. This
 * provides a convenient way to treat a raster layer as a color image
 * without having to explicitly cater for each of <tt>CELL</tt>,
 * <tt>FCELL</tt> and <tt>DCELL</tt> types.
 *
 *  \param fd field descriptor
 *  \param row row number
 *  \param colors pointer to Colors structure which holds color info
 *  \param[out] red red value
 *  \param[out] grn green value
 *  \param[out] blu blue value
 *  \param[out] nul null value
 *
 *  \return void
 */
void Rast_get_row_colors(int fd, int row, struct Colors *colors,
			 unsigned char *red, unsigned char *grn,
			 unsigned char *blu, unsigned char *nul)
{
    int cols = Rast_window_cols();
    int type = Rast_get_map_type(fd);
    int size = Rast_cell_size(type);
    void *array;
    unsigned char *set;
    void *p;
    int i;

    array = G_malloc(cols * size);

    Rast_get_row(fd, array, row, type);

    if (nul)
	for (i = 0, p = array; i < cols; i++, p = G_incr_void_ptr(p, size))
	    nul[i] = Rast_is_null_value(p, type);

    set = G_malloc(cols);

    Rast_lookup_colors(array, red, grn, blu, set, cols, colors, type);

    G_free(array);
    G_free(set);
}
