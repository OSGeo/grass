/*
 * Name
 *  lanczos.c -- use lanczos interpolation for given row, col
 *
 * Description
 *  lanczos returns the value in the buffer that is the result of
 *  lanczos interpolation for the given row, column indices.
 *  If the given row or column is outside the bounds of the input map,
 *  the corresponding point in the output map is set to NULL.
 *
 *  If single surrounding points in the interpolation matrix are
 *  NULL they where filled with their neighbor
 */

#include <grass/gis.h>
#include <grass/raster.h>
#include <math.h>
#include "global.h"

void p_lanczos(struct cache *ibuffer,	/* input buffer                  */
	     void *obufptr,	/* ptr in output buffer          */
	     int cell_type,	/* raster map type of obufptr    */
	     double *row_idx,	/* row index (decimal)           */
	     double *col_idx,	/* column index (decimal)        */
	     struct Cell_head *cellhd	/* information of output map     */
    )
{
    int row;			/* row indices for interp        */
    int col;			/* column indices for interp     */
    int i, j, k;
    DCELL t, u;			/* intermediate slope            */
    DCELL result;		/* result of interpolation       */
    DCELL cell[25];

    /* cut indices to integer */
    row = (int)floor(*row_idx);
    col = (int)floor(*col_idx);

    /* check for out of bounds of map - if out of bounds set NULL value     */
    if (row - 2 < 0 || row + 2 >= cellhd->rows ||
	col - 2 < 0 || col + 2 >= cellhd->cols) {
	Rast_set_null_value(obufptr, 1, cell_type);
	return;
    }

    k = 0;
    for (i = 0; i < 5; i++) {
	for (j = 0; j < 5; j++) {
	    const DCELL *cellp = CPTR(ibuffer, row - 2 + i, col - 2 + j);
	    if (Rast_is_d_null_value(cellp)) {
		Rast_set_null_value(obufptr, 1, cell_type);
		return;
	    }
	    cell[k++] = *cellp;
	}
    }

    /* do the interpolation  */
    t = *col_idx - 0.5 - col;
    u = *row_idx - 0.5 - row;

    result = Rast_interp_lanczos(t, u, cell);

    Rast_set_d_value(obufptr, result, cell_type);
}

void p_lanczos_f(struct cache *ibuffer,	/* input buffer                  */
	     void *obufptr,	/* ptr in output buffer          */
	     int cell_type,	/* raster map type of obufptr    */
	     double *row_idx,	/* row index (decimal)           */
	     double *col_idx,	/* column index (decimal)        */
	     struct Cell_head *cellhd	/* information of output map     */
    )
{
    int row;			/* row indices for interp        */
    int col;			/* column indices for interp     */
    DCELL *cellp, cell;

    /* cut indices to integer */
    row = (int)floor(*row_idx);
    col = (int)floor(*col_idx);

    /* check for out of bounds - if out of bounds set NULL value     */
    if (row < 0 || row >= cellhd->rows || col < 0 || col >= cellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    cellp = CPTR(ibuffer, row, col);
    /* if nearest is null, all the other interps will be null */
    if (Rast_is_d_null_value(cellp)) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }
    cell = *cellp;

    p_lanczos(ibuffer, obufptr, cell_type, row_idx, col_idx, cellhd);
    /* fallback to bicubic if lanczos is null */
    if (Rast_is_d_null_value(obufptr)) {
        p_cubic(ibuffer, obufptr, cell_type, row_idx, col_idx, cellhd);
	/* fallback to bilinear if cubic is null */
	if (Rast_is_d_null_value(obufptr)) {
	    p_bilinear(ibuffer, obufptr, cell_type, row_idx, col_idx, cellhd);
	    /* fallback to nearest if bilinear is null */
	    if (Rast_is_d_null_value(obufptr))
		Rast_set_d_value(obufptr, cell, cell_type);
	}
    }
}
