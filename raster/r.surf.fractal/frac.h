
/************************************************************************/

/***				  frac.h			      ***/

/***                    Header file for use with r.frac               ***/

/***		     Jo Wood, V 1.0  - 19th October, 1994	      ***/

/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
				/* programs. It sets up the necessary   */
				/* prototypes for GRASS library calls.  */

#define MAX(a,b) ((a)>(b) ? (a):(b))
#define SWAP(a,b) tempr=(a); (a)=(b); (b) = tempr

#define TWOPI 6.283185307179590	/* Constant value of 2 pi */

/* ------ Global variables ------ */

extern const char
 *rast_out_name,		/* Name of the raster output file.      */
 *mapset_out;

extern int
  fd_out,			/* File descriptor of output raster     */
  Steps;			/* Number of intermediate images.       */

extern double H;		/* Hausdorff-Besickovitch dimension.    */

/* interface.c */
int interface(int, char *[]);

/* process.c */
int process(void);
int data_reset(double *[2], int);

/* spec_syn.c */
int specsyn(double *[2], int);

/* write_rast.c */
int write_rast(double *[2], int, int);
