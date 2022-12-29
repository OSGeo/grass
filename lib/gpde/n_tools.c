
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:     	Array management functions 
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
#include <grass/glocale.h>


/*!
 * \brief Calculate the arithmetic mean of values a and b
 *
 * mean = (a+b)/2
 *
 * \param a double
 * \param b double
 * \return val double
 * */
double N_calc_arith_mean(double a, double b)
{
    double val = 0;

    val = (a + b) / 2.0;

    return val;
}

/*!
 * \brief Calculate the arithmetic mean of the values in vector a
 * of size n
 *
 * n = [0 ... size[
 * mean =  (a[0] + a[1] + ... + a[n])/size
 *
 * \param a double * -- the value vector 
 * \param size int -- the size of the vector a
 * \return val double
 * */
double N_calc_arith_mean_n(double *a, int size)
{
    double val = 0.0;
    int i;

    for (i = 0; i < size; i++)
	val += a[i];

    val = (val / (double)size);

    return val;
}


/*!
 * \brief Calculate the geometrical mean of values a and b
 *
 * mean = sqrt(a*b)
 *
 * \param a double
 * \param b double
 * \return val double
 * */
double N_calc_geom_mean(double a, double b)
{
    double val = 0;

    val = sqrt(a * b);

    return val;
}

/*!
 * \brief Calculate the geometrical mean of the values in vector a
 * of size n
 *
 * n = [0 ... size[
 * mean =  pow((a[0] * a[1] * ... * a[n]), 1.0/size)
 *
 * \param a double * -- the value vector 
 * \param size int -- the size of the vector a
 * \return val double
 * */
double N_calc_geom_mean_n(double *a, int size)
{
    double val = 1;
    int i;

    for (i = 0; i < size; i++)
	val *= a[i];

    val = (double)pow((long double)val, (long double)1.0 / (long double)size);

    return val;
}


/*!
 * \brief Calculate the harmonical mean of values a and b
 *
 * mean = 2*(a*b)/(a + b)
 *
 * \param a double
 * \param b double
 * \return val double -- if (a + b) == 0, a 0 is returned
 * */
double N_calc_harmonic_mean(double a, double b)
{
    double val = 0.0;

    if ((a + b) != 0)
	val = 2.0 * (a * b) / (a + b);

    return val;
}

/*!
 * \brief Calculate the harmonical mean of the values in vector a
 * of size n
 *
 * n = [0 ... size[
 * mean = 1/(1/size *(1/a[0] + 1/a[1] + ... + 1/a[n]))
 * 
 * \param a double * -- the value vector 
 * \param size int -- the size of the vector a
 * \return val double -- if one division with 0 is detected, 0 will be returned
 * */
double N_calc_harmonic_mean_n(double *a, int size)
{
    double val = 0;
    int i;

    for (i = 0; i < size; i++)
	if (a[i] != 0.0)
	    val += 1.0 / a[i];
	else
	    return 0.0;

    if (val == 0.0)
	return 0.0;
    else
	val = 1.0 / (1.0 / (double)size * val);

    return val;
}


/*!
 * \brief Calculate the quadratic mean of values a and b
 *
 * mean = sqrt((a*a + b*b)/2)
 *
 * \param a double
 * \param b double
 * \return val double 
 * */
double N_calc_quad_mean(double a, double b)
{
    double val = 0.0;

    val = sqrt((a * a + b * b) / 2.0);

    return val;
}

/*!
 * \brief Calculate the quadratic mean of the values in vector a
 * of size n
 *
 * n = [0 ... size[
 * mean = sqrt((a[0]*a[0] + a[1]*a[1] + ... + a[n]*a[n])/size)
 * 
 * \param a double * -- the value vector 
 * \param size int -- the size of the vector a
 * \return val double 
 * */
double N_calc_quad_mean_n(double *a, int size)
{
    double val = 0;
    int i;

    for (i = 0; i < size; i++)
	val += a[i] * a[i];

    val = sqrt(val / (double)size);

    return val;
}
