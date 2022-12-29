/*
 * Name
 *  bilinear.c -- use bilinear interpolation for given row, col
 *
 * Description
 *  bilinear interpolation for the given row, column indices.
 *  If the given row or column is outside the bounds of the input map,
 *  the point in the output map is set to NULL.
 *  If any of the 4 surrounding points to be used in the interpolation
 *  is NULL it is filled with is neighbor value
 */

#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "r.proj.h"

void p_bilinear(struct cache *ibuffer,	/* input buffer                  */
		void *obufptr,	/* ptr in output buffer          */
		int cell_type,	/* raster map type of obufptr    */
		double col_idx,	/* column index          */
		double row_idx,	/* row index                     */
		struct Cell_head *cellhd	/* information of output map     */
    )
{
    int row;			/* row indices for interp        */
    int col;			/* column indices for interp     */
    int i, j;
    FCELL t, u;			/* intermediate slope            */
    FCELL result;		/* result of interpolation       */
    FCELL c[2][2];

    /* cut indices to integer */
    row = (int)floor(row_idx - 0.5);
    col = (int)floor(col_idx - 0.5);

    /* check for out of bounds - if out of bounds set NULL value and return */
    if (row < 0 || row + 1 >= cellhd->rows || col < 0 || col + 1 >= cellhd->cols) {
	Rast_set_null_value(obufptr, 1, cell_type);
	return;
    }

    for (i = 0; i < 2; i++)
	for (j = 0; j < 2; j++) {
	    const FCELL cell = CVAL(ibuffer, row + i, col + j);
	    if (Rast_is_f_null_value(&cell)) {
		Rast_set_null_value(obufptr, 1, cell_type);
		return;
	    }
	    c[i][j] = cell;
	}

    /* do the interpolation  */
    t = col_idx - 0.5 - col;
    u = row_idx - 0.5 - row;

    result = Rast_interp_bilinear(t, u, c[0][0], c[0][1], c[1][0], c[1][1]);

    Rast_set_f_value(obufptr, result, cell_type);
}
