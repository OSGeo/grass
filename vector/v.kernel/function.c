#include <math.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include "global.h"


/* probability for gaussian distribution */
double gaussian2dBySigma(double d, double sigma)
{
    double res;

    res =
	1. / (2. * M_PI * sigma * sigma) * exp(-d * d / (2. * sigma * sigma));

    return (res);
}


double gaussianFunction(double x, double sigma, double dimension)
{
    return ((1. / (pow(2. * M_PI, dimension / 2.) * pow(sigma, dimension))) *
	    exp(-0.5 * pow(x / sigma, 2.)));
}


/* probability for gaussian distribution */
double gaussianKernel(double x, double term)
{
    return (term * exp(-(x * x) / 2.));
}


/*
   term1 = 1./(2.*M_PI*sigma*sigma)
   term2 = 2.*sigma*sigma;
 */
double gaussian2dByTerms(double d, double term1, double term2)
{
    double res;

    res = term1 * exp(-d * d / term2);

    return (res);
}


double segno(double x)
{
    double y;

    y = (x > 0 ? 1. : 0.) + (x < 0 ? -1. : 0.);

    return y;
}


double kernel1(double d, double rs, double lambda)
{
    double res;
    double a = lambda - 1.;

    if (lambda == 1.) {
	res = 1. / (M_PI * (d * d + rs * rs));
    }
    else {
	res =
	    segno(a) * (a / M_PI) * (pow(rs, 2. * a)) * (1 /
							 pow(d * d + rs * rs,
							     lambda));
    }

    /*  res=1./(M_PI*(d*d+rs*rs)); */
    return (res);
}


double invGaussian2d(double sigma, double prob)
{
    double d;

    d = sqrt(-2 * sigma * sigma * log(prob * M_PI * 2 * sigma * sigma));
    return (d);
}


/* euclidean distance between vectors x and y of length n */
double euclidean_distance(double *x, double *y, int n)
{
    int j;
    double out = 0.0;
    double tmp;

    for (j = 0; j < n; j++) {
	tmp = x[j] - y[j];
	out += tmp * tmp;
    }

    return sqrt(out);
}
