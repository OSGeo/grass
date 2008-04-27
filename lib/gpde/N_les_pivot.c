
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      linear equation system pivoting strategy
*  		part of the gpde library
*               
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
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
#include "grass/N_pde.h"
#include "solvers_local_proto.h"


#define TINY 1.0e-20



/*!
 * \brief Optimize the structure of the linear equation system with a common pivoting strategy
 *
 * Create a optimized linear equation system for
 * direct solvers: gauss and lu decomposition.
 *
 * The rows are permuted based on the pivot elements.
 *
 * This algorithm will modify the provided linear equation system
 * and should only be used with the gauss elimination and lu decomposition solver.
 *
 * \param les * N_les -- the linear equation system
 * \return int - the number of swapped rows
 *
 *
 * */
int N_les_pivot_create(N_les * les)
{
    int num = 0;		/*number of changed rows */
    int i, j, k;
    double max;
    int number = 0;
    double tmpval = 0.0, s = 0.0;
    double *link = NULL;

    G_debug(2, "N_les_pivot_create: swap rows if needed");
    for (i = 0; i < les->rows; i++) {
	max = fabs(les->A[i][i]);
	number = i;
	for (j = i; j < les->rows; j++) {
	    s = 0.0;
	    for (k = i; k < les->rows; k++) {
		s += fabs(les->A[j][i]);
	    }
	    /*search for the pivot element */
	    if (max < fabs(les->A[j][i]) / s) {
		max = fabs(les->A[j][i]);
		number = j;
	    }
	}
	if (max == 0) {
	    max = TINY;
	    G_warning("Matrix is singular");
	}
	/*if an pivot element was found, swap the les entries */
	if (number != i) {

	    G_debug(4, "swap row %i with row %i", i, number);

	    tmpval = les->b[number];
	    les->b[number] = les->b[i];
	    les->b[i] = tmpval;

	    link = les->A[number];
	    les->A[number] = les->A[i];
	    les->A[i] = link;
	    num++;
	}
    }

    return num;
}
