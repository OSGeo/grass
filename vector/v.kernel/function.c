#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "global.h"

static double (*kernelfn)(double term, double bandwidth, double x);

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
double gaussianKernel(double x, double termx)
{
    return (termx * exp(-(x * x) / 2.));
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


/*****************kernel density functions******************************/

double gaussianKernel4(double term, double bandwidth, double x)
{
    /* term is set by setKernelFunction */
    /* bandwidth is here SD */

    /*
    double term =
	1. / (pow(bandwidth, dimension) * pow((2. * M_PI), dimension / 2.));
    */

    x /= bandwidth;

    /* SD = radius (bandwidth) / 4 */
    return (term * exp((x * x) / -2.));
}

/* Note: these functions support currently only 1D and 2D, consider this for example 
 * before using them for 3d grid */
double uniformKernel(double term, double bandwidth, double x)
{
    /* term is set by setKernelFunction */

    if (x > bandwidth)
	return 0;

    /*
    if (dimension == 2)
	term = 2. / (M_PI * pow(bandwidth, 2));
    else
	term = 1. / bandwidth;
    term *= (1. / 2);

    x /= bandwidth;
    */

    return term;
}

double triangularKernel(double term, double bandwidth, double x)
{
    /* term is set by setKernelFunction */

    if (x > bandwidth)
	return 0;

    /*
    if (dimension == 2)
	term = 3. / (M_PI * pow(bandwidth, 2));
    else
	term = 1. / bandwidth;
    */

    x /= bandwidth;

    return term * (1 - x);
}

double epanechnikovKernel(double term, double bandwidth, double x)
{
    /* term is set by setKernelFunction */

    if (x > bandwidth)
	return 0;

    /*
    if (dimension == 2)
	term = 8. / (M_PI * 3. * pow(bandwidth, 2));
    else
	term = 1. / bandwidth;
    term *= (3. / 4.);
    */

    x /= bandwidth;

    /* return term * (3. / 4. * (1 - x * x)); */
    return term * (1 - x * x);
}

double quarticKernel(double term, double bandwidth, double x)
{
    /* term is set by setKernelFunction */

    if (x > bandwidth)
	return 0;

    /*
    if (dimension == 2)
	term = 16. / (M_PI * 5. * pow(bandwidth, 2));
    else
	term = 1. / bandwidth;
    term *= (15. / 16.);
    */

    x /= bandwidth;

    /* return term * (15. / 16. * pow(1 - x * x, 2)); */
    return term * pow(1 - x * x, 2);
}

double triweightKernel(double term, double bandwidth, double x)
{
    /* term is set by setKernelFunction */

    if (x > bandwidth)
	return 0;

    /*
    if (dimension == 2)
	term = 128. / (M_PI * 35. * pow(bandwidth, 2));
    else
	term = 1. / bandwidth;
    term *= (35. / 32);
    */

    x /= bandwidth;

    /* return term * (35. / 32 * pow(1 - x * x, 3)); */
    return term * pow(1 - x * x, 3);
}

double cosineKernel(double term, double bandwidth, double x)
{
    /* term is set by setKernelFunction */

    if (x > bandwidth)
	return 0;

    /*
    if (dimension == 2)
	term = 1. / (2 * (M_PI / 2 - 1) * pow(bandwidth, 2));
    else
	term = 1. / bandwidth;
    term *= (M_PI / 4.);
    */

    x /= bandwidth;

    /* return term * (M_PI / 4. * cos(M_PI / 2. * x)); */
    return term * cos(M_PI / 2. * x);
}

double kernelFunction(double term, double bandwidth, double x)
{
    return kernelfn(term, bandwidth, x);
}

void setKernelFunction(int function, int dimension, double bandwidth, double *term)
{
    switch (function) {
    case KERNEL_UNIFORM:
	kernelfn = uniformKernel;
	if (dimension == 2)
	    *term = 2. / (M_PI * pow(bandwidth, 2));
	else
	    *term = 1. / bandwidth;
	*term *= (1. / 2);
	break;
    case KERNEL_TRIANGULAR:
	kernelfn = triangularKernel;
	if (dimension == 2)
	    *term = 3. / (M_PI * pow(bandwidth, 2));
	else
	    *term = 1. / bandwidth;
	break;
    case KERNEL_EPANECHNIKOV:
	kernelfn = epanechnikovKernel;
	if (dimension == 2)
	    *term = 8. / (M_PI * 3. * pow(bandwidth, 2));
	else
	    *term = 1. / bandwidth;
	*term *= (3. / 4.);
	break;
    case KERNEL_QUARTIC:
	kernelfn = quarticKernel;
	if (dimension == 2)
	    *term = 16. / (M_PI * 5. * pow(bandwidth, 2));
	else
	    *term = 1. / bandwidth;
	*term *= (15. / 16.);
	break;
    case KERNEL_TRIWEIGHT:
	kernelfn = triweightKernel;
	if (dimension == 2)
	    *term = 128. / (M_PI * 35. * pow(bandwidth, 2));
	else
	    *term = 1. / bandwidth;
	*term *= (35. / 32);
	break;
    case KERNEL_GAUSSIAN:
	kernelfn = gaussianKernel4;
	*term =
	    1. / (pow(bandwidth, dimension) * pow((2. * M_PI), dimension / 2.));
	break;
    case KERNEL_COSINE:
	kernelfn = cosineKernel;
	if (dimension == 2)
	    *term = 1. / (2 * (M_PI / 2 - 1) * pow(bandwidth, 2));
	else
	    *term = 1. / bandwidth;
	*term *= (M_PI / 4.);
	break;
    default:
	G_fatal_error("Unknown kernel function");
    }
}
