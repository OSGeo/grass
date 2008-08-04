#include <stdio.h>
#include <math.h>
#include "bouman.h"

/* Global Variables: These variables are required as inputs  *
 * to the function called by ``solve''. Since the function is   *
 * called by a pointer, side inputs must be passed as global    *
 * variables root finding                                       */
static double ***N_glb;		/* N_glb[2][3][2] rate statistics */
static double *b_glb;		/* line search direction */
static double eps_glb;		/* precision */
static int M_glb;		/* number of classes */



void alpha_max(double ***N,	/* Transition probability statistics; N2[2][3][2] */
	       double *a,	/* Transition probability parameters; a[3] */
	       int M,		/* Number of class types */
	       double eps	/* Precision of convergence */
    )
{
    double b[3];

    /*       b[0] = 0.4;
       b[1] = 0.6/2;
       b[2] = 0.0; */

    b[0] = 3.0;
    b[1] = 2.0;
    b[2] = 0.0;
    line_search(N, a, M, b, eps);
}


void line_search(
		    /* line_search determines the maximum value of the log likelihood       *
		     * along the direction b, subject to the constraint a[0]+2*a[1]+a[3]<1. */
		    double ***N,	/* Transition probability statistics; N2[2][3][2] */
		    double *a,	/* Transition probability parameters; a[3] */
		    int M,	/* Number of class types */
		    double *b,	/* direction of search */
		    double eps	/* Precision of convergence */
    )
{
    int code;			/* error code for solve subroutine */
    double x;			/* distance along line */
    double max;			/* maximum value for x */

    normalize(b);

    a[0] = eps * b[0];
    a[1] = eps * b[1];
    a[2] = eps * b[2];

    /* enforce condition [1,2,1][a[0],a[1],a[2]]^t <1-eps */
    max = (1 - eps) / (b[0] + 2 * b[1] + b[2]);

    /* set global variables for solve routine */
    N_glb = N;
    b_glb = b;
    eps_glb = eps;
    M_glb = M;

    /* minimize on line. Avoid singular boundary */
    x = solve(func, eps, max, eps, &code);

    /* If derivative was positive on line, x=max. */
    if (code == 1)
	x = max;

    /* If derivative was negative on line. */
    if (code == -1)
	x = 0.0;

    /* compute a */
    a[0] = x * b[0];
    a[1] = x * b[1];
    a[2] = x * b[2];
}


int normalize(
		 /* normalize the vector b[3]. Return 0 if null vector */
		 double b[3]
    )
{
    double norm;

    norm = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
    if (norm == 0)
	return (0);
    b[0] = b[0] / norm;
    b[1] = b[1] / norm;
    b[2] = b[2] / norm;
    return (1);
}

double func(double x)
{
    double tmp[3], grad[3];

    tmp[0] = x * b_glb[0];
    tmp[1] = x * b_glb[1];
    tmp[2] = x * b_glb[2];

    gradient(grad, N_glb, tmp, M_glb);
    return (b_glb[0] * grad[0] + b_glb[1] * grad[1] + b_glb[2] * grad[2]);
}

double log_like(
		   /* compute log likelihood being maximized. This subroutine *
		    * is useful for debuging since the log likelihood must be *
		    * monotonically decreasing.                               */
		   double ***N,	/* transition statistics */
		   double a[3],	/* transition parameters */
		   int M	/* number of classes */
    )
{
    double iM, tmp, sum;
    int n1, n2, n3;

    iM = 1.0 / M;
    sum = 0;
    for (n1 = 0; n1 <= 1; n1++)
	for (n2 = 0; n2 <= 2; n2++)
	    for (n3 = 0; n3 <= 1; n3++) {
		tmp =
		    log(a[0] * (n1 - iM) + a[1] * (n2 - 2.0 * iM) +
			a[2] * (n3 - iM) + iM);
		sum += N[n1][n2][n3] * tmp;
	    }
    return (sum);
}


void gradient(
		 /* computes the gradient of the log likelihood being maximized. */
		 double grad[3],	/* gradient vector; output */
		 double ***N,	/* transition statistics; input */
		 double a[3],	/* transition parameters; input */
		 int M		/* number of classes */
    )
{
    double iM, tmp;
    int n1, n2, n3;

    iM = 1.0 / M;
    grad[0] = grad[1] = grad[2] = 0;
    for (n1 = 0; n1 <= 1; n1++)
	for (n2 = 0; n2 <= 2; n2++)
	    for (n3 = 0; n3 <= 1; n3++) {
		tmp =
		    a[0] * (n1 - iM) + a[1] * (n2 - 2.0 * iM) + a[2] * (n3 -
									iM) +
		    iM;
		tmp = 1 / tmp;
		grad[0] += tmp * (n1 - iM) * N[n1][n2][n3];
		grad[1] += tmp * (n2 - 2.0 * iM) * N[n1][n2][n3];
		grad[2] += tmp * (n3 - iM) * N[n1][n2][n3];
	    }
}
