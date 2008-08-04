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
 * 
 *  See: Richards, John A. (1993), Remote Sensing Digital Image Analysis,
 *       Springer-Verlag, Berlin, 2nd edition.
 */

#include <math.h>
#include <grass/gis.h>
#include "local_proto.h"



void p_cubic(FCELL ** ibuffer,	/* input buffer                  */
	     void *obufptr,	/* ptr in output buffer          */
	     int cell_type,	/* raster map type of obufptr    */
	     double *col_idx,	/* column index (decimal)        */
	     double *row_idx,	/* row index (decimal)           */
	     struct Cell_head *cellhd	/* information of output map     */
    )
{
    int row,			/* row indices for interp        */
      mrow[4],			/* row in matrix                 */
      col,			/* column indices for interp     */
      mcol[4];			/* column in matrix              */

    int i, j;
    FCELL t, u,			/* intermediate slope            */
      result,			/* result of interpolation       */
      val[4];			/* buffer for temporary values   */



    /* cut indices to integer */
    row = (int)floor(*row_idx);
    col = (int)floor(*col_idx);


    /* check for out of bounds of map - if out of bounds set NULL value  */
    /* check for NULL value                                              */
    if (row < 0 || row >= cellhd->rows ||
	col < 0 || col >= cellhd->cols ||
	G_is_f_null_value(&ibuffer[row][col])) {
	G_set_null_value(obufptr, 1, cell_type);
	return;
    }

    /* get matrix                */
    mrow[1] = (*row_idx < row) ? row - 1 : row;
    mrow[2] = mrow[1] + 1;
    mrow[3] = mrow[1] + 2;
    mrow[0] = mrow[1] - 1;

    mcol[1] = (*col_idx < col) ? col - 1 : col;
    mcol[2] = mcol[1] + 1;
    mcol[3] = mcol[1] + 2;
    mcol[0] = mcol[1] - 1;

    /* get relative index        */
    t = *col_idx - mcol[1];
    u = *row_idx - mrow[1];


    /* if single pixels out of bounds, fill with nearest neighbor        */
    mcol[1] = (mcol[1] < 0) ? mcol[2] : mcol[1];
    mcol[0] = (mcol[0] < 0) ? mcol[1] : mcol[0];
    mcol[2] = (mcol[2] >= cellhd->cols) ? mcol[1] : mcol[2];
    mcol[3] = (mcol[3] >= cellhd->cols) ? mcol[2] : mcol[3];

    mrow[1] = (mrow[1] < 0) ? mrow[2] : mrow[1];
    mrow[0] = (mrow[0] < 0) ? mrow[1] : mrow[0];
    mrow[2] = (mrow[2] >= cellhd->rows) ? mrow[1] : mrow[2];
    mrow[3] = (mrow[3] >= cellhd->rows) ? mrow[2] : mrow[3];


    /* Check all 16 pixels for NULL value        */
    /* (not an optimal solution - check later)   */
    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	    if (G_is_f_null_value(&ibuffer[mrow[i]][mcol[j]])) {
		mrow[i] = row;
		mcol[j] = col;
	    }

    /* and now the interpolation */
    G_debug(3, "Matrix used:");
    for (i = 0; i < 4; i++)
	G_debug(3, "%d %d %d %d",
		(int)ibuffer[mrow[i]][mcol[0]],
		(int)ibuffer[mrow[i]][mcol[1]],
		(int)ibuffer[mrow[i]][mcol[2]],
		(int)ibuffer[mrow[i]][mcol[3]]);

    for (i = 0; i < 4; i++) {
	FCELL *cp = ibuffer[mrow[i]];
	FCELL c0 = cp[mcol[0]];
	FCELL c1 = cp[mcol[1]];
	FCELL c2 = cp[mcol[2]];
	FCELL c3 = cp[mcol[3]];

	val[i] = (t * (t * (t * (c3 - 3 * c2 + 3 * c1 - c0) +
			    (-c3 + 4 * c2 - 5 * c1 + 2 * c0)) +
		       (c2 - c0)) + 2 * c1) / 2;

	G_debug(3, "Ipolval[%d] = %f", i, val[i]);
    }

    result = (u * (u * (u * (val[3] - 3 * val[2] + 3 * val[1] - val[0]) +
			(-val[3] + 4 * val[2] - 5 * val[1] + 2 * val[0])) +
		   (val[2] - val[0])) + 2 * val[1]) / 2;

    G_debug(3, "r1: %d  ridx: %f  c1: %d  cidx: %f  Value: %f",
	    row, u, col, t, result);

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
