/*!
 * \file gis/zero_cell.c
 *
 * \brief GIS Library - Zero cell buffer functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>

/*!
 * \brief Zero a raster CELL buffer.
 *
 * This routines assigns each member of the raster buffer array
 * <i>buf</i> to zero. It assumes that <i>buf</i> has been allocated
 * using G_allocate_cell_buf().
 *
 * \param buf data buffer
 */

void G_zero_cell_buf(CELL * buf)
{
    int i = G_window_cols();

    while (i--)
	*buf++ = 0;
}

/*!
 * \brief Zero a raster buffer.
 *
 * This routines assigns each member of the raster buffer array
 * <i>rast</i> to zero. It assumes that <i>rast</i> has been allocated
 * using G_allocate_cell_buf().
 *
 * \param rast data buffer
 * \param data_type raster type (CELL, FCELL, DCELL)
 */
void G_zero_raster_buf(void *rast, RASTER_MAP_TYPE data_type)
{
    int i;
    unsigned char *ptr;

    /* assuming that the size of unsigned char is 1 byte */
    i = G_window_cols() * G_raster_size(data_type);
    ptr = (unsigned char *)rast;

    while (i--)
	*ptr++ = 0;
}
