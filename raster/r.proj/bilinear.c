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
		double *col_idx,	/* column index          */
		double *row_idx,	/* row index                     */
		struct Cell_head *cellhd	/* information of output map     */
    )
{
    int row0,			/* lower row index for interp    */
      row1,			/* upper row index for interp    */
      col0,			/* lower column index for interp */
      col1;			/* upper column index for interp */
    FCELL t, u,			/* intermediate slope            */
      tu,			/* t * u                         */
      result;			/* result of interpolation       */
    FCELL *c00, *c01, *c10, *c11;

    /* cut indices to integer */
    row0 = (int)floor(*row_idx - 0.5);
    col0 = (int)floor(*col_idx - 0.5);
    row1 = row0 + 1;
    col1 = col0 + 1;

    /* check for out of bounds - if out of bounds set NULL value and return */
    if (row0 < 0 || row1 >= cellhd->rows || col0 < 0 || col1 >= cellhd->cols) {
	Rast_set_null_value(obufptr, 1, cell_type);
	return;
    }

    c00 = CPTR(ibuffer, row0, col0);
    c01 = CPTR(ibuffer, row0, col1);
    c10 = CPTR(ibuffer, row1, col0);
    c11 = CPTR(ibuffer, row1, col1);

    /* check for NULL values */
    if (Rast_is_f_null_value(c00) ||
	Rast_is_f_null_value(c01) ||
	Rast_is_f_null_value(c10) || Rast_is_f_null_value(c11)) {
	Rast_set_null_value(obufptr, 1, cell_type);
	return;
    }

    /* do the interpolation  */
    t = *col_idx - 0.5 - col0;
    u = *row_idx - 0.5 - row0;
    tu = t * u;

    result = Rast_interp_bilinear(t, u, *c00, *c01, *c10, *c11);

    Rast_set_f_value(obufptr, result, cell_type);
}
