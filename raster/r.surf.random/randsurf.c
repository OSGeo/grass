#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gmath.h>
#include <grass/glocale.h>


int randsurf(char *out,		/* Name of raster maps to be opened.    */
	     int min, int max,	/* Minimum and maximum cell values.     */
	     int int_map)
{				/* if map is to be written as a CELL map */
    int nrows, ncols;		/* Number of cell rows and columns      */

    DCELL *row_out_D;		/* Buffer just large enough to hold one */
    CELL *row_out_C;		/* row of the raster map layer.         */

    int fd_out;			/* File descriptor - used to identify   */

    /* open raster maps.                    */
    int row_count, col_count;

	/****** INITIALISE RANDOM NUMBER GENERATOR ******/
    G_math_rand(-1 * getpid());

	/****** OPEN CELL FILES AND GET CELL DETAILS ******/
    if (int_map) {
	if ((fd_out = Rast_open_new(out, CELL_TYPE)) < 0)
	    G_fatal_error(_("Unable to create raster map <%s>"), out);
    }
    else {
	if ((fd_out = Rast_open_new(out, DCELL_TYPE)) < 0)
	    G_fatal_error(_("Unable to create raster map <%s>"), out);
    }

    nrows = G_window_rows();
    ncols = G_window_cols();

    if (int_map)
	row_out_C = Rast_allocate_c_buf();
    else
	row_out_D = Rast_allocate_d_buf();

	/****** PASS THROUGH EACH CELL ASSIGNING RANDOM VALUE ******/
    for (row_count = 0; row_count < nrows; row_count++) {
	for (col_count = 0; col_count < ncols; col_count++) {
	    if (int_map)
		*(row_out_C + col_count) =
		    (CELL) (G_math_rand(2742) * (max + 1 - min) + min);
	    /* under represents first and last bin */
	    /*                  *(row_out_C + col_count) = (CELL) floor(rand1(2742)*(max-min)+min +0.5); */
	    else
		*(row_out_D + col_count) =
		    (DCELL) (G_math_rand(2742) * (max - min) + min);
	}

	/* Write contents row by row */
	if (int_map)
	    Rast_put_c_raster_row(fd_out, (CELL *) row_out_C);
	else
	    Rast_put_d_raster_row(fd_out, (DCELL *) row_out_D);
    }

    Rast_close(fd_out);

    return 0;
}
