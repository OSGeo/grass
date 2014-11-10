
/****************************************************************/
/* find_normal() - Function to find the set of normal equations */
/*                 that allow a quadratic trend surface to be   */
/*                 fitted through N  points using least squares */
/*                 V.1.0, Jo Wood, 27th November, 1994.         */

/****************************************************************/

#include "param.h"

void find_normal(double **normal,	/* Matrix of cross-products.    */
		 double *w)
{				/* Weights matrix.                */
    int edge = EDGE;		/* Store (wsize-1)/2 to save    */

    /* on computation               */

    double x, y,			/* Local coordinates of window. */
      x1 = 0, y1 = 0,		/* coefficients of X-products.  */
	x2 = 0, y2 = 0,
	x3 = 0, y3 = 0,
	x4 = 0, y4 = 0,
	xy2 = 0, x2y = 0, xy3 = 0, x3y = 0, x2y2 = 0, xy = 0, N = 0;

    int row, col;		/* Pass through local window.   */


    /* Initialise sums-of-squares and cross products matrix */

    for (row = 0; row < 6; row++)
	for (col = 0; col < 6; col++)
	    normal[row][col] = 0.0;


    /* Calculate matrix of sums of squares and cross products */

    for (row = 0; row < wsize; row++)
	for (col = 0; col < wsize; col++) {
	    x = resoln * (col - edge);
	    y = resoln * (row - edge);

	    x4 += x * x * x * x * *(w + row * wsize + col);
	    x2y2 += x * x * y * y * *(w + row * wsize + col);
	    x3y += x * x * x * y * *(w + row * wsize + col);
	    x3 += x * x * x * *(w + row * wsize + col);
	    x2y += x * x * y * *(w + row * wsize + col);
	    x2 += x * x * *(w + row * wsize + col);

	    y4 += y * y * y * y * *(w + row * wsize + col);
	    xy3 += x * y * y * y * *(w + row * wsize + col);
	    xy2 += x * y * y * *(w + row * wsize + col);
	    y3 += y * y * y * *(w + row * wsize + col);
	    y2 += y * y * *(w + row * wsize + col);

	    xy += x * y * *(w + row * wsize + col);

	    x1 += x * *(w + row * wsize + col);

	    y1 += y * *(w + row * wsize + col);

	    N += *(w + row * wsize + col);

	}

    /* --- Store cross-product matrix elements. --- */

    normal[0][0] = x4;
    normal[0][1] = normal[1][0] = x2y2;
    normal[0][2] = normal[2][0] = x3y;
    normal[0][3] = normal[3][0] = x3;
    normal[0][4] = normal[4][0] = x2y;
    normal[0][5] = normal[5][0] = x2;

    normal[1][1] = y4;
    normal[1][2] = normal[2][1] = xy3;
    normal[1][3] = normal[3][1] = xy2;
    normal[1][4] = normal[4][1] = y3;
    normal[1][5] = normal[5][1] = y2;

    normal[2][2] = x2y2;
    normal[2][3] = normal[3][2] = x2y;
    normal[2][4] = normal[4][2] = xy2;
    normal[2][5] = normal[5][2] = xy;

    normal[3][3] = x2;
    normal[3][4] = normal[4][3] = xy;
    normal[3][5] = normal[5][3] = x1;

    normal[4][4] = y2;
    normal[4][5] = normal[5][4] = y1;

    normal[5][5] = N;
}


/****************************************************************/
/* find_obs() - Function to find the observed vector as part of */
/*              the set of normal equations for least squares.  */
/*              V.1.0, Jo Wood, 11th December, 1994.            */

/****************************************************************/

void find_obs(DCELL * z,	/* Local window of elevs.       */
	      double *obs,	/* Observed column vector.      */
	      double *w)
{				/* Weighting matrix.              */

    int row, col,		/* Counts through local window. */
      edge = EDGE,		/* EDGE = (wsize-1)/2.          */
	offset;			/* Array offset for weights & z */

    double x, y;			/* Local window coordinates.    */

    for (row = 0; row < 6; row++)	/* Initialise column vector.        */
	obs[row] = 0.0;


    for (row = 0; row < wsize; row++)
	for (col = 0; col < wsize; col++) {
	    x = resoln * (col - edge);
	    y = resoln * (row - edge);
	    offset = row * wsize + col;

	    obs[0] += *(w + offset) * *(z + offset) * x * x;
	    obs[1] += *(w + offset) * *(z + offset) * y * y;
	    obs[2] += *(w + offset) * *(z + offset) * x * y;
	    obs[3] += *(w + offset) * *(z + offset) * x;
	    obs[4] += *(w + offset) * *(z + offset) * y;

	    if (!constrained)	/* If constrained, should remain 0.0 */
		obs[5] += *(w + offset) * *(z + offset);
	}
}


/****************************************************************/
/* find_weight() Function to find the weightings matrix for the */
/*               observed cell values.                          */
/*               Uses an inverse distance function that can be  */
/*               calibrated with an exponent (0= no decay,      */
/*               1=linear decay, 2=squared distance decay etc.) */
/*               V.1.1, Jo Wood, 11th May, 1995.                */

/****************************************************************/

void find_weight(double *weight_ptr)
{
    int edge = EDGE;		/* EDGE = (wsize-1)/2.          */
    int row, col;		/* Counts through the rows and  */

    /* columns of weights matrix.   */

    double dist;		/* Distance to centre of kernel. */


    /* --- Find inverse distance of all cells to centre. --- */

    for (row = 0; row < wsize; row++)
	for (col = 0; col < wsize; col++) {
	    dist = 1.0 /
		pow(sqrt
		    ((edge - col) * (edge - col) +
		     (edge - row) * (edge - row)) + 1.0, exponent);
	    *(weight_ptr + row * wsize + col) = dist;
	}
}
