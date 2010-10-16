
/*
 *      nearest.c - returns the nearest neighbor to a given
 *                  x,y position
 */

#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "global.h"

void p_nearest(struct cache *ibuffer,	/* input buffer                  */
	       void *obufptr,	/* ptr in output buffer          */
	       int cell_type,	/* raster map type of obufptr    */
	       double *row_idx,	/* row index in input matrix     */
	       double *col_idx,	/* column index in input matrix  */
	       struct Cell_head *cellhd	/* cell header of input layer    */
    )
{
    int row, col;		/* row/col of nearest neighbor   */
    DCELL *cellp;

    /* cut indices to integer */
    row = (int)floor(*row_idx);
    col = (int)floor(*col_idx);

    /* check for out of bounds - if out of bounds set NULL value     */
    if (row < 0 || row >= cellhd->rows || col < 0 || col >= cellhd->cols) {
	Rast_set_null_value(obufptr, 1, cell_type);
	return;
    }

    cellp = CPTR(ibuffer, row, col);

    if (Rast_is_d_null_value(cellp)) {
	Rast_set_null_value(obufptr, 1, cell_type);
	return;
    }

    Rast_set_d_value(obufptr, *cellp, cell_type);
}
