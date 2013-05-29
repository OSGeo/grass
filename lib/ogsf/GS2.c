/*!
   \file GS2.c

   \brief OGSF library - loading and manipulating surfaces (higher level functions)

   GRASS OpenGL gsurf OGSF Library 

   Plans for handling color maps:
   NOW:
   if able to load as unsigned char, make lookup table containing palette
   otherwise, load directly as packed color, set lookup = NULL
   MAYBE LATER:
   if able to load as POSITIVE short, make lookup table containing palette
   - may want to calculate savings first (ie,  numcells > 32768)
   (not exactly, it's Friday & time to go home - figure it later)
   otherwise, load directly as packed color, set lookup = NULL
   MESSY! - need to fix up!

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL (1993)
   \author Pierre de Mouveaux <p_de_mouveaux hotmail.com> (updated October 1999)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grass/config.h>

#if defined(OPENGL_X11) || defined(OPENGL_WINDOWS)
#include <GL/gl.h>
#include <GL/glu.h>
#elif defined(OPENGL_AQUA)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/ogsf.h>
#include <grass/glocale.h>

#include "gsget.h"
#include "rowcol.h"
#include "rgbpack.h"

/* Hack to make NVIZ2.2 query functions.("What's Here" and "Look at")
 * to work.
 * Uses gs_los_intersect1() instead of gs_los_intersect().
 * Pierre de Mouveaux - 31 oct. 1999. p_de_mouveaux@hotmail.com.
 */
#define NVIZ_HACK 1

int gsd_getViewport(GLint *, GLint *);

/* array of surface ids */
static int Surf_ID[MAX_SURFS];
static int Next_surf = 0;
static int SDref_surf = 0;

/* attributes array */
static float Default_const[MAX_ATTS];
static float Default_nulls[MAX_ATTS];

/* largest dimension */
static float Longdim;

/* N, S, W, E */
static float Region[4];
static geoview Gv;
static geodisplay Gd;
static struct Cell_head wind;
static int Buffermode;
static int Numlights = 0;
static int Resetlight = 1;
static int Modelshowing = 0;

void void_func(void)
{
    return;
}

/*!
   \brief Initialize OGSF library

   Get region settings - wind

   Set Region (NSWE array) and compute scale
 */
void GS_libinit(void)
{
    static int first = 1;

    G_get_set_window(&wind);

    Region[0] = wind.north;
    Region[1] = wind.south;
    Region[2] = wind.west;
    Region[3] = wind.east;

    /* scale largest dimension to GS_UNIT_SIZE */
    if ((wind.east - wind.west) > (wind.north - wind.south)) {
	Longdim = (wind.east - wind.west);
    }
    else {
	Longdim = (wind.north - wind.south);
    }

    Gv.scale = GS_UNIT_SIZE / Longdim;

    G_debug(1, "GS_libinit(): n=%f s=%f w=%f e=%f scale=%f first=%d",
	    Region[0], Region[1], Region[2], Region[3], Gv.scale, first);
    
    Cxl_func = void_func;
    Swap_func = void_func;

    
    if (first) {
	gs_init();
    }

    first = 0;

    return;
}

/*!
   \brief Get largest dimension

   \param[out] dim dimension

   \return 1
 */
int GS_get_longdim(float *dim)
{
    *dim = Longdim;

    G_debug(3, "GS_get_longdim(): dim=%g", *dim);

    return (1);
}

/*!
   \brief Get 2D region extent

   \param[out] n,s,w,e extent values

   \return 1
 */
int GS_get_region(float *n, float *s, float *w, float *e)
{
    *n = Region[0];
    *s = Region[1];
    *w = Region[2];
    *e = Region[3];

    return (1);
}

/*!
   \brief Set default attributes for map objects

   \param defs attributes array (dim MAX_ATTS)
   \param null_defs null attributes array (dim MAX_ATTS)
 */
void GS_set_att_defaults(float *defs, float *null_defs)
{
    int i;

    G_debug(3, "GS_set_att_defaults");

    for (i = 0; i < MAX_ATTS; i++) {
	Default_const[i] = defs[i];
	Default_nulls[i] = null_defs[i];
    }

    return;
}

/*!
   Check if surface exists

   \param id surface id

   \return 0 not found
   \return 1 found
 */
int GS_surf_exists(int id)
{
    int i, found = 0;

    G_debug(3, "GS_surf_exists(): id=%d", id);


    if (NULL == gs_get_surf(id)) {
	return (0);
    }

    for (i = 0; i < Next_surf && !found; i++) {
	if (Surf_ID[i] == id) {
	    found = 1;
	}
    }

    return (found);
}

/*!
   \brief Add new surface

   Note that origin has 1/2 cell added to represent center of cells
   because library assumes that east - west = (cols - 1) * ew_res,
   since left and right columns are on the edges.

   \return surface id
   \return -1 on error (MAX_SURFS exceded)
 */
int GS_new_surface(void)
{
    geosurf *ns;

    G_debug(3, "GS_new_surface():");

    if (Next_surf < MAX_SURFS) {
	ns = gs_get_new_surface();
	gs_init_surf(ns, wind.west + wind.ew_res / 2.,
		     wind.south + wind.ns_res / 2., wind.rows, wind.cols,
		     wind.ew_res, wind.ns_res);
	gs_set_defaults(ns, Default_const, Default_nulls);

	/* make default shine current */
	gs_set_att_src(ns, ATT_SHINE, CONST_ATT);

	Surf_ID[Next_surf] = ns->gsurf_id;
	++Next_surf;

	G_debug(3, "    id=%d", ns->gsurf_id);

	return (ns->gsurf_id);
    }



    return (-1);
}
void GS_set_light_reset(int i)
{
    Resetlight = i;
    if (i)
	Numlights = 0;
}
int GS_get_light_reset(void)
{
    return Resetlight;
}
/*!
   \brief Add new model light

   \return light model id
   \return -1 on error (MAX_LIGHTS exceded)
 */
int GS_new_light(void)
{
    int i;

    if (GS_get_light_reset()) {

	GS_set_light_reset(0);

	for (i = 0; i < MAX_LIGHTS; i++) {
	    Gv.lights[i].position[X] = Gv.lights[i].position[Y] = 0.0;
	    Gv.lights[i].position[Z] = 1.0;
	    Gv.lights[i].position[W] = 0.0;	/* infinite */
	    Gv.lights[i].color[0] = Gv.lights[i].color[1] =
		Gv.lights[i].color[2] = 1.0;
	    Gv.lights[i].ambient[0] = Gv.lights[i].ambient[1] =
		Gv.lights[i].ambient[2] = 0.2;
	    Gv.lights[i].shine = 32.0;
	}

	gsd_init_lightmodel();
    }

    if (Numlights < MAX_LIGHTS) {
	gsd_deflight(Numlights + 1, &(Gv.lights[Numlights]));
	gsd_switchlight(Numlights + 1, 1);

	return ++Numlights;
    }

    return -1;
}

/*!
   \brief Set light position

   \bug I think lights array doesnt match sgi_light array

   \param num light id (starts with 1)
   \param xpos,ypos,zpos coordinates (model)
   \param local local coordinate (for viewport)
 */
void GS_setlight_position(int num, float xpos, float ypos, float zpos,
			  int local)
{
    if (num) {
	num -= 1;
	if (num < Numlights) {
	    Gv.lights[num].position[X] = xpos;
	    Gv.lights[num].position[Y] = ypos;
	    Gv.lights[num].position[Z] = zpos;
	    Gv.lights[num].position[W] = (float)local;

	    gsd_deflight(num + 1, &(Gv.lights[num]));
	}
    }

    return;
}


/*!
   \brief Get light position

   \param num light id (starts at 1)
   \param[out] xpos,ypos,zpos coordinates
   \param[out] local ?
 */
void GS_getlight_position(int num, float *xpos, float *ypos, float *zpos,
			  int *local)
{
    if (num) {
	num -= 1;
	if (num < Numlights) {
	    *xpos = Gv.lights[num].position[X];
	    *ypos = Gv.lights[num].position[Y];
	    *zpos = Gv.lights[num].position[Z];
	    *local = (int)Gv.lights[num].position[W];

	}
    }

    return;
}

/*!
   \brief Set light color

   \param num light id (starts at 1)
   \param red,green,blue color values (from 0.0 to 1.0)
 */
void GS_setlight_color(int num, float red, float green, float blue)
{
    if (num) {
	num -= 1;
	if (num < Numlights) {
	    Gv.lights[num].color[0] = red;
	    Gv.lights[num].color[1] = green;
	    Gv.lights[num].color[2] = blue;

	    gsd_deflight(num + 1, &(Gv.lights[num]));
	}
    }

    return;
}

/*!
   \brief Get light color

   \param num light id (starts at 1)
   \param[out] red,green,blue color values
 */
void GS_getlight_color(int num, float *red, float *green, float *blue)
{
    if (num) {
	num -= 1;
	if (num < Numlights) {
	    *red = Gv.lights[num].color[0];
	    *green = Gv.lights[num].color[1];
	    *blue = Gv.lights[num].color[2];
	}
    }

    return;
}

/*!
   \brief Set light ambient

   Red, green, blue from 0.0 to 1.0

   \param num light id (starts at 1)
   \param red,green,blue color values
 */
void GS_setlight_ambient(int num, float red, float green, float blue)
{
    if (num) {
	num -= 1;
	if (num < Numlights) {
	    Gv.lights[num].ambient[0] = red;
	    Gv.lights[num].ambient[1] = green;
	    Gv.lights[num].ambient[2] = blue;

	    gsd_deflight(num + 1, &(Gv.lights[num]));
	}
    }

    return;
}

/*!
   \brief Get light ambient

   \param num light id (starts at 1)
   \param[out] red,green,blue color values
 */
void GS_getlight_ambient(int num, float *red, float *green, float *blue)
{
    if (num) {
	num -= 1;
	if (num < Numlights) {
	    *red = Gv.lights[num].ambient[0];
	    *green = Gv.lights[num].ambient[1];
	    *blue = Gv.lights[num].ambient[2];
	}
    }

    return;
}


/*!
   \brief Switch off all lights
 */
void GS_lights_off(void)
{
    int i;

    for (i = 0; i < Numlights; i++) {
	gsd_switchlight(i + 1, 0);
    }

    return;
}

/*!
   \brief Switch on all lights
 */
void GS_lights_on(void)
{
    int i;

    for (i = 0; i < Numlights; i++) {
	gsd_switchlight(i + 1, 1);
    }

    return;
}

/*!
   \brief Switch on/off light

   \param num light id (starts at 1)
   \param on non-zero for 'on' otherwise 'off'
 */
void GS_switchlight(int num, int on)
{
    if (num) {
	num -= 1;

	if (num < Numlights) {
	    gsd_switchlight(num + 1, on);
	}
    }

    return;
}

/*!
   \brief Check if transparency is set

   \return 0 transparency not set
   \return 1 transparency is set
 */
int GS_transp_is_set(void)
{
    return (gs_att_is_set(NULL, ATT_TRANSP) || (FC_GREY == gsd_getfc()));
}

/*!
   \brief Retrieves coordinates for lighting model position, at center of view

   \param pos[out] coordinates
 */
void GS_get_modelposition1(float pos[])
{
    /* TODO: Still needs work to handle other cases */
    /* this is a quick hack to get lighting adjustments debugged */
    /*
       GS_v3dir(Gv.from_to[FROM], Gv.from_to[TO], center);
       GS_v3mult(center, 1000);
       GS_v3add(center, Gv.from_to[FROM]);
     */

    gs_get_datacenter(pos);
    gs_get_data_avg_zmax(&(pos[Z]));

    G_debug(1, "GS_get_modelposition1(): model position: %f %f %f",
	    pos[X], pos[Y], pos[Z]);

    return;
}

/*!
   \brief Retrieves coordinates for lighting model position, at center of view

   Position at nearclip * 2: tried nearclip + siz, but since need to
   know position to calculate size, have two dependent variables
   (nearclip * 2) from eye.

   \param siz[out] size
   \param pos[out] coordinates (X, Y, Z)
 */
void GS_get_modelposition(float *siz, float *pos)
{
    float dist, near_h, dir[3];

    dist = 2. * Gd.nearclip;

    near_h = 2.0 * tan(4.0 * atan(1.) * Gv.fov / 3600.) * dist;
    *siz = near_h / 8.0;

    /* prevent clipping - would only happen if fov > ~127 degrees, at
       fov = 2.0 * atan(2.0) */

    if (*siz > Gd.nearclip) {
	*siz = Gd.nearclip;
    }

    GS_v3dir(Gv.from_to[FROM], Gv.from_to[TO], dir);

    pos[X] = Gv.from_to[FROM][X] + dir[X] * dist;
    pos[Y] = Gv.from_to[FROM][Y] + dir[Y] * dist;
    pos[Z] = Gv.from_to[FROM][Z] + dir[Z] * dist;

    return;
}


/*!
   \brief Set decoration, north arrow ??

   \todo scale used to calculate len of arrow still needs work
   needs go function that returns center / eye distance
   gsd_get_los function is not working correctly ??

   \param pt point value in true world coordinates (?)
   \param id surface id
   \param[out] pos2 output coordinates
 */
void GS_set_Narrow(int *pt, int id, float *pos2)
{
    geosurf *gs;
    float x, y, z;
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];

    if (GS_get_selected_point_on_surface(pt[X], pt[Y], &id, &x, &y, &z)) {
	gs = gs_get_surf(id);
	if (gs) {
	    z = gs->zmax;
	    pos2[X] = (float)x - gs->ox + gs->x_trans;
	    pos2[Y] = (float)y - gs->oy + gs->y_trans;
	    pos2[Z] = (float)z + gs->z_trans;

	    return;
	}
    }
    else {
	gs = gs_get_surf(id);

	/* Need to get model matrix, etc 
	 * to run gluUnProject
	 */
	gsd_pushmatrix();
	gsd_do_scale(1);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	if (gs) {
	    GLdouble out_near[3], out_far[3];
	    GLdouble factor;
	    GLdouble out[3];

	    z = (float)gs->zmax + gs->z_trans;

	    gluUnProject((GLdouble) pt[X], (GLdouble) pt[Y], (GLdouble) 0.,
			 modelMatrix, projMatrix, viewport,
			 &out_near[X], &out_near[Y], &out_near[Z]);
	    gluUnProject((GLdouble) pt[X], (GLdouble) pt[Y], (GLdouble) 1.,
			 modelMatrix, projMatrix, viewport,
			 &out_far[X], &out_far[Y], &out_far[Z]);

	    glPopMatrix();

	    factor = (out_near[Z] - z) / (out_near[Z] - out_far[Z]);

	    out[X] = out_near[X] - ((out_near[X] - out_far[X]) * factor);
	    out[Y] = out_near[Y] - ((out_near[Y] - out_far[Y]) * factor);
	    out[Z] = z;

	    pos2[X] = (float)out[X];
	    pos2[Y] = (float)out[Y];
	    pos2[Z] = (float)out[Z];

	    return;

	}
    }
    return;
}

/*!
   \brief Draw place marker

   Used to display query point for raster queries.

   \param id surface id
   \param pt point, X, Y value in true world coordinates
 */
void GS_draw_X(int id, float *pt)
{
    geosurf *gs;
    Point3 pos;
    float siz;
    gvstyle style;

    if ((gs = gs_get_surf(id))) {
	GS_get_longdim(&siz);
	style.size = siz / 200.;
	pos[X] = pt[X] - gs->ox;
	pos[Y] = pt[Y] - gs->oy;
	_viewcell_tri_interp(gs, pos);

	gsd_pushmatrix();

	gsd_do_scale(1);
	gsd_translate(gs->x_trans, gs->y_trans, gs->z_trans);
	gsd_linewidth(1);

	if (CONST_ATT == gs_get_att_src(gs, ATT_TOPO)) {
	    pos[Z] = gs->att[ATT_TOPO].constant;
	    gs = NULL;		/* tells gpd_obj to use given Z val */
	}
	style.color = Gd.bgcol;
	style.symbol = ST_GYRO;
	gpd_obj(gs, &style, pos);
	gsd_flush();

	gsd_popmatrix();
    }

    return;
}

/*!
   \brief Draw line on surface

   \param id surface id
   \param x1,y1,x2,y2 line nodes
 */
void GS_draw_line_onsurf(int id, float x1, float y1, float x2, float y2)
{
    float p1[2], p2[2];
    geosurf *gs;

    if ((gs = gs_get_surf(id))) {
	p1[X] = x1 - gs->ox;
	p1[Y] = y1 - gs->oy;
	p2[X] = x2 - gs->ox;
	p2[Y] = y2 - gs->oy;

	gsd_pushmatrix();

	gsd_do_scale(1);
	gsd_translate(gs->x_trans, gs->y_trans, gs->z_trans);
	gsd_linewidth(1);

	gsd_color_func(GS_default_draw_color());
	gsd_line_onsurf(gs, p1, p2);

	gsd_popmatrix();
	gsd_flush();
    }

    return;
}

/*!
   \brief Draw multiline on surface

   Like above but limits points in line to n or points found in segment,
   whichever is smaller.

   \param id surface id
   \param x1,y1,x2,y2 line nodes

   \return number of points used
 */
int GS_draw_nline_onsurf(int id, float x1, float y1, float x2, float y2,
			 float *lasp, int n)
{
    float p1[2], p2[2];
    geosurf *gs;
    int ret = 0;

    if ((gs = gs_get_surf(id))) {
	p1[X] = x1 - gs->ox;
	p1[Y] = y1 - gs->oy;
	p2[X] = x2 - gs->ox;
	p2[Y] = y2 - gs->oy;

	gsd_pushmatrix();

	gsd_do_scale(1);
	gsd_translate(gs->x_trans, gs->y_trans, gs->z_trans);
	gsd_linewidth(1);
	gsd_color_func(GS_default_draw_color());
	ret = gsd_nline_onsurf(gs, p1, p2, lasp, n);
	gsd_surf2real(gs, lasp);

	gsd_popmatrix();
	gsd_flush();
    }

    return (ret);
}

/*!
   \brief Draw flow-line on surace

   This is slow - should be moved to gs_ but GS_ good for testing
   and useful for app programmer

   \param id surface id
   \param x,y coordinates of flow-line
 */
void GS_draw_flowline_at_xy(int id, float x, float y)
{
    geosurf *gs;
    float nv[3], pdir[2], mult;
    float p1[2], p2[2], next[2];
    int i = 0;

    if ((gs = gs_get_surf(id))) {
	p1[X] = x;
	p1[Y] = y;
	/* multiply by 1.5 resolutions to ensure a crossing ? */
	mult = .1 * (VXRES(gs) > VYRES(gs) ? VXRES(gs) : VYRES(gs));

	GS_coordpair_repeats(p1, p1, 50);

	while (1 == GS_get_norm_at_xy(id, p1[X], p1[Y], nv)) {
	    if (nv[Z] == 1.0) {
		if (pdir[X] == 0.0 && pdir[Y] == 0.0) {
		    break;
		}

		p2[X] = p1[X] + (pdir[X] * mult);
		p2[Y] = p1[Y] + (pdir[Y] * mult);
	    }
	    else {
		/* use previous direction */
		GS_v2norm(nv);
		p2[X] = p1[X] + (nv[X] * mult);
		p2[Y] = p1[Y] + (nv[Y] * mult);
		pdir[X] = nv[X];
		pdir[Y] = nv[Y];
	    }

	    if (i > 2000) {
		break;
	    }

	    if (GS_coordpair_repeats(p1, p2, 0)) {
		break;
	    }

	    /* Think about this: */
	    /* degenerate line means edge or level edge ? */
	    /* next is filled with last point drawn */
	    if (2 > GS_draw_nline_onsurf(id, p1[X], p1[Y],
					 p2[X], p2[Y], next, 3)) {
		break;
	    }

	    p1[X] = next[X];
	    p1[Y] = next[Y];
	}

	G_debug(3, "GS_draw_flowline_at_xy(): dir: %f %f", nv[X], nv[Y]);
    }

    return;
}

/*!
   \brief Draw fringe around data (surface) at selected corners

   \param id surface id
   \param clr color
   \param elev elevation value
   \param where nw/ne/sw/se edges - 0 (turn off) 1 (turn on)
 */
void GS_draw_fringe(int id, unsigned long clr, float elev, int *where)
{
    geosurf *gs;

    G_debug(3, "GS_draw_fringe(): id: %d clr: %ld elev %f edges: %d %d %d %d",
	    id, clr, elev, where[0], where[1], where[2], where[3]);
    if ((gs = gs_get_surf(id)))
	gsd_display_fringe(gs, clr, elev, where);

}


/*!
   \brief Draw legend

   \todo add legend from list option
   make font loading more flexible

   \param name legend name
   \param fontbase font-base
   \param size ? 
   \param flags legend flags
   \param range values range
   \param pt ?
 */
int GS_draw_legend(const char *name, GLuint fontbase, int size, int *flags,
		   float *range, int *pt)
{
    int list_no;

    list_no = gsd_put_legend(name, fontbase, size, flags, range, pt);

    return (list_no);
}

/*!
   \brief Draw pre-defined list

   Uses glFlush() to ensure all drawing is complete
   before returning

   \param list_id list id
 */
void GS_draw_list(GLuint list_id)
{
    gsd_calllist(list_id);
    glFlush();
    return;
}

/*!
   \brief Draw all glLists

   Uses glFlush() to ensure all drawing is complete
   before returning
 */
void GS_draw_all_list(void)
{
    gsd_calllists(0);		/* not sure if 0 is right - MN */
    glFlush();
    return;
}

/*!
   \brief Delete pre-defined list

   \param list_id list id
 */
void GS_delete_list(GLuint list_id)
{
    gsd_deletelist(list_id, 1);

    return;
}

/*!
   \brief Draw lighting model
 */
void GS_draw_lighting_model1(void)
{
    static float center[3];
    float tcenter[3];

    if (!Modelshowing) {
	GS_get_modelposition1(center);
    }

    GS_v3eq(tcenter, center);

    gsd_zwritemask(0x0);
    gsd_backface(1);

    gsd_colormode(CM_AD);
    gsd_shademodel(DM_GOURAUD);
    gsd_pushmatrix();
    gsd_do_scale(1);

    if (Gv.vert_exag) {
	tcenter[Z] *= Gv.vert_exag;
	gsd_scale(1.0, 1.0, 1. / Gv.vert_exag);
    }

    gsd_drawsphere(tcenter, 0xDDDDDD, (float)(Longdim / 10.));
    gsd_popmatrix();
    Modelshowing = 1;

    gsd_backface(0);
    gsd_zwritemask(0xffffffff);

    return;
}

/*!
   \brief Draw lighting model

   Just turn off any cutting planes and draw it just outside near
   clipping plane, since lighting is infinite now
 */
void GS_draw_lighting_model(void)
{
    static float center[3], size;
    float tcenter[3], tsize;
    int i, wason[MAX_CPLANES];

    gsd_get_cplanes_state(wason);

    for (i = 0; i < MAX_CPLANES; i++) {
	if (wason[i]) {
	    gsd_cplane_off(i);
	}
    }


    if (!Modelshowing) {
	GS_get_modelposition(&size, center);
    }

    GS_v3eq(tcenter, center);
    tsize = size;

    gsd_zwritemask(0x0);
    gsd_backface(1);

    gsd_colormode(CM_DIFFUSE);
    gsd_shademodel(DM_GOURAUD);
    gsd_pushmatrix();
    gsd_drawsphere(tcenter, 0xDDDDDD, tsize);
    gsd_popmatrix();
    Modelshowing = 1;

    gsd_backface(0);
    gsd_zwritemask(0xffffffff);

    for (i = 0; i < MAX_CPLANES; i++) {
	if (wason[i]) {
	    gsd_cplane_on(i);
	}
    }

    gsd_flush();

    return;
}

/*!
   \brief Update current mask

   May be called to update total mask for a surface at convenient times
   instead of waiting until ready to redraw surface

   \param id surface id

   \return ?
 */
int GS_update_curmask(int id)
{
    geosurf *gs;

    gs = gs_get_surf(id);
    return (gs_update_curmask(gs));
}

/*!
   \brief Check if point is masked ?

   \param id surface id
   \param pt point

   \return 1 masked
   \return 0 not masked
   \return -1 on error, invalid surface id
 */
int GS_is_masked(int id, float *pt)
{
    geosurf *gs;
    Point3 tmp;

    if ((gs = gs_get_surf(id))) {
	tmp[X] = pt[X] - gs->ox;
	tmp[Y] = pt[Y] - gs->oy;

	return (gs_point_is_masked(gs, tmp));
    }

    return (-1);
}

/*!
   \brief Unset Scaled Difference surface
 */
void GS_unset_SDsurf(void)
{
    gsdiff_set_SDref(NULL);
    SDref_surf = 0;

    return;
}

/*!
   \brief Set surface as Scaled Difference surface

   \param id surface id

   \return 1 on success
   \return 0 on error, invalid surface id
 */
int GS_set_SDsurf(int id)
{
    geosurf *gs;

    if ((gs = gs_get_surf(id))) {
	gsdiff_set_SDref(gs);
	SDref_surf = id;

	return (1);
    }

    return (0);
}

/*!
   \brief Set ?

   \param scale scale value

   \return 1
 */
int GS_set_SDscale(float scale)
{
    gsdiff_set_SDscale(scale);

    return (1);
}

/*!
   \brief Get ?

   \param[out] id ?

   \return 1 on success
   \return 0 on error
 */
int GS_get_SDsurf(int *id)
{
    geosurf *gs;

    if ((gs = gsdiff_get_SDref())) {
	*id = SDref_surf;

	return (1);
    }

    return (0);
}

/*!
   \brief Get ?

   \param[out] scale value

   \return 1
 */
int GS_get_SDscale(float *scale)
{
    *scale = gsdiff_get_SDscale();

    return (1);
}

/*!
   \brief Update normals

   \param id surface id

   \return ?
 */
int GS_update_normals(int id)
{
    geosurf *gs;

    gs = gs_get_surf(id);

    return (gs_calc_normals(gs));
}

/*!
   \brief Get attributes

   \param id surface id
   \param att
   \param[out] set
   \param[out] constant
   \param[out] mapname

   \return 1 on success
   \return -1 on error (invalid surface id)
 */
int GS_get_att(int id, int att, int *set, float *constant, char *mapname)
{
    int src;
    geosurf *gs;

    gs = gs_get_surf(id);
    if (gs) {
	if (-1 != (src = gs_get_att_src(gs, att))) {
	    *set = src;

	    if (src == CONST_ATT) {
		*constant = gs->att[att].constant;
	    }
	    else if (src == MAP_ATT) {
		strcpy(mapname, gsds_get_name(gs->att[att].hdata));
	    }

	    return (1);
	}

	return (-1);
    }

    return (-1);
}

/*!
   \brief Get surface category on given position

   Prints "no data" or a description (i.e., "coniferous forest") to
   <i>catstr</i>. Usually call after GS_get_selected_point_on_surface().
   Define <i>att</i> as MAP_ATT

   \todo Allocate catstr using G_store()
   
   \param id surface id
   \param att attribute id (MAP_ATT)
   \param catstr cat string (must be allocated, dim?)
   \param x,y real coordinates

   \return -1 if no category info or point outside of window
   \return 1 on success
*/
int GS_get_cat_at_xy(int id, int att, char *catstr, float x, float y)
{
    int offset, drow, dcol, vrow, vcol;
    float ftmp, pt[3];
    typbuff *buff;
    geosurf *gs;

    *catstr = '\0';
    gs = gs_get_surf(id);

    if (NULL == gs) {
	return -1;
    }

    pt[X] = x;
    pt[Y] = y;

    gsd_real2surf(gs, pt);
    if (gs_point_is_masked(gs, pt)) {
	return -1;
    }

    if (!in_vregion(gs, pt)) {
	return -1;
    }

    if (MAP_ATT != gs_get_att_src(gs, att)) {
	sprintf(catstr, _("no category info"));
	return -1;
    }

    buff = gs_get_att_typbuff(gs, att, 0);

    vrow = Y2VROW(gs, pt[Y]);
    vcol = X2VCOL(gs, pt[X]);
    drow = VROW2DROW(gs, vrow);
    dcol = VCOL2DCOL(gs, vcol);

    offset = DRC2OFF(gs, drow, dcol);
    
    if (GET_MAPATT(buff, offset, ftmp)) {
	return
	    (Gs_get_cat_label(gsds_get_name(gs->att[att].hdata),
			      drow, dcol, catstr));
    }

    sprintf(catstr, _("no data"));

    return 1;
}

/*!
   \brief Get surface normal at x,y (real coordinates)

   Usually call after GS_get_selected_point_on_surface()

   \param id surface id
   \param x,y real coordinates
   \param[out] nv surface normal

   \return -1 if point outside of window or masked
   \return 1 on success
 */
int GS_get_norm_at_xy(int id, float x, float y, float *nv)
{
    int offset, drow, dcol, vrow, vcol;
    float pt[3];
    geosurf *gs;

    gs = gs_get_surf(id);

    if (NULL == gs) {
	return (-1);
    }

    if (gs->norm_needupdate) {
	gs_calc_normals(gs);
    }

    pt[X] = x;
    pt[Y] = y;

    gsd_real2surf(gs, pt);
    if (gs_point_is_masked(gs, pt)) {
	return (-1);
    }

    if (!in_vregion(gs, pt)) {
	return (-1);
    }

    vrow = Y2VROW(gs, pt[Y]);
    vcol = X2VCOL(gs, pt[X]);
    drow = VROW2DROW(gs, vrow);
    dcol = VCOL2DCOL(gs, vcol);

    offset = DRC2OFF(gs, drow, dcol);

    if (gs->norms) {
	FNORM(gs->norms[offset], nv);
    }
    else {
	/* otherwise must be a constant */
	nv[0] = 0.0;
	nv[1] = 0.0;
	nv[2] = 1.0;
    }

    return (1);
}

/*!
   \brief Get RGB color at given point

   Colors are translated to rgb and returned as Rxxx Gxxx Bxxx Usually
   call after GS_get_selected_point_on_surface().

   Prints NULL or the value (i.e., "921.5") to valstr

   \param id surface id
   \param att attribute id
   \param[out] valstr value string (allocated, dim?)
   \param x,y real coordinates
   
   \return -1 if point outside of window or masked
   \return 1 on success
 */
int GS_get_val_at_xy(int id, int att, char *valstr, float x, float y)
{
    int offset, drow, dcol, vrow, vcol;
    float ftmp, pt[3];
    typbuff *buff;
    geosurf *gs;

    *valstr = '\0';
    gs = gs_get_surf(id);
    
    if (NULL == gs) {
	return -1;
    }

    pt[X] = x;
    pt[Y] = y;

    gsd_real2surf(gs, pt);

    if (gs_point_is_masked(gs, pt)) {
	return -1;
    }

    if (!in_vregion(gs, pt)) {
	return (-1);
    }

    if (CONST_ATT == gs_get_att_src(gs, att)) {
	if (att == ATT_COLOR) {
	    int r, g, b, i;

	    i = gs->att[att].constant;
	    sprintf(valstr, "R%d G%d B%d",
		    INT_TO_RED(i, r), INT_TO_GRN(i, g), INT_TO_BLU(i, b));
	}
	else {
	    sprintf(valstr, "%f", gs->att[att].constant);
	}

	return 1;
    }
    else if (MAP_ATT != gs_get_att_src(gs, att)) {
	return -1;
    }

    buff = gs_get_att_typbuff(gs, att, 0);

    vrow = Y2VROW(gs, pt[Y]);
    vcol = X2VCOL(gs, pt[X]);
    drow = VROW2DROW(gs, vrow);
    dcol = VCOL2DCOL(gs, vcol);

    offset = DRC2OFF(gs, drow, dcol);

    if (GET_MAPATT(buff, offset, ftmp)) {
	if (att == ATT_COLOR) {
	    int r, g, b, i;

	    i = gs_mapcolor(gs_get_att_typbuff(gs, ATT_COLOR, 0),
			    &(gs->att[ATT_COLOR]), offset);
	    sprintf(valstr, "R%d G%d B%d",
		    INT_TO_RED(i, r), INT_TO_GRN(i, g), INT_TO_BLU(i, b));
	}
	else {
	    sprintf(valstr, "%f", ftmp);
	}

	return (1);
    }

    sprintf(valstr, "NULL");

    return (1);
}

/*!
   \brief Unset attribute

   \param id surface id
   \param att attribute id

   \return ?
 */
int GS_unset_att(int id, int att)
{
    geosurf *gs;

    gs = gs_get_surf(id);
    gs->mask_needupdate = 1;

    return (gs_set_att_src(gs, att, NOTSET_ATT));
}

/*!
   \brief Set attribute constant

   \param id surface id
   \param att attribute id
   \param constant value

   \return ?
 */
int GS_set_att_const(int id, int att, float constant)
{
    geosurf *gs;
    int ret;

    gs = gs_get_surf(id);
    ret = (gs_set_att_const(gs, att, constant));

    Gs_update_attrange(gs, att);

    return (ret);
}

/*!
   \brief Set mask mode

   Mask attribute special: constant is set to indicate invert or no

   \param id surface id
   \param mode id

   \return mode id
   \return -1 on error (invalid surface id)
 */
int GS_set_maskmode(int id, int mode)
{
    geosurf *gs;

    gs = gs_get_surf(id);

    if (gs) {
	gs->att[ATT_MASK].constant = mode;
	gs->mask_needupdate = 1;

	return (mode);
    }

    return (-1);
}

/*!
   \brief Get mask mode

   \param id surface id
   \param[out] mode id

   \return 1 on success
   \return -1 on error (invalid surface id)
 */
int GS_get_maskmode(int id, int *mode)
{
    geosurf *gs;

    gs = gs_get_surf(id);

    if (gs) {
	*mode = gs->att[ATT_MASK].constant;

	return (1);
    }

    return (-1);
}

/*!
   \brief Set client data

   \param id surface id
   \param clientd pointer to client data struct

   \return 1 on success
   \return -1 on error (invalid surface id)
 */
int GS_Set_ClientData(int id, void *clientd)
{
    geosurf *gs;

    gs = gs_get_surf(id);
    if (gs) {
	gs->clientdata = clientd;

	return (1);
    }

    return (-1);
}

/*!
   \brief Get client data

   \param id surface id

   \return pointer to client data
   \return NULL on error
 */
void *GS_Get_ClientData(int id)
{
    geosurf *gs;

    gs = gs_get_surf(id);
    if (gs) {
	return (gs->clientdata);
    }

    return (NULL);
}

/*!
   \brief Get number of surfaces

   \return number of surfaces
 */
int GS_num_surfs(void)
{
    return (gs_num_surfaces());
}

/*!
   \brief Get surface list

   Must be freed when not neeed!

   \param[out] numsurf number of available surfaces

   \return pointer to surface array
   \return NULL on error
 */
int *GS_get_surf_list(int *numsurfs)
{
    int i, *ret;

    *numsurfs = Next_surf;

    if (Next_surf) {
	ret = (int *)G_malloc(Next_surf * sizeof(int));

	for (i = 0; i < Next_surf; i++) {
	    ret[i] = Surf_ID[i];
	}

	return (ret);
    }

    return (NULL);
}

/*!
   \brief Delete surface

   \param id surface id

   \return 1 on success
   \return -1 on error
 */
int GS_delete_surface(int id)
{
    int i, j, found;
    
    found = FALSE;
    
    G_debug(1, "GS_delete_surface(): id=%d", id);
    
    if (GS_surf_exists(id)) {
	gs_delete_surf(id);
	for (i = 0; i < Next_surf && !found; i++) {
	    if (Surf_ID[i] == id) {
		found = TRUE;

		for (j = i; j < Next_surf; j++) {
		    Surf_ID[j] = Surf_ID[j + 1];
		}
	    }
	}
	
	gv_update_drapesurfs();

	if (found) {
	    --Next_surf;
	    return 1;
	}
    }

    return -1;
}


/*!
   \brief Load raster map as attribute

   \param id surface id
   \param filename filename
   \param att attribute descriptor

   \return -1 on error (invalid surface id)
   \return ?
 */
int GS_load_att_map(int id, const char *filename, int att)
{
    geosurf *gs;
    unsigned int changed;
    unsigned int atty;
    const char *mapset;
    struct Cell_head rast_head;
    int reuse, begin, hdata, ret, neg, has_null;
    typbuff *tbuff;

    G_debug(3, "GS_load_att_map(): map=%s", filename);

    reuse = ret = neg = has_null = 0;
    gs = gs_get_surf(id);

    if (NULL == gs) {
	return -1;
    }

    gs->mask_needupdate = (ATT_MASK == att || ATT_TOPO == att ||
			   (gs->nz_topo && ATT_TOPO == att) ||
			   (gs->nz_color && ATT_COLOR == att));

    gs_set_att_src(gs, att, MAP_ATT);

    /* Check against maps already loaded in memory   */
    /* if to be color attribute:
       - if packed color for another surface, OK to reuse
       - if unchanged, ok to reuse IF it's of type char (will have lookup)
     */
    begin = hdata = 1;

    /* Get MAPSET to ensure names are fully qualified */
    mapset = G_find_raster2(filename, "");
    if (mapset == NULL) {
	/* Check for valid filename */
	G_warning("Raster map <%s> not found", filename);
	return -1;
    }
    
    /* Check to see if map is in Region */
    Rast_get_cellhd(filename, mapset, &rast_head);
    if (rast_head.north <= wind.south ||
	rast_head.south >= wind.north ||
	rast_head.east <= wind.west || rast_head.west >= wind.east) {

	G_warning(_("Raster map <%s> is outside of current region. Load failed."),
		  G_fully_qualified_name(filename, mapset));
    }

    while (!reuse && (0 < hdata)) {
	changed = CF_COLOR_PACKED;
	atty = ATTY_FLOAT | ATTY_CHAR | ATTY_INT | ATTY_SHORT | ATTY_MASK;

	if (0 < (hdata = gsds_findh(filename, &changed, &atty, begin))) {

	    G_debug(3, "GS_load_att_map(): %s already has data handle %d.CF=%x",
		    filename, hdata, changed);

	    /* handle found */
	    if (ATT_COLOR == att) {
		if ((changed == CF_COLOR_PACKED) ||
		    (!changed && atty == ATTY_CHAR)) {
		    reuse = 1;
		}
	    }
	    else if (atty == ATTY_MASK && att != ATT_MASK) {
		reuse = 0;
		/* should also free mask data & share new - but need backward
		   reference? */
	    }
	    else if (!changed) {
		reuse = 1;
	    }
	}

	begin = 0;
    }

    if (reuse) {
	gs->att[att].hdata = hdata;
	gs_set_att_type(gs, att, atty);	/* ?? */

	/* free lookup  & set to NULL! */
	if (atty == ATTY_INT) {
	    if (gs->att[att].lookup) {
		free(gs->att[att].lookup);
		gs->att[att].lookup = NULL;
	    }
	}
	/* TODO: FIX THIS stuff with lookup sharing! */

	G_debug(3, "GS_load_att_map(): %s is being reused. hdata=%d",
		filename, hdata);
    }
    else {
	G_debug(3, "GS_load_att_map(): %s not loaded in correct form - loading now",
		filename);

	/* not loaded - need to get new dataset handle */
	gs->att[att].hdata = gsds_newh(filename);

	tbuff = gs_get_att_typbuff(gs, att, 1);

	/* TODO: Provide mechanism for loading certain attributes at
	   specified sizes, allow to scale or cap, or scale non-zero */
	if (ATT_MASK == att) {
	    atty = ATTY_MASK;
	}
	else {
	    atty = Gs_numtype(filename, &neg);
	}

#ifdef MAYBE_LATER
	if (att == ATT_COLOR && atty == ATTY_SHORT) {
	    atty = (neg ? ATTY_INT : ATTY_SHORT);
	}
#endif

	if (att == ATT_COLOR && atty == ATTY_SHORT) {
	    atty = ATTY_INT;
	}

	if (0 == gs_malloc_att_buff(gs, att, ATTY_NULL)) {
	    G_fatal_error(_("GS_load_att_map(): Out of memory. Unable to load map"));
	}

	switch (atty) {
	case ATTY_MASK:
	    if (0 == gs_malloc_att_buff(gs, att, ATTY_MASK)) {
		G_fatal_error(_("GS_load_att_map(): Out of memory. Unable to load map"));
	    }

	    ret = Gs_loadmap_as_bitmap(&wind, filename, tbuff->bm);
	    
	    break;
	case ATTY_CHAR:
	    if (0 == gs_malloc_att_buff(gs, att, ATTY_CHAR)) {
		G_fatal_error(_("GS_load_att_map(): Out of memory. Unable to load map"));
	    }

	    ret = Gs_loadmap_as_char(&wind, filename, tbuff->cb,
				     tbuff->nm, &has_null);

	    break;
	case ATTY_SHORT:
	    if (0 == gs_malloc_att_buff(gs, att, ATTY_SHORT)) {
		G_fatal_error(_("GS_load_att_map(): Out of memory. Unable to load map"));
	    }

	    ret = Gs_loadmap_as_short(&wind, filename, tbuff->sb,
				      tbuff->nm, &has_null);
	    break;
	case ATTY_FLOAT:
	    if (0 == gs_malloc_att_buff(gs, att, ATTY_FLOAT)) {
		G_fatal_error(_("GS_load_att_map(): Out of memory. Unable to load map"));
	    }

	    ret = Gs_loadmap_as_float(&wind, filename, tbuff->fb,
				      tbuff->nm, &has_null);

	    break;
	case ATTY_INT:
	default:
	    if (0 == gs_malloc_att_buff(gs, att, ATTY_INT)) {
		G_fatal_error(_("GS_load_att_map(): Out of memory. Unable to load map"));
	    }

	    ret = Gs_loadmap_as_int(&wind, filename, tbuff->ib,
				    tbuff->nm, &has_null);
	    break;

	}			/* Done with switch */

	if (ret == -1) {
	    gsds_free_data_buff(gs->att[att].hdata, ATTY_NULL);
	    return -1;
	}

	G_debug(4, "  has_null=%d", has_null);

	if (!has_null) {
	    gsds_free_data_buff(gs->att[att].hdata, ATTY_NULL);
	}
	else {
	    gs_update_curmask(gs);
	}

    }				/* end if not reuse */

    if (ATT_COLOR == att) {
#ifdef MAYBE_LATER
	if (ATTY_INT == atty) {
	    Gs_pack_colors(filename, tbuff->ib, gs->rows, gs->cols);
	    gsds_set_changed(gs->att[att].hdata, CF_COLOR_PACKED);
	    gs->att[att].lookup = NULL;
	}
	else {
	    gs_malloc_lookup(gs, att);
	    Gs_build_lookup(filename, gs->att[att].lookup);
	}
#else

	if (ATTY_CHAR == atty) {
	    if (!gs->att[att].lookup) {
		/* might already exist if reusing */
		gs_malloc_lookup(gs, att);
		Gs_build_256lookup(filename, gs->att[att].lookup);
	    }
	}
	else if (ATTY_FLOAT == atty) {
	    if (!reuse) {
		if (0 == gs_malloc_att_buff(gs, att, ATTY_INT)) {
		    G_fatal_error(_("GS_load_att_map(): Out of memory. Unable to load map"));
		}

		Gs_pack_colors_float(filename, tbuff->fb, tbuff->ib,
				     gs->rows, gs->cols);
		gsds_set_changed(gs->att[att].hdata, CF_COLOR_PACKED);
		gsds_free_data_buff(gs->att[att].hdata, ATTY_FLOAT);
		gs->att[att].lookup = NULL;
	    }
	}
	else {
	    if (!reuse) {
		Gs_pack_colors(filename, tbuff->ib, gs->rows, gs->cols);
		gsds_set_changed(gs->att[att].hdata, CF_COLOR_PACKED);
		gs->att[att].lookup = NULL;
	    }
	}
#endif
    }

    if (ATT_TOPO == att) {
	gs_init_normbuff(gs);
	/* S_DIFF: should also check here to see if this surface is a
	   reference surface for scaled differences, if so update references
	   to it */
    }

    if (ret < 0) {
	G_warning(_("Loading failed"));
    }

    if (-1 == Gs_update_attrange(gs, att)) {
	G_warning(_("Error finding range"));
    }

    return ret;
}

/*!
   \brief Draw surface

   \param id surface id
 */
void GS_draw_surf(int id)
{
    geosurf *gs;

    G_debug(3, "GS_draw_surf(): id=%d", id);

    gs = gs_get_surf(id);
    if (gs) {
	gsd_shademodel(gs->draw_mode & DM_GOURAUD);

	if (gs->draw_mode & DM_POLY) {
	    gsd_surf(gs);
	}

	if (gs->draw_mode & DM_WIRE) {
	    gsd_wire_surf(gs);
	}

	/* TODO: write wire/poly draw routines */
	if (gs->draw_mode & DM_WIRE_POLY) {
	    gsd_surf(gs);
	    gsd_wire_surf(gs);
	}
    }

    return;
}

/*!
   \brief Draw surface wire

   Overrides draw_mode for fast display

   \param id surface id
 */
void GS_draw_wire(int id)
{
    geosurf *gs;

    G_debug(3, "GS_draw_wire(): id=%d", id);

    gs = gs_get_surf(id);

    if (gs) {
	gsd_wire_surf(gs);
    }

    return;
}

/*!
   \brief Draw all wires

   Overrides draw_mode for fast display
 */
void GS_alldraw_wire(void)
{
    geosurf *gs;
    int i;

    for (i = 0; i < Next_surf; i++) {
	if ((gs = gs_get_surf(Surf_ID[i]))) {
	    gsd_wire_surf(gs);
	}
    }

    return;
}

/*!
   \brief Draw all surfaces
 */
void GS_alldraw_surf(void)
{
    int i;

    for (i = 0; i < Next_surf; i++) {
	GS_draw_surf(Surf_ID[i]);
    }

    return;
}

/*!
   \brief Set Z exag for surface

   \param id surface id
   \param exag z-exag value
 */
void GS_set_exag(int id, float exag)
{
    geosurf *gs;

    G_debug(3, "GS_set_exag");

    gs = gs_get_surf(id);

    if (gs) {
	if (gs->z_exag != exag) {
	    gs->norm_needupdate = 1;
	}

	gs->z_exag = exag;
    }

    return;
}

/*!
   \brief Set global z-exag value

   \param exag exag value to be set up
 */
void GS_set_global_exag(float exag)
{

    G_debug(3, "GS_set_global_exag");

    Gv.vert_exag = exag;
    /* GL_NORMALIZE */
    /* Only need to update norms gs_norms.c
     * if exag is used in norm equation which
     * it is not! If GL_NORMALIZE is disabled
     * will need to include.
     gs_setall_norm_needupdate();
     */

    return;
}

/*!
   \brief Get global z-exag value

   \return value
 */
float GS_global_exag(void)
{
    G_debug(3, "GS_global_exag(): %g", Gv.vert_exag);

    return (Gv.vert_exag);
}

/*!
   \brief Set wire color

   \todo error-handling

   \param id surface id
   \param colr color value
 */
void GS_set_wire_color(int id, int colr)
{
    geosurf *gs;

    G_debug(3, "GS_set_wire_color");

    gs = gs_get_surf(id);

    if (gs) {
	gs->wire_color = colr;
    }

    return;
}

/*!
   \brief Get wire color

   \param id surface id
   \param[out] colr color value

   \return 1 on success
   \return -1 on error
 */
int GS_get_wire_color(int id, int *colr)
{
    geosurf *gs;

    gs = gs_get_surf(id);

    if (gs) {
	*colr = gs->wire_color;

	return (1);
    }

    return (-1);
}

/*!
   \brief Set all draw-modes

   \param mode mode id

   \return 0 on success
   \return -1 on error
 */
int GS_setall_drawmode(int mode)
{
    int i;

    for (i = 0; i < Next_surf; i++) {
	if (0 != GS_set_drawmode(Surf_ID[i], mode)) {
	    return (-1);
	}
    }

    return (0);
}

/*!
   \brief Set draw mode

   \param id surface id
   \param mode mode type(s)

   \return 0 on success
   \return -1 on error (invalid surface id)
 */
int GS_set_drawmode(int id, int mode)
{
    geosurf *gs;

    G_debug(3, "GS_set_drawmode(): id=%d mode=%d", id, mode);

    gs = gs_get_surf(id);

    if (gs) {
	gs->draw_mode = mode;

	return (0);
    }

    return (-1);
}

/*!
   \brief Get draw mode

   \param id surface id
   \param[out] mode mode id

   \return 1 on success
   \return -1 on error (invalid surface id)
 */
int GS_get_drawmode(int id, int *mode)
{
    geosurf *gs;

    gs = gs_get_surf(id);

    if (gs) {
	*mode = gs->draw_mode;

	return (1);
    }

    return (-1);
}

/*!
   \brief Set no-zero ?

   \param id surface id
   \param att attribute id
   \param mode mode id
 */
void GS_set_nozero(int id, int att, int mode)
{
    geosurf *gs;

    G_debug(3, "GS_set_nozero");

    gs = gs_get_surf(id);

    if (gs) {
	if (att == ATT_TOPO) {
	    gs->nz_topo = mode;
	    gs->mask_needupdate = 1;
	}

	if (att == ATT_COLOR) {
	    gs->nz_color = mode;
	    gs->mask_needupdate = 1;
	}
    }

    return;
}

/*!
   \brief Get no-zero ?

   \param id surface id
   \param att attribute id
   \param[out] mode mode id

   \return -1 on error (invalid surface id)
   \return 1 on success
 */
int GS_get_nozero(int id, int att, int *mode)
{
    geosurf *gs;

    G_debug(3, "GS_set_nozero");

    gs = gs_get_surf(id);

    if (gs) {
	if (att == ATT_TOPO) {
	    *mode = gs->nz_topo;
	}
	else if (att == ATT_COLOR) {
	    *mode = gs->nz_color;
	}
	else {
	    return (-1);
	}

	return (1);
    }

    return (-1);
}

/*!
   \brief Set all draw resolutions

   \param xres,yres x/y resolution value
   \param xwire,ywire x/y wire value

   \return 0 on success
   \return -1 on error
 */
int GS_setall_drawres(int xres, int yres, int xwire, int ywire)
{
    int i;

    for (i = 0; i < Next_surf; i++) {
	if (0 != GS_set_drawres(Surf_ID[i], xres, yres, xwire, ywire)) {
	    return (-1);
	}
    }

    return (0);
}

/*!
   \brief Set draw resolution for surface

   \param id surface id
   \param xres,yres x/y resolution value
   \param xwire,ywire x/y wire value

   \return -1 on error
   \return 0 on success
 */
int GS_set_drawres(int id, int xres, int yres, int xwire, int ywire)
{
    geosurf *gs;

    G_debug(3, "GS_set_drawres() id=%d xyres=%d/%d xywire=%d/%d",
	    id, xres, yres, xwire, ywire);

    if (xres < 1 || yres < 1 || xwire < 1 || ywire < 1) {
	return (-1);
    }

    gs = gs_get_surf(id);

    if (gs) {
	if (gs->x_mod != xres || gs->y_mod != yres) {
	    gs->norm_needupdate = 1;
	}

	gs->x_mod = xres;
	gs->y_mod = yres;
	gs->x_modw = xwire;
	gs->y_modw = ywire;
    }

    return (0);
}

/*!
   \brief Get draw resolution of surface

   \param id surface id
   \param[out] xres,yres x/y resolution value
   \param[out] xwire,ywire x/y wire value
 */
void GS_get_drawres(int id, int *xres, int *yres, int *xwire, int *ywire)
{
    geosurf *gs;

    G_debug(3, "GS_get_drawres");

    gs = gs_get_surf(id);

    if (gs) {
	*xres = gs->x_mod;
	*yres = gs->y_mod;
	*xwire = gs->x_modw;
	*ywire = gs->y_modw;
    }

    return;
}

/*!
   \brief Get dimension of surface

   \param id surface id
   \param[out] rows,cols number of rows/cols
 */
void GS_get_dims(int id, int *rows, int *cols)
{
    geosurf *gs;

    gs = gs_get_surf(id);

    if (gs) {
	*rows = gs->rows;
	*cols = gs->cols;
    }

    return;
}

/*!
   \brief Get exag-value guess

   Use no_zero range because if zero IS data, then range won't be that
   much off (it's just a GUESS, after all), but if zero is NO data, could
   drastically affect guess

   \param id surface id
   \param[out] exag exag value

   \return 1 on success
   \return -1 on error
 */
int GS_get_exag_guess(int id, float *exag)
{
    geosurf *gs;
    float guess;

    gs = gs_get_surf(id);
    guess = 1.0;

    /* if gs is type const return guess = 1.0 */
    if (CONST_ATT == gs_get_att_src(gs, ATT_TOPO)) {
	*exag = guess;
	return (1);
    }

    if (gs) {
	if (gs->zrange_nz == 0.0) {
	    *exag = 0.0;

	    return (1);
	}

	G_debug(3, "GS_get_exag_guess(): %f %f", gs->zrange_nz, Longdim);

	while (gs->zrange_nz * guess / Longdim >= .25) {
	    guess *= .1;

	    G_debug(3, "GS_get_exag_guess(): %f", guess);
	}

	while (gs->zrange_nz * guess / Longdim < .025) {
	    guess *= 10.;

	    G_debug(3, "GS_get_exag_guess(): %f", guess);
	}

	*exag = guess;

	return (1);
    }

    return (-1);
}

/*!
   \brief Get Z extents for all loaded surfaces

   Treating zeros as "no data"

   \param[out] min min value
   \param[out] max max value
 */
void GS_get_zrange_nz(float *min, float *max)
{
    int i, first = 1;
    geosurf *gs;

    for (i = 0; i < Next_surf; i++) {
	if ((gs = gs_get_surf(Surf_ID[i]))) {
	    if (first) {
		first = 0;
		*min = gs->zmin_nz;
		*max = gs->zmax_nz;
	    }

	    if (gs->zmin_nz < *min) {
		*min = gs->zmin_nz;
	    }

	    if (gs->zmax_nz > *max) {
		*max = gs->zmax_nz;
	    }
	}
    }

    G_debug(3, "GS_get_zrange_nz(): min=%g max=%g", *min, *max);

    return;
}

/*!
   \brief Set translation (surface position)

   \param id surface id
   \param xtrans,ytrans,ztrans translation values
 */
void GS_set_trans(int id, float xtrans, float ytrans, float ztrans)
{
    geosurf *gs;

    gs = gs_get_surf(id);

    if (gs) {
	gs->x_trans = xtrans;
	gs->y_trans = ytrans;
	gs->z_trans = ztrans;
    }

    G_debug(3, "GS_set_trans(): id=%d, x=%f, y=%f, z=%f",
	    id, xtrans, ytrans, ztrans);

    return;
}

/*!
   \brief Get translation values (surface position)

   \param id surface id
   \param[out] xtrans,ytrans,ztrans trans values
 */
void GS_get_trans(int id, float *xtrans, float *ytrans, float *ztrans)
{
    geosurf *gs;

    gs = gs_get_surf(id);

    if (gs) {
	*xtrans = gs->x_trans;
	*ytrans = gs->y_trans;
	*ztrans = gs->z_trans;
    }

    G_debug(3, "GS_get_trans: id=%d, x=%f, y=%f, z=%f",
	    id, *xtrans, *ytrans, *ztrans);

    return;
}


/*!
   \brief Get default draw color

   \return color value
 */
unsigned int GS_default_draw_color(void)
{

    G_debug(3, "GS_default_draw_color");

    return ((unsigned int)Gd.bgcol);
}

/*!
   \brief Get background color

   \return color value
 */
unsigned int GS_background_color(void)
{
    return ((unsigned int)Gd.bgcol);
}

/*!
   \brief Sets which buffer to draw to

   \param where GSD_BOTH, GSD_FRONT, GSD_BACK
 */
void GS_set_draw(int where)
{
    Buffermode = where;

    switch (where) {
    case GSD_BOTH:
	gsd_frontbuffer(1);
	gsd_backbuffer(1);

	break;
    case GSD_FRONT:
	gsd_frontbuffer(1);
	gsd_backbuffer(0);

	break;
    case GSD_BACK:
    default:
	gsd_frontbuffer(0);
	gsd_backbuffer(1);

	break;
    }

    return;
}

/*
   \brief Ready to draw
 */
void GS_ready_draw(void)
{

    G_debug(3, "GS_ready_draw");

    gsd_set_view(&Gv, &Gd);

    return;
}

/*!
   \brief Draw done, swap buffers
 */
void GS_done_draw(void)
{

    G_debug(3, "GS_done_draw");

    if (GSD_BACK == Buffermode) {
	gsd_swapbuffers();
    }

    gsd_flush();

    return;
}

/*!
   \brief Set focus

   \param realto real coordinates to
 */
void GS_set_focus(float *realto)
{

    G_debug(3, "GS_set_focus(): %f,%f,%f", realto[0], realto[1], realto[2]);

    Gv.infocus = 1;
    GS_v3eq(Gv.real_to, realto);

    gsd_set_view(&Gv, &Gd);

    return;
}

/*!
   \brief Set real focus

   \param realto real coordinates to
 */
void GS_set_focus_real(float *realto)
{

    G_get_set_window(&wind);
    realto[X] = realto[X] - wind.west - (wind.ew_res / 2.);
    realto[Y] = realto[Y] - wind.south - (wind.ns_res / 2.);

    Gv.infocus = 1;
    GS_v3eq(Gv.real_to, realto);

    gsd_set_view(&Gv, &Gd);

    return;
}


/*!
   \brief Get focus

   OK to call with NULL argument if just want to check state

   \param realto real coordinates to

   \return ?
 */
int GS_get_focus(float *realto)
{

    G_debug(3, "GS_get_focus");

    if (Gv.infocus) {
	if (realto) {
	    GS_v3eq(realto, Gv.real_to);
	}
    }

    return (Gv.infocus);
}

/*!
   \brief Set focus to map center

   \param id surface id
 */
void GS_set_focus_center_map(int id)
{
    float center[3];
    geosurf *gs;

    G_debug(3, "GS_set_focus_center_map");

    gs = gs_get_surf(id);

    if (gs) {
	center[X] = (gs->xmax - gs->xmin) / 2.;
	center[Y] = (gs->ymax - gs->ymin) / 2.;
	center[Z] = (gs->zmax_nz + gs->zmin_nz) / 2.;

	/* not yet working
	   buff = gs_get_att_typbuff(gs, ATT_TOPO, 0);
	   offset = gs->rows*gs->cols/2 + gs->cols/2;
	   if (buff)
	   {
	   if (GET_MAPATT(buff, offset, tmp))
	   {
	   center[Z] = tmp;
	   }
	   }
	 */

	GS_set_focus(center);
    }
}

/*!
   \brief Move viewpoint 

   \param pt 'from' model coordinates
 */
void GS_moveto(float *pt)
{
    float ft[3];

    G_debug(3, "GS_moveto(): %f,%f,%f", pt[0], pt[1], pt[2]);

    if (Gv.infocus) {
	GS_v3eq(Gv.from_to[FROM], pt);
	/*
	   GS_v3eq(Gv.from_to[TO], Gv.real_to);
	 */
	GS_v3normalize(Gv.from_to[FROM], Gv.from_to[TO]);
	/* update inclination, look_dir if we're keeping these */
    }
    else {
	GS_v3eq(ft, Gv.from_to[TO]);
	GS_v3sub(ft, Gv.from_to[FROM]);
	GS_v3eq(Gv.from_to[FROM], pt);
	GS_v3eq(Gv.from_to[TO], pt);
	GS_v3add(Gv.from_to[TO], ft);
    }

    return;
}

/*!
   \brief Move position to (real)

   \param pt point real coordinates
 */
void GS_moveto_real(float *pt)
{
    gsd_real2model(pt);
    GS_moveto(pt);

    return;
}

/*!
   \brief Get z-extent for a single surface

   \param id surface id
   \param[out] min min z-value
   \param[out] max max z-value
   \param[out] mid middle z-value

   \return -1 on error (invalid surface id)
   \return ?
 */
int GS_get_zextents(int id, float *min, float *max, float *mid)
{
    geosurf *gs;

    if (NULL == (gs = gs_get_surf(id))) {
	return (-1);
    }

    G_debug(3, "GS_get_zextents(): id=%d", id);

    return (gs_get_zextents(gs, min, max, mid));
}

/*!
   \brief Get z-extent for all loaded surfaces

   \param[out] min min z-value
   \param[out] max max z-value
   \param doexag use z-exaggeration

   \return 1 on success
   \return -1 on error
 */
int GS_get_zrange(float *min, float *max, int doexag)
{
    int ret_surf, ret_vol;
    float surf_min, surf_max;
    float vol_min, vol_max;

    ret_surf = gs_get_zrange(&surf_min, &surf_max);
    ret_vol = gvl_get_zrange(&vol_min, &vol_max);

    if (ret_surf > 0 && ret_vol > 0) {
	*min = (surf_min < vol_min) ? surf_min : vol_min;
	*max = (surf_max < vol_max) ? surf_max : vol_max;
    }
    else if (ret_surf > 0) {
	*min = surf_min;
	*max = surf_max;
    }
    else if (ret_vol > 0) {
	*min = vol_min;
	*max = vol_max;
    }

    if (doexag) {
	*min *= Gv.vert_exag;
	*max *= Gv.vert_exag;
    }

    G_debug(3, "GS_get_zrange(): min=%g max=%g", *min, *max);
    return ((ret_surf > 0 || ret_vol > 0) ? (1) : (-1));
}

/*!
   \brief Get viewpoint 'from' position

   \param[out] fr from model coordinates
 */
void GS_get_from(float *fr)
{
    GS_v3eq(fr, Gv.from_to[FROM]);

    G_debug(3, "GS_get_from(): %f,%f,%f", fr[0], fr[1], fr[2]);

    return;
}

/*!
   \brief Get viewpoint 'from' real coordinates

   \param[out] fr 'from' real coordinates
 */
void GS_get_from_real(float *fr)
{
    GS_v3eq(fr, Gv.from_to[FROM]);
    gsd_model2real(fr);

    return;
}

/*!
   \brief Get 'to' real coordinates

   \param[out] to 'to' real coordinates
 */
void GS_get_to_real(float *to)
{
    float realto[3];

    G_get_set_window(&wind);
    GS_get_focus(realto);
    to[X] = realto[X] + wind.west + (wind.ew_res / 2.);
    to[Y] = realto[Y] + wind.south + (wind.ns_res / 2.);
    to[Z] = realto[Z];

    return;
}


/*!
   \brief Get zoom setup

   \param[out] a,b,c,d current viewport settings
   \param[out] maxx,maxy max viewport size
 */
void GS_zoom_setup(int *a, int *b, int *c, int *d, int *maxx, int *maxy)
{
    GLint tmp[4];
    GLint num[2];

    gsd_getViewport(tmp, num);
    *a = tmp[0];
    *b = tmp[1];
    *c = tmp[2];
    *d = tmp[3];
    *maxx = num[0];
    *maxy = num[1];

    return;
}

/*!
   \brief Get 'to' model coordinates

   \todo need set_to? - just use viewdir?

   \param[out] to 'to' model coordinates
 */
void GS_get_to(float *to)
{
    G_debug(3, "GS_get_to");

    GS_v3eq(to, Gv.from_to[TO]);

    return;
}

/*!
   \brief Get viewdir

   \param[out] dir viewdir value
 */
void GS_get_viewdir(float *dir)
{
    GS_v3dir(Gv.from_to[FROM], Gv.from_to[TO], dir);

    return;
}

/*!
   \brief Set viewdir

   Automatically turns off focus

   \param dir viewdir value
 */
void GS_set_viewdir(float *dir)
{
    float tmp[3];

    GS_v3eq(tmp, dir);
    GS_v3norm(tmp);
    GS_v3eq(Gv.from_to[TO], Gv.from_to[FROM]);
    GS_v3add(Gv.from_to[TO], tmp);

    GS_set_nofocus();
    gsd_set_view(&Gv, &Gd);

    return;
}

/*!
   \brief Set field of view

   \param fov fov value
 */
void GS_set_fov(int fov)
{
    Gv.fov = fov;

    return;
}

/*!
   \brief Get fied of view

   \return field of view, in 10ths of degrees
 */
int GS_get_fov(void)
{
    return (Gv.fov);
}

/*!
   \brief Get twist value

   10ths of degrees off twelve o'clock
 */
int GS_get_twist(void)
{
    return (Gv.twist);
}

/*!
   \brief Set viewpoint twist value

   10ths of degrees off twelve o'clock

   \param t tenths of degrees clockwise from 12:00.
 */
void GS_set_twist(int t)
{
    Gv.twist = t;

    return;
}

/*!
   \brief Set rotation params
 */
void GS_set_rotation(double angle, double x, double y, double z)
{
    Gv.rotate.rot_angle = angle;
    Gv.rotate.rot_axes[0] = x;
    Gv.rotate.rot_axes[1] = y;
    Gv.rotate.rot_axes[2] = z;
    Gv.rotate.do_rot = 1;

    return;
}

/*!
   \brief Stop scene rotation
 */
void GS_unset_rotation(void)
{
    Gv.rotate.do_rot = 0;
}

/*!
   \brief Reset scene rotation
 */
void GS_init_rotation(void)
{
    int i;

    for (i = 0; i < 16; i++) {
	if (i == 0 || i == 5 || i == 10 || i == 15)
	    Gv.rotate.rotMatrix[i] = 1.0;
	else
	    Gv.rotate.rotMatrix[i] = 0.0;
    }
    Gv.rotate.rot_angle = 0.0;
    Gv.rotate.rot_axes[0] = 0.0;
    Gv.rotate.rot_axes[1] = 0.0;
    Gv.rotate.rot_axes[2] = 0.0;
    Gv.rotate.do_rot = 0;
    
}
/*!
 * \brief Get rotation matrix
 */ 
void GS_get_rotation_matrix(double *matrix)
{
    int i;

    for (i = 0; i < 16; i++) {
	matrix[i] = Gv.rotate.rotMatrix[i];
    }
}

/*!
 * \brief Set rotation matrix
 */ 
void GS_set_rotation_matrix(double *matrix)
{
    int i;

    for (i = 0; i < 16; i++) {
	Gv.rotate.rotMatrix[i] = matrix[i];
    }
}

/*!
   \brief Unset focus
 */
void GS_set_nofocus(void)
{
    G_debug(3, "GS_set_nofocus");

    Gv.infocus = 0;

    return;
}

/*!
   \brief Set focus

   Make sure that the center of view is set
 */
void GS_set_infocus(void)
{
    G_debug(3, "GS_set_infocus");

    Gv.infocus = 1;

    return;
}

/*!
   \brief Set viewport

   \param left,right,bottom,top viewport extent values
 */
void GS_set_viewport(int left, int right, int bottom, int top)
{
    G_debug(3, "GS_set_viewport(): left=%d, right=%d, "
	    "bottom=%d, top=%d", left, right, bottom, top);

    gsd_viewport(left, right, bottom, top);

    return;
}

/*!
   \brief Send screen coords sx and sy, lib traces through surfaces; sets
   new center to point of nearest intersection.

   If no intersection, uses line of sight with length of current view
   ray (eye to center) to set new center.

   Reset center of view to screen coordinates sx, sy.

   \param sx,sy screen coordinates

   \return 1 on success
   \return 0 on error (invalid surface id)
 */
int GS_look_here(int sx, int sy)
{
    float x, y, z, len, los[2][3];
    Point3 realto, dir;
    int id;
    geosurf *gs;

    if (GS_get_selected_point_on_surface(sx, sy, &id, &x, &y, &z)) {
	gs = gs_get_surf(id);
	if (gs) {
	    realto[X] = x - gs->ox + gs->x_trans;
	    realto[Y] = y - gs->oy + gs->y_trans;
	    realto[Z] = z + gs->z_trans;
	    GS_set_focus(realto);

	    return (1);
	}
    }
    else {
	if (gsd_get_los(los, (short)sx, (short)sy)) {
	    len = GS_distance(Gv.from_to[FROM], Gv.real_to);
	    GS_v3dir(los[FROM], los[TO], dir);
	    GS_v3mult(dir, len);
	    realto[X] = Gv.from_to[FROM][X] + dir[X];
	    realto[Y] = Gv.from_to[FROM][Y] + dir[Y];
	    realto[Z] = Gv.from_to[FROM][Z] + dir[Z];
	    GS_set_focus(realto);

	    return (1);
	}
    }

    return (0);
}

/*!
   \brief Get selected point of surface

   Given screen coordinates sx and sy, find closest intersection of
   view ray with surfaces and return coordinates of intersection in x, y,
   z, and identifier of surface in id.

   \param sx,sy screen coordinates
   \param[out] id surface id
   \param[out] x,y,z point on surface (model coordinates?)

   \returns 0 if no intersections found
   \return number of intersections
 */
int GS_get_selected_point_on_surface(int sx, int sy, int *id, float *x,
				     float *y, float *z)
{
    float los[2][3], find_dist[MAX_SURFS], closest;
    Point3 point, tmp, finds[MAX_SURFS];
    int surfs[MAX_SURFS], i, iclose, numhits = 0;
    geosurf *gs;

    /* returns surface-world coords */
    gsd_get_los(los, (short)sx, (short)sy);

    if (!gs_setlos_enterdata(los)) {
	G_debug(3, "gs_setlos_enterdata(los): returns false");
	return (0);
    }

    for (i = 0; i < Next_surf; i++) {
	G_debug(3, "id=%d", i);

	gs = gs_get_surf(Surf_ID[i]);

	/* los_intersect expects surf-world coords (xy transl, no scaling) */

#if NVIZ_HACK
	if (gs_los_intersect1(Surf_ID[i], los, point)) {
#else
	if (gs_los_intersect(Surf_ID[i], los, point)) {
#endif
	    if (!gs_point_is_masked(gs, point)) {
		GS_v3eq(tmp, point);
		tmp[X] += gs->x_trans;
		tmp[Y] += gs->y_trans;
		tmp[Z] += gs->z_trans;
		find_dist[numhits] = GS_distance(los[FROM], tmp);
		gsd_surf2real(gs, point);
		GS_v3eq(finds[numhits], point);
		surfs[numhits] = Surf_ID[i];
		numhits++;
	    }
	}
    }

    for (i = iclose = 0; i < numhits; i++) {
	closest = find_dist[iclose];

	if (find_dist[i] < closest) {
	    iclose = i;
	}
    }

    if (numhits) {
	*x = finds[iclose][X];
	*y = finds[iclose][Y];
	*z = finds[iclose][Z];
	*id = surfs[iclose];
    }

    G_debug(3, "NumHits %d, next %d", numhits, Next_surf);

    return (numhits);
}

/*!
   \brief Set cplace rotation

   \param num cplace id
   \param dx,dy,dz rotation values
 */
void GS_set_cplane_rot(int num, float dx, float dy, float dz)
{
    gsd_cplane_setrot(num, dx, dy, dz);

    return;
}

/*!
   \brief Set cplace trans

   \param num cplace id
   \param dx,dy,dz rotation values
 */
void GS_set_cplane_trans(int num, float dx, float dy, float dz)
{
    gsd_cplane_settrans(num, dx, dy, dz);

    return;
}


/*!
   \brief Draw cplace

   \param num cplace id
 */
void GS_draw_cplane(int num)
{
    geosurf *gsurfs[MAX_SURFS];
    int nsurfs;

    nsurfs = gs_num_surfaces();
    if (2 == nsurfs) {
	/* testing */
	gs_getall_surfaces(gsurfs);
	gsd_draw_cplane_fence(gsurfs[0], gsurfs[1], num);
    }
    else {
	gsd_draw_cplane(num);
    }

    return;
}

/*!
   \brief Draw cplace fence ?

   \param hs1,hs2
   \param num cplane id

   \return 0 on error
   \return 1 on success
 */
int GS_draw_cplane_fence(int hs1, int hs2, int num)
{
    geosurf *gs1, *gs2;

    if (NULL == (gs1 = gs_get_surf(hs1))) {
	return (0);
    }

    if (NULL == (gs2 = gs_get_surf(hs2))) {
	return (0);
    }

    gsd_draw_cplane_fence(gs1, gs2, num);

    return (1);
}

/*!
   \brief Draw all cplace fences ?
 */
void GS_alldraw_cplane_fences(void)
{
    int onstate[MAX_CPLANES], i;

    gsd_get_cplanes_state(onstate);

    for (i = 0; i < MAX_CPLANES; i++) {
	if (onstate[i]) {
	    GS_draw_cplane_fence(Surf_ID[0], Surf_ID[1], i);
	}
    }

    return;
}

/*!
   \brief Set cplace

   \param num cplane id
 */
void GS_set_cplane(int num)
{
    gsd_cplane_on(num);

    return;
}

/*!
   \brief Unset clip place (turn off)

   \param num cplane id
 */
void GS_unset_cplane(int num)
{
    gsd_cplane_off(num);

    return;
}

/*!
   \brief Get axis scale

   \param sx,sy,sz x/y/z scale values
   \param doexag use vertical exaggeration
 */
void GS_get_scale(float *sx, float *sy, float *sz, int doexag)
{
    float zexag;

    zexag = doexag ? Gv.vert_exag : 1.;
    *sx = *sy = Gv.scale;
    *sz = Gv.scale * zexag;

    return;
}

/*!
   \brief Set fence color

   \param mode mode id
 */
void GS_set_fencecolor(int mode)
{
    gsd_setfc(mode);

    return;
}

/*!
   \brief Get fence color

   \return color value
 */
int GS_get_fencecolor(void)
{
    return gsd_getfc();
}

/*!
   \brief Measure distance "as the ball rolls" between two points on
   surface

   \param hs surface id
   \param x1,y1,x2,y2 two points on surface
   \param[out] dist measured distance
   \param use_exag use exag. surface

   \return 0 on error or if one or more points is not in region
   \return distance following terrain
 */
int GS_get_distance_alongsurf(int hs, float x1, float y1, float x2, float y2,
			      float *dist, int use_exag)
{
    geosurf *gs;
    float p1[2], p2[2];
    
    gs = gs_get_surf(hs);
    if (gs == NULL) {
	return 0;
    }
    
    p1[X] = x1;
    p1[Y] = y1;
    p2[X] = x2;
    p2[Y] = y2;
    gsd_real2surf(gs, p1);
    gsd_real2surf(gs, p2);

    G_debug(3, "GS_get_distance_alongsurf(): hs=%d p1=%f,%f p2=%f,%f",
	    hs, x1, y1, x2, y2);
    return gs_distance_onsurf(gs, p1, p2, dist, use_exag);
}

/*!
   \brief Save 3d view

   \param vname view file name
   \param surfid surface id

   \return ?
 */
int GS_save_3dview(const char *vname, int surfid)
{
    return (Gs_save_3dview(vname, &Gv, &Gd, &wind, gs_get_surf(surfid)));
}

/*!
   \brief Load 3d view

   \param vname view file name
   \param surfid surface id

   \return ?
 */
int GS_load_3dview(const char *vname, int surfid)
{

    return (Gs_load_3dview(vname, &Gv, &Gd, &wind, gs_get_surf(surfid)));

    /* what to do about lights - I guess, delete all &
       create any that exist in 3dview file */
}

/************************************************************************
* Following routines use Graphics Library
************************************************************************/

/*!
   \brief Init viewpoint

   \todo allow to set center?
 */
void GS_init_view(void)
{
    static int first = 1;

    G_debug(3, "GS_init_view");

    if (first) {
	first = 0;
	glMatrixMode(GL_MODELVIEW);

	/* OGLXXX doublebuffer: use GLX_DOUBLEBUFFER in attriblist */
	/* glxChooseVisual(*dpy, screen, *attriblist); */
	/* OGLXXX
	 * ZMIN not needed -- always 0.
	 * ZMAX not needed -- always 1.
	 * getgdesc other posiblilties:
	 *      glxGetConfig();
	 *      glxGetCurrentContext();
	 *      glxGetCurrentDrawable();
	 * GLint gdtmp;
	 * getgdesc other posiblilties:
	 *      glxGetConfig();
	 *      glxGetCurrentContext();
	 *      glxGetCurrentDrawable();
	 * GLint gdtmp;
	 * glDepthRange params must be scaled to [0, 1]
	 */
	glDepthRange(0.0, 1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	/* } */

	/* replace these with something meaningful */
	Gv.fov = 450;
	Gv.twist = 0;

	GS_init_rotation();

	Gv.from_to[FROM][X] = Gv.from_to[FROM][Y] =
	    Gv.from_to[FROM][Z] = GS_UNIT_SIZE / 2.;

	Gv.from_to[TO][X] = GS_UNIT_SIZE / 2.;
	Gv.from_to[TO][Y] = GS_UNIT_SIZE / 2.;
	Gv.from_to[TO][Z] = 0.;
	Gv.from_to[TO][W] = Gv.from_to[FROM][W] = 1.;

	Gv.real_to[W] = 1.;
	Gv.vert_exag = 1.;

	GS_v3eq(Gv.real_to, Gv.from_to[TO]);
	GS_v3normalize(Gv.from_to[FROM], Gv.from_to[TO]);

	/*
	   Gd.nearclip = 50;
	   Gd.farclip = 10000.;
	 */
	Gd.nearclip = 10.;
	Gd.farclip = 10000.;
	Gd.aspect = (float)GS_get_aspect();

	GS_set_focus(Gv.real_to);
    }

    return;
}

/*!
   \brief Clear view

   \param col color value
 */
void GS_clear(int col)
{
    G_debug(3, "GS_clear");

    col = col | 0xFF000000;

    /* OGLXXX
     * change glClearDepth parameter to be in [0, 1]
     * ZMAX not needed -- always 1.
     * getgdesc other posiblilties:
     *      glxGetConfig();
     *      glxGetCurrentContext();
     *      glxGetCurrentDrawable();
     * GLint gdtmp;
     */
    glClearDepth(1.0);
    glClearColor(((float)((col) & 0xff)) / 255.,
		 (float)((col) >> 8 & 0xff) / 255.,
		 (float)((col) >> 16 & 0xff) / 255.,
		 (float)((col) >> 24 & 0xff) / 255.);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    Gd.bgcol = col;
    Modelshowing = 0;
    gsd_flush();

    return;
}

/*!
   \brief Get aspect value

   \return aspect value
 */
double GS_get_aspect(void)
{
    int left, right, bottom, top;
    GLint tmp[4];

    /* OGLXXX
     * get GL_VIEWPORT:
     * You can probably do better than this.
     */
    glGetIntegerv(GL_VIEWPORT, tmp);
    left = tmp[0];
    right = tmp[0] + tmp[2] - 1;
    bottom = tmp[1];
    top = tmp[1] + tmp[3] - 1;

    G_debug(3, "GS_get_aspect(): left=%d, right=%d, top=%d, bottom=%d",
	    left, right, top, bottom);

    return ((double)(right - left) / (top - bottom));
}

/*!
   \brief Check for transparency

   Disabled.

   \return 1
 */
int GS_has_transparency(void)
{
    /* OGLXXX
     * getgdesc other posiblilties:
     *      glxGetConfig();
     *      glxGetCurrentContext();
     *      glxGetCurrentDrawable();
     * GLint gdtmp;
     * blending is ALWAYS supported.
     * This function returns whether it is enabled.
     * return((glGetIntegerv(GL_BLEND, &gdtmp), gdtmp));
     */

    return (1);
}
