/*
   do_zoom.c --

   This file contains routines to dump series of enlarged 
   images to ppm 
 */

/* gsf includes */

#define USE_GL_NORMALIZE

#include <grass/config.h>

/* Nvision includes */
#include "interface.h"

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OPENGL_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmu/Xmu.h>
#include <GL/glx.h>

static Display *dpy;
static int scr;
static GLXContext ctx;

#ifdef HAVE_PBUFFERS
static GLXPbuffer pbuffer;
#endif
#ifdef HAVE_PIXMAPS
Pixmap pixmap;
static GLXPixmap glxpixmap;
#endif
#endif /* OPENGL_X11 */

extern void swap_togl(void);

static int init_ctx(void);

/**********************************************/
int Nstart_zoom_cmd(Nv_data * data,	/* Local data */
		    Tcl_Interp * interp,	/* Current interpreter */
		    int argc,	/* Number of arguments */
		    char **argv	/* Argument strings */
    )
{
    int a, b, c, d;
    int a_orig, b_orig, c_orig, d_orig;
    int maxx, maxy;
    int img_width, img_height;
    int row, col, i, j, k, m;
    int XX, YY, var_i;
    int cnt = 1;
    double aspect;
    char pref[64], filename[1024], cmd[1024], cmd2[1024];
    char inform_text[128];

#if defined(OPENGL_X11) && (defined(HAVE_PBUFFERS) || defined(HAVE_PIXMAPS))
    int os_w;
    int os_h;
#endif

    /* Parse arguments */
    if (argc != 4) {
	interp->result =
	    "Error: should be Nstart_zoom file_name width height";
	return (TCL_ERROR);
    }

    /* Get Filename */
    strcpy(pref, argv[1]);

    /* get display aspect */
    GS_zoom_setup(&a_orig, &b_orig, &c_orig, &d_orig, &maxx, &maxy);
    aspect = (double)(c_orig - a_orig) / (d_orig - b_orig);

    /* create off-screen context if possible */
#if defined(OPENGL_X11) && (defined(HAVE_PBUFFERS) || defined(HAVE_PIXMAPS))
    os_w = atoi(argv[2]);
    os_h = atoi(argv[3]);

    if (aspect > 1.) {		/* width is greater than height */
	if (((os_w / aspect) - (int)(os_w / aspect)) > .5)
	    os_h = (int)(os_w / aspect) + 1;
	else
	    os_h = (os_w / aspect);
    }
    else {
	if (((os_h * aspect) - (int)(os_h * aspect)) > .5)
	    os_w = (int)(os_h * aspect) + 1;
	else
	    os_w = (int)(os_h * aspect);
    }

    Create_OS_Ctx(os_w, os_h);
#endif

    GS_zoom_setup(&a, &b, &c, &d, &maxx, &maxy);

    /* Calculate Maximum possible output size */
    if (aspect > 1.) {
	if (((maxx / aspect) - (int)(maxx / aspect)) > .5)
	    maxy = (int)(maxx / aspect) + 1;
	else
	    maxy = (int)(maxx / aspect);
	var_i = (maxx + (c - a) - 1) / (c - a);
    }
    else {
	if (((maxy * aspect) - (int)(maxy * aspect)) > .5)
	    maxx = (int)(maxy * aspect) + 1;
	else
	    maxx = (int)(maxy * aspect);
	var_i = (maxy + (d - b) - 1) / (d - b);
    }

    fprintf(stderr, "Final Assembled Image will be %d x %d\n", maxx, maxy);

    /* Set X & Y to zero for lower left corner */
    XX = YY = 0;
    /* Set output image width & height */
    img_width = c;
    img_height = d;

    /* Cycle through tiles according to Zoom factor */

    for (row = 1; row <= var_i; row++) {
	for (col = 1; col <= var_i; col++) {
	    GS_set_viewport(XX, maxx, YY, maxy);
	    Ndraw_all_together_cmd(data, interp, argc, argv);
	    sprintf(filename, "%s_%d_%d.ppm", pref, row, col);
	    /* Re-set image width or height if required */
	    if ((maxx + XX) < c)
		img_width = maxx + XX;
	    if ((maxy + YY) < d)
		img_height = maxy + YY;
	    /* Save tile to file */
	    sprintf(inform_text, "inform \"Writing Tile %d of %d\"",
		    cnt, (var_i * var_i));
	    Tcl_Eval(interp, inform_text);
	    fprintf(stderr, "Writing Tile %d of %d\n", cnt, (var_i * var_i));
	    GS_write_zoom(filename, img_width, img_height);

	    XX -= c;
	    cnt++;
	}			/* Done col */
	/* Reset XX and img_width */
	XX = 0;
	img_width = c;
	YY -= d;
    }				/* done row */

    /* Done writing ppm tiles */


    /* Cat ppm tiles together */
    sprintf(inform_text, "inform \"Assembling Tiles\"");
    Tcl_Eval(interp, inform_text);
    fprintf(stderr, "Assembling Tiles\n");
    strcpy(cmd2, "pnmcat -tb ");
    k = var_i;
    for (i = 1; i <= var_i; i++) {
	strcpy(cmd, "pnmcat -lr ");
	for (j = 1; j <= var_i; j++) {
	    sprintf(filename, "%s_%d_%d.ppm ", pref, i, j);
	    strcat(cmd, filename);
	}
	sprintf(filename, "> %stmp%d.ppm", pref, i);
	strcat(cmd, filename);
	sprintf(filename, "%stmp%d.ppm ", pref, k);
	strcat(cmd2, filename);
	if (system(cmd) != 0) {
	    fprintf(stderr, "pnmcat failed to create assembled image\n");
	    fprintf(stderr,
		    "Check that pnmcat is installed and path is set\n");
	}
	else {
	    for (m = 1; m <= var_i; m++) {
		sprintf(filename, "%s_%d_%d.ppm", pref, i, m);
		remove(filename);
	    }
	}
	k--;
    }
    sprintf(filename, "> %s.ppm", pref);
    strcat(cmd2, filename);
    if (system(cmd2) != 0) {
	fprintf(stderr, "pnmcat failed to create assembled images\n");
	fprintf(stderr, "Check that pnmcat is installed and path is set\n");
    }
    else {
	for (m = 1; m <= var_i; m++) {
	    sprintf(filename, "%stmp%d.ppm", pref, m);
	    remove(filename);
	}
    }

    GS_set_viewport(a_orig, c_orig, b_orig, d_orig);

#if defined(OPENGL_X11) && (defined(HAVE_PBUFFERS) || defined(HAVE_PIXMAPS))
    Destroy_OS_Ctx();
#endif

    fprintf(stderr, "Finished rendering max. size image\n");
    sprintf(inform_text, "inform \"Finished rendering max. size image\"");
    Tcl_Eval(interp, inform_text);
    return (TCL_OK);
}

/********************************************
 * callbacks for PBuffer and GLXPixmap to
 * swap buffers 
*********************************************/
static void swap_os(void)
{
#ifdef OPENGL_X11
#ifdef HAVE_PBUFFERS
    if (pbuffer)
	glXSwapBuffers(dpy, pbuffer);
#endif
#ifdef HAVE_PIXMAPS
    if (glxpixmap)
	glXSwapBuffers(dpy, glxpixmap);
#endif
#endif
}

/********************************************
 * open an off-screen render context 
********************************************/

static void create_pbuffer(int width, int height)
{
#ifdef HAVE_PBUFFERS
#if defined(GLX_PBUFFER_WIDTH) && defined(GLX_PBUFFER_HEIGHT)
    static int ver_major, ver_minor;
    static int fb_attrib[] = {
	GLX_DOUBLEBUFFER, False,
	GLX_RED_SIZE, 1,
	GLX_GREEN_SIZE, 1,
	GLX_BLUE_SIZE, 1,
	GLX_DEPTH_SIZE, 1,
	GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
	None
    };
    static GLXFBConfig *fbc;
    int pbuf_attrib[] = {
	GLX_PBUFFER_WIDTH, width,
	GLX_PBUFFER_HEIGHT, height,
	None
    };
    int elements;
    XVisualInfo *vi;

    if (fbc) {
	XFree(fbc);
	fbc = NULL;
    }

    if (getenv("GRASS_NO_GLX_PBUFFERS"))
	return;

    if (!ver_major)
	glXQueryVersion(dpy, &ver_major, &ver_minor);

    if (ver_minor < 3)
	return;

    fprintf(stderr, "Creating PBuffer Using GLX 1.3\n");

    fbc = glXChooseFBConfig(dpy, scr, fb_attrib, &elements);
    if (!fbc) {
	fprintf(stderr, "Unable to get FBConfig\n");
	return;
    }

    vi = glXGetVisualFromFBConfig(dpy, fbc[0]);
    if (!vi) {
	fprintf(stderr, "Unable to get Visual\n");
	return;
    }

    ctx = glXCreateContext(dpy, vi, NULL, False);
    if (!ctx) {
	fprintf(stderr, "Unable to create context\n");
	return;
    }

    pbuffer = glXCreatePbuffer(dpy, fbc[0], pbuf_attrib);
    if (!pbuffer) {
	fprintf(stderr, "Unable to create Pbuffer\n");
	return;
    }

    if (!glXMakeContextCurrent(dpy, pbuffer, pbuffer, ctx)) {
	fprintf(stderr, "Unable to use context\n");
	glXDestroyPbuffer(dpy, pbuffer);
	pbuffer = None;
	return;
    }
#endif
#endif
}

static void create_pixmap(int width, int height)
{
#ifdef HAVE_PIXMAPS
    static int att[] = {
	GLX_RGBA,
	GLX_RED_SIZE, 1,
	GLX_GREEN_SIZE, 1,
	GLX_BLUE_SIZE, 1,
	GLX_DEPTH_SIZE, 1,
	None
    };
    XVisualInfo *vi;

    if (getenv("GRASS_NO_GLX_PIXMAPS"))
	return;

    fprintf(stderr, "Create PixMap Using GLX 1.1\n");

    vi = glXChooseVisual(dpy, scr, att);
    if (!vi) {
	fprintf(stderr, "Unable to get Visual\n");
	return;
    }

    ctx = glXCreateContext(dpy, vi, NULL, GL_FALSE);
    if (!ctx) {
	fprintf(stderr, "Unable to create context\n");
	return;
    }

    pixmap = XCreatePixmap(dpy, RootWindow(dpy, vi->screen),
			   width, height, vi->depth);
    if (!pixmap) {
	fprintf(stderr, "Unable to create pixmap\n");
	return;
    }
    glxpixmap = glXCreateGLXPixmap(dpy, vi, pixmap);
    if (!glxpixmap) {
	fprintf(stderr, "Unable to create GLX pixmap\n");
	return;
    }
    if (!glXMakeCurrent(dpy, glxpixmap, ctx)) {
	fprintf(stderr, "Unable to use context\n");
	glXDestroyGLXPixmap(dpy, glxpixmap);
	glxpixmap = None;
	return;
    }
#endif
}

#ifdef OPENGL_X11
static int Error_Handler(ClientData closure, XErrorEvent * event)
{
    if (!dpy)
	return 0;
    XmuPrintDefaultErrorMessage(dpy, event, stderr);
    return 0;
}
#endif

int Create_OS_Ctx(int width, int height)
{
#if defined(OPENGL_X11) && (defined(HAVE_PBUFFERS) || defined(HAVE_PIXMAPS))
    static int initialized;

    dpy = togl_display();
    if (dpy == NULL) {
	fprintf(stderr, "Togl_Display Failed!\n");
	return (-1);
    }
    scr = togl_screen_number();

    if (!initialized) {
	int major, event, error;

	if (XQueryExtension(dpy, "GLX", &major, &event, &error))
	    Tk_CreateErrorHandler(dpy, -1, major, -1, Error_Handler, NULL);
	initialized = 1;
    }

    create_pbuffer(width, height);
#ifdef HAVE_PBUFFERS
    if (!pbuffer)
#endif
	create_pixmap(width, height);

    if (!pbuffer && !glxpixmap)
	return 1;

    /* hide togl canvas before init_ctx 
     * This prevents bindings from re-initializing
     * togl */
    hide_togl_win();

    /* Initalize off screen context */
    if (init_ctx() != 1) {
	fprintf(stderr, "Error: Failed to Initiailze drawing area\n");
	return (-1);
    }

    GS_set_swap_func(swap_os);

    GS_set_viewport(0, width, 0, height);
    GS_set_draw(GSD_BACK);
    GS_ready_draw();
    GS_alldraw_wire();
    GS_done_draw();

    return (1);
#else
    fprintf(stderr, "It appears that X is not available!\n");
    return (-1);
#endif /* defined(OPENGL_X11) && (defined(HAVE_PBUFFERS) || defined(HAVE_PIXMAPS)) */
}


/*****************************************************
 * destroy off-screen context 
*****************************************************/
int Destroy_OS_Ctx(void)
{
#ifdef OPENGL_X11
#ifdef HAVE_PBUFFERS
    if (pbuffer) {
	fprintf(stderr, "Destroy pbuffer\n");
	glXMakeCurrent(dpy, None, NULL);
	glXDestroyPbuffer(dpy, pbuffer);
	pbuffer = None;
    }
#endif
#ifdef HAVE_PIXMAPS
    if (ctx) {
	fprintf(stderr, "Destroy Context\n");
	glXMakeCurrent(dpy, None, NULL);
	glXDestroyContext(dpy, ctx);
	ctx = NULL;
    }
    if (glxpixmap) {
	fprintf(stderr, "Destroy GLXPixmap\n");
	glXDestroyGLXPixmap(dpy, glxpixmap);
	glxpixmap = None;
    }
    if (pixmap) {
	fprintf(stderr, "Destroy Pixmap\n");
	XFreePixmap(dpy, pixmap);
	pixmap = None;
    }
#endif
    GS_set_swap_func(swap_togl);
    show_togl_win();
    dpy = NULL;
#endif /* OPENGL_X11 */

    return (1);
}

/*****************************************************
 * Initialize graphics (lights) for new context 
*****************************************************/
static int init_ctx(void)
{
    float x, y, z;
    int num, w;
    float r, g, b;

    glMatrixMode(GL_MODELVIEW);
    glDepthRange(0.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* There should be two lights -- one stationary
     * light overhead, and the user defined light 
     */
    gsd_init_lightmodel();

    /* Get and set light atts */
    for (num = 1; num < 3; num++) {

	GS_getlight_position(num, &x, &y, &z, &w);
	GS_setlight_position(num, x, y, z, w);

	GS_getlight_color(num, &r, &g, &b);
	GS_setlight_color(num, r, g, b);

	GS_getlight_ambient(num, &r, &g, &b);
	GS_setlight_ambient(num, r, g, b);
    }

    GS_lights_on();

    return (1);

}
