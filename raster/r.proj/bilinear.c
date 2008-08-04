
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
 *
 *  See: Press, W.H. et al. (1992), Numerical recipes in C.
 */

#include <math.h>
#include <grass/gis.h>
#include "local_proto.h"

void p_bilinear(FCELL ** ibuffer,	/* input buffer                 */
		void *obufptr,	/* ptr in output buffer         */
		int cell_type,	/* raster map type of obufptr   */
		double *col_idx,	/* column index                 */
		double *row_idx,	/* row index                    */
		struct Cell_head *cellhd	/* information of output map    */
    )
{
    int row1,			/* lower row index for interp   */
      row2,			/* upper row index for interp   */
      col1,			/* lower column index for interp */
      col2,			/* upper column index for interp */
      row, col,			/* Coordinates of pixel center  */
      x1, x2, y1, y2;		/* temp. buffer indices         */
    FCELL t, u,			/* intermediate slope           */
      tu,			/* t * u                        */
      result;			/* result of interpolation      */



    /* cut indices to integer */
    row = (int)floor(*row_idx);
    col = (int)floor(*col_idx);

    /* check for out of bounds - if out of bounds set NULL value and return */
    /* check for NULL value                                                 */
    if (row < 0 || row >= cellhd->rows ||
	col < 0 || col >= cellhd->cols ||
	G_is_f_null_value(&ibuffer[row][col])) {
	G_set_null_value(obufptr, 1, cell_type);
	return;
    }

    /* get the four surrounding pixel positions     */
    row1 = (*row_idx < row) ? row - 1 : row;
    col1 = (*col_idx < col) ? col - 1 : col;

    row2 = row1 + 1;
    col2 = col1 + 1;

    x1 = (col1 < 0) ? col2 : col1;
    x2 = (col2 >= cellhd->cols) ? col1 : col2;
    y1 = (row1 < 0) ? row2 : row1;
    y2 = (row2 >= cellhd->rows) ? row1 : row2;

    /* check for NULL value - if a single pixel is NULL,           */
    /* fill it with neighbor value                                 */
    if (G_is_f_null_value(&ibuffer[y1][x1])) {
	y1 = row;
	x1 = col;
    }

    if (G_is_f_null_value(&ibuffer[y1][x2])) {
	y1 = row;
	x2 = col;
    }

    if (G_is_f_null_value(&ibuffer[y2][x1])) {
	y2 = row;
	x1 = col;
    }

    if (G_is_f_null_value(&ibuffer[y2][x2])) {
	y2 = row;
	x2 = col;
    }

    /* do the interpolation         */
    t = *col_idx - col1;
    u = *row_idx - row1;
    tu = t * u;


    result = ((1 - t - u + tu) * ibuffer[y1][x1]) +
	((t - tu) * ibuffer[y1][x2]) +
	((u - tu) * ibuffer[y2][x1]) + (tu * ibuffer[y2][x2]);

    switch (cell_type) {
    case CELL_TYPE:
	G_set_raster_value_c(obufptr, (CELL) result, cell_type);
	break;
    case FCELL_TYPE:
	G_set_raster_value_f(obufptr, (FCELL) result, cell_type);
	break;
    case DCELL_TYPE:
	G_set_raster_value_d(obufptr, (DCELL) result, cell_type);
	break;
    }
    return;
}
