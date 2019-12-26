/*!
   \file lib/ogsf/gsdrape.c

   \brief OGSF library - functions to intersect line segments with edges of surface polygons

   GRASS OpenGL gsurf OGSF Library 

   For efficiency, intersections are found without respect to which
   specific triangle edge is intersected, but on a broader sense with
   the horizontal, vertical, and diagonal seams in the grid, then
   the intersections are ordered.  If quadstrips are used for drawing 
   rather than tmesh, triangulation is not consistent; for diagonal
   intersections, the proper diagonal to intersect would need to be
   determined according to the algorithm used by qstrip (look at nearby
   normals). It may be faster to go ahead and find the intersections 
   with the other diagonals using the same methods, then at sorting 
   time determine which diagonal array to look at for each quad.  
   It would also require a mechanism for throwing out unused intersections
   with the diagonals during the ordering phase.
   Do intersections in 2D, fill line structure with 3D pts (maybe calling
   routine will cache for redrawing).  Get Z value by using linear interp 
   between corners.

   - check for easy cases:
   - single point
   - colinear with horizontal or vertical edges
   - colinear with diagonal edges of triangles
   - calculate three arrays of ordered intersections:
   - with vertical edges
   - with horizontal edges
   - with diagonal edges and interpolate Z, using simple linear interpolation.
   - eliminate duplicate intersections (need only compare one coord for each)
   - build ordered set of points.

   Return static pointer to 3D set of points.  Max number of intersections
   will be rows + cols + diags, so should allocate this number to initialize.
   Let calling routine worry about copying points for caching.

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown UI GMS Lab
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/ogsf.h>
#include <grass/glocale.h>

#include "gsget.h"
#include "rowcol.h"
#include "math.h"

#define	DONT_INTERSECT    0
#define	DO_INTERSECT      1
#define COLLINEAR         2

#define LERP(a,l,h)      ((l)+(((h)-(l))*(a)))
#define EQUAL(a,b)       (fabs((a)-(b))<EPSILON)
#define ISNODE(p,res)    (fmod((double)p,(double)res)<EPSILON)

#define SAME_SIGNS( a, b ) \
    ((a >= 0 && b >= 0) || (a < 0 && b < 0))

static int drape_line_init(int, int);
static Point3 *_gsdrape_get_segments(geosurf *, float *, float *, int *);
static float dist_squared_2d(float *, float *);

/* array of points to be returned */
static Point3 *I3d;

/* make dependent on resolution? */
static float EPSILON = 0.000001;

/*vertical, horizontal, & diagonal intersections */
static Point3 *Vi, *Hi, *Di;

static typbuff *Ebuf;		/* elevation buffer */
static int Flat;


/*!
   \brief Initizalize

   \param rows number of rows
   \param cols number of columns

   \return -1 on failure
   \return 1 on success
 */
static int drape_line_init(int rows, int cols)
{
    /* use G_calloc() [-> G_fatal_error] instead of calloc ? */
    if (NULL == (I3d = (Point3 *) calloc(2 * (rows + cols), sizeof(Point3)))) {
	return (-1);
    }

    if (NULL == (Vi = (Point3 *) calloc(cols, sizeof(Point3)))) {
	G_free(I3d);

	return (-1);
    }

    if (NULL == (Hi = (Point3 *) calloc(rows, sizeof(Point3)))) {
	G_free(I3d);
	G_free(Vi);

	return (-1);
    }

    if (NULL == (Di = (Point3 *) calloc(rows + cols, sizeof(Point3)))) {
	G_free(I3d);
	G_free(Vi);
	G_free(Hi);

	return (-1);
    }

    return (1);
}

/*!
   \brief Get segments

   \param gs surface (geosurf)
   \param bgn begin point
   \param end end point
   \param num

   \return pointer to Point3 struct
 */
static Point3 *_gsdrape_get_segments(geosurf * gs, float *bgn, float *end,
				     int *num)
{
    float f[3], l[3];
    int vi, hi, di;
    float dir[2], yres, xres;

    xres = VXRES(gs);
    yres = VYRES(gs);

    vi = hi = di = 0;
    GS_v2dir(bgn, end, dir);

    if (dir[X]) {
	vi = get_vert_intersects(gs, bgn, end, dir);
    }

    if (dir[Y]) {
	hi = get_horz_intersects(gs, bgn, end, dir);
    }

    if (!((end[Y] - bgn[Y]) / (end[X] - bgn[X]) == yres / xres)) {
	di = get_diag_intersects(gs, bgn, end, dir);
    }

    interp_first_last(gs, bgn, end, f, l);

    *num = order_intersects(gs, f, l, vi, hi, di);
    /* fills in return values, eliminates dupes (corners) */

    G_debug(5, "_gsdrape_get_segments vi=%d, hi=%d, di=%d, num=%d",
	    vi, hi, di, *num);

    return (I3d);
}

/*!
   \brief Calculate 2D distance

   \param p1 first point
   \param p2 second point

   \return distance
 */
static float dist_squared_2d(float *p1, float *p2)
{
    float dx, dy;

    dx = p2[X] - p1[X];
    dy = p2[Y] - p1[Y];

    return (dx * dx + dy * dy);
}

/*!
   \brief ADD

   \param gs surface (geosurf)

   \return -1 on failure
   \return 1 on success
 */
int gsdrape_set_surface(geosurf * gs)
{
    static int first = 1;

    if (first) {
	first = 0;

	if (0 > drape_line_init(gs->rows, gs->cols)) {
	    G_warning(_("Unable to process vector map - out of memory"));
	    Ebuf = NULL;

	    return (-1);
	}
    }

    Ebuf = gs_get_att_typbuff(gs, ATT_TOPO, 0);

    return (1);
}

/*!
   \brief Check if segment intersect vector region

   Clipping performed:
   - bgn and end are replaced so that both points are within viewregion
   - if seg intersects  

   \param gs surface (geosurf)
   \param bgn begin point
   \param end end point

   \return 0 if segment doesn't intersect the viewregion, or intersects only at corner
   \return otherwise returns 1
 */
int seg_intersect_vregion(geosurf * gs, float *bgn, float *end)
{
    float *replace, xl, yb, xr, yt, xi, yi;
    int inside = 0;

    xl = 0.0;
    xr = VCOL2X(gs, VCOLS(gs));
    yt = VROW2Y(gs, 0);
    yb = VROW2Y(gs, VROWS(gs));

    if (in_vregion(gs, bgn)) {
	replace = end;
	inside++;
    }

    if (in_vregion(gs, end)) {
	replace = bgn;
	inside++;
    }

    if (inside == 2) {
	return (1);
    }
    else if (inside) {
	/* one in & one out - replace gets first intersection */
	if (segs_intersect
	    (bgn[X], bgn[Y], end[X], end[Y], xl, yb, xl, yt, &xi, &yi)) {
	    /* left */
	}
	else if (segs_intersect
		 (bgn[X], bgn[Y], end[X], end[Y], xr, yb, xr, yt, &xi, &yi)) {
	    /* right */
	}
	else if (segs_intersect
		 (bgn[X], bgn[Y], end[X], end[Y], xl, yb, xr, yb, &xi, &yi)) {
	    /* bottom */
	}
	else if (segs_intersect
		 (bgn[X], bgn[Y], end[X], end[Y], xl, yt, xr, yt, &xi, &yi)) {
	    /* top */
	}

	replace[X] = xi;
	replace[Y] = yi;
    }
    else {
	/* both out - find 2 intersects & replace both */
	float pt1[2], pt2[2];

	replace = pt1;
	if (segs_intersect
	    (bgn[X], bgn[Y], end[X], end[Y], xl, yb, xl, yt, &xi, &yi)) {
	    replace[X] = xi;
	    replace[Y] = yi;
	    replace = pt2;
	    inside++;
	}

	if (segs_intersect
	    (bgn[X], bgn[Y], end[X], end[Y], xr, yb, xr, yt, &xi, &yi)) {
	    replace[X] = xi;
	    replace[Y] = yi;
	    replace = pt2;
	    inside++;
	}

	if (inside < 2) {
	    if (segs_intersect
		(bgn[X], bgn[Y], end[X], end[Y], xl, yb, xr, yb, &xi, &yi)) {
		replace[X] = xi;
		replace[Y] = yi;
		replace = pt2;
		inside++;
	    }
	}

	if (inside < 2) {
	    if (segs_intersect
		(bgn[X], bgn[Y], end[X], end[Y], xl, yt, xr, yt, &xi, &yi)) {
		replace[X] = xi;
		replace[Y] = yi;
		inside++;
	    }
	}

	if (inside < 2) {
	    return (0);		/* no intersect or only 1 point on corner */
	}

	/* compare dist of intersects to bgn - closest replaces bgn */
	if (GS_P2distance(bgn, pt1) < GS_P2distance(bgn, pt2)) {
	    bgn[X] = pt1[X];
	    bgn[Y] = pt1[Y];
	    end[X] = pt2[X];
	    end[Y] = pt2[Y];
	}
	else {
	    bgn[X] = pt2[X];
	    bgn[Y] = pt2[Y];
	    end[X] = pt1[X];
	    end[Y] = pt1[Y];
	}
    }

    return (1);
}

/*!
   \brief ADD

   \param gs surface (geosurf)
   \param bgn begin point (x,y)
   \param end end point (x,y)
   \param num

   \return pointer to Point3 struct
 */
Point3 *gsdrape_get_segments(geosurf * gs, float *bgn, float *end, int *num)
{
    gsdrape_set_surface(gs);

    if (!seg_intersect_vregion(gs, bgn, end)) {
	*num = 0;

	return (NULL);
    }

    if (CONST_ATT == gs_get_att_src(gs, ATT_TOPO)) {
	/* will probably want a force_drape option to get all intersects */
	I3d[0][X] = bgn[X];
	I3d[0][Y] = bgn[Y];
	I3d[0][Z] = gs->att[ATT_TOPO].constant;
	I3d[1][X] = end[X];
	I3d[1][Y] = end[Y];
	I3d[1][Z] = gs->att[ATT_TOPO].constant;
	*num = 2;

	return (I3d);
    }

    if (bgn[X] == end[X] && bgn[Y] == end[Y]) {
	float f[3], l[3];

	interp_first_last(gs, bgn, end, f, l);
	GS_v3eq(I3d[0], f);
	GS_v3eq(I3d[1], l);

	/* CHANGE (*num = 1) to reflect degenerate line ? */
	*num = 2;

	return (I3d);
    }

    Flat = 0;
    return (_gsdrape_get_segments(gs, bgn, end, num));
}


/*!
   \brief Get all segments

   \param gs surface (geosurf)
   \param bgn begin point
   \param end end point
   \param num

   \return pointer to Point3 struct
 */
Point3 *gsdrape_get_allsegments(geosurf * gs, float *bgn, float *end,
				int *num)
{
    gsdrape_set_surface(gs);

    if (!seg_intersect_vregion(gs, bgn, end)) {
	*num = 0;
	return (NULL);
    }

    if (bgn[X] == end[X] && bgn[Y] == end[Y]) {
	float f[3], l[3];

	interp_first_last(gs, bgn, end, f, l);
	GS_v3eq(I3d[0], f);
	GS_v3eq(I3d[1], l);
	*num = 2;

	return (I3d);
    }

    if (CONST_ATT == gs_get_att_src(gs, ATT_TOPO)) {
	Flat = 1;
    }
    else {
	Flat = 0;
    }

    return (_gsdrape_get_segments(gs, bgn, end, num));
}

/*!
   \brief ADD

   \param gs surface (geosurf)
   \param bgn begin point
   \param end end point
   \param f first
   \param l last
 */
void interp_first_last(geosurf * gs, float *bgn, float *end, Point3 f,
		       Point3 l)
{
    f[X] = bgn[X];
    f[Y] = bgn[Y];

    l[X] = end[X];
    l[Y] = end[Y];

    if (Flat) {
	f[Z] = l[Z] = gs->att[ATT_TOPO].constant;
    }
    else {
	viewcell_tri_interp(gs, Ebuf, f, 0);
	viewcell_tri_interp(gs, Ebuf, l, 0);
    }

    return;
}

/*!
   \brief ADD

   \param gs surface (geosurf)
   \param pt
 */
int _viewcell_tri_interp(geosurf * gs, Point3 pt)
{
    typbuff *buf;

    buf = gs_get_att_typbuff(gs, ATT_TOPO, 0);

    return (viewcell_tri_interp(gs, buf, pt, 0));
}

/*!
   \brief ADD

   In gsd_surf, tmesh draws polys like so:
   <pre>
   --------------
   |           /|
   |          / |          
   |         /  |         
   |        /   |        
   |       /    |       
   |      /     |      
   |     /      |     
   |    /       |    
   |   /        |   
   |  /         |  
   | /          | 
   |/           |
   --------------
   </pre>

   UNLESS the top right or bottom left point is masked, in which case a
   single triangle with the opposite diagonal is drawn.  This case is
   not yet handled here & should only occur on edges. 
   pt has X & Y coordinates in it, we interpolate Z here

   This could probably be much shorter, but not much faster.   

   \return 1 if point is in view region
   \return otherwise 0 (if masked)
 */
int viewcell_tri_interp(geosurf * gs, typbuff * buf, Point3 pt,
			int check_mask)
{
    Point3 p1, p2, p3;
    int offset, drow, dcol, vrow, vcol;
    float xmax, ymin, ymax, alpha;

    xmax = VCOL2X(gs, VCOLS(gs));
    ymax = VROW2Y(gs, 0);
    ymin = VROW2Y(gs, VROWS(gs));

    if (check_mask) {
	if (gs_point_is_masked(gs, pt)) {
	    return (0);
	}
    }

    if (pt[X] < 0.0 || pt[Y] > ymax) {
	/* outside on left or top */
	return (0);
    }

    if (pt[Y] < ymin || pt[X] > xmax) {
	/* outside on bottom or right */
	return (0);
    }

    if (CONST_ATT == gs_get_att_src(gs, ATT_TOPO)) {
	pt[Z] = gs->att[ATT_TOPO].constant;

	return (1);
    }
    else if (MAP_ATT != gs_get_att_src(gs, ATT_TOPO)) {
	return (0);
    }

    vrow = Y2VROW(gs, pt[Y]);
    vcol = X2VCOL(gs, pt[X]);

    if (vrow < VROWS(gs) && vcol < VCOLS(gs)) {
	/*not on bottom or right edge */
	if (pt[X] > 0.0 && pt[Y] < ymax) {
	    /* not on left or top edge */
	    p1[X] = VCOL2X(gs, vcol + 1);
	    p1[Y] = VROW2Y(gs, vrow);
	    drow = VROW2DROW(gs, vrow);
	    dcol = VCOL2DCOL(gs, vcol + 1);
	    offset = DRC2OFF(gs, drow, dcol);
	    GET_MAPATT(buf, offset, p1[Z]);	/* top right */

	    p2[X] = VCOL2X(gs, vcol);
	    p2[Y] = VROW2Y(gs, vrow + 1);
	    drow = VROW2DROW(gs, vrow + 1);
	    dcol = VCOL2DCOL(gs, vcol);
	    offset = DRC2OFF(gs, drow, dcol);
	    GET_MAPATT(buf, offset, p2[Z]);	/* bottom left */

	    if ((pt[X] - p2[X]) / VXRES(gs) > (pt[Y] - p2[Y]) / VYRES(gs)) {
		/* lower triangle */
		p3[X] = VCOL2X(gs, vcol + 1);
		p3[Y] = VROW2Y(gs, vrow + 1);
		drow = VROW2DROW(gs, vrow + 1);
		dcol = VCOL2DCOL(gs, vcol + 1);
		offset = DRC2OFF(gs, drow, dcol);
		GET_MAPATT(buf, offset, p3[Z]);	/* bottom right */
	    }
	    else {
		/* upper triangle */
		p3[X] = VCOL2X(gs, vcol);
		p3[Y] = VROW2Y(gs, vrow);
		drow = VROW2DROW(gs, vrow);
		dcol = VCOL2DCOL(gs, vcol);
		offset = DRC2OFF(gs, drow, dcol);
		GET_MAPATT(buf, offset, p3[Z]);	/* top left */
	    }

	    return (Point_on_plane(p1, p2, p3, pt));
	}
	else if (pt[X] == 0.0) {
	    /* on left edge */
	    if (pt[Y] < ymax) {
		vrow = Y2VROW(gs, pt[Y]);
		drow = VROW2DROW(gs, vrow);
		offset = DRC2OFF(gs, drow, 0);
		GET_MAPATT(buf, offset, p1[Z]);

		drow = VROW2DROW(gs, vrow + 1);
		offset = DRC2OFF(gs, drow, 0);
		GET_MAPATT(buf, offset, p2[Z]);

		alpha = (VROW2Y(gs, vrow) - pt[Y]) / VYRES(gs);
		pt[Z] = LERP(alpha, p1[Z], p2[Z]);
	    }
	    else {
		/* top left corner */
		GET_MAPATT(buf, 0, pt[Z]);
	    }

	    return (1);
	}
	else if (pt[Y] == gs->yrange) {
	    /* on top edge, not a corner */
	    vcol = X2VCOL(gs, pt[X]);
	    dcol = VCOL2DCOL(gs, vcol);
	    GET_MAPATT(buf, dcol, p1[Z]);

	    dcol = VCOL2DCOL(gs, vcol + 1);
	    GET_MAPATT(buf, dcol, p2[Z]);

	    alpha = (pt[X] - VCOL2X(gs, vcol)) / VXRES(gs);
	    pt[Z] = LERP(alpha, p1[Z], p2[Z]);

	    return (1);
	}
    }
    else if (vrow == VROWS(gs)) {
	/* on bottom edge */
	drow = VROW2DROW(gs, VROWS(gs));

	if (pt[X] > 0.0 && pt[X] < xmax) {
	    /* not a corner */
	    vcol = X2VCOL(gs, pt[X]);
	    dcol = VCOL2DCOL(gs, vcol);
	    offset = DRC2OFF(gs, drow, dcol);
	    GET_MAPATT(buf, offset, p1[Z]);

	    dcol = VCOL2DCOL(gs, vcol + 1);
	    offset = DRC2OFF(gs, drow, dcol);
	    GET_MAPATT(buf, offset, p2[Z]);

	    alpha = (pt[X] - VCOL2X(gs, vcol)) / VXRES(gs);
	    pt[Z] = LERP(alpha, p1[Z], p2[Z]);

	    return (1);
	}
	else if (pt[X] == 0.0) {
	    /* bottom left corner */
	    offset = DRC2OFF(gs, drow, 0);
	    GET_MAPATT(buf, offset, pt[Z]);

	    return (1);
	}
	else {
	    /* bottom right corner */
	    dcol = VCOL2DCOL(gs, VCOLS(gs));
	    offset = DRC2OFF(gs, drow, dcol);
	    GET_MAPATT(buf, offset, pt[Z]);

	    return (1);
	}
    }
    else {
	/* on right edge, not bottom corner */
	dcol = VCOL2DCOL(gs, VCOLS(gs));

	if (pt[Y] < ymax) {
	    vrow = Y2VROW(gs, pt[Y]);
	    drow = VROW2DROW(gs, vrow);
	    offset = DRC2OFF(gs, drow, dcol);
	    GET_MAPATT(buf, offset, p1[Z]);

	    drow = VROW2DROW(gs, vrow + 1);
	    offset = DRC2OFF(gs, drow, dcol);
	    GET_MAPATT(buf, offset, p2[Z]);

	    alpha = (VROW2Y(gs, vrow) - pt[Y]) / VYRES(gs);
	    pt[Z] = LERP(alpha, p1[Z], p2[Z]);

	    return (1);
	}
	else {
	    /* top right corner */
	    GET_MAPATT(buf, dcol, pt[Z]);

	    return (1);
	}
    }

    return (0);
}

/*!
   \brief ADD

   \param gs surface (geosurf)

   \return 1 
   \return 0 
 */
int in_vregion(geosurf * gs, float *pt)
{
    if (pt[X] >= 0.0 && pt[Y] <= gs->yrange) {
	if (pt[X] <= VCOL2X(gs, VCOLS(gs))) {
	    return (pt[Y] >= VROW2Y(gs, VROWS(gs)));
	}
    }

    return (0);
}

/*!
   \brief ADD

   After all the intersections between the segment and triangle
   edges have been found, they are in three lists.  (intersections
   with vertical, horizontal, and diagonal triangle edges)

   Each list is ordered in space from first to last segment points, 
   but now the lists need to be woven together.  This routine 
   starts with the first point of the segment and then checks the
   next point in each list to find the closest, eliminating duplicates
   along the way and storing the result in I3d.

   \param gs surface (geosurf)
   \param first first point
   \param last last point
   \param vi
   \param hi
   \param di

   \return
 */
int order_intersects(geosurf * gs, Point3 first, Point3 last, int vi, int hi,
		     int di)
{
    int num, i, found, cv, ch, cd, cnum;
    float dv, dh, dd, big, cpoint[2];

    cv = ch = cd = cnum = 0;
    num = vi + hi + di;

    cpoint[X] = first[X];
    cpoint[Y] = first[Y];

    if (in_vregion(gs, first)) {
	I3d[cnum][X] = first[X];
	I3d[cnum][Y] = first[Y];
	I3d[cnum][Z] = first[Z];
	cnum++;
    }

    /* TODO: big could still be less than first dist */
    big = gs->yrange * gs->yrange + gs->xrange * gs->xrange;	/*BIG distance */
    dv = dh = dd = big;

    for (i = 0; i < num; i = cv + ch + cd) {
	if (cv < vi) {
	    dv = dist_squared_2d(Vi[cv], cpoint);

	    if (dv < EPSILON) {
		cv++;
		continue;
	    }
	}
	else {
	    dv = big;
	}

	if (ch < hi) {
	    dh = dist_squared_2d(Hi[ch], cpoint);

	    if (dh < EPSILON) {
		ch++;
		continue;
	    }
	}
	else {
	    dh = big;
	}

	if (cd < di) {
	    dd = dist_squared_2d(Di[cd], cpoint);

	    if (dd < EPSILON) {
		cd++;
		continue;
	    }
	}
	else {
	    dd = big;
	}

	found = 0;

	if (cd < di) {
	    if (dd <= dv && dd <= dh) {
		found = 1;
		cpoint[X] = I3d[cnum][X] = Di[cd][X];
		cpoint[Y] = I3d[cnum][Y] = Di[cd][Y];
		I3d[cnum][Z] = Di[cd][Z];
		cnum++;

		if (EQUAL(dd, dv)) {
		    cv++;
		}

		if (EQUAL(dd, dh)) {
		    ch++;
		}

		cd++;
	    }
	}

	if (!found) {
	    if (cv < vi) {
		if (dv <= dh) {
		    found = 1;
		    cpoint[X] = I3d[cnum][X] = Vi[cv][X];
		    cpoint[Y] = I3d[cnum][Y] = Vi[cv][Y];
		    I3d[cnum][Z] = Vi[cv][Z];
		    cnum++;

		    if (EQUAL(dv, dh)) {
			ch++;
		    }

		    cv++;
		}
	    }
	}

	if (!found) {
	    if (ch < hi) {
		cpoint[X] = I3d[cnum][X] = Hi[ch][X];
		cpoint[Y] = I3d[cnum][Y] = Hi[ch][Y];
		I3d[cnum][Z] = Hi[ch][Z];
		cnum++;
		ch++;
	    }
	}

	if (i == cv + ch + cd) {
	    G_debug(5, "order_intersects(): stuck on %d", cnum);
	    G_debug(5, "order_intersects(): cv = %d, ch = %d, cd = %d", cv,
		    ch, cd);
	    G_debug(5, "order_intersects(): dv = %f, dh = %f, dd = %f", dv,
		    dh, dd);

	    break;
	}
    }

    if (EQUAL(last[X], cpoint[X]) && EQUAL(last[Y], cpoint[Y])) {
	return (cnum);
    }

    if (in_vregion(gs, last)) {
	/* TODO: check for last point on corner ? */
	I3d[cnum][X] = last[X];
	I3d[cnum][Y] = last[Y];
	I3d[cnum][Z] = last[Z];
	++cnum;
    }

    return (cnum);
}

/*!
   \brief ADD

   \todo For consistancy, need to decide how last row & last column are
   displayed - would it look funny to always draw last row/col with
   finer resolution if necessary, or would it be better to only show
   full rows/cols?

   Colinear already eliminated

   \param gs surface (geosurf)
   \param bgn begin point
   \param end end point
   \param dir direction

   \return
 */
int get_vert_intersects(geosurf * gs, float *bgn, float *end, float *dir)
{
    int fcol, lcol, incr, hits, num, offset, drow1, drow2;
    float xl, yb, xr, yt, z1, z2, alpha;
    float xres, yres, xi, yi;
    int bgncol, endcol, cols, rows;

    xres = VXRES(gs);
    yres = VYRES(gs);
    cols = VCOLS(gs);
    rows = VROWS(gs);

    bgncol = X2VCOL(gs, bgn[X]);
    endcol = X2VCOL(gs, end[X]);

    if (bgncol > cols && endcol > cols) {
	return 0;
    }

    if (bgncol == endcol) {
	return 0;
    }

    fcol = dir[X] > 0 ? bgncol + 1 : bgncol;
    lcol = dir[X] > 0 ? endcol : endcol + 1;

    /* assuming only showing FULL cols */
    incr = lcol - fcol > 0 ? 1 : -1;

    while (fcol > cols || fcol < 0) {
	fcol += incr;
    }

    while (lcol > cols || lcol < 0) {
	lcol -= incr;
    }

    num = abs(lcol - fcol) + 1;

    yb = gs->yrange - (yres * rows) - EPSILON;
    yt = gs->yrange + EPSILON;

    for (hits = 0; hits < num; hits++) {
	xl = xr = VCOL2X(gs, fcol);

	if (segs_intersect(bgn[X], bgn[Y], end[X], end[Y], xl, yt, xr, yb,
			   &xi, &yi)) {
	    Vi[hits][X] = xi;
	    Vi[hits][Y] = yi;

	    /* find data rows */
	    if (Flat) {
		Vi[hits][Z] = gs->att[ATT_TOPO].constant;
	    }
	    else {
		drow1 = Y2VROW(gs, Vi[hits][Y]) * gs->y_mod;
		drow2 = (1 + Y2VROW(gs, Vi[hits][Y])) * gs->y_mod;

		if (drow2 >= gs->rows) {
		    drow2 = gs->rows - 1;	/*bottom edge */
		}

		alpha =
		    ((gs->yrange - drow1 * gs->yres) - Vi[hits][Y]) / yres;

		offset = DRC2OFF(gs, drow1, fcol * gs->x_mod);
		GET_MAPATT(Ebuf, offset, z1);
		offset = DRC2OFF(gs, drow2, fcol * gs->x_mod);
		GET_MAPATT(Ebuf, offset, z2);
		Vi[hits][Z] = LERP(alpha, z1, z2);
	    }
	}

	/* if they don't intersect, something's wrong! */
	/* should only happen on endpoint, so it will be added later */
	else {
	    hits--;
	    num--;
	}

	fcol += incr;
    }

    return (hits);
}

/*!
   \brief Get horizontal intersects

   \param gs surface (geosurf)
   \param bgn begin point
   \param end end point
   \param dir 

   \return number of intersects
 */
int get_horz_intersects(geosurf * gs, float *bgn, float *end, float *dir)
{
    int frow, lrow, incr, hits, num, offset, dcol1, dcol2;
    float xl, yb, xr, yt, z1, z2, alpha;
    float xres, yres, xi, yi;
    int bgnrow, endrow, rows, cols;

    xres = VXRES(gs);
    yres = VYRES(gs);
    cols = VCOLS(gs);
    rows = VROWS(gs);

    bgnrow = Y2VROW(gs, bgn[Y]);
    endrow = Y2VROW(gs, end[Y]);
    if (bgnrow == endrow) {
	return 0;
    }

    if (bgnrow > rows && endrow > rows) {
	return 0;
    }

    frow = dir[Y] > 0 ? bgnrow : bgnrow + 1;
    lrow = dir[Y] > 0 ? endrow + 1 : endrow;

    /* assuming only showing FULL rows */
    incr = lrow - frow > 0 ? 1 : -1;

    while (frow > rows || frow < 0) {
	frow += incr;
    }

    while (lrow > rows || lrow < 0) {
	lrow -= incr;
    }

    num = abs(lrow - frow) + 1;

    xl = 0.0 - EPSILON;
    xr = xres * cols + EPSILON;

    for (hits = 0; hits < num; hits++) {
	yb = yt = VROW2Y(gs, frow);

	if (segs_intersect(bgn[X], bgn[Y], end[X], end[Y], xl, yt, xr, yb,
			   &xi, &yi)) {
	    Hi[hits][X] = xi;
	    Hi[hits][Y] = yi;

	    /* find data cols */
	    if (Flat) {
		Hi[hits][Z] = gs->att[ATT_TOPO].constant;
	    }
	    else {
		dcol1 = X2VCOL(gs, Hi[hits][X]) * gs->x_mod;
		dcol2 = (1 + X2VCOL(gs, Hi[hits][X])) * gs->x_mod;

		if (dcol2 >= gs->cols) {
		    dcol2 = gs->cols - 1;	/* right edge */
		}

		alpha = (Hi[hits][X] - (dcol1 * gs->xres)) / xres;

		offset = DRC2OFF(gs, frow * gs->y_mod, dcol1);
		GET_MAPATT(Ebuf, offset, z1);
		offset = DRC2OFF(gs, frow * gs->y_mod, dcol2);
		GET_MAPATT(Ebuf, offset, z2);
		Hi[hits][Z] = LERP(alpha, z1, z2);
	    }
	}

	/* if they don't intersect, something's wrong! */
	/* should only happen on endpoint, so it will be added later */
	else {
	    hits--;
	    num--;
	}

	frow += incr;
    }

    return (hits);
}

/*!
   \brief Get diagonal intersects

   Colinear already eliminated

   \param gs surface (geosurf)
   \param bgn begin point
   \param end end point
   \param dir ? (unused)

   \return number of intersects
 */
int get_diag_intersects(geosurf * gs, float *bgn, float *end, float *dir)
{
    int fdig, ldig, incr, hits, num, offset;
    int vrow, vcol, drow1, drow2, dcol1, dcol2;
    float xl, yb, xr, yt, z1, z2, alpha;
    float xres, yres, xi, yi, dx, dy;
    int diags, cols, rows, lower;
    Point3 pt;

    xres = VXRES(gs);
    yres = VYRES(gs);
    cols = VCOLS(gs);
    rows = VROWS(gs);
    diags = rows + cols;	/* -1 ? */

    /* determine upper/lower triangle for last */
    vrow = Y2VROW(gs, end[Y]);
    vcol = X2VCOL(gs, end[X]);
    pt[X] = VCOL2X(gs, vcol);
    pt[Y] = VROW2Y(gs, vrow + 1);
    lower = ((end[X] - pt[X]) / xres > (end[Y] - pt[Y]) / yres);
    ldig = lower ? vrow + vcol + 1 : vrow + vcol;

    /* determine upper/lower triangle for first */
    vrow = Y2VROW(gs, bgn[Y]);
    vcol = X2VCOL(gs, bgn[X]);
    pt[X] = VCOL2X(gs, vcol);
    pt[Y] = VROW2Y(gs, vrow + 1);
    lower = ((bgn[X] - pt[X]) / xres > (bgn[Y] - pt[Y]) / yres);
    fdig = lower ? vrow + vcol + 1 : vrow + vcol;

    /* adjust according to direction */
    if (ldig > fdig) {
	fdig++;
    }

    if (fdig > ldig) {
	ldig++;
    }

    incr = ldig - fdig > 0 ? 1 : -1;

    while (fdig > diags || fdig < 0) {
	fdig += incr;
    }

    while (ldig > diags || ldig < 0) {
	ldig -= incr;
    }

    num = abs(ldig - fdig) + 1;

    for (hits = 0; hits < num; hits++) {
	yb = gs->yrange - (yres * (fdig < rows ? fdig : rows)) - EPSILON;
	xl = VCOL2X(gs, (fdig < rows ? 0 : fdig - rows)) - EPSILON;
	yt = gs->yrange - (yres * (fdig < cols ? 0 : fdig - cols)) + EPSILON;
	xr = VCOL2X(gs, (fdig < cols ? fdig : cols)) + EPSILON;

	if (segs_intersect(bgn[X], bgn[Y], end[X], end[Y], xl, yb, xr, yt,
			   &xi, &yi)) {
	    Di[hits][X] = xi;
	    Di[hits][Y] = yi;

	    if (ISNODE(xi, xres)) {
		/* then it's also a ynode */
		num--;
		hits--;
		continue;
	    }

	    /* find data rows */
	    drow1 = Y2VROW(gs, Di[hits][Y]) * gs->y_mod;
	    drow2 = (1 + Y2VROW(gs, Di[hits][Y])) * gs->y_mod;

	    if (drow2 >= gs->rows) {
		drow2 = gs->rows - 1;	/* bottom edge */
	    }

	    /* find data cols */
	    if (Flat) {
		Di[hits][Z] = gs->att[ATT_TOPO].constant;
	    }
	    else {
		dcol1 = X2VCOL(gs, Di[hits][X]) * gs->x_mod;
		dcol2 = (1 + X2VCOL(gs, Di[hits][X])) * gs->x_mod;

		if (dcol2 >= gs->cols) {
		    dcol2 = gs->cols - 1;	/* right edge */
		}

		dx = DCOL2X(gs, dcol2) - Di[hits][X];
		dy = DROW2Y(gs, drow1) - Di[hits][Y];
		alpha =
		    sqrt(dx * dx + dy * dy) / sqrt(xres * xres + yres * yres);

		offset = DRC2OFF(gs, drow1, dcol2);
		GET_MAPATT(Ebuf, offset, z1);
		offset = DRC2OFF(gs, drow2, dcol1);
		GET_MAPATT(Ebuf, offset, z2);
		Di[hits][Z] = LERP(alpha, z1, z2);
	    }
	}

	/* if they don't intersect, something's wrong! */
	/* should only happen on endpoint, so it will be added later */
	else {
	    hits--;
	    num--;
	}

	fdig += incr;
    }

    return (hits);
}

/*!
   \brief Line intersect

   Author: Mukesh Prasad
   Modified for floating point: Bill Brown

   This function computes whether two line segments,
   respectively joining the input points (x1,y1) -- (x2,y2)
   and the input points (x3,y3) -- (x4,y4) intersect.
   If the lines intersect, the output variables x, y are
   set to coordinates of the point of intersection.

   \param x1,y1,x2,y2 coordinates of endpoints of one segment
   \param x3,y3,x4,y4 coordinates of endpoints of other segment
   \param[out] x,y coordinates of intersection point

   \return 0 no intersection
   \return 1 intersect
   \return 2 collinear
 */
int segs_intersect(float x1, float y1, float x2, float y2, float x3, float y3,
		   float x4, float y4, float *x, float *y)
{
    float a1, a2, b1, b2, c1, c2;	/* Coefficients of line eqns. */
    float r1, r2, r3, r4;	/* 'Sign' values */
    float denom, offset, num;	/* Intermediate values */

    /* Compute a1, b1, c1, where line joining points 1 and 2
     * is "a1 x  +  b1 y  +  c1  =  0".
     */
    a1 = y2 - y1;
    b1 = x1 - x2;
    c1 = x2 * y1 - x1 * y2;

    /* Compute r3 and r4.
     */
    r3 = a1 * x3 + b1 * y3 + c1;
    r4 = a1 * x4 + b1 * y4 + c1;

    /* Check signs of r3 and r4.  If both point 3 and point 4 lie on
     * same side of line 1, the line segments do not intersect.
     */

    if (!EQUAL(r3, 0.0) && !EQUAL(r4, 0.0) && SAME_SIGNS(r3, r4)) {
	return (DONT_INTERSECT);
    }

    /* Compute a2, b2, c2 */
    a2 = y4 - y3;
    b2 = x3 - x4;
    c2 = x4 * y3 - x3 * y4;

    /* Compute r1 and r2 */
    r1 = a2 * x1 + b2 * y1 + c2;
    r2 = a2 * x2 + b2 * y2 + c2;

    /* Check signs of r1 and r2.  If both point 1 and point 2 lie
     * on same side of second line segment, the line segments do
     * not intersect.
     */

    if (!EQUAL(r1, 0.0) && !EQUAL(r2, 0.0) && SAME_SIGNS(r1, r2)) {
	return (DONT_INTERSECT);
    }

    /* Line segments intersect: compute intersection point. 
     */
    denom = a1 * b2 - a2 * b1;

    if (denom == 0) {
	return (COLLINEAR);
    }

    offset = denom < 0 ? -denom / 2 : denom / 2;

    /* The denom/2 is to get rounding instead of truncating.  It
     * is added or subtracted to the numerator, depending upon the
     * sign of the numerator.
     */
    num = b1 * c2 - b2 * c1;

    *x = num / denom;

    num = a2 * c1 - a1 * c2;
    *y = num / denom;

    return (DO_INTERSECT);
}

/*!
   \brief Check if point is on plane

   Plane defined by three points here; user fills in unk[X] & unk[Y]

   \param p1,p2,p3 points defining plane
   \param unk point

   \return 1 point on plane
   \return 0 point not on plane
 */
int Point_on_plane(Point3 p1, Point3 p2, Point3 p3, Point3 unk)
{
    float plane[4];

    P3toPlane(p1, p2, p3, plane);

    return (XY_intersect_plane(unk, plane));
}

/*!
   \brief Check for intersection (point and plane)

   Ax + By + Cz + D = 0, so z = (Ax + By + D) / -C

   User fills in intersect[X] & intersect[Y]

   \param[out] intersect intersect coordinates
   \param plane plane definition

   \return 0 doesn't intersect
   \return 1 intesects
 */
int XY_intersect_plane(float *intersect, float *plane)
{
    float x, y;

    if (!plane[Z]) {
	return (0);		/* doesn't intersect */
    }

    x = intersect[X];
    y = intersect[Y];
    intersect[Z] = (plane[X] * x + plane[Y] * y + plane[W]) / -plane[Z];

    return (1);
}

/*!
   \brief Define plane 

   \param p1,p2,p3 three point on plane
   \param[out] plane plane definition

   \return 1
 */
int P3toPlane(Point3 p1, Point3 p2, Point3 p3, float *plane)
{
    Point3 v1, v2, norm;

    v1[X] = p1[X] - p3[X];
    v1[Y] = p1[Y] - p3[Y];
    v1[Z] = p1[Z] - p3[Z];

    v2[X] = p2[X] - p3[X];
    v2[Y] = p2[Y] - p3[Y];
    v2[Z] = p2[Z] - p3[Z];

    V3Cross(v1, v2, norm);

    plane[X] = norm[X];
    plane[Y] = norm[Y];
    plane[Z] = norm[Z];
    plane[W] = -p3[X] * norm[X] - p3[Y] * norm[Y] - p3[Z] * norm[Z];

    return (1);
}


/*!
   \brief Get cross product

   \param a,b,c 

   \return cross product c = a cross b
 */
int V3Cross(Point3 a, Point3 b, Point3 c)
{
    c[X] = (a[Y] * b[Z]) - (a[Z] * b[Y]);
    c[Y] = (a[Z] * b[X]) - (a[X] * b[Z]);
    c[Z] = (a[X] * b[Y]) - (a[Y] * b[X]);

    return (1);
}
