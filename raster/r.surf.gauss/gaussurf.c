/* updated to FP support 11/99 Markus Neteler */

/*****************/

/** gaussurf()	**/

/*****************/

#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>


int gaussurf(char *out,		/* Name of raster maps to be opened.    */
	     double mean, double sigma	/* Distribution parameters.             */
    )
{
    int nrows, ncols;		/* Number of cell rows and columns      */

    DCELL *row_out;		/* Buffer just large enough to hold one */

    /* row of the raster map layer.         */

    int fd_out;			/* File descriptor - used to identify   */

    /* open raster maps.                    */
    struct History history;	/* cmd line history metadata            */

    int row_count, col_count;

	/****** INITIALISE RANDOM NUMBER GENERATOR ******/

    G_math_rand(-1 * getpid());

	/****** OPEN CELL FILES AND GET CELL DETAILS ******/

    fd_out = G_open_raster_new(out, DCELL_TYPE);

    nrows = G_window_rows();
    ncols = G_window_cols();

    row_out = G_allocate_d_raster_buf();


	/****** PASS THROUGH EACH CELL ASSIGNING RANDOM VALUE ******/

    for (row_count = 0; row_count < nrows; row_count++) {
	for (col_count = 0; col_count < ncols; col_count++)
	    *(row_out + col_count) =
		(DCELL) (G_math_rand_gauss(2742, sigma) + mean);

	/* Write contents row by row */
	G_put_d_raster_row(fd_out, (DCELL *) row_out);
    }


	/****** CLOSE THE CELL FILE ******/

    G_close_cell(fd_out);
    G_short_history(out, "raster", &history);
    G_command_history(&history);
    G_write_history(out, &history);

    return 0;
}
