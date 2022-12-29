
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      upwinding stabilization algorithms
* 		part of the gpde library
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <math.h>
#include <grass/N_pde.h>


/*! \brief full upwinding stabilization algorithm
 *
 * The arguments are values to compute the local peclet number
 *
 * \param sprod double -- the scalar produkt between the velocity vector and the normal vector between two points
 * \param distance double -- distance between two points
 * \param D double -- diffusion/dispersion tensor part between two points
 *
 * \return the weighting factor
 * */
double N_full_upwinding(double sprod, double distance, double D)
{
    double z;

    if (D == 0)
	return 0.5;

    /*compute the local peclet number */
    z = sprod * distance / D;

    if (z > 0)
	return 1;
    if (z == 0)
	return 0.5;
    if (z < 0)
	return 0;

    return 0;
}

/*! \brief exponential upwinding stabilization algorithm
 *
 * The arguments are values to compute the local peclet number
 *
 * \param sprod double -- the scalar produkt between the velocity vector and the normal vector between two points
 * \param distance double -- distance between two points
 * \param D double -- diffusion/dispersion tensor part between two points
 *
 * \return the weighting factor
 * */
double N_exp_upwinding(double sprod, double distance, double D)
{
    double z;

    if (D == 0)
	return 0.5;

    /*compute the local peclet number */
    z = sprod * distance / D;

    if (z != 0)
	return (1 - (1 / z) * (1 - (z / (exp(z) - 1))));

    return 0.5;
}
