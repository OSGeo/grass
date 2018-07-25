/*!
   \file lib/ogsf/gsd_views.c

   \brief OGSF library - manipulating views (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL (January 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <grass/config.h>

#if defined(OPENGL_X11) || defined(OPENGL_WINDOWS)
#include <GL/gl.h>
#include <GL/glu.h>
#elif defined(OPENGL_AQUA)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include <grass/ogsf.h>
#include "math.h"

/*!
   \brief ADD

   \param vect
   \param sx, sy  screen coordinates

   \return 1
 */
int gsd_get_los(float (*vect)[3], short sx, short sy)
{
    double fx, fy, fz, tx, ty, tz;
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];

    GS_ready_draw();
    glPushMatrix();

    gsd_do_scale(1);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glPopMatrix();

    /* OGLXXX XXX I think this is backwards gluProject(XXX); */
    /* WAS: mapw(Vobj, sx, sy, &fx, &fy, &fz, &tx, &ty, &tz); */
    gluUnProject((GLdouble) sx, (GLdouble) sy, 0.0, modelMatrix,
		 projMatrix, viewport, &fx, &fy, &fz);
    gluUnProject((GLdouble) sx, (GLdouble) sy, 1.0, modelMatrix,
		 projMatrix, viewport, &tx, &ty, &tz);
    vect[FROM][X] = fx;
    vect[FROM][Y] = fy;
    vect[FROM][Z] = fz;
    vect[TO][X] = tx;
    vect[TO][Y] = ty;
    vect[TO][Z] = tz;

    /* DEBUG - should just be a dot */
    /* OGLXXX frontbuffer: other possibilities include GSD_BOTH */
    GS_set_draw((1) ? GSD_FRONT : GSD_BACK);
    glPushMatrix();
    gsd_do_scale(1);
    gsd_linewidth(3);
    gsd_color_func(0x8888FF);

    /* OGLXXX for multiple, independent line segments: use GL_LINES */
    glBegin(GL_LINE_STRIP);
    glVertex3fv(vect[FROM]);
    glVertex3fv(vect[TO]);
    glEnd();

    gsd_linewidth(1);
    glPopMatrix();

    /* OGLXXX frontbuffer: other possibilities include GSD_BOTH */
    GS_set_draw((0) ? GSD_FRONT : GSD_BACK);

    return (1);
}

#if 0
/*!
   \brief Set view

   Establishes viewing & projection matrices

   \param gv view (geoview)
   \param dp display (geodisplay)
 */
void gsd_set_view(geoview * gv, geodisplay * gd)
{
    double up[3];
    GLint mm;

    /* will expand when need to check for in focus, ortho, etc. */

    gsd_check_focus(gv);
    gsd_get_zup(gv, up);

    gd->aspect = GS_get_aspect();

    glGetIntegerv(GL_MATRIX_MODE, &mm);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective((double).1 * (gv->fov), (double)gd->aspect,
		   (double)gd->nearclip, (double)gd->farclip);

    glMatrixMode(mm);

    glLoadIdentity();

    /* update twist parm */
    glRotatef((float)(gv->twist / 10.), 0.0, 0.0, 1.0);

    /* OGLXXX lookat: replace UPx with vector */
    gluLookAt((double)gv->from_to[FROM][X], (double)gv->from_to[FROM][Y],
	      (double)gv->from_to[FROM][Z], (double)gv->from_to[TO][X],
	      (double)gv->from_to[TO][Y], (double)gv->from_to[TO][Z],
	      (double)up[X], (double)up[Y], (double)up[Z]);

    /* have to redefine clipping planes when view changes */

    gsd_update_cplanes();

    return;
}
#endif
/*!
   \brief Set view

   Establishes viewing & projection matrices

   \param gv view (geoview)
   \param dp display (geodisplay)
 */
void gsd_set_view(geoview * gv, geodisplay * gd)
{
    double up[3];
    float pos[3];
    int i;
    GLdouble modelMatrix[16];
    GLint mm;

    /* will expand when need to check for in focus, ortho, etc. */

    gsd_check_focus(gv);
    gsd_get_zup(gv, up);

    gd->aspect = GS_get_aspect();

    glGetIntegerv(GL_MATRIX_MODE, &mm);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective((double).1 * (gv->fov), (double)gd->aspect,
		   (double)gd->nearclip, (double)gd->farclip);

    glMatrixMode(mm);
    
    glLoadIdentity();

    /* update twist parm */
    glRotatef((float)(gv->twist / 10.), 0.0, 0.0, 1.0);

    /* OGLXXX lookat: replace UPx with vector */
    gluLookAt((double)gv->from_to[FROM][X], (double)gv->from_to[FROM][Y],
	      (double)gv->from_to[FROM][Z], (double)gv->from_to[TO][X],
	      (double)gv->from_to[TO][Y], (double)gv->from_to[TO][Z],
	      (double)up[X], (double)up[Y], (double)up[Z]);
	      
    /* rotate to get rotation matrix and then save it*/
    if (gv->rotate.do_rot) {

	glPushMatrix();
	glLoadMatrixd(gv->rotate.rotMatrix);

	glRotated(gv->rotate.rot_angle, gv->rotate.rot_axes[0], 
		  gv->rotate.rot_axes[1], gv->rotate.rot_axes[2]);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	for (i = 0; i < 16; i++) {
	    gv->rotate.rotMatrix[i] = modelMatrix[i];
	}

	glPopMatrix();
    }
    
    gs_get_datacenter(pos);
    gsd_surf2model(pos);
    /* translate rotation center to view center, rotate and translate back */
    glTranslatef(pos[0], pos[1], pos[2]);
    glMultMatrixd(gv->rotate.rotMatrix);
    glTranslatef(-pos[0], -pos[1], -pos[2]);

    /* have to redefine clipping planes when view changes */

    gsd_update_cplanes();

    return;
}
/*!
   \brief Check focus

   \param gv view (geoview)
 */
void gsd_check_focus(geoview * gv)
{
    float zmax, zmin;

    GS_get_zrange(&zmin, &zmax, 0);

    if (gv->infocus) {
	GS_v3eq(gv->from_to[TO], gv->real_to);
	gv->from_to[TO][Z] -= zmin;
	GS_v3mult(gv->from_to[TO], gv->scale);
	gv->from_to[TO][Z] *= gv->vert_exag;

	GS_v3normalize(gv->from_to[FROM], gv->from_to[TO]);
    }

    return;
}

/*!
   \brief Get z-up vector (z-direction)

   \param gv view (geoview)
   \param up up vector
 */
void gsd_get_zup(geoview * gv, double *up)
{
    float alpha;
    float zup[3], fup[3];

    /* neg alpha OK since sin(-x) = -sin(x) */
    alpha =
	(2.0 * atan(1.0)) - acos(gv->from_to[FROM][Z] - gv->from_to[TO][Z]);

    zup[X] = gv->from_to[TO][X];
    zup[Y] = gv->from_to[TO][Y];

    if (sin(alpha)) {
	zup[Z] = gv->from_to[TO][Z] + 1 / sin(alpha);
    }
    else {
	zup[Z] = gv->from_to[FROM][Z] + 1.0;
    }

    GS_v3dir(gv->from_to[FROM], zup, fup);

    up[X] = fup[X];
    up[Y] = fup[Y];
    up[Z] = fup[Z];

    return;
}

/*!
   \brief ADD

   \param gv view (geoview)

   \return ?
 */
int gsd_zup_twist(geoview * gv)
{
    float fr_to[2][4];
    float look_theta, pi;
    float alpha, beta;
    float zup[3], yup[3], zupmag, yupmag;

    pi = 4.0 * atan(1.0);

    /* *************************************************************** */
    /* This block of code is used to keep pos z in the up direction,
     * correcting for SGI system default which is pos y in the up
     * direction.  Involves finding up vectors for both y up and
     * z up, then determining angle between them.  LatLon mode uses y as
     * up direction instead of z, so no correction necessary.  Next rewrite,
     * we should use y as up for all drawing.
     */
    GS_v3eq(fr_to[FROM], gv->from_to[FROM]);
    GS_v3eq(fr_to[TO], gv->from_to[TO]);

    /* neg alpha OK since sin(-x) = -sin(x) */
    alpha = pi / 2.0 - acos(fr_to[FROM][Z] - fr_to[TO][Z]);

    zup[X] = fr_to[TO][X];
    zup[Y] = fr_to[TO][Y];

    if (sin(alpha)) {
	zup[Z] = fr_to[TO][Z] + 1 / sin(alpha);
    }
    else {
	zup[Z] = fr_to[FROM][Z] + 1.0;
    }

    zupmag = GS_distance(fr_to[FROM], zup);

    yup[X] = fr_to[TO][X];
    yup[Z] = fr_to[TO][Z];

    /* neg beta OK since sin(-x) = -sin(x) */
    beta = pi / 2.0 - acos(fr_to[TO][Y] - fr_to[FROM][Y]);

    if (sin(beta)) {
	yup[Y] = fr_to[TO][Y] - 1 / sin(beta);
    }
    else {
	yup[Y] = fr_to[FROM][Y] + 1.0;
    }

    yupmag = GS_distance(fr_to[FROM], yup);

    look_theta = (1800.0 / pi) *
	acos(((zup[X] - fr_to[FROM][X]) * (yup[X] - fr_to[FROM][X])
	      + (zup[Y] - fr_to[FROM][Y]) * (yup[Y] - fr_to[FROM][Y])
	      + (zup[Z] - fr_to[FROM][Z]) * (yup[Z] - fr_to[FROM][Z])) /
	     (zupmag * yupmag));

    if (fr_to[TO][X] - fr_to[FROM][X] < 0.0) {
	look_theta = -look_theta;
    }

    if (fr_to[TO][Z] - fr_to[FROM][Z] < 0.0) {
	/* looking down */
	if (fr_to[TO][Y] - fr_to[FROM][Y] < 0.0) {
	    look_theta = 1800 - look_theta;
	}
    }
    else {
	/* looking up */
	if (fr_to[TO][Y] - fr_to[FROM][Y] > 0.0) {
	    look_theta = 1800 - look_theta;
	}
    }

    return ((int)(gv->twist + 1800 + look_theta));
}

/*!
   \brief Set current scale

   \param doexag use z-exaggeration
 */
void gsd_do_scale(int doexag)
{
    float sx, sy, sz;
    float min, max;

    GS_get_scale(&sx, &sy, &sz, doexag);
    gsd_scale(sx, sy, sz);
    GS_get_zrange(&min, &max, 0);
    gsd_translate(0.0, 0.0, -min);

    return;
}

/*!
   \brief Convert real to model coordinates

   \param point[in,out] 3d point (Point3)
 */
void gsd_real2model(Point3 point)
{
    float sx, sy, sz;
    float min, max, n, s, w, e;

    GS_get_region(&n, &s, &w, &e);
    GS_get_scale(&sx, &sy, &sz, 1);
    GS_get_zrange(&min, &max, 0);
    point[X] = (point[X] - w) * sx;
    point[Y] = (point[Y] - s) * sy;
    point[Z] = (point[Z] - min) * sz;

    return;
}

/*!
   \brief Convert model to real coordinates

   \param point[in,out] 3d point (x,y,z)
 */
void gsd_model2real(Point3 point)
{
    float sx, sy, sz;
    float min, max, n, s, w, e;

    GS_get_region(&n, &s, &w, &e);
    GS_get_scale(&sx, &sy, &sz, 1);
    GS_get_zrange(&min, &max, 0);
    point[X] = (sx ? point[X] / sx : 0.0) + w;
    point[Y] = (sy ? point[Y] / sy : 0.0) + s;
    point[Z] = (sz ? point[Z] / sz : 0.0) + min;

    return;
}

/*!
   \brief Convert model to surface coordinates

   \param gs surface (geosurf)
   \param point 3d point (Point3)
 */
void gsd_model2surf(geosurf * gs, Point3 point)
{
    float min, max, sx, sy, sz;

    /* so far, only one geographic "region" allowed, so origin of
       surface is same as origin of model space, but will need to provide 
       translations here to make up the difference, so not using gs yet */

    if (gs) {
	/* need to undo z scaling & translate */
	GS_get_scale(&sx, &sy, &sz, 1);
	GS_get_zrange(&min, &max, 0);

	point[Z] = (sz ? point[Z] / sz : 0.0) + min;

	/* need to unscale x & y */
	point[X] = (sx ? point[X] / sx : 0.0);
	point[Y] = (sy ? point[Y] / sy : 0.0);
    }

    return;
}
/*!
   \brief Convert surface to model coordinates

   \param point 3d point (Point3)
 */
void gsd_surf2model(Point3 point)
{
    float min, max, sx, sy, sz;

    /* need to undo z scaling & translate */
    GS_get_scale(&sx, &sy, &sz, 1);
    GS_get_zrange(&min, &max, 0);

    point[Z] = (sz ? (point[Z] - min) * sz : 0.0);

    /* need to unscale x & y */
    point[X] = (sx ? point[X] * sx : 0.0);
    point[Y] = (sy ? point[Y] * sy : 0.0);


    return;
}
/*!
   \brief Convert surface to real coordinates

   \param gs surface (geosurf)
   \param[in,out] point 3d point (Point3)
 */
void gsd_surf2real(geosurf * gs, Point3 point)
{
    if (gs) {
	point[X] += gs->ox;
	point[Y] += gs->oy;
    }

    return;
}

/*!
   \brief Convert real to surface coordinates

   \param gs surface (geosurf)
   \param[in,out] point 3d point (Point3)
 */
void gsd_real2surf(geosurf * gs, Point3 point)
{
    if (gs) {
	point[X] -= gs->ox;
	point[Y] -= gs->oy;
    }

    return;
}
