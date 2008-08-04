
/*
 *      nearest.c - returns the nearest neighbor to a given
 *                  x,y position
 */

#include <math.h>
#include <grass/gis.h>
#include "r.proj.h"

void p_nearest(struct cache *ibuffer,	/* input buffer                  */
	       void *obufptr,	/* ptr in output buffer          */
	       int cell_type,	/* raster map type of obufptr    */
	       double *col_idx,	/* column index in input matrix  */
	       double *row_idx,	/* row index in input matrix     */
	       struct Cell_head *cellhd	/* cell header of input layer    */
    )
{
    int row, col;		/* row/col of nearest neighbor   */
    FCELL *cellp;

    /* cut indices to integer */
    row = (int)floor(*row_idx);
    col = (int)floor(*col_idx);

    /* check for out of bounds - if out of bounds set NULL value     */
    if (row < 0 || row >= cellhd->rows || col < 0 || col >= cellhd->cols) {
	G_set_null_value(obufptr, 1, cell_type);
	return;
    }

    cellp = CPTR(ibuffer, row, col);

    if (G_is_f_null_value(cellp)) {
	G_set_null_value(obufptr, 1, cell_type);
	return;
    }

    G_set_raster_value_f(obufptr, *cellp, cell_type);
}
