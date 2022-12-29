/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Radim Blazek
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <grass/vector.h>

/* 
 *  dig_line_box ()
 *  set box to points extent
 */
int dig_line_box(const struct line_pnts *Points, struct bound_box * Box)
{
    int i;

    if (Points->n_points <= 0) {
	G_zero(Box, sizeof(struct bound_box));
	return 0;
    }

    Box->E = Points->x[0];
    Box->W = Points->x[0];
    Box->N = Points->y[0];
    Box->S = Points->y[0];
    Box->T = Points->z[0];
    Box->B = Points->z[0];

    for (i = 1; i < Points->n_points; i++) {
	if (Points->x[i] > Box->E)
	    Box->E = Points->x[i];
	else if (Points->x[i] < Box->W)
	    Box->W = Points->x[i];

	if (Points->y[i] > Box->N)
	    Box->N = Points->y[i];
	else if (Points->y[i] < Box->S)
	    Box->S = Points->y[i];

	if (Points->z[i] > Box->T)
	    Box->T = Points->z[i];
	else if (Points->z[i] < Box->B)
	    Box->B = Points->z[i];
    }

    return 1;
}

/*
 *  dig_box_copy ()
 *  Copy B to A.
 */
int dig_box_copy(struct bound_box * A, struct bound_box * B)
{

    A->N = B->N;
    A->S = B->S;
    A->E = B->E;
    A->W = B->W;
    A->T = B->T;
    A->B = B->B;

    return 1;
}

/*
 * dig_box_extend ()
 * Extend A by B.
 */
int dig_box_extend(struct bound_box * A, struct bound_box * B)
{

    if (B->N > A->N)
	A->N = B->N;
    if (B->S < A->S)
	A->S = B->S;
    if (B->E > A->E)
	A->E = B->E;
    if (B->W < A->W)
	A->W = B->W;
    if (B->T > A->T)
	A->T = B->T;
    if (B->B < A->B)
	A->B = B->B;

    return 1;
}
