
/*****************************************************************************
*
* MODULE:       Grass numerical math interface
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> googlemail <dot> com
*               
* PURPOSE:      grass blas implementation
* 		part of the gmath library
*               
* COPYRIGHT:    (C) 2010 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <grass/gmath.h>

#if defined(HAVE_ATLAS)
#include <cblas.h>
#endif


/*!
 * \brief Compute the dot product of vector x and y 
 * using the ATLAS routine cblas_ddot 
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_f_x_dot_y, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (float *)
 * \param y       (float *)
 * \param rows (int)
 * \return (double)
 *
 * */
double G_math_ddot(double *x, double *y, int rows)
{
#if defined(HAVE_ATLAS)
    return cblas_ddot(rows, x, 1, y, 1);
#else
    double val;

    G_math_d_x_dot_y(x, y, &val, rows);
    return val;
#endif
}


/*!
 * \brief Compute the dot product of vector x and y 
 * using the ATLAS routine cblas_sdsdot 
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_f_x_dot_y, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (float *)
 * \param y       (float *)
 * \param a       (float)
 * \param rows (int)
 * \return (float)
 *
 * */
float G_math_sdsdot(float *x, float *y, float a, int rows)
{
#if defined(HAVE_ATLAS)
    return cblas_sdsdot(rows, a, x, 1, y, 1);
#else
    float val;

    G_math_f_x_dot_y(x, y, &val, rows);
    return a + val;
#endif
}

/*!
 * \brief Compute the euclidean norm of vector x  
 * using the ATLAS routine cblas_dnrm2 
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_d_euclid_norm, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (double *)
 * \param rows (int)
 * \return (double)
 *
 * */
double G_math_dnrm2(double *x, int rows)
{
#if defined(HAVE_ATLAS)
    return cblas_dnrm2(rows, x, 1);
#else
    double val;

    G_math_d_euclid_norm(x, &val, rows);
    return val;
#endif
}

/*!
 * \brief Compute the absolute sum norm of vector x  
 * using the ATLAS routine cblas_dasum 
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_d_asum_norm, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (double *)
 * \param rows (int)
 * \return (double)
 *
 * */
double G_math_dasum(double *x, int rows)
{
#if defined(HAVE_ATLAS)
    return cblas_dasum(rows, x, 1);
#else
    double val;

    G_math_d_asum_norm(x, &val, rows);
    return val;
#endif
}

/*!
 * \brief Compute the maximum norm of vector x  
 * using the ATLAS routine cblas_idamax 
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_d_max_norm, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (double *)
 * \param rows (int)
 * \return (double)
 *
 * */
double G_math_idamax(double *x, int rows)
{
#if defined(HAVE_ATLAS)
    return cblas_idamax(rows, x, 1);
#else
    double val;

    G_math_d_max_norm(x, &val, rows);
    return val;
#endif
}

/*!
 * \brief Scale vector x with scalar a
 * using the ATLAS routine cblas_dscal
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_d_ax_by, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (double *)
 * \param a       (double)
 * \param rows (int)
 * \return (void)
 *
 * */
void G_math_dscal(double *x, double a, int rows)
{
#if defined(HAVE_ATLAS)
    cblas_dscal(rows, a, x, 1);
#else
    G_math_d_ax_by(x, x, x, a, 0.0, rows);
#endif

    return;
}

/*!
 * \brief  Copy vector x to vector y
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_d_copy
 *
 * \param x       (double *)
 * \param y       (double *)
 * \param rows (int)
 * \return (void)
 *
 * */
void G_math_dcopy(double *x, double *y, int rows)
{
#if defined(HAVE_ATLAS)
    cblas_dcopy(rows, x, 1, y, 1);
#else
    G_math_d_copy(x, y, rows);
#endif

    return;
}


/*!
 * \brief Scale vector x with scalar a and add it to y 
 *
 * \f[ {\bf z} = a{\bf x} + {\bf y} \f]
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_d_ax_by, the 
 * grass implementatiom

 *
 * \param x      (double *)
 * \param y      (double *)
 * \param a      (double)
 * \param rows (int)
 * \return (void)
 * 
 * */
void G_math_daxpy(double *x, double *y, double a, int rows)
{
#if defined(HAVE_ATLAS)
    cblas_daxpy(rows, a, x, 1, y, 1);
#else
    G_math_d_ax_by(x, y, y, a, 1.0, rows);
#endif

    return;
}

/****************************************************************** */

/********* F L O A T / S I N G L E   P E P R E C I S I O N ******** */

/****************************************************************** */

/*!
 * \brief Compute the dot product of vector x and y 
 * using the ATLAS routine cblas_sdot 
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_f_x_dot_y, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (float *)
 * \param y       (float *)
 * \param rows (int)
 * \return (float)
 *
 * */
float G_math_sdot(float *x, float *y, int rows)
{
#if defined(HAVE_ATLAS)
    return cblas_sdot(rows, x, 1, y, 1);
#else
    float val;

    G_math_f_x_dot_y(x, y, &val, rows);
    return val;
#endif
}

/*!
 * \brief Compute the euclidean norm of vector x  
 * using the ATLAS routine cblas_dnrm2 
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_f_euclid_norm, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (float *)
 * \param rows (int)
 * \return (float)
 *
 * */
float G_math_snrm2(float *x, int rows)
{
#if defined(HAVE_ATLAS)
    return cblas_snrm2(rows, x, 1);
#else
    float val;

    G_math_f_euclid_norm(x, &val, rows);
    return val;
#endif
}

/*!
 * \brief Compute the absolute sum norm of vector x  
 * using the ATLAS routine cblas_dasum 
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_f_asum_norm, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (float *)
 * \param rows (int)
 * \return (float)
 *
 * */
float G_math_sasum(float *x, int rows)
{
#if defined(HAVE_ATLAS)
    return cblas_sasum(rows, x, 1);
#else
    float val;

    G_math_f_asum_norm(x, &val, rows);
    return val;
#endif
}

/*!
 * \brief Compute the maximum norm of vector x  
 * using the ATLAS routine cblas_idamax 
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_f_max_norm, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (float *)
 * \param rows (int)
 * \return (float)
 *
 * */
float G_math_isamax(float *x, int rows)
{
#if defined(HAVE_ATLAS)
    return cblas_isamax(rows, x, 1);
#else
    float val;

    G_math_f_max_norm(x, &val, rows);
    return val;
#endif
}

/*!
 * \brief Scale vector x with scalar a
 * using the ATLAS routine cblas_dscal
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_f_ax_by, the OpenMP multi threaded 
 * grass implementatiom
 *
 * \param x       (float *)
 * \param a       (float)
 * \param rows (int)
 * \return (float)
 *
 * */
void G_math_sscal(float *x, float a, int rows)
{
#if defined(HAVE_ATLAS)
    cblas_sscal(rows, a, x, 1);
#else
    G_math_f_ax_by(x, x, x, a, 0.0, rows);
#endif

    return;
}

/*!
 * \brief  Copy vector x to vector y
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_f_copy, the 
 * grass implementatiom
 *
 * \param x       (float *)
 * \param y       (float *)
 * \param rows (int)
 * \return (void)
 *
 * */
void G_math_scopy(float *x, float *y, int rows)
{
#if defined(HAVE_ATLAS)
    cblas_scopy(rows, x, 1, y, 1);
#else
    G_math_f_copy(x, y, rows);
#endif

    return;
}


/*!
 * \brief Scale vector x with scalar a and add it to y 
 *
 * \f[ {\bf z} = a{\bf x} + {\bf y} \f]
 *
 * If grass was not compiled with ATLAS support
 * it will call #G_math_f_ax_by, the 
 * grass implementatiom

 *
 * \param x      (float *)
 * \param y      (float *)
 * \param a      (float)
 * \param rows (int)
 * \return (void)
 * 
 * */
void G_math_saxpy(float *x, float *y, float a, int rows)
{
#if defined(HAVE_ATLAS)
    cblas_saxpy(rows, a, x, 1, y, 1);
#else
    G_math_f_ax_by(x, y, y, a, 1.0, rows);
#endif

    return;
}
