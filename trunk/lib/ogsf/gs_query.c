/*!
   \file lib/ogsf/gs_query.c

   \brief OGSF library - query (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL (January 1994)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <math.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

/*!
   \brief Values needed for Ray-Convex Polyhedron Intersection Test below
   originally by Eric Haines, erich@eye.com
 */
#ifndef	HUGE_VAL
#define	HUGE_VAL	1.7976931348623157e+308
#endif

/* return codes */
#define	MISSED		 0
#define	FRONTFACE	 1
#define	BACKFACE	-1
/* end Ray-Convex Polyhedron Intersection Test values */


/*!
   \brief Crude method of intersecting line of sight with closest part of surface. 

   Uses los vector to determine the point of first intersection
   which is returned in point. Returns 0 if los doesn't intersect. 

   \param surfid surface id
   \param los should be in surf-world coordinates
   \param[out] point intersect point (real)

   \return 0 on failure
   \return 1 on success
 */
int gs_los_intersect1(int surfid, float (*los)[3], float *point)
{
    float dx, dy, dz, u_d[3];
    float a[3], incr, min_incr, tlen, len;
    int outside, above, below, edge, istep;
    float b[3];
    geosurf *gs;
    typbuff *buf;

    G_debug(3, "gs_los_intersect1():");

    if (NULL == (gs = gs_get_surf(surfid))) {
	return (0);
    }

    if (0 == GS_v3dir(los[FROM], los[TO], u_d)) {
	return (0);
    }

    buf = gs_get_att_typbuff(gs, ATT_TOPO, 0);

    istep = edge = below = 0;

    len = 0.0;
    tlen = GS_distance(los[FROM], los[TO]);

    incr = tlen / 1000.0;
    min_incr = incr / 1000.0;

    dx = incr * u_d[X];
    dy = incr * u_d[Y];
    dz = incr * u_d[Z];

    a[X] = los[FROM][X];
    a[Y] = los[FROM][Y];
    a[Z] = los[FROM][Z];

    b[X] = a[X] - gs->x_trans;
    b[Y] = a[Y] - gs->y_trans;

    if (viewcell_tri_interp(gs, buf, b, 0)) {
	/* expects surface coords */
	b[Z] += gs->z_trans;

	if (a[Z] < b[Z]) {
	    /*  viewing from below surface  */
	    /*    don't use this method 
	       fprintf(stderr,"view from below\n");
	       below = 1;
	     */

	    return (0);
	}
    }

    while (incr > min_incr) {
	outside = 0;
	above = 0;
	b[X] = a[X] - gs->x_trans;
	b[Y] = a[Y] - gs->y_trans;

	if (viewcell_tri_interp(gs, buf, b, 0)) {
	    /* ignores masks */
	    b[Z] += gs->z_trans;
	    above = (a[Z] > b[Z]);
	}
	else {
	    outside = 1;

	    if (istep > 10) {
		edge = 1;
		below = 1;
	    }
	}

	while (outside || above) {
	    a[X] += dx;
	    a[Y] += dy;
	    a[Z] += dz;
	    len += incr;
	    outside = 0;
	    above = 0;
	    b[X] = a[X] - gs->x_trans;
	    b[Y] = a[Y] - gs->y_trans;

	    if (viewcell_tri_interp(gs, buf, b, 0)) {
		b[Z] += gs->z_trans;
		above = (a[Z] > b[Z]);
	    }
	    else {
		outside = 1;
	    }

	    if (len > tlen) {
		return 0;	/* over surface *//* under surface */
	    }
	}

	/* could look for spikes here - see if any data points along 
	   shadow of line on surf go above los */

	/* back up several spots? */
	a[X] -= (1.0 * dx);
	a[Y] -= (1.0 * dy);
	a[Z] -= (1.0 * dz);
	incr /= 2.0;
	++istep;
	dx = incr * u_d[X];
	dy = incr * u_d[Y];
	dz = incr * u_d[Z];
    }

    if ((edge) && (b[Z] - (a[Z] + dz * 2.0) > incr * u_d[Z])) {
	G_debug(3, "  looking under surface");

	return 0;
    }

    point[X] = b[X];
    point[Y] = b[Y];
    point[Z] = b[Z] - gs->z_trans;

    return (1);
}

/*!
   \brief Crude method of intersecting line of sight with closest part of surface. 

   This version uses the shadow of the los projected down to
   the surface to generate a line_on_surf, then follows each
   point in that line until the los intersects it.

   \param surfid surface id
   \param los should be in surf-world coordinates
   \param[out] point intersect point (real)

   \return 0 on failure
   \return 1 on success
 */
int gs_los_intersect(int surfid, float **los, float *point)
{
    double incr;
    float p1, p2, u_d[3];
    int above, ret, num, i, usedx;
    float a[3], b[3];
    float bgn[3], end[3], a1[3];
    geosurf *gs;
    typbuff *buf;
    Point3 *points;

    G_debug(3, "gs_los_intersect");

    if (NULL == (gs = gs_get_surf(surfid))) {
	return (0);
    }

    if (0 == GS_v3dir(los[FROM], los[TO], u_d)) {
	return (0);
    }

    buf = gs_get_att_typbuff(gs, ATT_TOPO, 0);

    GS_v3eq(bgn, los[FROM]);
    GS_v3eq(end, los[TO]);

    bgn[X] -= gs->x_trans;
    bgn[Y] -= gs->y_trans;

    end[X] -= gs->x_trans;
    end[Y] -= gs->y_trans;

    /* trans? */
    points = gsdrape_get_allsegments(gs, bgn, end, &num);

    /* DEBUG
       {
       float t1[3], t2[3];

       t1[X] = los[FROM][X] ;
       t1[Y] = los[FROM][Y] ;

       t2[X] = los[TO][X] ;
       t2[Y] = los[TO][Y] ;

       GS_set_draw(GSD_FRONT);
       gsd_pushmatrix();
       gsd_do_scale(1);
       gsd_translate(gs->x_trans, gs->y_trans, gs->z_trans);
       gsd_linewidth(1);
       gsd_color_func(GS_default_draw_color());
       gsd_line_onsurf(gs, t1, t2);
       gsd_popmatrix();
       GS_set_draw(GSD_BACK);
       gsd_flush();
       }
       fprintf(stderr,"%d points to check\n", num);
       fprintf(stderr,"point0 = %.6lf %.6lf %.6lf FT =%.6lf %.6lf %.6lf\n",
       points[0][X],points[0][Y],points[0][Z],
       los[FROM][X],los[FROM][Y],los[FROM][Z]);
       fprintf(stderr,"incr1 = %.6lf: %.6lf %.6lf %.6lf\n",incr,u_d[X],u_d[Y],u_d[Z]);
       fprintf(stderr,"first point below surf\n");
       fprintf(stderr,"incr2 = %f\n", (float)incr);
       fprintf(stderr,"(%d/%d) %f > %f\n", i,num, a[Z], points[i][Z]);
       fprintf(stderr,"incr3 = %f\n", (float)incr);
       fprintf(stderr,"all points above surf\n");
     */

    if (num < 2) {
	G_debug(3, "  %d points to check", num);

	return (0);
    }

    /* use larger of deltas for better precision */
    usedx = (fabs(u_d[X]) > fabs(u_d[Y]));
    if (usedx) {
	incr = ((points[0][X] - (los[FROM][X] - gs->x_trans)) / u_d[X]);
    }
    else if (u_d[Y]) {
	incr = ((points[0][Y] - (los[FROM][Y] - gs->y_trans)) / u_d[Y]);
    }
    else {
	point[X] = los[FROM][X] - gs->x_trans;
	point[Y] = los[FROM][Y] - gs->y_trans;

	return (viewcell_tri_interp(gs, buf, point, 1));
    }

    /* DEBUG
       fprintf(stderr,"-----------------------------\n");
       fprintf(stderr,"%d points to check\n", num);
       fprintf(stderr,"incr1 = %.6lf: %.9f %.9f %.9f\n",incr,u_d[X],u_d[Y],u_d[Z]);
       fprintf(stderr,
       "\tpoint0 = %.6f %.6f %.6f\n\tFT = %.6f %.6f %.6f\n\tpoint%d = %.6f %.6f\n",
       points[0][X],points[0][Y],points[0][Z],
       los[FROM][X],los[FROM][Y],los[FROM][Z],
       num-1, points[num-1][X],points[num-1][Y]);
     */

    /* This should bring us right above (or below) the first point */
    a[X] = los[FROM][X] + incr * u_d[X] - gs->x_trans;
    a[Y] = los[FROM][Y] + incr * u_d[Y] - gs->y_trans;
    a[Z] = los[FROM][Z] + incr * u_d[Z] - gs->z_trans;

    if (a[Z] < points[0][Z]) {
	/*  viewing from below surface  */
	/*  don't use this method */
	/* DEBUG
	   fprintf(stderr,"first point below surf\n");
	   fprintf(stderr,"aZ= %.6f point0 = %.6f %.6f %.6f FT =%.6f %.6f %.6f\n",
	   a[Z], points[0][X],points[0][Y],points[0][Z],
	   los[FROM][X],los[FROM][Y],los[FROM][Z]);
	 */
	return (0);
    }

    GS_v3eq(a1, a);
    GS_v3eq(b, a);

    for (i = 1; i < num; i++) {
	if (usedx) {
	    incr = ((points[i][X] - a1[X]) / u_d[X]);
	}
	else {
	    incr = ((points[i][Y] - a1[Y]) / u_d[Y]);
	}

	a[X] = a1[X] + (incr * u_d[X]);
	a[Y] = a1[Y] + (incr * u_d[Y]);
	a[Z] = a1[Z] + (incr * u_d[Z]);
	above = (a[Z] >= points[i][Z]);

	if (above) {
	    GS_v3eq(b, a);
	    continue;
	}

	/* 
	 * Now we know b[Z] is above points[i-1] 
	 * and a[Z] is below points[i]
	 * Since there should only be one polygon along this seg,
	 * just interpolate to intersect 
	 */

	if (usedx) {
	    incr = ((a[X] - b[X]) / u_d[X]);
	}
	else {
	    incr = ((a[Y] - b[Y]) / u_d[Y]);
	}

	if (1 == (ret = segs_intersect(1.0, points[i][Z],
				       0.0, points[i - 1][Z],
				       1.0, a[Z], 0.0, b[Z], &p1, &p2))) {
	    point[X] = points[i - 1][X] + (u_d[X] * incr * p1);
	    point[Y] = points[i - 1][Y] + (u_d[Y] * incr * p1);
	    point[Z] = p2;

	    return (1);
	}

	G_debug(3, "  line of sight error %d", ret);

	return 0;
    }

    /* over surface */
    return 0;
}

/*!
   \brief Ray-Convex Polyhedron Intersection Test 

   Originally by Eric Haines, erich@eye.com

   This test checks the ray against each face of a polyhedron, checking whether
   the set of intersection points found for each ray-plane intersection
   overlaps the previous intersection results.  If there is no overlap (i.e.
   no line segment along the ray that is inside the polyhedron), then the
   ray misses and returns 0; else 1 is returned if the ray is entering the
   polyhedron, -1 if the ray originates inside the polyhedron.  If there is
   an intersection, the distance and the nunber of the face hit is returned.

   \param org,dir origin and direction of ray 
   \param tmax maximum useful distance along ray
   \param phdrn list of planes in convex polyhedron
   \param ph_num number of planes in convex polyhedron
   \param[out] tresult distance of intersection along ray
   \param[out] pn number of face hit (0 to ph_num-1)

   \return FACE code
 */
int RayCvxPolyhedronInt(Point3 org, Point3 dir, double tmax, Point4 * phdrn,
			int ph_num, double *tresult, int *pn)
{
    double tnear, tfar, t, vn, vd;
    int fnorm_num, bnorm_num;	/* front/back face # hit */

    tnear = -HUGE_VAL;
    tfar = tmax;

    /* Test each plane in polyhedron */
    for (; ph_num--;) {
	/* Compute intersection point T and sidedness */
	vd = DOT3(dir, phdrn[ph_num]);
	vn = DOT3(org, phdrn[ph_num]) + phdrn[ph_num][W];

	if (vd == 0.0) {
	    /* ray is parallel to plane - check if ray origin is inside plane's
	       half-space */
	    if (vn > 0.0) {
		/* ray origin is outside half-space */
		return (MISSED);
	    }
	}
	else {
	    /* ray not parallel - get distance to plane */
	    t = -vn / vd;

	    if (vd < 0.0) {
		/* front face - T is a near point */
		if (t > tfar) {
		    return (MISSED);
		}

		if (t > tnear) {
		    /* hit near face, update normal */
		    fnorm_num = ph_num;
		    tnear = t;
		}
	    }
	    else {
		/* back face - T is a far point */
		if (t < tnear) {
		    return (MISSED);
		}

		if (t < tfar) {
		    /* hit far face, update normal */
		    bnorm_num = ph_num;
		    tfar = t;
		}
	    }
	}
    }

    /* survived all tests */
    /* Note: if ray originates on polyhedron, may want to change 0.0 to some
     * epsilon to avoid intersecting the originating face.
     */
    if (tnear >= 0.0) {
	/* outside, hitting front face */
	*tresult = tnear;
	*pn = fnorm_num;

	return (FRONTFACE);
    }
    else {
	if (tfar < tmax) {
	    /* inside, hitting back face */
	    *tresult = tfar;
	    *pn = bnorm_num;

	    return (BACKFACE);
	}
	else {
	    /* inside, but back face beyond tmax */
	    return (MISSED);
	}
    }
}

/*!
   \brief Get data bounds for plane

   \param[out] planes
 */
void gs_get_databounds_planes(Point4 * planes)
{
    float n, s, w, e, b, t;
    Point3 tlfront, brback;

    GS_get_zrange(&b, &t, 0);
    gs_get_xrange(&w, &e);
    gs_get_yrange(&s, &n);

    tlfront[X] = tlfront[Y] = 0.0;
    tlfront[Z] = t;

    brback[X] = e - w;
    brback[Y] = n - s;
    brback[Z] = b;

    /* top */
    planes[0][X] = planes[0][Y] = 0.0;
    planes[0][Z] = 1.0;
    planes[0][W] = -(DOT3(planes[0], tlfront));

    /* bottom */
    planes[1][X] = planes[1][Y] = 0.0;
    planes[1][Z] = -1.0;
    planes[1][W] = -(DOT3(planes[1], brback));

    /* left */
    planes[2][Y] = planes[2][Z] = 0.0;
    planes[2][X] = -1.0;
    planes[2][W] = -(DOT3(planes[2], tlfront));

    /* right */
    planes[3][Y] = planes[3][Z] = 0.0;
    planes[3][X] = 1.0;
    planes[3][W] = -(DOT3(planes[3], brback));

    /* front */
    planes[4][X] = planes[4][Z] = 0.0;
    planes[4][Y] = -1.0;
    planes[4][W] = -(DOT3(planes[4], tlfront));

    /* back */
    planes[5][X] = planes[5][Z] = 0.0;
    planes[5][Y] = 1.0;
    planes[5][W] = -(DOT3(planes[5], brback));

    return;
}

/*!
   Gets all current cutting planes & data bounding planes

   Intersects los with resulting convex polyhedron, then replaces los[FROM] with
   first point on ray inside data.

   \param[out] los

   \return 0 on failure
   \return 1 on success
 */
int gs_setlos_enterdata(Point3 * los)
{
    Point4 planes[12];		/* MAX_CPLANES + 6  - should define this */
    Point3 dir;
    double dist, maxdist;
    int num, ret, retp;		/* might want to tell if retp is a clipping plane */

    gs_get_databounds_planes(planes);
    num = gsd_get_cplanes(planes + 6);
    GS_v3dir(los[FROM], los[TO], dir);
    maxdist = GS_distance(los[FROM], los[TO]);

    ret = RayCvxPolyhedronInt(los[0], dir, maxdist,
			      planes, num + 6, &dist, &retp);

    if (ret == MISSED) {
	return (0);
    }

    if (ret == FRONTFACE) {
	GS_v3mult(dir, (float)dist);
	GS_v3add(los[FROM], dir);
    }

    return (1);
}

/***********************************************************************/
/* DEBUG ****
   void pr_plane(int pnum)
   {
   switch(pnum)
   {
   case 0:
   fprintf(stderr,"top plane");

   break;
   case 1:
   fprintf(stderr,"bottom plane");

   break;
   case 2:
   fprintf(stderr,"left plane");

   break;
   case 3:
   fprintf(stderr,"right plane");

   break;
   case 4:
   fprintf(stderr,"front plane");

   break;
   case 5:
   fprintf(stderr,"back plane");

   break;
   default:
   fprintf(stderr,"clipping plane %d", 6 - pnum);

   break;
   }

   return;
   }
   ******* */
