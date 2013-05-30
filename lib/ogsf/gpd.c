/*!
  \file lib/ogsf/gpd.c
  
  \brief OGSF library - loading and manipulating point sets (lower level)
  
  (C) 1999-2008, 2011 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Bill Brown USACERL, GMSL/University of Illinois (December 1993)
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
*/

#include <stdlib.h>
#include <math.h>

#include <grass/ogsf.h>

#include "rowcol.h"

#define CHK_FREQ 50

/* BOB -- border allowed outside of viewport */
#define v_border 50

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
  \brief Draw point representing object
  
  Do normal transforms before calling
  
  Note gs: NULL if 3d obj or const elev surface
  
  \param gs surface (geosurf)
  \param style object displaying style (highlighted or not)
  \param pt 3d point (Point3)
 */
void gpd_obj(geosurf * gs, gvstyle * style, Point3 pt)
{
    float sz, lpt[3];
    float siz[3];

    gsd_color_func(style->color);
    sz = GS_global_exag();
    GS_v3eq(lpt, pt);		/* CHANGING Z OF POINT PASSED, so use copy */

    switch (style->symbol) {
    case ST_HISTOGRAM:
	gsd_colormode(CM_DIFFUSE);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	siz[0] = style->size;	/*TODO: Fix historgam drawing */
	siz[1] = style->size;
	siz[2] = style->size;

	gsd_box(lpt, style->color, siz);

	gsd_popmatrix();
	gsd_colormode(CM_COLOR);

	break;
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

	gsd_diamond(lpt, style->color, style->size);
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

	gsd_draw_box(lpt, style->color, style->size);
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

	gsd_sphere(lpt, style->size);
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

	gsd_draw_gyro(lpt, style->color, style->size);
	gsd_popmatrix();

	break;
    case ST_ASTER:
	gsd_colormode(CM_COLOR);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	gsd_draw_asterisk(lpt, style->color, style->size);
	gsd_popmatrix();

	break;
    case ST_CUBE:
	gsd_colormode(CM_DIFFUSE);
	gsd_pushmatrix();

	if (sz) {
	    lpt[Z] *= sz;
	    gsd_scale(1.0, 1.0, 1. / sz);
	}

	gsd_cube(lpt, style->color, style->size);
	gsd_popmatrix();
	gsd_colormode(CM_COLOR);

	break;
    default:
    case ST_X:
	gsd_colormode(CM_COLOR);
	gsd_x(gs, lpt, style->color, style->size);

	break;
    }

    return;
}

/*!
  \brief Draw 2D point set
  
  Need to think about translations - If user translates surface,
  sites should automatically go with it, but translating sites should
  translate it relative to surface on which it's displayed
  
  Handling mask checking here
  
  \todo prevent scaling by 0 
  
  \param gp site (geosite)
  \param gs surface (geosurf)
  \param do_fast (unused)
  
  \return 0 on failure
  \return 1 on success
*/
int gpd_2dsite(geosite * gp, geosurf * gs, int do_fast)
{
    float site[3], konst;
    int src, check;
    geopoint *gpt;
    typbuff *buf;
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    GLint window[4];

    if (GS_check_cancel()) {
	return 0;
    }

    if (!gs)
	return 1;

    gs_update_curmask(gs);
    
    src = gs_get_att_src(gs, ATT_TOPO);
    
    if (src == CONST_ATT) {
	konst = gs->att[ATT_TOPO].constant;
    }
    else {
	buf = gs_get_att_typbuff(gs, ATT_TOPO, 0);
    }
    
    /* Get viewport parameters for view check */
    gsd_getwindow(window, viewport, modelMatrix, projMatrix);
    
    gsd_pushmatrix();
    gsd_do_scale(1);
    gsd_translate(gs->x_trans, gs->y_trans, gs->z_trans);
    gsd_linewidth(gp->style->width);
    check = 0;
    
    for (gpt = gp->points; gpt; gpt = gpt->next) {
	if (!(++check % CHK_FREQ)) {
	    if (GS_check_cancel()) {
		gsd_linewidth(1);
		gsd_popmatrix();
		
		return 0;
	    }
	}
	
	site[X] = gpt->p3[X] + gp->x_trans - gs->ox;
	site[Y] = gpt->p3[Y] + gp->y_trans - gs->oy;
	
	if (gs_point_is_masked(gs, site)) {
	    continue;
	}
	
	if (src == MAP_ATT) {
	    if (viewcell_tri_interp(gs, buf, site, 1)) {
		/* returns 0 if outside or masked */
		site[Z] += gp->z_trans;
		
		if (gsd_checkpoint(site, window,
				   viewport, modelMatrix, projMatrix))
		    continue;
	    }
	}
	else if (src == CONST_ATT) {
	    if (gs_point_in_region(gs, site, NULL)) {
		site[Z] = konst + gp->z_trans;
		if (gsd_checkpoint(site, window,
				   viewport, modelMatrix, projMatrix))
		    continue;
	    }
	}
	
	if (gpt->highlighted > 0)
	    gpd_obj(gs, gp->hstyle, site);
	else if (gp->tstyle && gp->tstyle->active)
	    gpd_obj(gs, gpt->style, site);
	else
	    gpd_obj(gs, gp->style, site);
    }

    gsd_linewidth(1);
    gsd_popmatrix();
    
    return 1;
}

/*!                                                            
  \brief Draw 3D point set
  
  \param gp site (geosite)
  \param xo,yo
  \param do_fast (unused)
  
  \return 0 on success
  \return 1 on failure
*/
int gpd_3dsite(geosite * gp, float xo, float yo, int do_fast)
{
    float site[3], tz;
    int check;
    geopoint *gpt;
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    GLint window[4];

    if (GS_check_cancel()) {
	return 0;
    }

    gsd_getwindow(window, viewport, modelMatrix, projMatrix);

    gsd_pushmatrix();

    gsd_do_scale(1);

    tz = GS_global_exag();
    site[Z] = 0.0;

    check = 0;

    gsd_linewidth(gp->style->width);

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

	if (gsd_checkpoint(site, window, viewport, modelMatrix, projMatrix))
	    continue;
	else
	    /* clip points outside default region? */
	{
	    if (gpt->highlighted > 0)
		gpd_obj(NULL, gp->hstyle, site);
	    else if (gp->tstyle && gp->tstyle->active)
		gpd_obj(NULL, gpt->style, site);
	    else
		gpd_obj(NULL, gp->style, site);
	}
    }

    gsd_linewidth(1);
    gsd_popmatrix();

    return 1;
}
