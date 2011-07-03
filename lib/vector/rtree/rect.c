
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
#include <stdlib.h>
#include <assert.h>
#include "index.h"

#include <float.h>
#include <math.h>

#define BIG_NUM (FLT_MAX/4.0)


#define Undefined(x) ((x)->boundary[0] > (x)->boundary[NUMDIMS])
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


/*-----------------------------------------------------------------------------
| Initialize a rectangle to have all 0 coordinates.
-----------------------------------------------------------------------------*/
void RTreeInitRect(struct RTree_Rect *R)
{
    register struct RTree_Rect *r = R;
    register int i;

    for (i = 0; i < NUMSIDES; i++)
	r->boundary[i] = (RectReal) 0;
}


/*-----------------------------------------------------------------------------
| Return a rect whose first low side is higher than its opposite side -
| interpreted as an undefined rect.
-----------------------------------------------------------------------------*/
struct RTree_Rect RTreeNullRect(void)
{
    struct RTree_Rect r;
    register int i;

    r.boundary[0] = (RectReal) 1;
    r.boundary[NUMDIMS] = (RectReal) - 1;
    for (i = 1; i < NUMDIMS; i++)
	r.boundary[i] = r.boundary[i + NUMDIMS] = (RectReal) 0;
    return r;
}


#if 0

/*-----------------------------------------------------------------------------
| Fills in random coordinates in a rectangle.
| The low side is guaranteed to be less than the high side.
-----------------------------------------------------------------------------*/
void RTreeRandomRect(struct RTree_Rect *R)
{
    register struct RTree_Rect *r = R;
    register int i;
    register RectReal width;

    for (i = 0; i < NUMDIMS; i++) {
	/* width from 1 to 1000 / 4, more small ones
	 */
	width = drand48() * (1000 / 4) + 1;

	/* sprinkle a given size evenly but so they stay in [0,100]
	 */
	r->boundary[i] = drand48() * (1000 - width);	/* low side */
	r->boundary[i + NUMDIMS] = r->boundary[i] + width;	/* high side */
    }
}


/*-----------------------------------------------------------------------------
| Fill in the boundaries for a random search rectangle.
| Pass in a pointer to a rect that contains all the data,
| and a pointer to the rect to be filled in.
| Generated rect is centered randomly anywhere in the data area,
| and has size from 0 to the size of the data area in each dimension,
| i.e. search rect can stick out beyond data area.
-----------------------------------------------------------------------------*/
void RTreeSearchRect(struct RTree_Rect *Search, struct RTree_Rect *Data)
{
    register struct RTree_Rect *search = Search, *data = Data;
    register int i, j;
    register RectReal size, center;

    assert(search);
    assert(data);

    for (i = 0; i < NUMDIMS; i++) {
	j = i + NUMDIMS;	/* index for high side boundary */
	if (data->boundary[i] > -BIG_NUM && data->boundary[j] < BIG_NUM) {
	    size = (drand48() * (data->boundary[j] -
				 data->boundary[i] + 1)) / 2;
	    center = data->boundary[i] + drand48() *
		(data->boundary[j] - data->boundary[i] + 1);
	    search->boundary[i] = center - size / 2;
	    search->boundary[j] = center + size / 2;
	}
	else {			/* some open boundary, search entire dimension */

	    search->boundary[i] = -BIG_NUM;
	    search->boundary[j] = BIG_NUM;
	}
    }
}

#endif

/*-----------------------------------------------------------------------------
| Print out the data for a rectangle.
-----------------------------------------------------------------------------*/
void RTreePrintRect(struct RTree_Rect *R, int depth)
{
    register struct RTree_Rect *r = R;
    register int i;

    assert(r);

    RTreeTabIn(depth);
    fprintf(stdout, "rect:\n");
    for (i = 0; i < NUMDIMS; i++) {
	RTreeTabIn(depth + 1);
	fprintf(stdout, "%f\t%f\n", r->boundary[i], r->boundary[i + NUMDIMS]);
    }
}

/*-----------------------------------------------------------------------------
| Calculate the n-dimensional volume of a rectangle
-----------------------------------------------------------------------------*/
RectReal RTreeRectVolume(struct RTree_Rect *R, struct RTree *t)
{
    register struct RTree_Rect *r = R;
    register int i;
    register RectReal volume = (RectReal) 1;

    assert(r);
    if (Undefined(r))
	return (RectReal) 0;

    for (i = 0; i < t->ndims; i++)
	volume *= r->boundary[i + NUMDIMS] - r->boundary[i];
    assert(volume >= 0.0);
    return volume;
}


/*-----------------------------------------------------------------------------
| Define the NUMDIMS-dimensional volume the unit sphere in that dimension into
| the symbol "UnitSphereVolume"
| Note that if the gamma function is available in the math library and if the
| compiler supports static initialization using functions, this is
| easily computed for any dimension. If not, the value can be precomputed and
| taken from a table. The following code can do it either way.
-----------------------------------------------------------------------------*/

#ifdef gamma

/* computes the volume of an N-dimensional sphere. */
/* derived from formule in "Regular Polytopes" by H.S.M Coxeter */
static double sphere_volume(double dimension)
{
    double log_gamma, log_volume;

    log_gamma = gamma(dimension / 2.0 + 1);
    log_volume = dimension / 2.0 * log(M_PI) - log_gamma;
    return exp(log_volume);
}
static const double UnitSphereVolume = sphere_volume(NUMDIMS);

#else

/* Precomputed volumes of the unit spheres for the first few dimensions */
const double UnitSphereVolumes[] = {
    0.000000,			/* dimension   0 */
    2.000000,			/* dimension   1 */
    3.141593,			/* dimension   2 */
    4.188790,			/* dimension   3 */
    4.934802,			/* dimension   4 */
    5.263789,			/* dimension   5 */
    5.167713,			/* dimension   6 */
    4.724766,			/* dimension   7 */
    4.058712,			/* dimension   8 */
    3.298509,			/* dimension   9 */
    2.550164,			/* dimension  10 */
    1.884104,			/* dimension  11 */
    1.335263,			/* dimension  12 */
    0.910629,			/* dimension  13 */
    0.599265,			/* dimension  14 */
    0.381443,			/* dimension  15 */
    0.235331,			/* dimension  16 */
    0.140981,			/* dimension  17 */
    0.082146,			/* dimension  18 */
    0.046622,			/* dimension  19 */
    0.025807,			/* dimension  20 */
};

#if NUMDIMS > 20
#	error "not enough precomputed sphere volumes"
#endif
#define UnitSphereVolume UnitSphereVolumes[NUMDIMS]

#endif


/*-----------------------------------------------------------------------------
| Calculate the n-dimensional volume of the bounding sphere of a rectangle
-----------------------------------------------------------------------------*/

#if 0
/*
 * A fast approximation to the volume of the bounding sphere for the
 * given Rect. By Paul B.
 */
RectReal RTreeRectSphericalVolume(struct RTree_Rect *R, struct RTree *t)
{
    register struct RTree_Rect *r = R;
    register int i;
    RectReal maxsize = (RectReal) 0, c_size;

    assert(r);
    if (Undefined(r))
	return (RectReal) 0;
    for (i = 0; i < t->ndims; i++) {
	c_size = r->boundary[i + NUMDIMS] - r->boundary[i];
	if (c_size > maxsize)
	    maxsize = c_size;
    }
    return (RectReal) (pow(maxsize / 2, NUMDIMS) *
		       UnitSphereVolumes[t->ndims]);
}
#endif

/*
 * The exact volume of the bounding sphere for the given Rect.
 */
RectReal RTreeRectSphericalVolume(struct RTree_Rect *r, struct RTree *t)
{
    int i;
    double sum_of_squares = 0, radius, half_extent;

    assert(r);
    if (Undefined(r))
	return (RectReal) 0;
    for (i = 0; i < t->ndims; i++) {
	half_extent = (r->boundary[i + NUMDIMS] - r->boundary[i]) / 2;

	sum_of_squares += half_extent * half_extent;
    }
    radius = sqrt(sum_of_squares);
    return (RectReal) (pow(radius, t->ndims) * UnitSphereVolumes[t->ndims]);
}


/*-----------------------------------------------------------------------------
| Calculate the n-dimensional surface area of a rectangle
-----------------------------------------------------------------------------*/
RectReal RTreeRectSurfaceArea(struct RTree_Rect *r, struct RTree *t)
{
    int i, j;
    RectReal j_extent, sum = (RectReal) 0;

    assert(r);
    if (Undefined(r))
	return (RectReal) 0;

    for (i = 0; i < t->ndims; i++) {
	RectReal face_area = (RectReal) 1;

	for (j = 0; j < t->ndims; j++)
	    /* exclude i extent from product in this dimension */
	    if (i != j) {
		j_extent = r->boundary[j + NUMDIMS] - r->boundary[j];

		face_area *= j_extent;
	    }
	sum += face_area;
    }
    return 2 * sum;
}


/*-----------------------------------------------------------------------------
| Calculate the n-dimensional margin of a rectangle
| the margin is the sum of the lengths of the edges
-----------------------------------------------------------------------------*/
RectReal RTreeRectMargin(struct RTree_Rect *r, struct RTree *t)
{
    int i;
    RectReal margin = 0.0;

    assert(r);

    for (i = 0; i < t->ndims; i++) {
	margin += r->boundary[i + NUMDIMS] - r->boundary[i];
    }

    return margin;
}


/*-----------------------------------------------------------------------------
| Combine two rectangles, make one that includes both.
-----------------------------------------------------------------------------*/
struct RTree_Rect RTreeCombineRect(struct RTree_Rect *r, struct RTree_Rect *rr, struct RTree *t)
{
    int i, j;
    struct RTree_Rect new_rect;

    assert(r && rr);

    if (Undefined(r))
	return *rr;

    if (Undefined(rr))
	return *r;

    for (i = 0; i < t->ndims; i++) {
	new_rect.boundary[i] = MIN(r->boundary[i], rr->boundary[i]);
	j = i + NUMDIMS;
	new_rect.boundary[j] = MAX(r->boundary[j], rr->boundary[j]);
    }

    /* set remaining boundaries to zero */
    for (; i < NUMDIMS; i++) {
	new_rect.boundary[i] = MIN(r->boundary[i], rr->boundary[i]);
	j = i + NUMDIMS;
	new_rect.boundary[j] = MAX(r->boundary[j], rr->boundary[j]);
    }

    return new_rect;
}


/*-----------------------------------------------------------------------------
| Decide whether two rectangles are identical.
-----------------------------------------------------------------------------*/
int RTreeCompareRect(struct RTree_Rect *r, struct RTree_Rect *s, struct RTree *t)
{
    register int i, j;

    assert(r && s);

    for (i = 0; i < t->ndims; i++) {
	j = i + NUMDIMS;	/* index for high sides */
	if (r->boundary[i] != s->boundary[i] ||
	    r->boundary[j] != s->boundary[j]) {
	    return 0;
	}
    }
    return 1;
}


/*-----------------------------------------------------------------------------
| Decide whether two rectangles overlap.
-----------------------------------------------------------------------------*/
int RTreeOverlap(struct RTree_Rect *r, struct RTree_Rect *s, struct RTree *t)
{
    register int i, j;

    assert(r && s);

    for (i = 0; i < t->ndims; i++) {
	j = i + NUMDIMS;	/* index for high sides */
	if (r->boundary[i] > s->boundary[j] ||
	    s->boundary[i] > r->boundary[j]) {
	    return FALSE;
	}
    }
    return TRUE;
}


/*-----------------------------------------------------------------------------
| Decide whether rectangle r is contained in rectangle s.
-----------------------------------------------------------------------------*/
int RTreeContained(struct RTree_Rect *r, struct RTree_Rect *s, struct RTree *t)
{
    register int i, j, result;

    assert(r && s);

    /* undefined rect is contained in any other */
    if (Undefined(r))
	return TRUE;

    /* no rect (except an undefined one) is contained in an undef rect */
    if (Undefined(s))
	return FALSE;

    result = TRUE;
    for (i = 0; i < t->ndims; i++) {
	j = i + NUMDIMS;	/* index for high sides */
	result = result && r->boundary[i] >= s->boundary[i]
	    && r->boundary[j] <= s->boundary[j];
    }
    return result;
}
