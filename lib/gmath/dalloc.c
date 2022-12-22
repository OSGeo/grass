/**
 * \file dalloc.c
 *
 * \brief Matrix memory management functions.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * \author GRASS GIS Development Team
 *
 * \date 2004-2006
 */

#include <stdlib.h>
#include <grass/gis.h>

/**
 * \fn double *G_alloc_vector (size_t n)
 *
 * \brief Vector matrix memory allocation.
 *
 * Allocate a vector (array) of <b>n</b> doubles initialized to zero.
 *
 * \param[in] n size of vector to allocate
 * \return double *
 */

double *G_alloc_vector(size_t n)
{
    return (double *)G_calloc(n, sizeof(double));
}

/**
 * \fn double **G_alloc_matrix (int rows,int cols)
 *
 * \brief Matrix memory allocation.
 *
 * Allocate a matrix of <b>rows</b> by <b>cols</b> doubles initialized
 * to zero.
 *
 * \param[in] rows number of rows in matrix
 * \param[in] cols number of columns in matrix
 * \return double **
 */

double **G_alloc_matrix(int rows, int cols)
{
    double **m;
    int i;

    m = (double **)G_calloc(rows, sizeof(double *));
    m[0] = (double *)G_calloc((size_t)rows * cols, sizeof(double));
    for (i = 1; i < rows; i++)
        m[i] = m[i - 1] + cols;

    return m;
}

/**
 * \fn float *G_alloc_fvector (size_t n)
 *
 * \brief Floating point vector memory allocation.
 *
 * Allocate a vector (array) of <b>n</b> floats initialized to zero.
 *
 * \param[in] n size of vector to allocate
 * \return float *
 */

float *G_alloc_fvector(size_t n)
{
    return (float *)G_calloc(n, sizeof(float));
}

/**
 * \fn float **G_alloc_fmatrix (int rows, int cols)
 *
 * \brief Floating point matrix memory allocation.
 *
 * Allocate a matrix of <b>rows</b> by <b>cols</b> floats initialized
 * to zero.
 *
 *  \param[in] rows number of rows in matrix
 *  \param[in] cols number of columns in matrix
 *  \return float **
 */

float **G_alloc_fmatrix(int rows, int cols)
{
    float **m;
    int i;

    m = (float **)G_calloc(rows, sizeof(float *));
    m[0] = (float *)G_calloc((size_t)rows * cols, sizeof(float));
    for (i = 1; i < rows; i++)
        m[i] = m[i - 1] + cols;

    return m;
}

/**
 * \fn void G_free_vector (double *v)
 *
 * \brief Vector memory deallocation.
 *
 * Deallocate a vector (array) of doubles.
 *
 *  \param[in,out] v vector to free
 *  \return void
 */

void G_free_vector(double *v)
{
    G_free(v);
    v = NULL;

    return;
}

/**
 * \fn void G_free_fvector (float *v)
 *
 * \brief Vector memory deallocation.
 *
 * Deallocate a vector (array) of floats.
 *
 *  \param[in,out] v vector to free
 *  \return void
 */

void G_free_fvector(float *v)
{
    G_free(v);
    v = NULL;

    return;
}

/**
 * \fn void G_free_matrix (double **m)
 *
 * \brief Matrix memory deallocation.
 *
 * Deallocate a matrix of doubles.
 *
 *  \param[in,out] m matrix to free
 *  \return void
 */

void G_free_matrix(double **m)
{
    G_free(m[0]);
    G_free(m);
    m = NULL;

    return;
}

/**
 * \fn void G_free_fmatrix (float **m)
 *
 * \brief Floating point matrix memory deallocation.
 *
 * Deallocate a matrix of floats.
 *
 *  \param[in,out] m matrix to free
 *  \return void
 */

void G_free_fmatrix(float **m)
{
    G_free(m[0]);
    G_free(m);
    m = NULL;

    return;
}
