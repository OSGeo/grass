
/****************************************************************************
 *
 * MODULE:       r3.showdspf
 * AUTHOR(S):    Bill Brown, CERL
 *               ported from IrisGL to OpenGL by Ken Sakai, Steve Hall,
 *               Lockheed Martin Space Systems,
 *               Sunnyale, California, Modification date: March 2000
 * PURPOSE:      Visualization program which loads the isosurfaces previously calculated
 *               using r3.mkdspf
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/raster3d.h>
#include <grass/config.h>

#define TOGGLE(x) ((x) = (x) ? 0 : 1)

#include <Xm/Xm.h>
#include <Xm/Form.h>

#ifdef HAVE_GL_GLWMDRAWA_H
#include <GL/GLwMDrawA.h>
#else
#ifdef HAVE_X11_GLW_GLWMDRAWA_H
#include <X11/GLw/GLwMDrawA.h>
#endif
#endif

#include "vizual.h"

#include "kns_defines.h"
#include "kns_globals.h"

#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>

#ifdef __MINGW32__
#include <process.h>
#else
#include <sys/wait.h>
#endif

#ifndef WAIT_ANY
#define WAIT_ANY ((pid_t) -1)
#endif

GLuint Material_1_Dlist;
OGLMotifWindowData MainOGLWindow;
OGLMotifWindowData ColormapWindow;
GLuint MainDlist;
XtAppContext App_context;

file_info Headfax;	/* contains info about data itself */
file_info G3header;	/* contains info about data itself */
int G_sign;
int X_sign;
long D_offset;		/*offset to data in grid3 file */

void set_threshold_button(int i);
char *check_get_any_dspname();
void do__bbox(struct dspec *D_spec);
void init_dspec(struct dspec *D_spec, char *ctable);
void options();
void Toggle_swapbuffers(struct dspec *D_spec);
void clear_screen();
void check_limits(struct dspec *D_spec, int axis);
void copy_head(file_info * g3head, file_info * head);

char *rindex();
static char ctablefile[256];
static struct dspec D_spec;
static struct Cap D_Cap;
static void do_draw();
static void do__draw();
static void do__draw_solid();
void init_bounds(struct dspec *D_spec);
void get_trackball_rotation_matrix(float mat[4][4]);
void set_trackball_rotations(struct dspec *D_spec);
void setSingleSelectionMode();
void setMultipleSelectionMode();
int isSingleSelectionMode();

void do_draw_multiple_thresholds(file_info * Headp, file_info * G3p,
				 struct dspec *D_spec, struct Cap *Cap,
				 unsigned int type);
int dumpgif(char *name);
void do_draw_with_display_list(struct dspec *D_spec);


#define DEBUG 1
int main(int argc, char **argv)
{
    char buff[300], wname[200];
    long Window[3];
    char *dsp;
    XEvent event;
    int i, fdes[2];
    void *g3map;
    RASTER3D_Region g3reg;
    char *p, *mapset;
    double dmin, dmax;
    fd_set set;
    struct timeval timeout;
    pid_t pid;
    struct Option *g3, *dspf, *colr;

    G_gisinit(argv[0]);

    g3 = G_define_option();
    g3->key = "grid3";
    g3->type = TYPE_STRING;
    g3->required = YES;
    g3->gisprompt = "old,grid3,3d-raster";
    g3->description = "Name of an existing 3D raster map";

    dspf = G_define_option();
    dspf->key = "dspf";
    dspf->type = TYPE_STRING;
    dspf->required = YES;
    dspf->description = "Name of existing display file";

    colr = G_define_option();
    colr->key = "color";
    colr->type = TYPE_STRING;
    colr->required = NO;
    colr->description = "Name of existing color table";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* set-up for select() */
    if (pipe(fdes))
	G_fatal_error("Unable to open pipe");
    pid = fork();
    if (pid == (pid_t) 0) {	/* child */
	close(fdes[1]);

	/* use this to name graphics window */
	strcpy(wname, g3->answer);

	if (NULL == (dsp =
		     check_get_any_dspname(dspf->answer, g3->answer, NULL)))
	    exit(EXIT_FAILURE);


	/* TODO - check color file */


	/* changed 7/93; normals were pointing to interior as default */
	G_sign = 1;
	X_sign = -1;


	/* opens grid3 file to read in original data 
	 */

	Rast3d_set_error_fun(Rast3d_print_error);

	/* open g3 file for reading */
	if (NULL == (mapset = G_find_file2("grid3", g3->answer, ""))) {
	    sprintf(buff, "Unable to find 3D raster map for <%s>", g3->answer);
	    G_fatal_error(buff);
	}

	g3map = Rast3d_open_cell_old(g3->answer, mapset, RASTER3D_DEFAULT_WINDOW,
				RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);

	if (NULL == g3map) {
	    sprintf(buff, "Unable to open 3D raster map <%s>", g3->answer);
	    G_fatal_error(buff);
	}

	if (0 == Rast3d_range_load(g3map)) {
	    sprintf(buff, "Unable to read range of 3D raster map <%s>", g3->answer);
	    G_fatal_error(buff);
	}
	Rast3d_range_min_max(g3map, &dmin, &dmax);
	Rast3d_get_region_struct_map(g3map, &g3reg);

	viz_make_header(&G3header, dmin, dmax, &g3reg);
	init_caps(&D_Cap, &g3reg);

	/* open display file for reading */
	/* changes 9/2000: Bev Wallace <beverly.t.wallace@lmco.com> */
	/*    sprintf(buff,"grid3/%s/dsp", g3->answer);
	 *   if(NULL == (mapset = G_find_file2 (buff, dsp, ""))){
	 */
	/* Remove the mapset within buff, add to G_find_file2 - Bev Wallace */
	i = strcspn(wname, "@");
	if (i > 0)
	    wname[i] = (char)NULL;
	sprintf(buff, "grid3/%s/dsp", wname);
	if (NULL == (mapset = G_find_file2(buff, dsp, mapset))) {
	    sprintf(buff, "Unable to find display file for <%s>", dsp);
	    G_fatal_error(buff);
	}
	if ((Headfax.dspfinfp = G_fopen_old(buff, dsp, mapset)) == NULL) {
	    fprintf(stderr, "Unable to open <%s> for reading\n",
		    Headfax.dspfinfp);
	    exit(EXIT_FAILURE);
	}

	/* read header info from dspf file into GLOBAL variable Headfax */
	if (dfread_header(&Headfax) < 0) {
	    fprintf(stderr, "Unable to read display file header\n");
	    exit(EXIT_FAILURE);
	}

	/* set 3dmap for data in Headfax */
	Headfax.g3mapin = g3map;

	/* currently seems rather redundant, but may have future use to
	   keep them separate. */
	copy_head(&G3header, &Headfax);

	/* INIT */
	init_dspec(&D_spec, colr->answer);

	/* INITIALIZATION OF THE D_spec.B or D_spec.E CAN HAPPEN MORE THAN ONCE */
	init_bounds(&D_spec);

	D_spec.Swap_buf = 1;

	init_graphics(wname, argc, argv, &D_spec);
	draw_colortable(&D_spec, &Headfax, Window);


	winset_main();

	D_spec.c_flag = 1;	/* reset flag */
	Toggle_swapbuffers(&D_spec);	/* make sure they sync up */
	Toggle_swapbuffers(&D_spec);

	for (;;) {
	    int nbytes, fdstatus;

	    timeout.tv_sec = 0;
	    timeout.tv_usec = 10000;
	    FD_ZERO(&set);
	    FD_SET(fdes[0], &set);

	    if ((fdstatus =
		 select(FD_SETSIZE, &set, NULL, NULL, &timeout)) == 1) {
		if (FD_ISSET(fdes[0], &set) &&
		    (nbytes = read(fdes[0], buff, 299)) > 0) {
		    p = buff;
		    p[nbytes] = '\0';
		    dispatch_cmd(p);
		}
	    }
	    else if (fdstatus == -1) {
		G_fatal_error("File Descriptor error");
	    }

	    while (XtAppPending(App_context)) {
		XtAppNextEvent(App_context, &event);
		XtDispatchEvent(&event);
	    }

	}
    }
    else if (pid < (pid_t) 0) {
	G_fatal_error("Fork failed!");
    }
    else {			/* parent */

	close(fdes[0]);

	options();
	/* get input from the keyboard */
	fprintf(stderr, "enter desired manipulations then press return\n\n");
	fprintf(stderr, "Q ? + - r d l L (xyz)# (XYZ)# S B(xyz)# "
		"E(xyz)# R g C c w W i h t T# \n");
	fprintf(stderr, " > ");
	fflush(stderr);

	for (;;) {
	    int linelen;

	    if (waitpid(WAIT_ANY, NULL, WNOHANG) != 0)
		break;		/* Child exited */

	    if (NULL == fgets(buff, 300, stdin))
		break;
	    linelen = strlen(buff);

	    if (write(fdes[1], &buff, linelen) != linelen)
		G_fatal_error("Unable to write to child process.");

	    if (waitpid(WAIT_ANY, NULL, WNOHANG) != 0)
		break;		/* Child exited */

	    /* get input from the keyboard */
	    fprintf(stderr,
		    "enter desired manipulations then press return\n\n");
	    fprintf(stderr,
		    "Q ? + - r d l L (xyz)# (XYZ)# S B(xyz)# "
		    "E(xyz)# R g C c w W i h t T# \n");
	    fprintf(stderr, " > ");
	    fflush(stderr);
	}

	fprintf(stderr, "Goodbye!\n\n");

    }

    return 0;
}



int dispatch_cmd(char *p)
{
    int tmp, tmp2, drawable_cmd;
    char cmd, axis;
    static int dobox = DRAW_BBOX;
    char cfilename[128], ctablename[128];
    int j;
    int tmp4;

    drawable_cmd = 0;
    while (*p) {		/* assign valid keyboard entries to D_spec structure */
	cmd = *p++;
	switch (cmd) {
	case '#':		/* rest of line is a comment */
	    *p = 0;		/* throw away rest of line */
	    break;
	case 'n':
	    G_sign = -G_sign;
	    X_sign = -X_sign;
	    break;
	case 'b':
	    dobox = dobox ? 0 : DRAW_BBOX;
	    break;

	case '+':
	    if (!isSingleSelectionMode())
		setSingleSelectionMode();
	    D_spec.Thresh++;
	    if (D_spec.Thresh > Headfax.linefax.nthres - 1)
		D_spec.Thresh = 0;

	    set_threshold_button(D_spec.Thresh + 1);
	    do_draw(&Headfax, &G3header, &D_spec, &D_Cap, DRAW_ISO | dobox);
	    drawable_cmd = 1;
	    break;

	case '-':
	    D_spec.Thresh--;
	    if (!isSingleSelectionMode())
		setSingleSelectionMode();
	    if (D_spec.Thresh < 0)
		D_spec.Thresh = Headfax.linefax.nthres - 1;

	    set_threshold_button(D_spec.Thresh + 1);
	    do_draw(&Headfax, &G3header, &D_spec, &D_Cap, DRAW_ISO | dobox);
	    drawable_cmd = 1;
	    break;
	case '?':
	    {
		int i;

		for (i = 0; i < Headfax.linefax.nthres; i++)
		    fprintf(stderr, "%c %3d for threshold value %5.2f\n",
			    i == D_spec.Thresh ? '*' : ' ', i + 1,
			    Headfax.linefax.tvalue[i]);
		fprintf(stderr, "Rotations: X %d  Y %d  Z %d\n", D_spec.xrot,
			D_spec.yrot, D_spec.zrot);
	    }
	    break;

	case 'l':		/* list thresholds */
	    D_spec.nt = 0;
	    G_squeeze(p);
	    while (isdigit(*p)) {
		int i;

		i = atoi(p);
		if (i < 1 || i > Headfax.linefax.nthres)
		    fprintf(stderr,
			    "Range is 1 to %d\n", Headfax.linefax.nthres);
		else {
		    D_spec.t[D_spec.nt] = i - 1;
		    D_spec.nt++;

		}
		p++;
		if (i >= 10)
		    p++;
		if (i >= 100)
		    p++;
		G_squeeze(p);
	    }
	    *p = 0;		/* throw away rest of line */
	    break;
	case 'L':		/*display list */
	    if (!D_spec.nt)
		break;
	    if (D_spec.c_flag)
		clear_screen();
	    tmp = D_spec.c_flag;
	    D_spec.c_flag = 0;
	    tmp2 = D_spec.Thresh;
	    D_spec.Swap_buf = 0;
	    if (isSingleSelectionMode())
		setMultipleSelectionMode();
	    if (D_spec.nt) {
		for (j = 0; j < Headfax.linefax.nthres; j++) {
		    unset_threshold_button(j + 1);
		}
		for (j = 0; j < D_spec.nt; j++) {
		    set_threshold_button(D_spec.t[j] + 1);
		}
	    }
	    do_draw_multiple_thresholds(&Headfax, &G3header, &D_spec, &D_Cap,
					DRAW_ISO | dobox);
	    new_swapbuffers();
	    D_spec.Swap_buf = 1;
	    D_spec.c_flag = tmp;
	    D_spec.Thresh = tmp2;
	    drawable_cmd = 1;
	    break;
	case 't':		/* show only this threshold */
	    G_squeeze(p);
	    if (isdigit(*p)) {
		int i;

		i = atoi(p);
		if (i < 1 || i > Headfax.linefax.nthres)
		    fprintf(stderr, "Range is 1 to %d\n",
			    Headfax.linefax.nthres);
		D_spec.Thresh = i - 1;
		if (!isSingleSelectionMode())
		    setSingleSelectionMode();

		set_threshold_button(D_spec.Thresh + 1);
		do_draw(&Headfax, &G3header, &D_spec, &D_Cap,
			DRAW_ISO | dobox);
		drawable_cmd = 1;
		*p = 0;		/* throw away rest of line */
	    }
	    else
		fprintf(stderr, "check keyboard entry instructions \n"), *p =
		    0;
	    break;
	case 'T':		/* show thresholds between hi & lo */
	    G_squeeze(p);
	    if (2 != sscanf(p, "%d %d", &(D_spec.low), &(D_spec.hi))) {
		 /*DEBUG*/
		    fprintf(stderr, ":><>>>  T %d %d\n", (D_spec.low),
			    (D_spec.hi));
		fprintf(stderr, "check keyboard entry instructions \n");
		*p = 0;
		D_spec.low = 0;
		D_spec.hi = Headfax.linefax.nthres - 1;
	    }
	    else {
		D_spec.low--;	/* convert from user to internal #s */
		D_spec.hi--;
	    }
	    drawable_cmd = 1;
	    *p = 0;		/* throw away rest of line */
	    break;

	case 'B':		/* initial value along specified axis */
	case 'E':		/* ending  value along specified axis */
	    G_squeeze(p);
	    axis = *p++;
	    G_squeeze(p);
	    if (isdigit(*p)) {
		G_squeeze(p);
		tmp = atoi(p);
		*p = 0;		/*throw away rest of line */
	    }
	    else {
		fprintf(stderr, "enter number also\n");
		break;
	    }
	    switch (axis) {
	    case 'x':
		if (cmd == 'B')
		    D_spec.B[X] = tmp;
		else
		    D_spec.E[X] = tmp;
		tmp = X;
		break;
	    case 'y':
		if (cmd == 'B')
		    D_spec.B[Y] = tmp;
		else
		    D_spec.E[Y] = tmp;
		tmp = Y;
		break;
	    case 'z':
		if (cmd == 'B')
		    D_spec.B[Z] = tmp;
		else
		    D_spec.E[Z] = tmp;
		tmp = Z;
		break;
	    }
	    check_limits(&D_spec, tmp);
	    break;
	case 'R':
	    init_bounds(&D_spec);
	    break;
	case 'S':		/* Specular highlight */
	    G_squeeze(p);
	    if (isdigit(*p)) {
		D_spec.Specular = (float)atof(p);
		*p = 0;
		change_spec(D_spec.Specular);
	    }
	    else
		fprintf(stderr, "check keyboard entry instructions \n"), *p =
		    0;
	    break;
	case 'r':
	    fprintf(stderr, " - Rotation Mode -\n"
		    " 1) Drag with LEFT mouse button to rotate\n"
		    " 2) Drag right/left with MIDDLE mouse button to zoom in/out\n"
		    " 3) Click RIGHT mouse button to exit Rotation Mode\n\n");
	    tmp4 = D_spec.c_flag;
	    D_spec.c_flag = 1;
	    rotate_model(&D_spec);
	    clear_screen();
	    D_spec.c_flag = tmp4;
	    break;
	case 's':		/* use swapbuffers */
	    /*
	       Toggle_swapbuffers (&D_spec);
	     */
	    break;
	case 'd':		/* draw it */
	    G_squeeze(p);
	    if (isdigit(*p)) {
		int i;

		i = atoi(p);

		if (i == 1)
		    D_spec.Thresh = D_spec.low;
		else
		    D_spec.Thresh = D_spec.hi;
	    }
	    if (!isSingleSelectionMode())
		setSingleSelectionMode();
	    set_threshold_button(D_spec.Thresh + 1);

	    do_draw(&Headfax, &G3header, &D_spec, &D_Cap, DRAW_ISO | dobox);
	    drawable_cmd = 1;
	    *p = 0;
	    break;
	case 'u':		/* Update the screen in double buffer mode */
	    new_swapbuffers();
	    break;
	case 'D':		/* draw solid */
	    do_draw(&Headfax, &G3header, &D_spec, &D_Cap, DRAW_SOLID | dobox);
	    drawable_cmd = 1;
	    break;
	case 'x':		/* rotate around x-axis */
	    G_squeeze(p);
	    if (isdigit(*p) || *p == '-' || *p == '+') {
		D_spec.xrot = atoi(p);
		fprintf(stderr, "(RotX, RotY, RotZ) = (%d, %d, %d)\n\n",
			D_spec.xrot, D_spec.yrot, D_spec.zrot);
		set_trackball_rotations(&D_spec);
		do_draw_with_display_list(&D_spec);
		*p = 0;
	    }
	    else
		fprintf(stderr, "check keyboard entry instructions \n"), *p =
		    0;
	    break;
	case 'X':		/* scale in x-direction */
	    G_squeeze(p);
	    if (isdigit(*p) || *p == '-' || *p == '+') {
		D_spec.xscale = atof(p);
		do_draw(&Headfax, &G3header, &D_spec, &D_Cap, dobox);
		*p = 0;
	    }
	    else
		fprintf(stderr, "check keyboard entry instructions \n"), *p =
		    0;
	    break;
	case 'y':		/* rotate around y-axis */
	    G_squeeze(p);
	    if (isdigit(*p) || *p == '-' || *p == '+') {
		D_spec.yrot = atoi(p);
		fprintf(stderr, "(RotX, RotY, RotZ) = (%d, %d, %d)\n\n",
			D_spec.xrot, D_spec.yrot, D_spec.zrot);
		set_trackball_rotations(&D_spec);
		do_draw_with_display_list(&D_spec);
		*p = 0;
	    }
	    else
		fprintf(stderr, "check keyboard entry instructions \n"), *p =
		    0;
	    break;
	case 'Y':		/* scale in y direction */
	    G_squeeze(p);
	    if (isdigit(*p) || *p == '-' || *p == '+') {
		D_spec.yscale = atof(p);
		do_draw(&Headfax, &G3header, &D_spec, &D_Cap, dobox);
		*p = 0;
	    }
	    else
		fprintf(stderr, "check keyboard entry instructions \n"), *p =
		    0;
	    break;
	case 'z':		/* rotate around z-axis */
	    G_squeeze(p);
	    if (isdigit(*p) || *p == '-' || *p == '+') {
		D_spec.zrot = atoi(p);
		fprintf(stderr, "(RotX, RotY, RotZ) = (%d, %d, %d)\n\n",
			D_spec.xrot, D_spec.yrot, D_spec.zrot);
		set_trackball_rotations(&D_spec);
		do_draw_with_display_list(&D_spec);
		*p = 0;
	    }
	    else
		fprintf(stderr, "check keyboard entry instructions \n"), *p =
		    0;
	    break;
	case 'Z':		/* scale in z direction */
	    G_squeeze(p);
	    if (isdigit(*p) || *p == '-' || *p == '+') {
		D_spec.zscale = atof(p);
		do_draw(&Headfax, &G3header, &D_spec, &D_Cap, dobox);
		*p = 0;
	    }
	    else
		fprintf(stderr, "check keyboard entry instructions \n"), *p =
		    0;
	    break;
	case 'g':		/* toggle grid */
	    TOGGLE(D_spec.grid);
	    do_draw(&Headfax, &G3header, &D_spec, &D_Cap, DRAW_ISO | dobox);
	    break;
	case 'C':		/* toggle clear flag */
	    TOGGLE(D_spec.c_flag);
	    break;
	case 'c':		/* redraw the screen */
	    clear_screen();
	    new_swapbuffers();
	    break;
	case 'w':		/* dump image to file */
	    dumpgif(G_squeeze(++p));
	    *p = 0;
	    break;
	case 'W':		/* TEST dump image to file */
	    dumprect(G_squeeze(++p));
	    *p = 0;
	    break;
	    /* TEST READ image from file */
	case 'i':
	    loadrect(G_squeeze(++p));
	    *p = 0;
	    break;
	case 'Q':		/* QUIT */
	    if (D_spec.cfile != NULL)
		fclose(D_spec.cfile);
	    exit(0);

	    break;
	case 'h':		/* help */
	    options();
	    break;
	case 'p':		/* display a single plane 1-6 */
#ifdef NEVER
	    /* This code cores so I am taking it out.  Ken Sakai */

	    G_squeeze(p);
	    if (isdigit(*p)) {
		D_spec.plane = atoi(p) - 1;
		if (D_spec.plane >= 0 && D_spec.plane <= 5) {
		    /* call plane drawing code */
		    do_draw(&Headfax, &G3header, &D_spec, &D_Cap,
			    DRAW_CAP | ((1 << D_spec.plane) << 16));
		    drawable_cmd = 1;
		}
		else
		    fprintf(stderr, "must be number between 1-6\n");
	    }
	    else {
		/* call code to draw all planes */
		do_draw(&Headfax, &G3header, &D_spec, &D_Cap,
			DRAW_CAP | (0x3f << 16));
		drawable_cmd = 1;
	    }
	    *p = 0;
#endif
	    break;
	case 'I':		/* toggle in_out flag */
	    TOGGLE(D_spec.in_out);
	    break;
	case 'F':		/* new color File */
	    G_squeeze(p);
	    if (2 != sscanf(p, "%s %s", cfilename, ctablename))
		no_color_file(&D_spec, ctablefile);
	    else {
		if (0 > new_color_file(cfilename, ctablename, &D_spec))
		    no_color_file(&D_spec, ctablefile);
	    }
	    *p = 0;
	    break;
	default:
	    *p = 0;
	    break;

	}
    }
    return (drawable_cmd ? 2 : 1);
}


void dspf_get_zscale(float *zscale)
{
    *zscale = D_spec.zscale;
}

/* note that currently using G3header */
void dspf_get_res(float *xres, float *yres, float *zres)
{
    *xres = (G3header.east - G3header.west) / G3header.xdim;
    *yres = (G3header.north - G3header.south) / G3header.ydim;
    *zres = (G3header.top - G3header.bottom) / G3header.zdim;
}

/* note that currently using G3header */
void dspf_getorigin(float *west, float *south, float *bottom)
{
#ifdef DEBUG
    static int first = 1;

    if (first < 10) {		/* DEBUG */
	fprintf(stderr, "WEST = %f\nSOUTH = %f\nBOTTOM = %f\n", *west, *south,
		*bottom);
	first++;
    }
#endif

    *west = G3header.west;
    *south = G3header.south;
    *bottom = G3header.bottom;
}


/*-------------------------------------------------------------------
  do_draw ()
  This function is the original immediate mode drawing routine that
  calculates as well as draws the current isosurface dataset.  An OpenGL
  display list which stores all of the raw OpenGL drawing directives
  is created by this function that can be used to redraw
  the geometry without having to recompute the isosurface data. The use
  of this OpenGL display list by functions such as do_draw_with_display_list()
  GREATLY speeds up the process of rendering, especially when rendering is
  being done over the network.
  -------------------------------------------------------------------
*/
void
do_draw(file_info * Headp, file_info * G3p, struct dspec *D_spec,
	struct Cap *Cap, unsigned int type)
{
    static double x, y, z;
    float mat[4][4];


    x = Headfax.xdim * D_spec->xscale / 2;
    y = Headfax.ydim * D_spec->yscale / 2;
    z = Headfax.zdim * D_spec->zscale / 2;

    glPushMatrix();
    glTranslatef(0.0, 0.0, D_spec->ztrans);	/* move it away from eye */

    if (D_spec->c_flag)
	clear_screen();

    /* Do trackball rotations */
    get_trackball_rotation_matrix(mat);
    glMultMatrixf((float *)mat);

    glTranslatef(-x, -y, -z);

    glDeleteLists(MainDlist, 1);
    glNewList(MainDlist, GL_COMPILE_AND_EXECUTE);

    if (type & DRAW_BBOX)
	do__bbox(D_spec);

    if (type & DRAW_ISO)
	do__draw(Headp, D_spec);
    else if (type & DRAW_SOLID)
	do__draw_solid(Headp, G3p, D_spec, Cap);
    else if (type & DRAW_CAP)
	draw_cap_side(D_spec, Headp, G3p, Cap, (type >> 16) & 0x3f);
    /* bring over argument to DRAW_CAP in high 16 bits to low 16 */
    /*    and AND with   00111111  for sides 1-6 */


    /* todo, only draw visible sides, and dont use Zbuffer */
    if (type & DRAW_BBOX)
	do__bbox(D_spec);

    glEndList();


    if (D_spec->Swap_buf)
	new_swapbuffers();
    glPopMatrix();


}

/*-------------------------------------------------------------------
  void draw_ctable()
  This function draws the colortable window.
  -------------------------------------------------------------------
*/
void draw_ctable()
{
    long a[2];

    draw_colortable(&D_spec, &Headfax, a);
}


/*-------------------------------------------------------------------
  void draw_multiple()
  This function draws multiple thresholds in immediate mode and updates
  the current display list
  -------------------------------------------------------------------
*/
void draw_multiple()
{
    int temp;

    if (!D_spec.nt)
	return;

    temp = D_spec.Thresh;
    D_spec.Swap_buf = 0;
    do_draw_multiple_thresholds(&Headfax, &G3header, &D_spec, &D_Cap,
				DRAW_ISO | DRAW_BBOX);
    new_swapbuffers();
    D_spec.Swap_buf = 1;
    D_spec.Thresh = temp;

}

/*-------------------------------------------------------------------
  do_draw_immediate_mode()
  This function draws the current scene in immediate mode and updates
  the current display list
  -------------------------------------------------------------------
*/
void do_draw_immediate_mode()
{
    do_draw(&Headfax, &G3header, &D_spec, &D_Cap, DRAW_ISO | DRAW_BBOX);
}

/*-------------------------------------------------------------------
  do_draw_with_display_list()
  This function draws the current scene display list
  -------------------------------------------------------------------
*/
void do_draw_with_display_list(struct dspec *D_spec)
{
    static double x, y, z;
    float mat[4][4];


    x = Headfax.xdim * D_spec->xscale / 2;
    y = Headfax.ydim * D_spec->yscale / 2;
    z = Headfax.zdim * D_spec->zscale / 2;


    glPushMatrix();
    glTranslatef(0.0, 0.0, D_spec->ztrans);	/* move it away from eye */

    if (D_spec->c_flag)
	clear_screen();

    /* first  xyz, then yxz, now zyx */

    /* Do trackball rotations */
    get_trackball_rotation_matrix(mat);
    glMultMatrixf((float *)mat);


    glTranslatef(-x, -y, -z);

    glCallList(MainDlist);

    if (D_spec->Swap_buf)
	new_swapbuffers();
    glPopMatrix();


}




/*-------------------------------------------------------------------
  do_draw_no_transformations()
  This function draws a single thresholds with no translational or
  rotational transformations.
  -------------------------------------------------------------------
*/
void
do_draw_no_transformations(file_info * Headp, file_info * G3p,
			   struct dspec *D_spec, struct Cap *Cap,
			   unsigned int type)
{

    if (type & DRAW_BBOX)
	do__bbox(D_spec);

    if (type & DRAW_ISO)
	do__draw(Headp, D_spec);
    else if (type & DRAW_SOLID)
	do__draw_solid(Headp, G3p, D_spec, Cap);
    else if (type & DRAW_CAP)
	draw_cap_side(D_spec, Headp, G3p, Cap, (type >> 16) & 0x3f);
    /* bring over argument to DRAW_CAP in high 16 bits to low 16 */
    /*    and AND with   00111111  for sides 1-6 */

}

/*-------------------------------------------------------------------
  do_draw_multiple_thresholds()
  This function computes and draws multiple thresholds at once. OpenGL 
  drawing commands are stored in an OpenGL display list for later use.
  -------------------------------------------------------------------
*/
void
do_draw_multiple_thresholds(file_info * Headp, file_info * G3p,
			    struct dspec *D_spec, struct Cap *Cap,
			    unsigned int type)
{
    static int first = 1;
    static double x, y, z;
    int j;
    float mat[4][4];

    x = Headfax.xdim * D_spec->xscale / 2;
    y = Headfax.ydim * D_spec->yscale / 2;
    z = Headfax.zdim * D_spec->zscale / 2;


    glPushMatrix();
    glTranslatef(0.0, 0.0, D_spec->ztrans);	/* move it away from eye */

    if (D_spec->c_flag)
	clear_screen();

    /* Do trackball rotations */
    get_trackball_rotation_matrix(mat);
    glMultMatrixf((float *)mat);

    glTranslatef(-x, -y, -z);

    glDeleteLists(MainDlist, 1);
    glNewList(MainDlist, GL_COMPILE_AND_EXECUTE);

    for (j = 0; j < D_spec->nt; j++) {
	D_spec->Thresh = D_spec->t[j];
	do_draw_no_transformations(Headp, G3p, D_spec, Cap, type);
    }
    glEndList();


    if (D_spec->Swap_buf)
	new_swapbuffers();
    glPopMatrix();


}



void do__bbox(struct dspec *D_spec)
{
    static int first = 1;
    static float x, y, z;
    static float c[8][3];	/*holds the corner points for bounding box */
    static float gxl[100][3], gxh[100][3], gyl[100][3], gyh[100][3];
    int gx, gy;

    glDisable(GL_LIGHTING);

    /*
     **  do rotations here
     */
    /*if (first) */
    {
	x = Headfax.xdim * D_spec->xscale;
	y = Headfax.ydim * D_spec->yscale;
	z = Headfax.zdim * D_spec->zscale;

	/* init corner array */
	c[1][0] = c[2][0] = c[5][0] = c[6][0] = x;
	c[0][0] = c[3][0] = c[4][0] = c[7][0] = 0;
	c[0][1] = c[1][1] = c[4][1] = c[5][1] = y;
	c[3][1] = c[2][1] = c[7][1] = c[6][1] = 0;
	c[0][2] = c[1][2] = c[3][2] = c[2][2] = z;
	c[4][2] = c[5][2] = c[7][2] = c[6][2] = 0;

	/* init grid array */
	if (D_spec->grid) {
	    for (gy = 1; gy < Headfax.ydim; gy++) {
		gyl[gy][0] = 0;
		gyh[gy][0] = x;
		gyl[gy][1] = gy * D_spec->yscale;
		gyh[gy][1] = gy * D_spec->yscale;
		gyl[gy][2] = 0;
		gyh[gy][2] = 0;
	    }

	    for (gx = 1; gx < Headfax.xdim; gx++) {
		gxl[gx][0] = gx * D_spec->xscale;
		gxh[gx][0] = gx * D_spec->xscale;
		gxl[gx][1] = 0;
		gxh[gx][1] = y;
		/*gxl[gx][2] =  z; gxh[gx][2] =  z; */
		gxl[gx][2] = 0;
		gxh[gx][2] = 0;
	    }


	    /*init_plane(c,p,sides); this is a much earlier idea JCM */
	    /* build plane vertex info (3verts)from the above info  used for normals */
	    /* based on the planes as defined in cap_data.c */
	    /* to have normals pointing the correct direction CCW ordering of vertices */

	    /*side 0  xy plane z = zdim */
	    D_spec->p[0][0][0] = c[0][0];
	    D_spec->p[0][0][1] = c[0][1];
	    D_spec->p[0][0][2] = c[0][2];
	    D_spec->p[0][1][0] = c[3][0];
	    D_spec->p[0][1][1] = c[3][1];
	    D_spec->p[0][1][2] = c[3][2];
	    D_spec->p[0][2][0] = c[1][0];
	    D_spec->p[0][2][1] = c[1][1];
	    D_spec->p[0][2][2] = c[1][2];

	    /*side 1 xy plane  z = 0 */
	    D_spec->p[1][0][0] = c[4][0];
	    D_spec->p[1][0][1] = c[4][1];
	    D_spec->p[1][0][2] = c[4][2];
	    D_spec->p[1][1][0] = c[5][0];
	    D_spec->p[1][1][1] = c[5][1];
	    D_spec->p[1][1][2] = c[5][2];
	    D_spec->p[1][2][0] = c[7][0];
	    D_spec->p[1][2][1] = c[7][1];
	    D_spec->p[1][2][2] = c[7][2];

	    /*side 2 yz plane x=xdim */
	    D_spec->p[2][0][0] = c[1][0];
	    D_spec->p[2][0][1] = c[1][1];
	    D_spec->p[2][0][2] = c[1][2];
	    D_spec->p[2][1][0] = c[2][0];
	    D_spec->p[2][1][1] = c[2][1];
	    D_spec->p[2][1][2] = c[2][2];
	    D_spec->p[2][2][0] = c[5][0];
	    D_spec->p[2][2][1] = c[5][1];
	    D_spec->p[2][2][2] = c[5][2];

	    /*side 3 yz plane x=0 */
	    D_spec->p[3][0][0] = c[6][0];
	    D_spec->p[3][0][1] = c[6][1];
	    D_spec->p[3][0][2] = c[6][2];
	    D_spec->p[3][1][0] = c[7][0];
	    D_spec->p[3][1][1] = c[7][1];
	    D_spec->p[3][1][2] = c[7][2];
	    D_spec->p[3][2][0] = c[3][0];
	    D_spec->p[3][2][1] = c[3][1];
	    D_spec->p[3][2][2] = c[3][2];

	    /*side 4 zx plane y=ydim */
	    D_spec->p[4][0][0] = c[4][0];
	    D_spec->p[4][0][1] = c[4][1];
	    D_spec->p[4][0][2] = c[4][2];
	    D_spec->p[4][1][0] = c[0][0];
	    D_spec->p[4][1][1] = c[0][1];
	    D_spec->p[4][1][2] = c[0][2];
	    D_spec->p[4][2][0] = c[1][0];
	    D_spec->p[4][2][1] = c[1][1];
	    D_spec->p[4][2][2] = c[1][2];

	    /*side 5 zx plane y=0 */
	    D_spec->p[5][0][0] = c[7][0];
	    D_spec->p[5][0][1] = c[7][1];
	    D_spec->p[5][0][2] = c[7][2];
	    D_spec->p[5][1][0] = c[6][0];
	    D_spec->p[5][1][1] = c[6][1];
	    D_spec->p[5][1][2] = c[6][2];
	    D_spec->p[5][2][0] = c[3][0];
	    D_spec->p[5][2][1] = c[3][1];
	    D_spec->p[5][2][2] = c[3][2];
	}
	first = 0;
    }

    /* draw bounding box */

    glColor3ub(255, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex3fv(c[0]);
    glVertex3fv(c[1]);
    glVertex3fv(c[2]);
    glVertex3fv(c[3]);
    glEnd();
    glBegin(GL_LINES);
    glVertex3fv(c[0]);
    glVertex3fv(c[4]);
    glVertex3fv(c[1]);
    glVertex3fv(c[5]);
    glVertex3fv(c[2]);
    glVertex3fv(c[6]);
    glVertex3fv(c[3]);
    glVertex3fv(c[7]);
    glEnd();


    glBegin(GL_LINE_LOOP);
    glVertex3fv(c[4]);
    glVertex3fv(c[5]);
    glVertex3fv(c[6]);
    glVertex3fv(c[7]);
    glEnd();
    glRasterPos3f(-0.5, y, z);
    new_charstr("0");
    glRasterPos3f(x + 0.5, y, z);
    new_charstr("1");
    glRasterPos3f(-0.5, 0, z);
    new_charstr("3");
    glRasterPos3f(x + 0.5, 0, z);
    new_charstr("2");

    glRasterPos3f(-0.5, y, 0);
    new_charstr("4");
    glRasterPos3f(x + 0.5, y, 0);
    new_charstr("5");
    glRasterPos3f(-0.5, 0, 0);
    new_charstr("7");
    glRasterPos3f(x + 0.5, 0, 0);
    new_charstr("6");

    if (D_spec->grid) {
	for (gy = 1; gy < Headfax.ydim; gy++) {
	    glRasterPos3f(-0.5, gy * D_spec->yscale, 0);
	    /*          sprintf(label,"%d",gy);new_charstr(label); */
	    glBegin(GL_LINE_STRIP);
	    glVertex3fv(gyl[gy]);
	    glVertex3fv(gyh[gy]);
	    glEnd();
	    glRasterPos3f(x + 0.5, gy * D_spec->yscale, 0);
	    /*
	       new_charstr(label);
	     */
	}

	for (gx = 1; gx < Headfax.xdim; gx++) {
	    glRasterPos3f(gx * D_spec->xscale, -0.5, 0);
	    /*
	       sprintf(label,"%d",gx);new_charstr(label);
	     */
	    glBegin(GL_LINE_STRIP);
	    glVertex3fv(gxl[gx]);
	    glVertex3fv(gxh[gx]);
	    glEnd();
	    glRasterPos3f(gx * D_spec->xscale, y + 0.5, 0);
	    /*
	       new_charstr(label);
	     */
	}
    }
    glEnable(GL_LIGHTING);

}

void do__draw(file_info * Headp, struct dspec *D_spec)
{
    static int first = 1;

    if (first)
	first = 0;
    else
	reset_reads(&Headfax);

    /*    fprintf (stderr, "Threshold %d = %f\n", 
       D_spec->Thresh + 1, Headp->linefax.tvalue[D_spec->Thresh]);
       fflush(stderr); */
    switch (Headp->linefax.litmodel) {
    case 1:
	fdraw_polys(D_spec);
	break;
    case 2:
      case3:
	gdraw_polys(D_spec);
	break;
    }
}

void
do__draw_solid(file_info * Headp, file_info * G3header, struct dspec *D_spec,
	       struct Cap *Cap)
{
    int min, max;

    min = D_spec->low;
    max = D_spec->hi;


    D_spec->Thresh = min;
    do__draw(Headp, D_spec);	/* Draw low thresh */
    D_spec->Thresh = max;
    do__draw(Headp, D_spec);	/* Draw hi  thresh */

    build_thresh_arrays(D_spec, Headp);
    /*  draw_cap_side (D_spec, Headp, G3header, Cap, -1); */
    /* hey - so we cannot watch solids! MN 2001 */
}


/*************************** init_bounds *************************************/
/* this subroutine resets the display boundaries along the xyz axis */

void init_bounds(struct dspec *D_spec)
{
    D_spec->B[X] = 0;
    D_spec->B[Y] = 0;
    D_spec->B[Z] = 0;
    D_spec->E[X] = Headfax.xdim;
    D_spec->E[Y] = Headfax.ydim;
    D_spec->E[Z] = Headfax.zdim;
}

void init_dspec(struct dspec *D_spec, char *ctable)
{
    D_spec->Thresh = 0;
    D_spec->nt = 0;		/* number of indexes chosen (cumulative) */
    D_spec->xscale = 1.0;
    D_spec->yscale = 1.0;
    D_spec->zscale = 1.0;
    D_spec->xrot = 0;
    D_spec->yrot = 0;
    D_spec->zrot = 0;		/* angle in degrees */
    D_spec->Xrot = 0;
    D_spec->Yrot = 0;
    D_spec->Zrot = 0;		/* indicates if do autorotate */
    D_spec->ztrans = 0;
    D_spec->Specular = 10;
    D_spec->low = 0;
    D_spec->hi = Headfax.linefax.nthres - 1;
    D_spec->in_out = 0;		/*defined as INSIDE */
    D_spec->grid = 0;
    if (0 > get_color_table(ctable, D_spec->ctable)) {
	fprintf(stderr, "Using default color table\n");
	get_default_table(&Headfax, D_spec->ctable);
    }

}

/******************************* options ************************************/
void options()
{
    /*DISPLAY INSTRUCTIONS FOR THE KEYBOARD INTERACTIVE PROGRAM */
    fprintf(stderr, "\nTHE INTERACTIVE OPTIONS ARE:\n\n");
    fprintf(stderr, "?, (t #), (T # #), +, -\n");
    fprintf(stderr, "(x #) (y #) (z #) (X #) (Y #) (Z #)\n ");
    fprintf(stderr, "B(x,y,z)#), (E(x,y,z)#), R, d ,g, s ,W, w,i,c,Q\n");
    fprintf(stderr, "\nUSAGE AND MEANING:\n\n");
    fprintf(stderr, "?         lists available thresholds\n");
    fprintf(stderr, "l index# index# ...  add thresholds to display list\n");
    fprintf(stderr,
	    "L        display list of thresholds entered with \"l\" directive\n");
    /*
       fprintf(stderr,"t index#  add threshold to display list \n");
     */
    fprintf(stderr, "T index#  reset so only this threshold is displayed\n");
    fprintf(stderr,
	    "+(+++)    display thresholds with consecutively increasing index#\n");
    fprintf(stderr,
	    "-(---)    display thresholds with consecutively decreasing index#\n\n");
    fprintf(stderr,
	    "x int#    absolute rotation around x-axis in degrees(int) \n");
    fprintf(stderr,
	    "y int#    absolute rotation around y-axis in degrees(int) \n");
    fprintf(stderr,
	    "z int#    absolute rotation around z-axis in degrees(int) \n");
    fprintf(stderr, "r  rotate_model\n");
    fprintf(stderr, "g  toggle grid display\n");

    fprintf(stderr, "X int#   scale model in x\n");
    fprintf(stderr, "Y int#   scale model in y\n");
    fprintf(stderr, "Z int#   scale model in z\n\n");
    fprintf(stderr, "S int#    specular highlight control\n");
    fprintf(stderr, "B(x,y,z)int#  begin display along (x,y,z) axis at #\n");
    fprintf(stderr, "E(x,y,z)int#  end display along (x,y,z)axis #\n");
    fprintf(stderr, "R   resets display along axis to show all data\n\n");
    fprintf(stderr, "C   toggles the c_flag\n");
    fprintf(stderr, "c   clears the display (no thresholds)\n");
    fprintf(stderr, "w filename  write gif file image\n");
    fprintf(stderr, "W filename  dump raw image buffer file\n");
    fprintf(stderr, "i filename  read raw image buffer file\n");
    fprintf(stderr, "d   draw \n");
    fprintf(stderr, "Q   QUIT\n");
    fprintf(stderr, "h   help\n");
}

void Toggle_swapbuffers(struct dspec *D_spec)
{
    clear_screen();

}

void clear_screen()
{

    glDisable(GL_LIGHTING);
    glClearColor(1., 1., 1., 1.);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING);

}

/********************************************************************/
/* check begin & end planes for specified axis to make sure:
   1.) Begin < End
   2.) Begin > 0 & End < max */

/********************************************************************/
void check_limits(struct dspec *D_spec, int axis)
{
    int max;
    int tmp;

    max = (axis == X) ? Headfax.xdim :
	(axis == Y) ? Headfax.ydim : Headfax.zdim;

    if (D_spec->B[axis] > D_spec->E[axis]) {
	tmp = D_spec->B[axis];
	D_spec->B[axis] = D_spec->E[axis];
	D_spec->E[axis] = tmp;
    }

    if (D_spec->B[axis] < 0)
	D_spec->B[axis] = 0;
    if (D_spec->E[axis] > max)
	D_spec->E[axis] = max;
}
void do_translate(file_info * Headp, struct dspec *D_spec)
{
    float xd, yd, zd, trd;

    xd = Headp->xdim * D_spec->xscale;
    yd = Headp->ydim * D_spec->yscale;
    zd = Headp->zdim * D_spec->zscale;

    /* pick greatest dimension to use for translation of viewer from origin */
    if (xd < yd)
	trd = yd;
    else
	trd = xd;
    if (trd < zd)
	trd = zd;

    glTranslatef(0.0, 0.0, -trd * 1.6);	/* move it away from eye */
}

void copy_head(file_info * g3head, file_info * head)
{
    head->north = g3head->north;
    head->south = g3head->south;
    head->east = g3head->east;
    head->west = g3head->west;
    head->top = g3head->top;
    head->bottom = g3head->bottom;
    head->ns_res = g3head->ns_res;
    head->ew_res = g3head->ew_res;
    head->tb_res = g3head->tb_res;
    head->zone = g3head->zone;
    head->proj = g3head->proj;
}
