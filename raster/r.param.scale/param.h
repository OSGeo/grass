/************************************************************************/
/*** 								      ***/
/***				param.h				      ***/
/***  Header file for use with r.param.scale			      ***/
/***  Jo Wood, ASSIST, Dept of Geography, University of Leicester     ***/
/***  V1.0  - 7th February, 1993				      ***/
/***								      ***/
/************************************************************************/

#include <grass/gis.h>
				/* programs. It sets up the necessary 	*/
				/* prototypes for GRASS library calls.	*/
#include <math.h>

#define EDGE ((wsize-1)/2)	/* Number of rows/cols that make up the	*/
				/* 'blank' edge around raster.		*/
#define MAX_WSIZE 69		/* Maximum dimensions of window.	*/
				/* Some useful labels.			*/
#define TRUE 1
#define FALSE 0

#define RAD2DEG 57.29578
#define DEG2RAD 0.017453293

#define TINY 1.0e-20;

/* Bug? start with 1 as G_set_cats() doesn't accept 0 category */
#define FLAT ((CELL)1)
#define PIT ((CELL)2)
#define CHANNEL ((CELL)3)
#define PASS ((CELL)4)
#define RIDGE ((CELL)5)
#define PEAK ((CELL)6)

#define NUM_CATS ((CELL)7)

#define ELEV   1
#define SLOPE  2
#define ASPECT 3
#define PROFC  4
#define PLANC  5
#define LONGC  6
#define CROSC  7
#define MINIC  8
#define MAXIC  9
#define FEATURE 10

/* The six quadratic coefficients are stored in the array coeff */

#define C_A coeff[0]
#define C_B coeff[1]
#define C_C coeff[2]
#define C_D coeff[3]
#define C_E coeff[4]
#define C_F coeff[5]

/* ------ Declare functions ----- */

DCELL param(int ptype, double *coeff);	/* Calculates terrain parameters. */
void interface(int argc, char **argv);
void open_files(void);
void process(void);
void close_down(void);
void write_cols(void);
void write_cats(void);
void find_normal(double **normal,	/* Matrix of cross-products.	*/
		 double *w);		/* Weights matrix.		*/
void find_obs(DCELL  *z,			/* Local window of elevs.	*/
	      double *obs,		/* Observed column vector.	*/
	      double  *w);		/* Weighting matrix.		*/
void find_weight(double *weight_ptr);
DCELL feature(double *coeff);	/* Set of six quadratic coefficents. 	*/

/* ------ Global variables ------ */

#ifndef MAIN
    extern			/* Externally defined if not main()	*/
#endif

char		*rast_in_name,	/* Name of the raster file to process.	*/
    		*rast_out_name,	/* Name of the raster output file.	*/

    		*mapset_in,	/* If no problems, these will be names 	*/
		*mapset_out,	/* of mapsets containing the files to 	*/
				/* be processed. Otherwise, error code.	*/

		constrained;	/* Flag that forces quadtratic through	*/
				/* the central cell of the window.	*/

#ifndef MAIN
    extern			/* Externally defined if not main()	*/
#endif

int		fd_in,		/* File descriptor for input and	*/
		fd_out,		/* output raster files.			*/

		wsize,		/* Size of local processing window.	*/

		mparam;		/* Morphometric parameter to calculate.	*/


#ifndef MAIN
    extern			/* Externally defined if not main()	*/
#endif

double      	resoln,		/* Planimetric resolution.		*/
		exponent,	/* Distance weighting exponent.		*/
		zscale,		/* Vertical scaling factor.		*/

	      	slope_tol,	/* Vertical tolerences for surface	*/
		curve_tol;	/* feature identification.		*/

