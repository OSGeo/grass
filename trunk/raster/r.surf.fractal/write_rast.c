/* updated to GRASS 5FP 11/99 Markus Neteler */

/***************************************************************************/

/***                                                                     ***/

/***                            write_rast()                             ***/

/***  Extracts real component from complex array and writes as raster.   ***/

/***                  Jo Wood, V1.0, 20th October, 1994  		 ***/

/***                                                                     ***/

/***************************************************************************/

#include <grass/raster.h>
#include <grass/glocale.h>
#include "frac.h"

int write_rast(double *data[2],	/* Array holding complex data.          */
	       int nn,		/* Size of side of array.               */
	       int step		/* Version of file to send.             */
    )
{

    /*------------------------------------------------------------------*/
    /*                              INITIALISE                          */

    /*------------------------------------------------------------------*/

    DCELL *row_out;		/* Buffers to hold raster rows.         */

    char file_name[GNAME_MAX];	/* Name of each file to be written      */
    struct History history;	/* cmd line history metadata */

    int nrows,			/* Will store the current number of     */
      ncols,			/* rows and columns in the raster.      */
      row, col;			/* Counts through each row and column   */

    /* of the input raster.                 */

    nrows = Rast_window_rows();	/* Find out the number of rows and      */
    ncols = Rast_window_cols();	/* columns of the raster view.          */

    row_out = Rast_allocate_d_buf();

    /*------------------------------------------------------------------*/
    /*         Open new file and set the output file descriptor.        */

    /*------------------------------------------------------------------*/

    if (Steps != step)
	sprintf(file_name, "%s.%d", rast_out_name, step);
    else
	strcpy(file_name, rast_out_name);

    fd_out = Rast_open_new(file_name, DCELL_TYPE);

    /*------------------------------------------------------------------*/
    /*  Extract real component of transform and save as a GRASS raster. */

    /*------------------------------------------------------------------*/

    for (row = 0; row < nrows; row++) {
	for (col = 0; col < ncols; col++)
	    *(row_out + col) = (DCELL) (*(data[0] + row * nn + col) * 100000);

	Rast_put_row(fd_out, (DCELL *) row_out, DCELL_TYPE);
    }

    Rast_close(fd_out);
    Rast_short_history(file_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(file_name, &history);

    return 0;
}
