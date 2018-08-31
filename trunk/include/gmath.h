/******************************************************************************
 * gmath.h
 * Top level header file for gmath units

 * @Copyright David D.Gray <ddgray@armadce.demon.co.uk>
 * 27th. Sep. 2000
 * Last updated: $Id$
 *

 * This file is part of GRASS GIS. It is free software. You can 
 * redistribute it and/or modify it under the terms of 
 * the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 ******************************************************************************/

#ifndef GRASS_GMATH_H
#define GRASS_GMATH_H

#include <grass/config.h>

#ifdef CTYPESGEN
#undef __attribute__
#define __attribute__(x)
#endif

#include <stddef.h>

/*solver names */
#define G_MATH_SOLVER_DIRECT_GAUSS "gauss"
#define G_MATH_SOLVER_DIRECT_LU "lu"
#define G_MATH_SOLVER_DIRECT_CHOLESKY "cholesky"
#define G_MATH_SOLVER_ITERATIVE_JACOBI "jacobi"
#define G_MATH_SOLVER_ITERATIVE_SOR "sor"
#define G_MATH_SOLVER_ITERATIVE_CG "cg"
#define G_MATH_SOLVER_ITERATIVE_PCG "pcg"
#define G_MATH_SOLVER_ITERATIVE_BICGSTAB "bicgstab"

/*preconditioner */
#define G_MATH_DIAGONAL_PRECONDITION 1
#define G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION 2
#define G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION 3
#define G_MATH_ROWSCALE_MAXNORM_PRECONDITION 4

/*!
 * \brief The row vector of the sparse matrix
 * */
typedef struct
{
    double *values;		/*The non null values of the row */
    unsigned int cols;		/*Number of entries */
    unsigned int *index;	/*the index number */
} G_math_spvector;

#include <grass/defs/gmath.h>

#endif /* GRASS_GMATH_H */
