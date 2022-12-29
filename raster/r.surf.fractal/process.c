
/***************************************************************************/

/***                                                                     ***/

/***                             process()                               ***/

/***          Reads in a raster maps row by row for processing          ***/

/***                  Jo Wood, V1.0, 13th September, 1994		 ***/

/***                                                                     ***/

/***************************************************************************/

#include <grass/raster.h>
#include <grass/glocale.h>
#include "frac.h"
/* gmath.h must go last, as it can interfere with ANSI headers */
#include <grass/gmath.h>

int process(void)
{

    /*-------------------------------------------------------------------*/
    /*                              INITIALISE                           */

    /*-------------------------------------------------------------------*/

    int nrows,			/* Will store the current number of     */
      ncols,			/* rows and columns in the raster.      */
      nn;			/* Size of raster to nearest power of 2. */

    double *data[2];		/* Array holding complex data.          */


    /*------------------------------------------------------------------*/
    /*                       GET DETAILS OF INPUT RASTER                */

    /*------------------------------------------------------------------*/

    nrows = Rast_window_rows();	/* Find out the number of rows and */
    ncols = Rast_window_cols();	/* columns of the raster view.     */

    nn = G_math_max_pow2(MAX(nrows, ncols));	/* Find smallest power of 2 that   */
    /* largest side of raster will fit. */

    /*------------------------------------------------------------------*/
    /*                      CREATE SQUARE ARRAY OF SIDE 2^n             */

    /*------------------------------------------------------------------*/

    if (nn * nn * sizeof(double) < 1)
	G_fatal_error(_("Unable to allocate data buffer. "
			"Check current region with g.region."));
    
    data[0] = (double *)G_malloc(nn * nn * sizeof(double));
    data[1] = (double *)G_malloc(nn * nn * sizeof(double));

    /*------------------------------------------------------------------*/
    /*                   Apply spectral synthesis algorithm.            */

    /*------------------------------------------------------------------*/

    specsyn(data, nn);

    G_free(data[0]);
    G_free(data[1]);

    return 0;
}

/***************************************************/
/*  Initialize real & complex components to zero   */

/***************************************************/

int data_reset(double *data[2], int nn)
{
    register double *dptr0 = data[0], *dptr1 = data[1];
    int total_size = nn * nn, count;

    for (count = 0; count < total_size; count++)
	*dptr0++ = *dptr1++ = 0.0;

    return 0;
}
