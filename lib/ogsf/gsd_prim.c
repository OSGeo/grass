/*
    $Id$

    gsd_prim.c
    Bill Brown, USACERL  
    January 1993
    Primitive drawing functions
*/

/* DEBUG */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/config.h>

#if defined(OPENGL_X11) || defined(OPENGL_WINDOWS)
#include <GL/gl.h>
#include <GL/glu.h>
#elif defined(OPENGL_AQUA)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include <grass/gis.h>
#include <grass/gstypes.h>

#define USE_GL_NORMALIZE

#define RED_MASK 0x000000FF
#define GRN_MASK 0x0000FF00
#define BLU_MASK 0x00FF0000
#define ALP_MASK 0xFF000000

#define INT_TO_RED(i, r)    (r = (i & RED_MASK))
#define INT_TO_GRN(i, g)    (g = (i & GRN_MASK) >> 8)
#define INT_TO_BLU(i, b)    (b = (i & BLU_MASK) >> 16)
#define INT_TO_ALP(i, a)    (a = (i & ALP_MASK) >> 24)

#define MAX_OBJS 64
/* ^ TMP - move to gstypes */

/* define border width (pixels) for viewport check */
#define border 15

static GLuint ObjList[MAX_OBJS];
static int numobjs = 0;

static int Shade;

static float ogl_light_amb[MAX_LIGHTS][4];
static float ogl_light_diff[MAX_LIGHTS][4];
static float ogl_light_spec[MAX_LIGHTS][4];
static float ogl_light_pos[MAX_LIGHTS][4];
static float ogl_mat_amb[4];
static float ogl_mat_diff[4];
static float ogl_mat_spec[4];
static float ogl_mat_emis[4];
static float ogl_mat_shin;

/************************************************************************/
/* Mostly for flushing drawing commands accross a network - glFlush
*  doesn't block, so if blocking is desired use glFinish.
*/
void gsd_flush(void)
{
    glFlush();

    return;
}

/************************************************************************/
/* Call glColorMaterial before enabling the GL_COLOR_MATERIAL */
void gsd_colormode(int cm)
{
    switch (cm) {
    case CM_COLOR:

	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);

	break;
    case CM_EMISSION:

	glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);

	break;
    case CM_DIFFUSE:

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);

	break;
    case CM_AD:

	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);

	break;
    case CM_NULL:

	/* OGLXXX
	   * lmcolor: if LMC_NULL,  use:
	   * glDisable(GL_COLOR_MATERIAL);
	   * LMC_NULL: use glDisable(GL_COLOR_MATERIAL);
	 */
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);

	break;
    default:

	glDisable(GL_COLOR_MATERIAL);
	break;
    }

    return;
}

/************************************************************************/
void show_colormode(void)
{
    GLint mat;

    glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER, &mat);
    fprintf(stderr, "Color Material: %d\n", mat);

    return;
}

/************************************************************************/
void gsd_circ(float x, float y, float rad)
{
    GLUquadricObj *qobj = gluNewQuadric();
    gluQuadricDrawStyle(qobj, GLU_SILHOUETTE);
    glPushMatrix();
    glTranslatef(x, y, 0.);
    gluDisk(qobj, 0., rad, 32, 1);
    glPopMatrix();
    gluDeleteQuadric(qobj);

    return;
}

/************************************************************************/
void gsd_disc(float x, float y, float z, float rad)
{
    GLUquadricObj *qobj = gluNewQuadric();
    gluQuadricDrawStyle(qobj, GLU_FILL);
    glPushMatrix();
    glTranslatef(x, y, z);
    gluDisk(qobj, 0., rad, 32, 1);
    glPopMatrix();
    gluDeleteQuadric(qobj);

    return;
}

/************************************************************************/
void gsd_sphere(float *center, float siz)
{
    static int first = 1;
    static GLUquadricObj *QOsphere;

    if (first) {
	QOsphere = gluNewQuadric();

	if (QOsphere) {
	    gluQuadricNormals(QOsphere, GLU_SMOOTH);	/* default */
	    gluQuadricTexture(QOsphere, GL_FALSE);	/* default */
	    gluQuadricOrientation(QOsphere, GLU_OUTSIDE);	/* default */
	    gluQuadricDrawStyle(QOsphere, GLU_FILL);
	}

	first = 0;
    }

    glPushMatrix();
    glTranslatef(center[0], center[1], center[2]);
    gluSphere(QOsphere, (double) siz, 24, 24);
    glPopMatrix();

    return;
}

/************************************************************************/
void gsd_zwritemask(unsigned long n)
{
    /* OGLXXX glDepthMask is boolean only */
    glDepthMask((GLboolean) (n));

    return;
}

/************************************************************************/
void gsd_backface(int n)
{
    glCullFace(GL_BACK);
    (n) ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);

    return;
}

/************************************************************************/
void gsd_linewidth(short n)
{
    glLineWidth((GLfloat) (n));

    return;
}

/************************************************************************/
void gsd_bgnqstrip(void)
{
    glBegin(GL_QUAD_STRIP);

    return;
}

/************************************************************************/
void gsd_endqstrip(void)
{
    glEnd();

    return;
}

/************************************************************************/
void gsd_bgntmesh(void)
{
    glBegin(GL_TRIANGLE_STRIP);

    return;
}

/************************************************************************/
void gsd_endtmesh(void)
{
    glEnd();

    return;
}

/************************************************************************/
void gsd_bgntstrip(void)
{
    glBegin(GL_TRIANGLE_STRIP);

    return;
}

/************************************************************************/
void gsd_endtstrip(void)
{
    glEnd();

    return;
}

/************************************************************************/
void gsd_bgntfan(void)
{
    glBegin(GL_TRIANGLE_FAN);

    return;
}

/************************************************************************/
void gsd_endtfan(void)
{
    glEnd();

    return;
}

/************************************************************************/
void gsd_swaptmesh(void)
{
    /* OGLXXX
     * swaptmesh not supported, maybe glBegin(GL_TRIANGLE_FAN)
     * swaptmesh()
     */

     /*DELETED*/;

    return;
}

/************************************************************************/
void gsd_bgnpolygon(void)
{
    /* OGLXXX
     * special cases for polygons:
     *  independant quads: use GL_QUADS
     *  independent triangles: use GL_TRIANGLES
     */
    glBegin(GL_POLYGON);

    return;
}

/************************************************************************/
void gsd_endpolygon(void)
{
    glEnd();

    return;
}

/************************************************************************/
void gsd_bgnline(void)
{
    /* OGLXXX for multiple, independent line segments: use GL_LINES */
    glBegin(GL_LINE_STRIP);
    return;
}

/************************************************************************/
void gsd_endline(void)
{
    glEnd();

    return;
}

/************************************************************************/
void gsd_shademodel(int bool)
{
    Shade = bool;

    if (bool) {
	glShadeModel(GL_SMOOTH);
    }
    else {
	glShadeModel(GL_FLAT);
    }

    return;
}

/************************************************************************/
int gsd_getshademodel(void)
{
    return (Shade);
}

/************************************************************************/
void gsd_bothbuffer(void)
{
    /* OGLXXX frontbuffer: other possibilities include GL_FRONT_AND_BACK */
    glDrawBuffer(GL_FRONT_AND_BACK);

    return;
}

/************************************************************************/
void gsd_frontbuffer(int bool)
{
    /* OGLXXX frontbuffer: other possibilities include GL_FRONT_AND_BACK */
    glDrawBuffer((bool) ? GL_FRONT : GL_BACK);

    return;
}

/************************************************************************/
void gsd_backbuffer(int bool)
{
    /* OGLXXX backbuffer: other possibilities include GL_FRONT_AND_BACK */
    glDrawBuffer((bool) ? GL_BACK : GL_FRONT);
    return;
}

/************************************************************************/
void gsd_swapbuffers(void)
{
    /* OGLXXX swapbuffers: 
       glXSwapBuffers(*display, window);
       replace display and window */

    Swap_func();

    return;
}

/************************************************************************/
void gsd_popmatrix(void)
{
    glPopMatrix();

    return;
}

/************************************************************************/
void gsd_pushmatrix(void)
{
    glPushMatrix();

    return;
}

/************************************************************************/
void gsd_scale(float xs, float ys, float zs)
{
    glScalef(xs, ys, zs);

    return;
}

/************************************************************************/
void gsd_translate(float dx, float dy, float dz)
{
    glTranslatef(dx, dy, dz);

    return;
}

/************************************************************************/
void gsd_getwindow(int *window, int *viewport, double *modelMatrix,
		   double *projMatrix)
{
    gsd_pushmatrix();
    gsd_do_scale(1);

    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);
    gsd_popmatrix();

    window[0] = viewport[1] + viewport[3] + border;
    window[1] = viewport[1] - border;
    window[2] = viewport[0] - border;
    window[3] = viewport[0] + viewport[2] + border;

    return;

}

/************************************************************************/
int gsd_checkpoint(float pt[4],
		   int window[4],
		   int viewport[4],
		   double modelMatrix[16], double projMatrix[16])
{
    GLdouble fx, fy, fz;

    gluProject((GLdouble) pt[X], (GLdouble) pt[Y], (GLdouble) pt[Z],
	       modelMatrix, projMatrix, viewport, &fx, &fy, &fz);

    if (fx < window[2] || fx > window[3]
	|| fy < window[1] || fy > window[0])
	return 1;
    else
	return 0;

}

/************************************************************************/
void gsd_rot(float angle, char axis)
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    char wrnMsg[512];

    switch (axis) {
    case 'x':
    case 'X':

	x = 1.0;
	y = 0.0;
	z = 0.0;

	break;
    case 'y':
    case 'Y':

	x = 0.0;
	y = 1.0;
	z = 0.0;

	break;
    case 'z':
    case 'Z':

	x = 0.0;
	y = 0.0;
	z = 1.0;

	break;
    default:

	sprintf(wrnMsg, "gsd_rot(): %c is an invalid axis ", axis);
	strcat(wrnMsg, "specification. Rotation ignored\n");
	strcat(wrnMsg, "Please advise GRASS developers of this error.\n");

	G_warning(wrnMsg);
	return;
    }

    glRotatef((GLfloat) angle, x, y, z);

    return;
}

/************************************************************************/
void gsd_litvert_func(float *norm, unsigned long col, float *pt)
{
    glNormal3fv(norm);
    gsd_color_func(col);
    glVertex3fv(pt);

    return;
}

/************************************************************************/
void gsd_litvert_func2(float *norm, unsigned long col, float *pt)
{
    glNormal3fv(norm);
    glVertex3fv(pt);

    return;
}

/************************************************************************/
void gsd_vert_func(float *pt)
{
    glVertex3fv(pt);

    return;
}

/************************************************************************/
void gsd_color_func(unsigned int col)
{
    GLbyte r, g, b, a;

    /* OGLXXX
     * cpack: if argument is not a variable
     * might need to be:
     *  glColor4b(($1)&0xff, ($1)>>8&0xff, ($1)>>16&0xff, ($1)>>24&0xff)
     */
    INT_TO_RED(col, r);
    INT_TO_GRN(col, g);
    INT_TO_BLU(col, b);
    INT_TO_ALP(col, a);
    glColor4ub(r, g, b, a);

    return;
}

/************************************************************************/
void gsd_init_lightmodel(void)
{

    glEnable(GL_LIGHTING);

    /* normal vector renormalization */
#ifdef USE_GL_NORMALIZE
    {
	glEnable(GL_NORMALIZE);
    }
#endif

    /* OGLXXX
     * Ambient:
     *  If this is a light model lmdef, then use 
     *      glLightModelf and GL_LIGHT_MODEL_AMBIENT.
     *      Include ALPHA parameter with ambient
     */

    /* Default is front face lighting, infinite viewer
     */
    ogl_mat_amb[0] = 0.1;
    ogl_mat_amb[1] = 0.1;
    ogl_mat_amb[2] = 0.1;
    ogl_mat_amb[3] = 1.0;

    ogl_mat_diff[0] = 0.8;
    ogl_mat_diff[1] = 0.8;
    ogl_mat_diff[2] = 0.8;
    ogl_mat_diff[3] = 0.8;

    ogl_mat_spec[0] = 0.8;
    ogl_mat_spec[1] = 0.8;
    ogl_mat_spec[2] = 0.8;
    ogl_mat_spec[3] = 0.8;

    ogl_mat_emis[0] = 0.0;
    ogl_mat_emis[1] = 0.0;
    ogl_mat_emis[2] = 0.0;
    ogl_mat_emis[3] = 0.0;

    ogl_mat_shin = 25.0;

    /* OGLXXX
     * attenuation: see glLightf man page: (ignored for infinite lights)
     * Add GL_LINEAR_ATTENUATION.
     sgi_lmodel[0] = GL_CONSTANT_ATTENUATION;
     sgi_lmodel[1] = 1.0;
     sgi_lmodel[2] = 0.0;
     sgi_lmodel[3] = ;
     */

    /* OGLXXX
     * lmdef other possibilities include:
     *  glLightf(light, pname, *params);
     *  glLightModelf(pname, param);
     * Check list numbering.
     * Translate params as needed.
     */
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ogl_mat_amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ogl_mat_diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ogl_mat_spec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ogl_mat_emis);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, ogl_mat_shin);

    /* OGLXXX lmbind: check object numbering. */
    /* OGLXXX
     * lmbind: check object numbering.
     * Use GL_FRONT in call to glMaterialf.
     * Use GL_FRONT in call to glMaterialf.
     if(1) {glCallList(1); glEnable(LMODEL);} else glDisable(LMODEL);
     if(1) {glCallList(1); glEnable(GL_FRONT);} else glDisable(GL_FRONT);
     */


    return;
}

/************************************************************************/
/* set_shin, set_emis  flags */
/* sh, em    sh & em should be 0. - 1. */
/* emcolor   packed colors to use for emission */
void gsd_set_material(int set_shin, int set_emis, float sh, float em,
		      int emcolor)
{
    if (set_shin) {
	ogl_mat_spec[0] = sh;
	ogl_mat_spec[1] = sh;
	ogl_mat_spec[2] = sh;
	ogl_mat_spec[3] = sh;

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ogl_mat_spec);

	ogl_mat_shin = 60. + (int) (sh * 68.);

	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, ogl_mat_shin);
    }

    if (set_emis) {
	ogl_mat_emis[0] = (em * (emcolor & 0x0000FF)) / 255.;
	ogl_mat_emis[1] = (em * ((emcolor & 0x00FF00) >> 8)) / 255.;
	ogl_mat_emis[2] = (em * ((emcolor & 0xFF0000) >> 16)) / 255.;

	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ogl_mat_emis);
    }

    return;
}

/************************************************************************/
void gsd_deflight(int num, struct lightdefs *vals)
{
    if (num > 0 && num <= MAX_LIGHTS) {
	ogl_light_pos[num - 1][0] = vals->position[X];
	ogl_light_pos[num - 1][1] = vals->position[Y];
	ogl_light_pos[num - 1][2] = vals->position[Z];
	ogl_light_pos[num - 1][3] = vals->position[W];

	glLightfv(GL_LIGHT0 + num, GL_POSITION, ogl_light_pos[num - 1]);

	ogl_light_diff[num - 1][0] = vals->color[0];
	ogl_light_diff[num - 1][1] = vals->color[1];
	ogl_light_diff[num - 1][2] = vals->color[2];
	ogl_light_diff[num - 1][3] = .3;

	glLightfv(GL_LIGHT0 + num, GL_DIFFUSE, ogl_light_diff[num - 1]);

	ogl_light_amb[num - 1][0] = vals->ambient[0];
	ogl_light_amb[num - 1][1] = vals->ambient[1];
	ogl_light_amb[num - 1][2] = vals->ambient[2];
	ogl_light_amb[num - 1][3] = .3;

	glLightfv(GL_LIGHT0 + num, GL_AMBIENT, ogl_light_amb[num - 1]);

	ogl_light_spec[num - 1][0] = vals->color[0];
	ogl_light_spec[num - 1][1] = vals->color[1];
	ogl_light_spec[num - 1][2] = vals->color[2];
	ogl_light_spec[num - 1][3] = .3;

	glLightfv(GL_LIGHT0 + num, GL_SPECULAR, ogl_light_spec[num - 1]);
    }

    return;
}

/************************************************************************/
/* on = 0 turns them off */
void gsd_switchlight(int num, int on)
{
    short defin;

    defin = on ? num : 0;

    if (defin) {
	glEnable(GL_LIGHT0 + num);
    }
    else {
	glDisable(GL_LIGHT0 + num);
    }

    return;
}

/************************************************************************/
int gsd_getimage(unsigned char **pixbuf, unsigned int *xsize,
		 unsigned int *ysize)
{
    GLuint l, r, b, t;

    /* OGLXXX
     * get GL_VIEWPORT:
     * You can probably do better than this.
     */
    GLint tmp[4];

    glGetIntegerv(GL_VIEWPORT, tmp);
    l = tmp[0];
    r = tmp[0] + tmp[2] - 1;
    b = tmp[1];
    t = tmp[1] + tmp[3] - 1;

    *xsize = r - l + 1;
    *ysize = t - b + 1;

    *pixbuf = malloc(*xsize * *ysize * 4);

    if (!*pixbuf)
	return (0);

    glReadBuffer(GL_FRONT);

    /* OGLXXX lrectread: see man page for glReadPixels */
    glReadPixels(l, b, (r) - (l) + 1, (t) - (b) + 1, GL_RGBA,
		 GL_UNSIGNED_BYTE, *pixbuf);
    return (1);
}

/************************************************************************/
int gsd_getViewport(GLint tmp[4], GLint num[2])
{

    /* Save current viewport to tmp */
    glGetIntegerv(GL_VIEWPORT, tmp);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, num);

    return (1);
}


/************************************************************************/
int gsd_writeView(unsigned char **pixbuf, unsigned int xsize,
		  unsigned int ysize)
{

    /* Malloc Buffer for image */
    *pixbuf = malloc(xsize * ysize * 4);
    if (!*pixbuf) {
	fprintf(stderr, "MALLOC Failed\n");
	return (0);
    }

    /* Read image buffer */
    glReadBuffer(GL_FRONT);

    /* Read Pixels into Buffer */
    glReadPixels(0, 0, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, *pixbuf);
    return (1);
}

/************************************************************************/
void gsd_blend(int yesno)
{
    if (yesno) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);
    }

    return;
}

/************************************************************************/
void gsd_def_clipplane(int num, double *params)
{
    int wason = 0;

    /* OGLXXX see man page for glClipPlane equation */
    if (glIsEnabled(GL_CLIP_PLANE0 + (num))) {
	wason = 1;
    }

    glClipPlane(GL_CLIP_PLANE0 + (num), params);

    if (wason) {
	glEnable(GL_CLIP_PLANE0 + (num));
    }
    else {
	glDisable(GL_CLIP_PLANE0 + (num));
    }

    return;
}

/************************************************************************/
void gsd_set_clipplane(int num, int able)
{
    /* OGLXXX see man page for glClipPlane equation */
    if (able) {
	glEnable(GL_CLIP_PLANE0 + (num));
    }
    else {
	glDisable(GL_CLIP_PLANE0 + (num));
    }

    return;
}

/************************************************************************/
/* does nothing */
/* only called from src.contrib/GMSL/NVIZ2.2/src/glwrappers.c */
void gsd_finish(void)
{
    return;
}

/************************************************************************/
void gsd_viewport(int l, int r, int b, int t)
{
    /* Screencoord */
    glViewport(l, b, r, t);

    return;
}

/************************************************************************/
/* first time called, gets a bunch of objects, then hands them back
 * when needed
*/
int gsd_makelist(void)
{
    int i;

    if (numobjs) {
	if (numobjs < MAX_OBJS) {
	    numobjs++;

	    return (numobjs);
	}

	return (-1);
    }
    else {
	ObjList[0] = glGenLists(MAX_OBJS);

	for (i = 1; i < MAX_OBJS; i++) {
	    ObjList[i] = ObjList[0] + i;
	}
	numobjs = 1;

	return (numobjs);
    }

}

/************************************************************************/
void gsd_bgnlist(int listno, int do_draw)
{
    if (do_draw) {
	glNewList(ObjList[listno], GL_COMPILE_AND_EXECUTE);
    }
    else {
	glNewList(ObjList[listno], GL_COMPILE);
    }

    return;
}

/************************************************************************/
void gsd_endlist(void)
{
    glEndList();

    return;
}

/*****************************************************************/
void gsd_deletelist(GLuint listno, int range)
{
    int i;

    for (i = 1; i < MAX_OBJS; i++) {
	if (i == listno) {
	    glDeleteLists(ObjList[i], 1);
	    numobjs--;
	    if (numobjs < 1)
		numobjs = 1;
	    return;
	}
    }
}

/************************************************************************/
void gsd_calllist(int listno)
{
    glCallList(ObjList[listno]);

    return;
}


/************************************************************************/
void gsd_calllists(int listno)
{
    int i;

    gsd_pushmatrix();
    for (i = 1; i < MAX_OBJS; i++) {
	glCallList(ObjList[i]);
	glFlush();
    }
    gsd_popmatrix();

    gsd_call_label();
    return;
}
