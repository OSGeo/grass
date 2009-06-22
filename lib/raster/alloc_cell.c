/*!
 * \file gis/alloc_cell.c
 *
 * \brief GIS Library - Raster allocation routines.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>

/* convert type "RASTER_MAP_TYPE" into index */
#define F2I(map_type) \
	(map_type == CELL_TYPE ? 0 : (map_type == FCELL_TYPE ? 1 : 2))

static const int type_size[3] = { sizeof(CELL), sizeof(FCELL), sizeof(DCELL) };

/*!
 * \brief Returns size of a raster CELL in bytes.
 *
 *  - If <i>data_type</i> is CELL_TYPE, returns sizeof(CELL)
 *  - If <i>data_type</i> is FCELL_TYPE, returns sizeof(FCELL)
 *  - If <i>data_type</i> is DCELL_TYPE, returns sizeof(DCELL)
 *
 * \param data_type raster type (CELL, FCELL, DCELL)
 *
 * \return raster type size
 */

size_t Rast_raster_size(RASTER_MAP_TYPE data_type)
{
    return (type_size[F2I(data_type)]);
}

/*!
 * \brief Allocate memory for a CELL type raster map.
 *
 * This routine allocates a buffer of type CELL just large enough to 
 * hold one row of raster data based on the number of columns in the 
 * active region.
 *
 \code
 CELL *cell;
 cell = Rast_allocate_cell_buf();
 \endcode
 *
 * If larger buffers are required, the routine G_malloc() can be used.
 * The routine is generally used with each open cell file.
 *
 * <b>Note:</b> Rast_allocate_raster_buf() or G_alloc_c_raster_buf() is
 * preferred over Rast_allocate_cell_buf().
 *
 * Prints error message and calls exit() on error.
 *
 * \return CELL * pointer to allocated buffer
 */
CELL *Rast_allocate_cell_buf(void)
{
    return (CELL *) G_calloc(G_window_cols() + 1, sizeof(CELL));
}

/*!
 * \brief Allocate memory for a raster map of given type
 *
 * Allocate an array of CELL, FCELL, or DCELL (depending on
 * <i>data_type</i>) based on the number of columns in the current
 * region.
 *
 * \param data_type raster type (CELL, FCELL, DCELL)
 *
 * \return pointer to allocated buffer
 */
void *Rast_allocate_raster_buf(RASTER_MAP_TYPE data_type)
{
    return (void *)G_calloc(G_window_cols() + 1, Rast_raster_size(data_type));
}

/*!
 * \brief Allocates memory for a raster map of type CELL.
 *
 * Allocate an array of CELL based on the number of columns in the
 * current region.
 *
 * \return pointer to allocated buffer
 */
CELL *Rast_allocate_c_raster_buf(void)
{
    return (CELL *) G_calloc(G_window_cols() + 1, sizeof(CELL));
}

/*!
 * \brief Allocates memory for a raster map of type FCELL.
 *
 * Allocate an array of FCELL based on the number of columns in the 
 * current region.
 *
 * \return pointer to allocated buffer
 */
FCELL *Rast_allocate_f_raster_buf(void)
{
    return (FCELL *) G_calloc(G_window_cols() + 1, sizeof(FCELL));
}

/*!
 * \brief Allocates memory for a raster map of type DCELL.
 *
 * Allocate an array of DCELL based on the number of columns in the
 * current region.
 *
 * \return pointer to allocated buffer
 */
DCELL *Rast_allocate_d_raster_buf(void)
{
    return (DCELL *) G_calloc(G_window_cols() + 1, sizeof(DCELL));
}

/*!
 * \brief Allocates memory for a null buffer.
 *
 * Allocate an array of char based on the number of columns in the 
 * current region.
 *
 * \return pointer to allocated buffer
 */
char *Rast_allocate_null_buf(void)
{
    return (char *)G_calloc(G_window_cols() + 1, sizeof(char));
}

/*!
 * \brief Allocates memory for null bits.
 *
 * Allocates an array of unsigned char based on <i>cols</i>.
 *
 * \param cols number of columns in region
 * 
 * \return pointer to allocated buffer
 */
unsigned char *Rast__allocate_null_bits(int cols)
{
    return (unsigned char *)G_calloc(Rast__null_bitstream_size(cols) + 1,
				     sizeof(unsigned char));
}

/*!
 * \brief Determines null bitstream size.
 *
 * \param cols number of columns
 *
 * \return -1 if <i>cols</i> is invalid (<= 0)
 * \return size of null bistream
 */
int Rast__null_bitstream_size(int cols)
{
    if (cols <= 0)
	return -1;

    return (cols / 8 + (cols % 8 != 0));
}
