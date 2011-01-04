
/****************************************************************************
* MODULE:       R-Tree library 
*              
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*               Markus Metz - file-based and memory-based R*-tree
*               
* PURPOSE:      Multidimensional index
*
* COPYRIGHT:    (C) 2010 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/
#include <stdio.h>
#include <math.h>

#ifndef ABS
#	define ABS(a) ((a) > 0 ? (a) : -(a))
#endif

#define EP .0000000001

double sphere_volume(double dimension)
{
    double log_gamma, log_volume;

    log_gamma = lgamma(dimension / 2.0 + 1);
    log_volume = dimension / 2.0 * log(M_PI) - log_gamma;
    return exp(log_volume);
}

int main()
{
    double dim = 0, delta = 1;

    while (ABS(delta) > EP)
	if (sphere_volume(dim + delta) > sphere_volume(dim))
	    dim += delta;
	else
	    delta /= -2;
    fprintf(stdout, "max volume = %.10f at dimension %.10f\n",
	    sphere_volume(dim), dim);
    return 0;
}
