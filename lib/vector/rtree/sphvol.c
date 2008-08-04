
/****************************************************************************
* MODULE:       R-Tree library 
*              
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*               
* PURPOSE:      Multidimensional index
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/

/*
 *                   SPHERE VOLUME
 *                   by Daniel Green
 *                   dgreen@superliminal.com
 *
 * Calculates and prints the volumes of the unit hyperspheres for
 * dimensions zero through the given value, or 9 by default.
 * Prints in the form of a C array of double called sphere_volumes.
 *
 * From formule in "Regular Polytopes" by H.S.M Coxeter, the volume
 * of a hypersphere of dimension d is:
 *        Pi^(d/2) / gamma(d/2 + 1)
 * 
 * This implementation works by first computing the log of the above
 * function and then returning the exp of that value in order to avoid
 * instabilities due to the huge values that the real gamma function
 * would return.
 *
 * Multiply the output volumes by R^n to get the volume of an n
 * dimensional sphere of radius R.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>

static void print_volume(int dimension, double volume)
{
    fprintf(stdout, "\t%.6f,  /* dimension %3d */\n", volume, dimension);
}

static double sphere_volume(double dimension)
{
    double log_gamma, log_volume;

    log_gamma = gamma(dimension / 2.0 + 1);
    log_volume = dimension / 2.0 * log(M_PI) - log_gamma;
    return exp(log_volume);
}

extern int main(int argc, char *argv[])
{
    int dim, max_dims = 9;

    if (2 == argc)
	max_dims = atoi(argv[1]);

    fprintf(stdout, "static const double sphere_volumes[] = {\n");
    for (dim = 0; dim < max_dims + 1; dim++)
	print_volume(dim, sphere_volume(dim));
    fprintf(stdout, "};\n");
    return 0;
}
