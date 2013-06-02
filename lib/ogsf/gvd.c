/*!
   \file lib/ogsf/gvd.c

   \brief OGSF library - loading and manipulating vector sets (lower level functions)

   (C) 1999-2008, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.
   
   \author Bill Brown USACERL (December 1993)
   \author Doxygenized by Martin Landa (June 2008)
 */

#include <stdio.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

#include "rowcol.h"

#define CHK_FREQ 5
/* check for cancel every CHK_FREQ lines */

/*!
   \brief Clip segment

   \todo to use fast clipping and move to gs.c

   \param gs surface
   \param bgn begin point
   \param end end point
   \param region region settings 

   \return 1 segment inside region
   \return 0 segment outside region
 */
int gs_clip_segment(geosurf * gs, float *bgn, float *end, float *region)
{
    float top, bottom, left, right;

    if (!region) {
	top = gs->yrange;
	bottom = VROW2Y(gs, VROWS(gs));
	left = 0.0;
	right = VCOL2X(gs, VCOLS(gs));
    }
    else {
	top = region[0];
	bottom = region[1];
	left = region[2];
	right = region[3];
    }

    /* for now, ignore any segments with an end outside */
    return (bgn[X] >= left && bgn[X] <= right &&
	    end[X] >= left && end[X] <= right &&
	    bgn[Y] >= bottom && bgn[Y] <= top &&
	    end[Y] >= bottom && end[Y] <= top);
}

/*!
   \brief Draw vector set

   Need to think about translations - If user translates surface,
   vector should automatically go with it, but translating vector should
   translate it relative to surface on which it's displayed?

   Handling mask checking here, but may be more appropriate to
   handle in get_drape_segments?

   \param gv vector set
   \param gs surface
   \param do_fast non-zero for fast mode

   \return
 */
int gvd_vect(geovect * gv, geosurf * gs, int do_fast)
{
    int i, j, k;
    float bgn[3], end[3], tx, ty, tz, konst;
    float zmin, zmax, fudge;
    Point3 *points;
    int npts, src, check;
    geoline *gln;

    G_debug(5, "gvd_vect(): id=%d", gv->gvect_id);

    if (GS_check_cancel()) {
	return 0;
    }

    gs_update_curmask(gs);

    src = gs_get_att_src(gs, ATT_TOPO);
    GS_get_scale(&tx, &ty, &tz, 1);
    gs_get_zrange(&zmin, &zmax);
    fudge = (zmax - zmin) / 500.;

    if (src == CONST_ATT) {
	konst = gs->att[ATT_TOPO].constant;
	bgn[Z] = end[Z] = konst + gv->z_trans;
    }

    gsd_pushmatrix();

    /* avoid scaling by zero */
    if (tz == 0.0) {
	src = CONST_ATT;
	konst = 0.0;
	bgn[Z] = end[Z] = konst;
	gsd_do_scale(0);
    }
    else {
	gsd_do_scale(1);
    }

    gsd_translate(gs->x_trans, gs->y_trans, gs->z_trans + fudge);

    gsd_colormode(CM_COLOR);

    check = 0;
    if (do_fast) {
	if (!gv->fastlines) {
	    gv_decimate_lines(gv);
	}

	gln = gv->fastlines;
    }
    else {
	gln = gv->lines;
    }

    for (; gln; gln = gln->next) {
	G_debug(5, "gvd_vect(): type = %d dims = %d", gln->type, gln->dims);

	if (!(++check % CHK_FREQ)) {
	    if (GS_check_cancel()) {
		gsd_linewidth(1);
		gsd_popmatrix();

		return 0;
	    }
	}

	if (gln->highlighted > 0) {
	    gsd_color_func(gv->hstyle->color);
	    gsd_linewidth(gv->hstyle->width);
	}
	else if (gv->tstyle && gv->tstyle->active) {
	    gsd_color_func(gln->style->color);
	    gsd_linewidth(gln->style->width);
	}
	else {
	    gsd_color_func(gv->style->color);
	    gsd_linewidth(gv->style->width);
	}

	/* line */
	if (gln->type == OGSF_LINE) {	
	    /* 2D line */
	    if (gln->dims == 2) {	
		G_debug(5, "gvd_vect(): 2D vector line");
		for (k = 0; k < gln->npts - 1; k++) {
		    bgn[X] = gln->p2[k][X] + gv->x_trans - gs->ox;
		    bgn[Y] = gln->p2[k][Y] + gv->y_trans - gs->oy;
		    end[X] = gln->p2[k + 1][X] + gv->x_trans - gs->ox;
		    end[Y] = gln->p2[k + 1][Y] + gv->y_trans - gs->oy;

		    if (src == MAP_ATT) {
			points = gsdrape_get_segments(gs, bgn, end, &npts);
			gsd_bgnline();

			for (i = 0, j = 0; i < npts; i++) {
			    if (gs_point_is_masked(gs, points[i])) {
				if (j) {
				    gsd_endline();
				    gsd_bgnline();
				    j = 0;
				}
				continue;
			    }
			    points[i][Z] += gv->z_trans;
			    gsd_vert_func(points[i]);
			    j++;
			    if (j > 250) {
				gsd_endline();
				gsd_bgnline();
				gsd_vert_func(points[i]);
				j = 1;
			    }
			}
			gsd_endline();
		    }
		    /* need to handle MASK! */
		    else if (src == CONST_ATT) {
			/* for now - but later, do seg intersect maskedge */
			if (gs_point_is_masked(gs, bgn) ||
			    gs_point_is_masked(gs, end))
			    continue;

			if (gs_clip_segment(gs, bgn, end, NULL)) {
			    gsd_bgnline();
			    gsd_vert_func(bgn);
			    gsd_vert_func(end);
			    gsd_endline();
			}
		    }
		}
	    }
	    /* 3D line */
	    else {		
		G_debug(5, "gvd_vect(): 3D vector line");
		points = (Point3 *) malloc(sizeof(Point3));

		gsd_color_func(gv->style->color);
		gsd_bgnline();
		for (k = 0; k < gln->npts; k++) {
		    points[0][X] =
			(float)(gln->p3[k][X] + gv->x_trans - gs->ox);
		    points[0][Y] =
			(float)(gln->p3[k][Y] + gv->y_trans - gs->oy);
		    points[0][Z] = (float)(gln->p3[k][Z] + gv->z_trans);

		    gsd_vert_func(points[0]);
		}
		gsd_endline();
		free(points);
	    }
	}
	/* polygon */
	else if (gln->type == OGSF_POLYGON) {	
	    /* 3D polygon */
	    if (gln->dims == 3) {	
		G_debug(5, "gvd_vect(): draw 3D polygon");

		/* We want at least 3 points */
		if (gln->npts >= 3) {
		    points = (Point3 *) malloc(2 * sizeof(Point3));
		    glEnable(GL_NORMALIZE);

		    glEnable(GL_COLOR_MATERIAL);
		    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

		    glEnable(GL_LIGHTING);
		    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

		    glShadeModel(GL_FLAT);

		    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		    glBegin(GL_POLYGON);
		    glColor3f(1.0, 0, 0);
		    gsd_color_func(gv->style->color);
		    glNormal3fv(gln->norm);

		    for (k = 0; k < gln->npts; k++) {
			points[0][X] =
			    (float)(gln->p3[k][X] + gv->x_trans - gs->ox);
			points[0][Y] =
			    (float)(gln->p3[k][Y] + gv->y_trans - gs->oy);
			points[0][Z] = (float)(gln->p3[k][Z] + gv->z_trans);
			glVertex3fv(points[0]);
		    }
		    glEnd();
		    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
		    G_free(points);
		}
	    }
	    else {		
		/* 2D polygons */
		/* TODO */
	    }
	}
    }

    gsd_linewidth(1);
    gsd_popmatrix();

    return 1;
}

/*!
   \brief Draw line on surface

   \param gs surface
   \param bgn first line point
   \param end end line point
   \param color color value
 */
void gvd_draw_lineonsurf(geosurf * gs, float *bgn, float *end, int color)
{
    Point3 *points;
    int npts, i, j;

    gsd_color_func(color);
    points = gsdrape_get_segments(gs, bgn, end, &npts);
    gsd_bgnline();

    for (i = 0, j = 0; i < npts; i++) {
	if (gs_point_is_masked(gs, points[i])) {
	    if (j) {
		gsd_endline();
		gsd_bgnline();
		j = 0;
	    }

	    continue;
	}

	gsd_vert_func(points[i]);
	j++;

	if (j > 250) {
	    gsd_endline();
	    gsd_bgnline();
	    gsd_vert_func(points[i]);
	    j = 1;
	}
    }

    gsd_endline();

    return;
}
