
/*****************************************************************************/

/***                                                                       ***/

/***                             process()                                 ***/

/***          Reads in a raster map row by row for processing.		   ***/

/***           Jo Wood, Project ASSIST, V1.0 7th February 1993             ***/

/***                                                                       ***/

/*****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
#include "param.h"
#include "nrutil.h"


void process(void)
{

    /*--------------------------------------------------------------------------*/
    /*                              INITIALISE                                  */

    /*--------------------------------------------------------------------------*/


    DCELL *row_in,		/* Buffer large enough to hold `wsize'  */
     *row_out = NULL,		/* raster rows. When GRASS reads in a   */
	/* raster row, each element is of type  */
	/* DCELL        */
	*window_ptr,		/* Stores local terrain window.         */
	centre,			/* Elevation of central cell in window. */
	*window_cell;		/* Stores last read cell in window. */

    CELL *featrow_out = NULL;	/* store features in CELL */

    struct Cell_head region;	/* Structure to hold region information */

    int nrows,			/* Will store the current number of     */
      ncols,			/* rows and columns in the raster.      */
      row, col,			/* Counts through each row and column   */
	/* of the input raster.                 */
      wind_row,			/* Counts through each row and column   */
      wind_col,			/* of the local neighbourhood window.   */
     *index_ptr;		/* Row permutation vector for LU decomp. */

    double **normal_ptr,	/* Cross-products matrix.               */
     *obs_ptr,			/* Observed vector.                     */
      temp;			/* Unused */

    double *weight_ptr;		/* Weighting matrix for observed values. */
    int found_null;             /* If null was found among window cells  */

    /*--------------------------------------------------------------------------*/
    /*                     GET RASTER AND WINDOW DETAILS                        */

    /*--------------------------------------------------------------------------*/

    G_get_window(&region);	/* Fill out the region structure (the   */
    /* geographical limits etc.)            */

    nrows = Rast_window_rows();	/* Find out the number of rows and      */
    ncols = Rast_window_cols();	/* columns of the raster.               */


    if ((region.ew_res / region.ns_res >= 1.01) ||	/* If EW and NS resolns are    */
	(region.ns_res / region.ew_res >= 1.01)) {	/* >1% different, warn user.      */
	G_warning(_("E-W and N-S grid resolutions are different. Taking average."));
	resoln = (region.ns_res + region.ew_res) / 2;
    }
    else
	resoln = region.ns_res;


    /*--------------------------------------------------------------------------*/
    /*              RESERVE MEMORY TO HOLD Z VALUES AND MATRICES                */

    /*--------------------------------------------------------------------------*/

    row_in = (DCELL *) G_malloc(ncols * sizeof(DCELL) * wsize);
    /* Reserve `wsize' rows of memory.      */

    if (mparam != FEATURE) {
	row_out = Rast_allocate_buf(DCELL_TYPE);	/* Initialise output row buffer.     */
	Rast_set_d_null_value(row_out, ncols);
    }
    else {
	featrow_out = Rast_allocate_buf(CELL_TYPE);	/* Initialise output row buffer.  */
	Rast_set_c_null_value(featrow_out, ncols);
    }

    window_ptr = (DCELL *) G_malloc(SQR(wsize) * sizeof(DCELL));
    /* Reserve enough memory for local wind. */

    weight_ptr = (double *)G_malloc(SQR(wsize) * sizeof(double));
    /* Reserve enough memory weights matrix. */

    normal_ptr = dmatrix(0, 5, 0, 5);	/* Allocate memory for 6*6 matrix       */
    index_ptr = ivector(0, 5);	/* and for 1D vector holding indices    */
    obs_ptr = dvector(0, 5);	/* and for 1D vector holding observed z */


    /* ---------------------------------------------------------------- */
    /* -            CALCULATE LEAST SQUARES COEFFICIENTS              - */
    /* ---------------------------------------------------------------- */

    /*--- Calculate weighting matrix. ---*/

    find_weight(weight_ptr);

    /* Initial coefficients need only be found once since they are 
       constant for any given window size. The only element that 
       changes is the observed vector (RHS of normal equations). */

    /*--- Find normal equations in matrix form. ---*/

    find_normal(normal_ptr, weight_ptr);


    /*--- Apply LU decomposition to normal equations. ---*/

    if (constrained) {
	G_ludcmp(normal_ptr, 5, index_ptr, &temp);
	/* To constrain the quadtratic 
	   through the central cell, ignore 
	   the calculations involving the
	   coefficient f. Since these are 
	   all in the last row and column of
	   the matrix, simply redimension.   */
	/* disp_matrix(normal_ptr,obs_ptr,obs_ptr,5);
	 */
    }

    else {
	G_ludcmp(normal_ptr, 6, index_ptr, &temp);
	/* disp_matrix(normal_ptr,obs_ptr,obs_ptr,6);
	 */
    }


    /*--------------------------------------------------------------------------*/
    /*          PROCESS INPUT RASTER AND WRITE OUT RASTER LINE BY LINE          */

    /*--------------------------------------------------------------------------*/

    if (mparam != FEATURE)
	for (wind_row = 0; wind_row < EDGE; wind_row++)
	    Rast_put_row(fd_out, row_out, DCELL_TYPE);	/* Write out the edge cells as NULL.    */
    else
	for (wind_row = 0; wind_row < EDGE; wind_row++)
	    Rast_put_row(fd_out, featrow_out, CELL_TYPE);	/* Write out the edge cells as NULL.    */

    for (wind_row = 0; wind_row < wsize - 1; wind_row++)
	Rast_get_row(fd_in, row_in + (wind_row * ncols), wind_row,
			 DCELL_TYPE);
    /* Read in enough of the first rows to  */
    /* allow window to be examined.         */

    for (row = EDGE; row < (nrows - EDGE); row++) {
	G_percent(row + 1, nrows - EDGE, 2);

	Rast_get_row(fd_in, row_in + ((wsize - 1) * ncols), row + EDGE,
			 DCELL_TYPE);

	for (col = EDGE; col < (ncols - EDGE); col++) {
	    /* Find central z value */
	    centre = *(row_in + EDGE * ncols + col);
	    /* Test for no data and propagate */
	    if (Rast_is_d_null_value(&centre)) {
		if (mparam == FEATURE)
		    Rast_set_c_null_value(featrow_out + col, 1);
		else {
		    Rast_set_d_null_value(row_out + col, 1);
		}
		continue;
	    }
	    found_null = FALSE;
	    for (wind_row = 0; wind_row < wsize; wind_row++) {
		if (found_null)
		    break;
		for (wind_col = 0; wind_col < wsize; wind_col++) {

		    /* Express all window values relative   */
		    /* to the central elevation.            */
		    window_cell = row_in + (wind_row * ncols) + col + wind_col - EDGE;
		    /* Test for no data and propagate */
		    if (Rast_is_d_null_value(window_cell)) {
			if (mparam == FEATURE)
			    Rast_set_c_null_value(featrow_out + col, 1);
			else {
			    Rast_set_d_null_value(row_out + col, 1);
			}
			found_null = TRUE;
			break;
		    }
		    *(window_ptr + (wind_row * wsize) + wind_col) =
			*(window_cell) - centre;
		}
	    }

	    if (found_null)
		continue;

	    /*--- Use LU back substitution to solve normal equations. ---*/
	    find_obs(window_ptr, obs_ptr, weight_ptr);

	    /*      disp_wind(window_ptr); 
	       disp_matrix(normal_ptr,obs_ptr,obs_ptr,6);
	     */

	    if (constrained) {
		G_lubksb(normal_ptr, 5, index_ptr, obs_ptr);
		/*
		   disp_matrix(normal_ptr,obs_ptr,obs_ptr,5);
		 */
	    }

	    else {
		G_lubksb(normal_ptr, 6, index_ptr, obs_ptr);
		/*      
		   disp_matrix(normal_ptr,obs_ptr,obs_ptr,6);
		 */

	    }

	    /*--- Calculate terrain parameter based on quad. coefficients. ---*/
	    if (mparam == FEATURE)
		*(featrow_out + col) = (CELL) feature(obs_ptr);
	    else
		*(row_out + col) = param(mparam, obs_ptr);

	    if (mparam == ELEV)
		*(row_out + col) += centre;	/* Add central elevation back */
	}

	if (mparam != FEATURE)
	    Rast_put_row(fd_out, row_out, DCELL_TYPE);	/* Write the row buffer to the output   */
	/* raster.                              */
	else			/* write FEATURE to CELL */
	    Rast_put_row(fd_out, featrow_out, CELL_TYPE);	/* Write the row buffer to the output       */
	/* raster.                              */

	/* 'Shuffle' rows down one, and read in */
	/*  one new row.                        */
	for (wind_row = 0; wind_row < wsize - 1; wind_row++)
	    for (col = 0; col < ncols; col++)
		*(row_in + (wind_row * ncols) + col) =
		    *(row_in + ((wind_row + 1) * ncols) + col);
    }

    if (mparam != FEATURE)
	Rast_set_d_null_value(row_out, ncols);
    else
	Rast_set_c_null_value(featrow_out, ncols);
    for (wind_row = 0; wind_row < EDGE; wind_row++) {
	if (mparam != FEATURE)
	    Rast_put_row(fd_out, row_out, DCELL_TYPE);	/* Write out the edge cells as NULL. */
	else
	    Rast_put_row(fd_out, featrow_out, CELL_TYPE);	/* Write out the edge cells as NULL. */
    }

    /*--------------------------------------------------------------------------*/
    /*     FREE MEMORY USED TO STORE RASTER ROWS, LOCAL WINDOW AND MATRICES     */

    /*--------------------------------------------------------------------------*/

    G_free(row_in);
    if (mparam != FEATURE)
	G_free(row_out);
    else
	G_free(featrow_out);

    G_free(window_ptr);
    free_dmatrix(normal_ptr, 0, 5, 0, 5);
    free_dvector(obs_ptr, 0, 5);
    free_ivector(index_ptr, 0, 5);
}
