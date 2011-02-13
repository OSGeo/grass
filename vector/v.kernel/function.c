#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "global.h"

/*********************** Gaussian ****************************/
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
	res = segno(a) * (a / M_PI) * (pow(rs, 2. * a)) *
	    (1 / pow(d * d + rs * rs, lambda));
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


/********************************************************************/
double gaussianKernel2(int dimension, double bandwidth, double x)
{
    double term =
	1. / (pow(bandwidth, dimension) * pow((2. * M_PI), dimension / 2.));

    x /= bandwidth;

    return (term * exp(-(x * x) / 2.));
}

/* Note: these functions support currently only 1D and 2D, consider this for example 
 * before using them for 3d grid */
double uniformKernel(int dimension, double bandwidth, double x)
{
    double k;

    if (x > bandwidth)
	return 0;

    if (dimension == 2)
	k = 2. / (M_PI * pow(bandwidth, 2));
    else
	k = 1. / bandwidth;

    x /= bandwidth;

    return k * (1. / 2);
}

double triangularKernel(int dimension, double bandwidth, double x)
{
    double k;

    if (x > bandwidth)
	return 0;

    if (dimension == 2)
	k = 3. / (M_PI * pow(bandwidth, 2));
    else
	k = 1. / bandwidth;

    x /= bandwidth;

    return k * (1 - x);
}

double epanechnikovKernel(int dimension, double bandwidth, double x)
{
    double k;

    if (x > bandwidth)
	return 0;

    if (dimension == 2)
	k = 8. / (M_PI * 3. * pow(bandwidth, 2));
    else
	k = 1. / bandwidth;

    x /= bandwidth;

    return k * (3. / 4. * (1 - x * x));
}

double quarticKernel(int dimension, double bandwidth, double x)
{
    double k;

    if (x > bandwidth)
	return 0;

    if (dimension == 2)
	k = 16. / (M_PI * 5. * pow(bandwidth, 2));
    else
	k = 1. / bandwidth;

    x /= bandwidth;

    return k * (15. / 16. * pow(1 - x * x, 2));
}

double triweightKernel(int dimension, double bandwidth, double x)
{
    double k;

    if (x > bandwidth)
	return 0;

    if (dimension == 2)
	k = 128. / (M_PI * 35. * pow(bandwidth, 2));
    else
	k = 1. / bandwidth;

    x /= bandwidth;

    return k * (35. / 32 * pow(1 - x * x, 3));
}

double cosineKernel(int dimension, double bandwidth, double x)
{
    double k;

    if (x > bandwidth)
	return 0;

    if (dimension == 2)
	k = 1. / (2 * (M_PI / 2 - 1) * pow(bandwidth, 2));
    else
	k = 1. / bandwidth;

    x /= bandwidth;

    return k * (M_PI / 4. * cos(M_PI / 2. * x));
}

double kernelFunction(int function, int dimension, double bandwidth, double x)
{
    if (dimension > 2 && function != KERNEL_GAUSSIAN) {
	G_fatal_error(_("Dimension > 2 supported only by gaussian function"));
    }
    switch (function) {
    case KERNEL_UNIFORM:
	return uniformKernel(dimension, bandwidth, x);
	break;
    case KERNEL_TRIANGULAR:
	return triangularKernel(dimension, bandwidth, x);
	break;
    case KERNEL_EPANECHNIKOV:
	return epanechnikovKernel(dimension, bandwidth, x);
	break;
    case KERNEL_QUARTIC:
	return quarticKernel(dimension, bandwidth, x);
	break;
    case KERNEL_TRIWEIGHT:
	return triweightKernel(dimension, bandwidth, x);
	break;
    case KERNEL_GAUSSIAN:
	return gaussianKernel2(dimension, bandwidth, x);
	break;
    case KERNEL_COSINE:
	return cosineKernel(dimension, bandwidth, x);
	break;
    }
    G_fatal_error("Unknown kernel function");
}
