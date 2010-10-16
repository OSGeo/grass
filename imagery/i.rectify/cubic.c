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

#include <grass/gis.h>
#include <grass/raster.h>
#include <math.h>
#include "global.h"

void p_cubic(struct cache *ibuffer,	/* input buffer                  */
	     void *obufptr,	/* ptr in output buffer          */
	     int cell_type,	/* raster map type of obufptr    */
	     double *row_idx,	/* row index (decimal)           */
	     double *col_idx,	/* column index (decimal)        */
	     struct Cell_head *cellhd	/* information of output map     */
    )
{
    int row,			/* row indices for interp        */
      col;			/* column indices for interp     */
    int i, j;
    DCELL t, u,			/* intermediate slope            */
      result,			/* result of interpolation       */
      val[4];			/* buffer for temporary values   */
    DCELL *cellp[4][4];

    /* cut indices to integer and get nearest cells */
    /* example: *row_idx = 2.1
     * nearest rows are 0, 1, 2 and 3, not 1, 2, 3 and 4
     * row 0 streches from 0 to 1, row 4 from 4 to 5
     * 2.1 - 1 = 1.1
     * 4 - 2.1 = 1.9 */
    *row_idx -= 0.5;
    *col_idx -= 0.5;
    row = (int)floor(*row_idx);
    col = (int)floor(*col_idx);

    /* check for out of bounds of map - if out of bounds set NULL value     */
    if (row - 1 < 0 || row + 2 >= cellhd->rows ||
	col - 1 < 0 || col + 2 >= cellhd->cols) {
	Rast_set_null_value(obufptr, 1, cell_type);
	return;
    }

    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	    cellp[i][j] = CPTR(ibuffer, row - 1 + i, col - 1 + j);

    /* check for NULL value                                         */
    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++) {
	    if (Rast_is_d_null_value(cellp[i][j])) {
		Rast_set_null_value(obufptr, 1, cell_type);
		return;
	    }
	}

    /* do the interpolation  */
    t = *col_idx - col;
    u = *row_idx - row;

    for (i = 0; i < 4; i++) {
	DCELL **tmp = cellp[i];

	val[i] = Rast_interp_cubic(t, *tmp[0], *tmp[1], *tmp[2], *tmp[3]);
    }

    result = Rast_interp_cubic(u, val[0], val[1], val[2], val[3]);

    Rast_set_d_value(obufptr, result, cell_type);
}
