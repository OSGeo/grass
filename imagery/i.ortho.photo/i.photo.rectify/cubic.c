/*
 * Name
 *  cubic.c -- use cubic convolution interpolation for given row, col
 *
 * Description
 *  cubic returns the value in the buffer that is the result of cubic
 *  convolution interpolation for the given row, column indices.
 *  If the given row or column is outside the bounds of the input map,
 *  the corresponding point in the output map is set to NULL.
 *
 *  If single surrounding points in the interpolation matrix are
 *  NULL they where filled with their neighbor
 */

#include <math.h>
#include "global.h"

void p_cubic(struct cache *ibuffer,    /* input buffer                */
	     void *obufptr,	       /* ptr in output buffer        */
	     int cell_type,	       /* raster map type of obufptr  */
	     double *row_idx,	       /* row index (decimal)         */
	     double *col_idx,	       /* column index (decimal)      */
	     struct Cell_head *cellhd  /* information of output map   */
    )
{
    int row;			/* row indices for interp        */
    int col;			/* column indices for interp     */
    int i, j;
    double t, u;		/* intermediate slope            */
    DCELL result;		/* result of interpolation       */
    DCELL val[4];		/* buffer for temporary values   */
    DCELL cell[4][4];

    /* cut indices to integer */
    row = (int)floor(*row_idx - 0.5);
    col = (int)floor(*col_idx - 0.5);

    /* check for out of bounds of map - if out of bounds set NULL value     */
    if (row - 1 < 0 || row + 2 >= cellhd->rows ||
	col - 1 < 0 || col + 2 >= cellhd->cols) {
	G_set_null_value(obufptr, 1, cell_type);
	return;
    }

    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++) {
	    const DCELL *cellp = CPTR(ibuffer, row - 1 + i, col - 1 + j);
	    if (G_is_d_null_value(cellp)) {
		G_set_null_value(obufptr, 1, cell_type);
		return;
	    }
	    cell[i][j] = *cellp;
	}

    /* do the interpolation  */
    t = *col_idx - 0.5 - col;
    u = *row_idx - 0.5 - row;

    for (i = 0; i < 4; i++) {
	val[i] = G_interp_cubic(t, cell[i][0], cell[i][1], cell[i][2], cell[i][3]);
    }

    result = G_interp_cubic(u, val[0], val[1], val[2], val[3]);

    G_set_raster_value_d(obufptr, result, cell_type);
}
