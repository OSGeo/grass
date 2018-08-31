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
    /* FIXME - allow seed to be specified for repeatability */
    G_math_srand_auto();

	/****** OPEN CELL FILES AND GET CELL DETAILS ******/
    fd_out = Rast_open_new(out, int_map ? CELL_TYPE : DCELL_TYPE);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    if (int_map)
	row_out_C = Rast_allocate_c_buf();
    else
	row_out_D = Rast_allocate_d_buf();

	/****** PASS THROUGH EACH CELL ASSIGNING RANDOM VALUE ******/
    for (row_count = 0; row_count < nrows; row_count++) {
	G_percent(row_count, nrows, 2);
	for (col_count = 0; col_count < ncols; col_count++) {
	    if (int_map)
		*(row_out_C + col_count) =
		    (CELL) (G_math_rand() * (max + 1 - min) + min);
	    /* under represents first and last bin */
	    /*                  *(row_out_C + col_count) = (CELL) floor(rand1(2742)*(max-min)+min +0.5); */
	    else
		*(row_out_D + col_count) =
		    (DCELL) (G_math_rand() * (max - min) + min);
	}

	/* Write contents row by row */
	if (int_map)
	    Rast_put_c_row(fd_out, (CELL *) row_out_C);
	else
	    Rast_put_d_row(fd_out, (DCELL *) row_out_D);
    }
    G_percent(1, 1, 1);
    
    Rast_close(fd_out);

    return 0;
}
