/*!
 * \file lib/raster/zero_cell.c
 *
 * \brief Raster Library - Zero cell buffer functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>

/*!
 * \brief Zero a raster buffer.
 *
 * This routines assigns each member of the raster buffer array
 * <i>rast</i> to zero. It assumes that <i>rast</i> has been allocated
 * using Rast_allocate_c_buf().
 *
 * \param rast data buffer
 * \param data_type raster type (CELL_TYPE, FCELL_TYPE, DCELL_TYPE)
 */
void Rast_zero_buf(void *rast, RASTER_MAP_TYPE data_type)
{
    memset(rast, 0, Rast_window_cols() * Rast_cell_size(data_type));
}

void Rast_zero_input_buf(void *rast, RASTER_MAP_TYPE data_type)
{
    memset(rast, 0, Rast_input_window_cols() * Rast_cell_size(data_type));
}

void Rast_zero_output_buf(void *rast, RASTER_MAP_TYPE data_type)
{
    memset(rast, 0, Rast_output_window_cols() * Rast_cell_size(data_type));
}
