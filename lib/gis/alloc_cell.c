
/**
 * \file alloc_cell.c
 *
 * \brief GIS Library - Raster allocation routines.
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

#include <math.h>
#include <grass/gis.h>

/* convert type "RASTER_MAP_TYPE" into index */
#define F2I(map_type) \
	(map_type == CELL_TYPE ? 0 : (map_type == FCELL_TYPE ? 1 : 2))

static int type_size[3] = { sizeof(CELL), sizeof(FCELL), sizeof(DCELL) };


/**
 * \brief Returns size of a raster CELL in bytes.
 *
 * If <b>data_type</b> is CELL_TYPE, returns sizeof(CELL)
 * If <b>data_type</b> is FCELL_TYPE, returns sizeof(FCELL)
 * If <b>data_type</b> is DCELL_TYPE, returns sizeof(DCELL)
 *
 * \param[in] data_type
 * \return int
 */

size_t G_raster_size(RASTER_MAP_TYPE data_type)
{
    return (type_size[F2I(data_type)]);
}


/**
 * \brief Allocate memory for a CELL type raster map.
 *
 * This routine allocates a buffer of type CELL just large enough to 
 * hold one row of raster data based on the number of columns in the 
 * active region.<br>
 \code
 CELL *cell;
 cell = G_allocate_cell_buf ();
 \endcode
 * If larger buffers are required, the routine <i>G_malloc</i> can be used.
 * The routine is generally used with each open cell file.<br>
 * <b>Note:</b> <i>G_allocate_raster_buf()</i> or 
 * <i>G_alloc_c_raster_buf()</i> is preferred over 
 * <i>G_allocate_cell_buf()</i>.
 *
 * \return CELL * Pointer to allocated buffer
 * \return Prints error message and calls <i>exit()</i> on error
 */

CELL *G_allocate_cell_buf(void)
{
    return (CELL *) G_calloc(G_window_cols() + 1, sizeof(CELL));
}


/**
 * \brief Allocate memory for a raster map of type <b>data_type</b>.
 *
 * Allocate an array of CELL, FCELL, or DCELL (depending on 
 * <b>data_type</b>) based on the number of columns in the current 
 * region.
 *
 * \param[in] data_type
 * \return void * 
 */

void *G_allocate_raster_buf(RASTER_MAP_TYPE data_type)
{
    return (void *)G_calloc(G_window_cols() + 1, G_raster_size(data_type));
}


/**
 * \brief Allocates memory for a raster map of type CELL.
 *
 * Allocate an array of CELL based on the number of columns in the 
 * current region.
 *
 * \return CELL * 
 */

CELL *G_allocate_c_raster_buf(void)
{
    return (CELL *) G_calloc(G_window_cols() + 1, sizeof(CELL));
}


/**
 * \brief Allocates memory for a raster map of type FCELL.
 *
 * Allocate an array of FCELL based on the number of columns in the 
 * current region.
 *
 * \return FCELL * 
 */

FCELL *G_allocate_f_raster_buf(void)
{
    return (FCELL *) G_calloc(G_window_cols() + 1, sizeof(FCELL));
}


/**
 * \brief Allocates memory for a raster map of type DCELL.
 *
 * Allocate an array of DCELL based on the number of columns in the 
 * current region.
 *
 * \return DCELL * 
 */

DCELL *G_allocate_d_raster_buf(void)
{
    return (DCELL *) G_calloc(G_window_cols() + 1, sizeof(DCELL));
}


/**
 * \brief Allocates memory for a null buffer.
 *
 * Allocate an array of char based on the number of columns in the 
 * current region.
 *
 * \return char * 
 */

char *G_allocate_null_buf(void)
{
    return (char *)G_calloc(G_window_cols() + 1, sizeof(char));
}


/**
 * \brief Allocates memory for null bits.
 *
 * Allocates an array of unsigned char based on <b>cols</b>.
 *
 * \param[in] cols number of columns in region
 * \return unsigned char *
 */

unsigned char *G__allocate_null_bits(int cols)
{
    return (unsigned char *)G_calloc(G__null_bitstream_size(cols) + 1,
				     sizeof(unsigned char));
}


/**
 * \brief Determines null bitstream size.
 *
 * \param[in] cols number of columns
 * \return -1 if <b>cols</b> is invalid (<= 0)
 * \return size of null bistream
 */

int G__null_bitstream_size(int cols)
{
    if (cols <= 0)
	return -1;

    return (cols / 8 + (cols % 8 != 0));
}
