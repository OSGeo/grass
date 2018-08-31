/* Updated by Michel Wurtz 12/99 */

/*****************************************************************************/

/***                                                                       ***/

/***                               spec_syn()                              ***/

/***          Creates a fractal surface using spectral synthesis.	   ***/

/***        Algorithm adapted from Peitgen and Sauper (1988), p.108.	   ***/

/***                    Jo Wood, V1.0, 19th October, 1994                  ***/

/***	   Modified to allow multiple realisations of same surface,	   ***/

/***   			Jo Wood, V1.1 15th October, 1995.		   ***/

/***                                                                       ***/

/*****************************************************************************/

#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "frac.h"
#include <grass/gmath.h>


int specsyn(double *data[2],	/* Array holding complex data to transform. */
	    int nn		/* Size of array in one dimension.          */
    )
{
    /* ---------------------------------------------------------------- */
    /* ---                      Initialise                          --- */
    /* ---------------------------------------------------------------- */

    int row, col,		/* Counts through half the data array.  */
      row0, col0,		/* 'other half' of the array.           */
      coeff;			/* No. of Fourier coeffficents to calc. */

    double phase, rad,		/* polar coordinates of Fourier coeff.  */
     *temp[2];

    /* FIXME - allow seed to be specified for repeatability */
    G_math_srand_auto();	/* Reset random number generator.       */

    temp[0] = (double *)G_malloc(nn * nn * sizeof(double));
    temp[1] = (double *)G_malloc(nn * nn * sizeof(double));

    /* ---------------------------------------------------------------- */
    /* ---                   Create fractal surface                 --- */
    /* ---------------------------------------------------------------- */


    /* Calculate all the preliminary random coefficients. */
    /* ================================================== */

    G_message(_("Preliminary surface calculations..."));
    data_reset(data, nn);

    for (row = 0; row <= nn / 2; row++)
	for (col = 0; col <= nn / 2; col++) {
	    /* Generate random Fourier coefficients. */

	    phase = TWOPI * G_math_rand();

	    if ((row != 0) || (col != 0))
		rad =
		    pow(row * row + col * col,
			-(H + 1) / 2.0) * G_math_rand_gauss(1.);
	    else
		rad = 0.0;

	    /* Fill half the array with coefficients. */

	    *(data[0] + row * nn + col) = rad * cos(phase);
	    *(data[1] + row * nn + col) = rad * sin(phase);

	    /* Fill other half of array with coefficients. */

	    if (row == 0)
		row0 = 0;
	    else
		row0 = nn - row;

	    if (col == 0)
		col0 = 0;
	    else
		col0 = nn - col;

	    *(data[0] + row0 * nn + col0) = rad * cos(phase);
	    *(data[1] + row0 * nn + col0) = -rad * sin(phase);
	}

    *(temp[1] + nn / 2) = 0.0;
    *(temp[1] + nn * nn / 2) = 0.0;
    *(temp[1] + nn * nn / 2 + nn / 2) = 0.0;

    for (row = 1; row < nn / 2; row++)
	for (col = 1; col < nn / 2; col++) {
	    phase = TWOPI * G_math_rand();
	    rad =
		pow(row * row + col * col,
		    -(H + 1) / 2.0) * G_math_rand_gauss(1.);

	    *(data[0] + row * nn + nn - col) = rad * cos(phase);
	    *(data[1] + row * nn + nn - col) = rad * sin(phase);

	    *(data[0] + (nn - row) * nn + col) = rad * cos(phase);
	    *(data[1] + (nn - row) * nn + col) = -rad * sin(phase);
	}

    /* Transfer random coeffients to array before ifft transform */
    /* ========================================================= */

    for (coeff = 0; coeff < Steps; coeff++) {
	G_message(_("Calculating surface %d (of %d)..."), coeff + 1, Steps);
	data_reset(temp, nn);

	for (row = 0; row <= (coeff + 1) * nn / (Steps * 2); row++)
	    for (col = 0; col <= (coeff + 1) * nn / (Steps * 2); col++) {
		if (row == 0)
		    row0 = 0;
		else
		    row0 = nn - row;

		if (col == 0)
		    col0 = 0;
		else
		    col0 = nn - col;

		*(temp[0] + row * nn + col) = *(data[0] + row * nn + col);
		*(temp[1] + row * nn + col) = *(data[1] + row * nn + col);

		*(temp[0] + row0 * nn + col0) = *(data[0] + row0 * nn + col0);
		*(temp[1] + row0 * nn + col0) = *(data[1] + row0 * nn + col0);
	    }

	for (row = 1; row < (coeff + 1) * nn / (Steps * 2); row++)
	    for (col = 1; col < (coeff + 1) * nn / (Steps * 2); col++) {
		*(temp[0] + row * nn + nn - col) =
		    *(data[0] + row * nn + nn - col);
		*(temp[1] + row * nn + nn - col) =
		    *(data[1] + row * nn + nn - col);

		*(temp[0] + (nn - row) * nn + col) =
		    *(data[0] + (nn - row) * nn + col);
		*(temp[1] + (nn - row) * nn + col) =
		    *(data[1] + (nn - row) * nn + col);
	    }

	fft(1, temp, nn * nn, nn, nn);	/* Perform inverse FFT and */
	write_rast(temp, nn, coeff + 1);	/* write out raster.       */
    }

    /* Free memory. */
    /* ============ */

    G_free(temp[0]);
    G_free(temp[1]);

    return 0;
}
