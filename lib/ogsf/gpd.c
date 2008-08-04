/*!
   \file gpd.c

   \brief OGSF library - loading and manipulating point sets

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois (December 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>
#include <math.h>

#include <grass/gstypes.h>

#include "rowcol.h"

#define CHK_FREQ 50

/* BOB -- border allowed outside of viewport */
#define v_border 50

/* ACS_MODIFY_BEGIN site_attr management ************************************** */
static float _cur_size_;

/*!
   \brief Substitutes gpd_obj()

   \param gs surface (geosurf)
   \param gp site (geosite)
   \param gpt point (point)
   \param site point

   \return 0
 */
int gpd_obj_site_attr(geosurf * gs, geosite * gp, geopoint * gpt, Point3 site)
{
    float size, z, y, x, z_scale, z_offset;
    int marker, color, i, ii, iii;
    int use_attr, has_drawn;
    int _put_aside_;

    _put_aside_ = 0;
    _cur_size_ = gp->size;

    z_scale = GS_global_exag();
    z_offset = 0.0;

    has_drawn = 0;

    for (i = 0; i < GPT_MAX_ATTR; i++) {
	color = gp->color;
	marker = gp->marker;
	size = gp->size;
	use_attr = 0;

	if (gp->use_attr[i] & ST_ATT_COLOR) {
	    use_attr = 1;
	    color = gpt->color[i];
	}

	if (gp->use_attr[i] & ST_ATT_MARKER) {
	    use_attr = 1;
	    marker = gpt->marker[i];
	}

	if (gp->use_attr[i] & ST_ATT_SIZE) {
	    use_attr = 1;
	    size = gpt->size[i] * gp->size;
	    if (gp->marker == ST_HISTOGRAM)
		_put_aside_ = 1;
	}

	/* ACS_MODIFY_BEGIN site_highlight management ********************************* */
	if (gpt->highlight_color)
	    color = gpt->highlight_color_value;
	if (gpt->highlight_marker)
	    marker = gpt->highlight_marker_value;
	if (gpt->highlight_size)
	    size *= gpt->highlight_size_value;
	/* ACS_MODIFY_END site_highlight management *********************************** */

	if (_put_aside_) {
	    if (use_attr == 1) {
		has_drawn = 1;

/*******************************************************************************
		fixed size = gp->size
		this is mailny intended for "histograms" that grow in z, but not in xy

        square filling to right and then up

         15 14 13 12
          8  7  6 11
          3  2  5 10
          0  1  4  9

*******************************************************************************/
		x = site[X];
		y = site[Y];

		ii = (int)(sqrt(i));
		iii = ii * ii + ii;

		if (i <= iii) {
		    site[X] += ii * 2.2 * gp->size;
		    site[Y] += (i - ii) * 2.2 * gp->size;
		}
		else {
		    site[X] += (ii - (i - iii)) * 2.2 * gp->size;
		    site[Y] += ii * 2.2 * gp->size;

		}

		gpd_obj(gs, color, size, marker, site);

		site[X] = x;
		site[Y] = y;
	    }
	}
	else {
	    if (i > 0)
		z_offset += size;
	    if (use_attr == 1) {
		has_drawn = 1;

		z = site[Z];
		site[Z] += z_offset / z_scale;

		gpd_obj(gs, color, size, marker, site);

		site[Z] = z;
	    }

	    z_offset += size;
	}
    }

    if (has_drawn == 0)
	gpd_obj(gs, color, size, marker, site);

    return (0);
}

/* ACS_MODIFY_END site_attr management **************************************** */

/*!
   \brief Check if point is in region

   Check for cancel every CHK_FREQ points

   \param gs surface (geosurf)
   \param pt point (array(X,Y,Z))
   \param region region settings (array (top,bottom,left,right))

   \return 0 point outside of region
   \return 1 point inside region
 */
int gs_point_in_region(geosurf * gs, float *pt, float *region)
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

    return (pt[X] >= left && pt[X] <= right &&
	    pt[Y] >= bottom && pt[Y] <= top);
}

/*!
   \brief ADD

   Do normal transforms before calling

   Note gs: NULL if 3d obj or const elev surface

   \todo add size1, size2 & dir1, dir2 (eg azimuth, elevation) variables

   \param gs surface (geosurf)
   \param size
   \param marker
   \param pt 3d point (Point3)
 */
void gpd_obj(geosurf * gs, int color, float size, int marker, Point3 pt)
{
    float sz, lpt[3];
    float siz[3];

    gsd_color_func(color);
    sz = GS_global_exag();
    GS_v3eq(lpt, pt);		/* CHANGING Z OF POINT PASSED, so use copy */

    switch (marker) {
	/* ACS_MODIFY_BEGIN site_attr management ************************************** */
    case ST_HISTOGRAM:
	gsd_colormode(CM_DIFFUSE);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	siz[0] = _cur_size_;
	siz[1] = _cur_size_;
	siz[2] = size;

	gsd_box(lpt, color, siz);

	gsd_popmatrix();
	gsd_colormode(CM_COLOR);

	break;
	/* ACS_MODIFY_END   site_attr management ************************************** */
    case ST_DIAMOND:
	/*
	   gsd_colormode(CM_AD);
	 */
	gsd_colormode(CM_DIFFUSE);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	gsd_diamond(lpt, color, size);
	gsd_popmatrix();
	gsd_colormode(CM_COLOR);

	break;
    case ST_BOX:
	gsd_colormode(CM_COLOR);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	gsd_draw_box(lpt, color, size);
	gsd_popmatrix();

	break;
    case ST_SPHERE:
	/*
	   gsd_colormode(CM_AD);
	 */
	gsd_colormode(CM_DIFFUSE);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	gsd_sphere(lpt, size);
	gsd_popmatrix();
	gsd_colormode(CM_COLOR);

	break;
    case ST_GYRO:
	gsd_colormode(CM_COLOR);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	gsd_draw_gyro(lpt, color, size);
	gsd_popmatrix();

	break;
    case ST_ASTER:
	gsd_colormode(CM_COLOR);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	gsd_draw_asterisk(lpt, color, size);
	gsd_popmatrix();

	break;
    case ST_CUBE:
	gsd_colormode(CM_DIFFUSE);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	gsd_cube(lpt, color, size);
	gsd_popmatrix();
	gsd_colormode(CM_COLOR);

	break;
    default:
    case ST_X:
	gsd_colormode(CM_COLOR);
	gsd_x(gs, lpt, color, size);

	break;
    }

    return;
}

/*!
   \brief ADD

   Need to think about translations - If user translates surface,
   sites should automatically go with it, but translating sites should
   translate it relative to surface on which it's displayed

   Handling mask checking here

   \todo prevent scaling by 0 

   \param gp site (geosite)
   \param gs surface (geosurf)
   \param do_fast (unused)

   \return 0
   \return 1
 */
int gpd_2dsite(geosite * gp, geosurf * gs, int do_fast)
{
    float site[3], konst;
    float size;
    int src, check, marker, color;
    geopoint *gpt;
    typbuff *buf;
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    GLint window[4];


    if (GS_check_cancel()) {
	return (0);
    }

    if (gs) {
	gs_update_curmask(gs);

	src = gs_get_att_src(gs, ATT_TOPO);

	if (src == CONST_ATT) {
	    konst = gs->att[ATT_TOPO].constant;
	    site[Z] = konst;
	}
	else {
	    buf = gs_get_att_typbuff(gs, ATT_TOPO, 0);
	}

	/* Get viewport parameters for view check */
	gsd_getwindow(window, viewport, modelMatrix, projMatrix);

	gsd_pushmatrix();

	gsd_do_scale(1);

	gsd_translate(gs->x_trans, gs->y_trans, gs->z_trans);

	gsd_linewidth(gp->width);

	check = 0;
	color = gp->color;
	marker = gp->marker;
	size = gp->size;

	for (gpt = gp->points; gpt; gpt = gpt->next) {
	    if (!(++check % CHK_FREQ)) {
		if (GS_check_cancel()) {
		    gsd_linewidth(1);
		    gsd_popmatrix();

		    return (0);
		}
	    }

	    site[X] = gpt->p3[X] + gp->x_trans - gs->ox;
	    site[Y] = gpt->p3[Y] + gp->y_trans - gs->oy;

	    if (gs_point_is_masked(gs, site)) {
		continue;
	    }

	    /* TODO: set other dynamic attributes */
	    if (gp->attr_mode & ST_ATT_COLOR) {
		color = gpt->iattr;
	    }

	    if (src == MAP_ATT) {
		if (viewcell_tri_interp(gs, buf, site, 1)) {
		    /* returns 0 if outside or masked */
		    site[Z] += gp->z_trans;

		    if (gsd_checkpoint
			(site, window, viewport, modelMatrix, projMatrix))
			continue;
		    else
			/* ACS_MODIFY_OneLine site_attr management - was: gpd_obj(gs, color, size, marker, site); */
			gpd_obj_site_attr(gs, gp, gpt, site);
		}
	    }
	    else if (src == CONST_ATT) {
		if (gs_point_in_region(gs, site, NULL)) {
		    site[Z] += gp->z_trans;
		    if (gsd_checkpoint
			(site, window, viewport, modelMatrix, projMatrix))
			continue;
		    else
			/* ACS_MODIFY_OneLine site_attr management - was: gpd_obj(NULL, color, size, marker, site); */
			gpd_obj_site_attr(NULL, gp, gpt, site);
		}
	    }
	}

	gsd_linewidth(1);
	gsd_popmatrix();
    }

    return (1);
}

/*!                                                            
   \brief ADD

   \param gp site (geosite)
   \param xo,yo
   \param do_fast (unused)

   \return 0
   \return 1
 */
int gpd_3dsite(geosite * gp, float xo, float yo, int do_fast)
{
    float site[3], tz;
    float size;
    int check, color, marker;
    geopoint *gpt;
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    GLint window[4];

    if (GS_check_cancel()) {
	return (0);
    }

    gsd_getwindow(window, viewport, modelMatrix, projMatrix);

    gsd_pushmatrix();

    gsd_do_scale(1);

    tz = GS_global_exag();
    site[Z] = 0.0;

    check = 0;
    color = gp->color;
    marker = gp->marker;
    size = gp->size;

    gsd_linewidth(gp->width);

    for (gpt = gp->points; gpt; gpt = gpt->next) {
	if (!(++check % CHK_FREQ)) {
	    if (GS_check_cancel()) {
		gsd_linewidth(1);
		gsd_popmatrix();

		return (0);
	    }
	}

	site[X] = gpt->p3[X] + gp->x_trans - xo;
	site[Y] = gpt->p3[Y] + gp->y_trans - yo;

	if (tz) {
	    site[Z] = gpt->p3[Z] + gp->z_trans;
	}

	/* TODO: set other dynamic attributes */
	if (gp->attr_mode & ST_ATT_COLOR) {
	    color = gpt->iattr;
	}

	if (gsd_checkpoint(site, window, viewport, modelMatrix, projMatrix))
	    continue;
	else
	    /* clip points outside default region? */
	    /* ACS_MODIFY_OneLine site_attr management - was: gpd_obj(NULL, color, size, marker, site); */
	    gpd_obj_site_attr(NULL, gp, gpt, site);
    }

    gsd_linewidth(1);
    gsd_popmatrix();

    return (1);
}
