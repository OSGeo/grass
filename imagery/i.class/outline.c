#define OUTLINE

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"
#include "local_proto.h"


#define V     Region.vertex	/* converted vertex list */
#define VN    Region.vertex_npoints

#define P     Region.perimeter	/* perimeter point list */
#define PN    Region.perimeter_npoints

#define A     Region.point
#define AN    Region.npoints

#define extrema(x,y,z) (((x<y)&&(z<y))||((x>y)&&(z>y)))
#define non_extrema(x,y,z) (((x<y)&&(y<z))||((x>y)&&(y>z)))

int outline(void)
{
    int first;
    int skip;			/* boolean */
    int np;			/* perimeter estimate */
    POINT tmp[MAX_VERTEX];	/* temp vertex list   */
    int cur;
    int prev;
    int next;
    double tmp_n, tmp_e;
    POINT temp;


    /* convert alarm area points to data row/col verticies */
    Menu_msg("Preparing outline...");

    for (cur = 0; cur < AN; cur++) {
	temp.y = view_to_row(Region.view, A[cur].y);
	temp.x = view_to_col(Region.view, A[cur].x);
	tmp_n = row_to_northing(&(Region.view->cell.head), temp.y, 0.5);
	tmp_e = col_to_easting(&(Region.view->cell.head), temp.x, 0.5);
	tmp[cur].y = G_northing_to_row(tmp_n, &Band_cellhd);
	tmp[cur].x = G_easting_to_col(tmp_e, &Band_cellhd);
    }

    /* find first edge which is not horizontal */

    first = -1;
    prev = AN - 1;
    for (cur = 0; cur < AN; prev = cur++)
	if (tmp[cur].y != tmp[prev].y) {
	    first = cur;
	    break;
	}
    if (first < 0) {
	G_warning(_("Absurd polygon."));
	return (0);
    }

    /* copy tmp to vertex list collapsing adjacent horizontal edges */

    skip = 0;
    VN = 0;
    cur = first;		/* stmt not necssary */

    do {
	if (signalflag.interrupt)
	    return (0);
	if (!skip) {
	    V[VN].x = tmp[cur].x;
	    V[VN].y = tmp[cur].y;
	    VN++;
	}

	prev = cur++;
	if (cur >= AN)
	    cur = 0;
	if ((next = cur + 1) >= AN)
	    next = 0;

	skip = ((tmp[prev].y == tmp[cur].y) && (tmp[next].y == tmp[cur].y));
    }
    while (cur != first);


    /* count points on the perimeter */

    np = 0;
    prev = VN - 1;
    for (cur = 0; cur < VN; prev = cur++)
	np += abs(V[prev].y - V[cur].y);


    /* allocate perimeter list */

    P = (POINT *) G_malloc(np * sizeof(POINT));
    if (!P) {
	G_warning(_("Outlined area is too large."));
	return (0);
    }

    /* store the perimeter points */

    PN = 0;
    prev = VN - 1;
    for (cur = 0; cur < VN; prev = cur++) {
	if (signalflag.interrupt)
	    return (0);
	edge(V[prev].x, V[prev].y, V[cur].x, V[cur].y);
    }

    /*
     * now decide which verticies should be included
     *    local extrema are excluded
     *    local non-extrema are included
     *    verticies of horizontal edges which are pseudo-extrema
     *      are excluded.
     *    one vertex of horizontal edges which are pseudo-non-extrema
     *      are included.
     */


    prev = VN - 1;
    cur = 0;
    do {
	if (signalflag.interrupt)
	    return (0);
	next = cur + 1;
	if (next >= VN)
	    next = 0;

	if (extrema(V[prev].y, V[cur].y, V[next].y))
	    skip = 1;
	else if (non_extrema(V[prev].y, V[cur].y, V[next].y))
	    skip = 0;
	else {
	    skip = 0;
	    if (++next >= VN)
		next = 0;
	    if (extrema(V[prev].y, V[cur].y, V[next].y))
		skip = 1;
	}

	if (!skip)
	    edge_point(V[cur].x, V[cur].y);

	cur = next;
	prev = cur - 1;
    }
    while (cur != 0);


    /* sort the edge points by row and then by col */

    Menu_msg("Sorting...");
    qsort(P, (size_t) PN, sizeof(POINT), edge_order);

    Menu_msg("");
    return (1);
}
