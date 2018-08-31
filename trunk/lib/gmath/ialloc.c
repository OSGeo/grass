
/**
 * \file ialloc.c
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
 * \fn int *G_alloc_ivector (size_t n)
 *
 * \brief Vector matrix memory allocation.
 *
 * Allocate a vector (array) of <b>n</b> integers initialized to zero.
 *
 * \param[in] n size of vector to allocate
 * \return integer * 
 */
int *G_alloc_ivector(size_t n)
{
    return (int *)G_calloc(n, sizeof(int));
}

/**
 * \fn int **G_alloc_imatrix (int rows, int cols)
 *
 * \brief Matrix memory allocation.
 *
 * Allocate a matrix of <b>rows</b> by <b>cols</b> integers initialized
 * to zero.
 *
 * \param[in] rows number of rows in matrix
 * \param[in] cols number of columns in matrix
 * \return int ** 
 */
int **G_alloc_imatrix(int rows, int cols)
{
    int **m;
    int i;

    m = (int **)G_calloc(rows, sizeof(int *));
    m[0] = (int *)G_calloc((size_t) rows * cols, sizeof(int));
    for (i = 1; i < rows; i++)
	m[i] = m[i - 1] + cols;

    return m;
}

/**
 * \fn void G_free_ivector(int *v)
 *
 * \brief Vector memory deallocation.
 *
 * Deallocate a vector (array) of integers.
 *
 *  \param[in,out] v vector to free
 *  \return void
 */
void G_free_ivector(int *v)
{
    G_free(v);
    v = NULL;

    return;
}

/**
 * \fn int G_free_imatrix (int **m)
 *
 * \brief Matrix memory deallocation.
 *
 * Deallocate a matrix of integers.
 *
 *  \param[in,out] m matrix to free
 *  \return void
 */
void G_free_imatrix(int **m)
{
    G_free(m[0]);
    G_free(m);
    m = NULL;

    return;
}
