
/**
 * \file zero_cell.c
 *
 * \brief GIS Library - Zero cell buffer functions.
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
 * \brief Zero a raster CELL buffer.
 *
 * This routines assigns each member of the raster buffer array 
 * <b>buf</b> to zero. It assumes that <b>buf</b> has been allocated 
 * using <i>G_allocate_cell_buf.</i>
 *
 * \param[in] buf
 * \return always returns 0
 */

int G_zero_cell_buf(CELL * buf)
{
    int i = G_window_cols();

    while (i--)
	*buf++ = 0;

    return 0;
}


/**
 * \brief Zero a raster buffer.
 *
 * This routines assigns each member of the raster buffer array 
 * <b>rast</b> to zero. It assumes that <b>rast</b> has been allocated 
 * using <i>G_allocate_cell_buf.</i>
 *
 *  \param[in,out] rast
 *  \param[in] data_type RASTER_MAP_TYPE
 *  \return always returns 0
 */

int G_zero_raster_buf(void *rast, RASTER_MAP_TYPE data_type)
{
    int i;
    unsigned char *ptr;

    /* assuming that the size of unsigned char is 1 byte */
    i = G_window_cols() * G_raster_size(data_type);
    ptr = (unsigned char *)rast;

    while (i--)
	*ptr++ = 0;

    return 0;
}
