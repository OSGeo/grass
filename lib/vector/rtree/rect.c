
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


#define Undefined(x, t) ((x)->boundary[0] > (x)->boundary[t->ndims_alloc])
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**-----------------------------------------------------------------------------
 \brief Create a new rectangle for a given tree

 This method allocates a new rectangle and initializes
 the internal boundary coordinates based on the tree dimension.

 Hence a call to RTreeNewBoundary() is not necessary.

 \param t: The pointer to a RTree struct
 \return A new allocated RTree_Rect struct
-----------------------------------------------------------------------------*/
struct RTree_Rect *RTreeAllocRect(struct RTree *t)
{
    struct RTree_Rect *r;

    assert(t);

    r = (struct RTree_Rect *)malloc(sizeof(struct RTree_Rect));

    assert(r);

    r->boundary = RTreeAllocBoundary(t);
    return r;
}

/**----------------------------------------------------------------------------
 \brief Delete a rectangle

 This method deletes (free) the allocated memory of a rectangle.

 \param r: The pointer to the rectangle to be deleted
-----------------------------------------------------------------------------*/
void RTreeFreeRect(struct RTree_Rect *r)
{
    assert(r);
    RTreeFreeBoundary(r);
    free(r);
}

/**----------------------------------------------------------------------------
 \brief Allocate the boundary array of a rectangle for a given tree

 This method allocated the boundary coordinates array in
 provided rectangle. It does not release previously allocated memory.

 \param r: The  pointer to rectangle to initialize the boundary coordinates.
           This is usually a rectangle that was created on the stack or
           self allocated.
 \param t: The pointer to a RTree struct
-----------------------------------------------------------------------------*/
RectReal *RTreeAllocBoundary(struct RTree *t)
{
    RectReal *boundary = (RectReal *)malloc(t->rectsize);

    assert(boundary);

    return boundary;
}

/**----------------------------------------------------------------------------
 \brief Delete the boundary of a rectangle

 This method deletes (free) the memory of the boundary of a rectangle
 and sets the boundary pointer to NULL.

 \param r: The pointer to the rectangle to delete the boundary from.
-----------------------------------------------------------------------------*/
void RTreeFreeBoundary(struct RTree_Rect *r)
{
    assert(r);
    if (r->boundary)
        free(r->boundary);
    r->boundary = NULL;
}

/**-----------------------------------------------------------------------------
 \brief Initialize a rectangle to have all 0 coordinates.
-----------------------------------------------------------------------------*/
void RTreeInitRect(struct RTree_Rect *r, struct RTree *t)
{
    register int i;

    for (i = 0; i < t->ndims_alloc; i++)
	r->boundary[i] = r->boundary[i + t->ndims_alloc] = (RectReal) 0;
}

/**-----------------------------------------------------------------------------
 \brief Set one dimensional coordinates of a rectangle for a given tree.

 All coordinates of the rectangle will be initialized to 0 before
 the x coordinates are set.

 \param r: The pointer to the rectangle
 \param t: The pointer to the RTree
 \param x_min: The lower x coordinate
 \param x_max: The higher x coordinate
-----------------------------------------------------------------------------*/
void RTreeSetRect1D(struct RTree_Rect *r, struct RTree *t, double x_min,
                   double x_max)
{
    RTreeInitRect(r, t);
    r->boundary[0] = (RectReal)x_min;
    r->boundary[t->ndims_alloc] = (RectReal)x_max;
}

/**-----------------------------------------------------------------------------
 \brief Set two dimensional coordinates of a rectangle for a given tree.

 All coordinates of the rectangle will be initialized to 0 before
 the x and y coordinates are set.

 \param r: The pointer to the rectangle
 \param t: The pointer to the RTree
 \param x_min: The lower x coordinate
 \param x_max: The higher x coordinate
 \param y_min: The lower y coordinate
 \param y_max: The higher y coordinate
-----------------------------------------------------------------------------*/
void RTreeSetRect2D(struct RTree_Rect *r, struct RTree *t, double x_min,
                   double x_max, double y_min, double y_max)
{
    RTreeInitRect(r, t);
    r->boundary[0] = (RectReal)x_min;
    r->boundary[t->ndims_alloc] = (RectReal)x_max;
    r->boundary[1] = (RectReal)y_min;
    r->boundary[1 + t->ndims_alloc] = (RectReal)y_max;
}

/**-----------------------------------------------------------------------------
 \brief Set three dimensional coordinates of a rectangle for a given tree.

 All coordinates of the rectangle will be initialized to 0 before
 the x,y and z coordinates are set.

 \param r: The pointer to the rectangle
 \param t: The pointer to the RTree
 \param x_min: The lower x coordinate
 \param x_max: The higher x coordinate
 \param y_min: The lower y coordinate
 \param y_max: The higher y coordinate
 \param z_min: The lower z coordinate
 \param z_max: The higher z coordinate
-----------------------------------------------------------------------------*/
void RTreeSetRect3D(struct RTree_Rect *r, struct RTree *t, double x_min,
                   double x_max, double y_min, double y_max, double z_min,
                   double z_max)
{
    RTreeInitRect(r, t);
    r->boundary[0] = (RectReal)x_min;
    r->boundary[t->ndims_alloc] = (RectReal)x_max;
    r->boundary[1] = (RectReal)y_min;
    r->boundary[1 + t->ndims_alloc] = (RectReal)y_max;
    r->boundary[2] = (RectReal)z_min;
    r->boundary[2 + t->ndims_alloc] = (RectReal)z_max;
}

/**-----------------------------------------------------------------------------
 \brief Set 4 dimensional coordinates of a rectangle for a given tree.

 All coordinates of the rectangle will be initialized to 0 before
 the x,y,z and t coordinates are set.

 \param r: The pointer to the rectangle
 \param t: The pointer to the RTree
 \param x_min: The lower x coordinate
 \param x_max: The higher x coordinate
 \param y_min: The lower y coordinate
 \param y_max: The higher y coordinate
 \param z_min: The lower z coordinate
 \param z_max: The higher z coordinate
 \param t_min: The lower t coordinate
 \param t_max: The higher t coordinate
-----------------------------------------------------------------------------*/
void RTreeSetRect4D(struct RTree_Rect *r, struct RTree *t, double x_min,
                   double x_max, double y_min, double y_max, double z_min,
                   double z_max, double t_min, double t_max)
{
    assert(t->ndims >= 4);

    RTreeInitRect(r, t);
    r->boundary[0] = (RectReal)x_min;
    r->boundary[t->ndims_alloc] = (RectReal)x_max;
    r->boundary[1] = (RectReal)y_min;
    r->boundary[1 + t->ndims_alloc] = (RectReal)y_max;
    r->boundary[2] = (RectReal)z_min;
    r->boundary[2 + t->ndims_alloc] = (RectReal)z_max;
    r->boundary[3] = (RectReal)t_min;
    r->boundary[3 + t->ndims_alloc] = (RectReal)t_max;
}
/*-----------------------------------------------------------------------------
| Return a rect whose first low side is higher than its opposite side -
| interpreted as an undefined rect.
-----------------------------------------------------------------------------*/
void RTreeNullRect(struct RTree_Rect *r, struct RTree *t)
{
    register int i;

    /* assert(r); */

    r->boundary[0] = (RectReal) 1;
    r->boundary[t->nsides_alloc - 1] = (RectReal) - 1;
    for (i = 1; i < t->ndims_alloc; i++)
	r->boundary[i] = r->boundary[i + t->ndims_alloc] = (RectReal) 0;

    return;
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
void RTreePrintRect(struct RTree_Rect *R, int depth, struct RTree *t)
{
    register struct RTree_Rect *r = R;
    register int i;

    assert(r);

    RTreeTabIn(depth);
    fprintf(stdout, "rect:\n");
    for (i = 0; i < t->ndims_alloc; i++) {
	RTreeTabIn(depth + 1);
	fprintf(stdout, "%f\t%f\n", r->boundary[i], r->boundary[i + t->ndims_alloc]);
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

    /* assert(r); */

    if (Undefined(r, t))
	return (RectReal) 0;

    for (i = 0; i < t->ndims; i++)
	volume *= r->boundary[i + t->ndims_alloc] - r->boundary[i];
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
static const double UnitSphereVolume = sphere_volume(20);

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

    /* assert(r); */

    if (Undefined(r, t))
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
    double sum_of_squares = 0, extent;

    /* assert(r); */

    if (Undefined(r, t))
	return (RectReal) 0;

    for (i = 0; i < t->ndims; i++) {
	extent = (r->boundary[i + t->ndims_alloc] - r->boundary[i]);

	/* extent should be half extent : /4 */
	sum_of_squares += extent * extent / 4.;
    }

    return (RectReal) (pow(sqrt(sum_of_squares), t->ndims) * UnitSphereVolumes[t->ndims]);
}


/*-----------------------------------------------------------------------------
| Calculate the n-dimensional surface area of a rectangle
-----------------------------------------------------------------------------*/
RectReal RTreeRectSurfaceArea(struct RTree_Rect *r, struct RTree *t)
{
    int i, j;
    RectReal face_area, sum = (RectReal) 0;

    /*assert(r); */

    if (Undefined(r, t))
	return (RectReal) 0;

    for (i = 0; i < t->ndims; i++) {
	face_area = (RectReal) 1;

	for (j = 0; j < t->ndims; j++)
	    /* exclude i extent from product in this dimension */
	    if (i != j) {
		face_area *= (r->boundary[j + t->ndims_alloc] - r->boundary[j]);
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

    /* assert(r); */

    for (i = 0; i < t->ndims; i++) {
	margin += r->boundary[i + t->ndims_alloc] - r->boundary[i];
    }

    return margin;
}


/*-----------------------------------------------------------------------------
| Combine two rectangles, make one that includes both.
-----------------------------------------------------------------------------*/
void RTreeCombineRect(struct RTree_Rect *r1, struct RTree_Rect *r2,
		      struct RTree_Rect *r3, struct RTree *t)
{
    int i, j;

    /* assert(r1 && r2 && r3); */

    if (Undefined(r1, t)) {
	for (i = 0; i < t->nsides_alloc; i++)
	    r3->boundary[i] = r2->boundary[i];

	return;
    }

    if (Undefined(r2, t)) {
	for (i = 0; i < t->nsides_alloc; i++)
	    r3->boundary[i] = r1->boundary[i];

	return;
    }

    for (i = 0; i < t->ndims; i++) {
	r3->boundary[i] = MIN(r1->boundary[i], r2->boundary[i]);
	j = i + t->ndims_alloc;
	r3->boundary[j] = MAX(r1->boundary[j], r2->boundary[j]);
    }
    for (i = t->ndims; i < t->ndims_alloc; i++) {
	r3->boundary[i] = 0;
	j = i + t->ndims_alloc;
	r3->boundary[j] = 0;
    }
}


/*-----------------------------------------------------------------------------
| Expand first rectangle to cover second rectangle.
-----------------------------------------------------------------------------*/
int RTreeExpandRect(struct RTree_Rect *r1, struct RTree_Rect *r2,
		     struct RTree *t)
{
    int i, j, ret = 0;

    /* assert(r1 && r2); */

    if (Undefined(r2, t))
	return ret;

    for (i = 0; i < t->ndims; i++) {
	if (r1->boundary[i] > r2->boundary[i]) {
	    r1->boundary[i] = r2->boundary[i];
	    ret = 1;
	}
	j = i + t->ndims_alloc;
	if (r1->boundary[j] < r2->boundary[j]) {
	    r1->boundary[j] = r2->boundary[j];
	    ret = 1;
	}
    }

    for (i = t->ndims; i < t->ndims_alloc; i++) {
	r1->boundary[i] = 0;
	j = i + t->ndims_alloc;
	r1->boundary[j] = 0;
    }
    
    return ret;
}


/*-----------------------------------------------------------------------------
| Decide whether two rectangles are identical.
-----------------------------------------------------------------------------*/
int RTreeCompareRect(struct RTree_Rect *r, struct RTree_Rect *s, struct RTree *t)
{
    register int i, j;

    /* assert(r && s); */

    for (i = 0; i < t->ndims; i++) {
	j = i + t->ndims_alloc;	/* index for high sides */
	if (r->boundary[i] != s->boundary[i] ||
	    r->boundary[j] != s->boundary[j]) {
	    return 0;
	}
    }
    return 1;
}


/*-----------------------------------------------------------------------------
| Decide whether two rectangles overlap or touch.
-----------------------------------------------------------------------------*/
int RTreeOverlap(struct RTree_Rect *r, struct RTree_Rect *s, struct RTree *t)
{
    register int i, j;

    /* assert(r && s); */

    for (i = 0; i < t->ndims; i++) {
	j = i + t->ndims_alloc;	/* index for high sides */
	if (r->boundary[i] > s->boundary[j] ||
	    s->boundary[i] > r->boundary[j]) {
	    return FALSE;
	}
    }
    return TRUE;
}


/*-----------------------------------------------------------------------------
| Decide whether rectangle s is contained in rectangle r.
-----------------------------------------------------------------------------*/
int RTreeContained(struct RTree_Rect *r, struct RTree_Rect *s, struct RTree *t)
{
    register int i, j;

    /* assert(r && s); */

    /* undefined rect is contained in any other */
    if (Undefined(r, t))
	return TRUE;

    /* no rect (except an undefined one) is contained in an undef rect */
    if (Undefined(s, t))
	return FALSE;

    for (i = 0; i < t->ndims; i++) {
	j = i + t->ndims_alloc;	/* index for high sides */
	if (s->boundary[i] < r->boundary[i] ||
	    s->boundary[j] > r->boundary[j])
	    return FALSE;
    }
    return TRUE;
}


/*-----------------------------------------------------------------------------
| Decide whether rectangle s fully contains rectangle r.
-----------------------------------------------------------------------------*/
int RTreeContains(struct RTree_Rect *r, struct RTree_Rect *s, struct RTree *t)
{
    register int i, j;

    /* assert(r && s); */

    /* undefined rect is contained in any other */
    if (Undefined(r, t))
	return TRUE;

    /* no rect (except an undefined one) is contained in an undef rect */
    if (Undefined(s, t))
	return FALSE;

    for (i = 0; i < t->ndims; i++) {
	j = i + t->ndims_alloc;	/* index for high sides */
	if (s->boundary[i] > r->boundary[i] ||
	    s->boundary[j] < r->boundary[j])
	    return FALSE;
    }
    return TRUE;
}
