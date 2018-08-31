
/****************************************************************************
 *
 * MODULE:       m.nviz.script  (nee d.nviz)
 * AUTHOR(S):    Bob Covill <bcovill@tekmap.ns.ca>
 * PURPOSE:      interactively create fly-through script for NVIZ
 * Functions:
 *   main --      parse parameters and get key frame coorinates
 *   do_profile-- calculate camera and eye coordinates from
 *                raster map
 *   move --      part of screen coords
 *   cont --      part of screen coords 
 *   read_rast -- return camera and eye coordinates
 *
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "local.h"
#include <grass/glocale.h>

int cut_val, frame = 0;
int height_flag = 0;
double OLD_DEPTH;
char img_name[512];
int no_frames;
int cnt = 1;
int key_frames = 0;
int off_screen = 0;
double key_time = 0.;
FILE *fp, *fp2;

int main(int argc, char *argv[])
{
    const char *name;
    char outfile[GNAME_MAX];
    int fd, projection;
    char buf[512], buf1[1024], buf2[1024];
    int screen_x, screen_y;
    int i, k;
    int frame_start = 0;
    double e1, e2, n1, n2;
    RASTER_MAP_TYPE data_type;
    struct Cell_head window;
    struct
    {
	struct Option *opt1, *route, *name, *output, *dist, *ht, *frames,
	    *start;
	struct Flag *f, *c, *k, *o, *e;
    }
    parm;
    struct GModule *module;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("miscellaneous"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("vector"));
    G_add_keyword(_("visualization"));
    module->description = _("Creates fly-through script to run in NVIZ.");

    parm.opt1 = G_define_standard_option(G_OPT_R_INPUT);

    parm.output = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.output->description = _("Name of output script");

    parm.name = G_define_option();
    parm.name->key = "name";
    parm.name->type = TYPE_STRING;
    parm.name->required = NO;
    parm.name->description = _("Prefix of output images (default = NVIZ)");

    parm.route = G_define_option();
    parm.route->key = "route";
    parm.route->type = TYPE_STRING;
    parm.route->required = NO;
    parm.route->multiple = YES;
    parm.route->key_desc = "east,north";
    parm.route->description = _("Route coordinates (east,north)");

    parm.dist = G_define_option();
    parm.dist->key = "dist";
    parm.dist->type = TYPE_DOUBLE;
    parm.dist->required = YES;
    parm.dist->description = _("Camera layback distance (in map units)");

    parm.ht = G_define_option();
    parm.ht->key = "ht";
    parm.ht->type = TYPE_DOUBLE;
    parm.ht->required = YES;
    parm.ht->description = _("Camera height above terrain");

    parm.frames = G_define_option();
    parm.frames->key = "frames";
    parm.frames->type = TYPE_INTEGER;
    parm.frames->required = YES;
    parm.frames->description = _("Number of frames");

    parm.start = G_define_option();
    parm.start->key = "start";
    parm.start->type = TYPE_INTEGER;
    parm.start->required = NO;
    parm.start->description = _("Start frame number (default=0)");


    parm.f = G_define_flag();
    parm.f->key = 'f';
    parm.f->description = _("Full render -- Save images");

    parm.c = G_define_flag();
    parm.c->key = 'c';
    parm.c->description = _("Fly at constant elevation (ht)");

    parm.k = G_define_flag();
    parm.k->key = 'k';
    parm.k->description =
	_("Include command in the script to output a KeyFrame file");

    parm.o = G_define_flag();
    parm.o->key = 'o';
    parm.o->description = _("Render images off-screen");

    parm.e = G_define_flag();
    parm.e->key = 'e';
    parm.e->description = _("Enable vector and sites drawing");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* check arguments */
    if (!parm.route->answer)
	G_fatal_error(_("Either -i flag and/or route parameter must be used"));

    /* get GRASS parameters */
    G_get_window(&window);
    projection = G_projection();
    D_do_conversions(&window, 0, 1, 0, 1);

    /* setup screen coords */
    screen_x = ((int)D_get_d_west() + (int)D_get_d_east()) / 2;
    screen_y = ((int)D_get_d_north() + (int)D_get_d_south()) / 2;

    /* get camera parameters */
    DIST = atof(parm.dist->answer);
    HT = atof(parm.ht->answer);
    no_frames = atoi(parm.frames->answer);

    if (parm.start->answer)
	frame_start = atoi(parm.start->answer);

    if (parm.c->answer)
	height_flag = 1;
    if (parm.k->answer)
	key_frames = 1;
    if (parm.o->answer && !parm.f->answer)
	G_fatal_error(_("Off-screen only available with full render mode"));
    if (parm.o->answer)
	off_screen = 1;

    /* Initialize coords */
    e1 = e2 = n1 = n2 = -9999.;

    G_begin_distance_calculations();

    /* Open Input File for reading */
    name = parm.opt1->answer;

    /* Open Raster File */
    fd = Rast_open_old(name, "");

    /* Set Image name */
    if (parm.name->answer)
	sprintf(img_name, "%s", parm.name->answer);
    else
	sprintf(img_name, "NVIZ");


    /* Open ASCII file for output */
    /* append ".nvscr" to filename if it doesn't already have it */
    strncpy(outfile, parm.output->answer, GNAME_MAX - 7);
    outfile[GNAME_MAX - 7] = '\0';	/* strncpy() doesn't null terminate the string */
    if (strcmp(&outfile[strlen(outfile) - 6], ".nvscr") != 0)
	strcat(outfile, ".nvscr");

    if (NULL == (fp = fopen(outfile, "w")))
	G_fatal_error(_("Unable to open file <%s>"), outfile);

    /* Get Raster Type */
    data_type = Rast_get_map_type(fd);
    /* Done with file */

    /* Output initial startup stuff */
    sprintf(buf1,
	    "## REGION: n=%f s=%f e=%f w=%f\n## Input=%s Dist=%f Ht=%f\n",
	    window.north, window.south, window.east, window.west, outfile,
	    DIST, HT);

    sprintf(buf2, "\nset FRAMES %d\n", no_frames);
    strcat(buf1, buf2);
    fprintf(fp, "%s", buf1);

    sprintf(buf1, "SendScriptLine \"Nclear_keys\"");
    sprintf(buf2, "\nSendScriptLine \"Nupdate_frames\"");
    strcat(buf1, buf2);
    fprintf(fp, "%s", buf1);

    sprintf(buf1, "\nSendScriptLine \"Nset_numsteps $FRAMES\"");
    sprintf(buf2, "\nSendScriptLine \"Nupdate_frames\"\n");
    strcat(buf1, buf2);
    fprintf(fp, "%s", buf1);

    /* Use Linear mode for smooth frame transition */
    sprintf(buf1, "\nSendScriptLine \"Nset_interp_mode linear\"");
    sprintf(buf2, "\nSendScriptLine \"Nupdate_frames\"\n\n");
    strcat(buf1, buf2);
    fprintf(fp, "%s", buf1);

    /* eanble vector and sites drawing */
    if (parm.e->answer) {
	sprintf(buf1, "\nSendScriptLine \"Nshow_vect on\"");
	sprintf(buf2, "\nSendScriptLine \"Nshow_sites on\"\n\n");
	strcat(buf1, buf2);
	fprintf(fp, "%s", buf1);
    }

    /* Coords from Command Line */
    for (i = 0; parm.route->answers[i]; i += 2) {
	/* Test for number coordinate pairs */
	k = i;
    }

    if (k < 6) {
	/* Only one coordinate pair supplied */
	G_fatal_error(_("You must provide at least four points %d"), k);

    }
    else {
	for (i = 0; i <= k - 2; i += 2) {
	    sscanf(parm.route->answers[i], "%lf", &e1);
	    sscanf(parm.route->answers[i + 1], "%lf", &n1);
	    sscanf(parm.route->answers[i + 2], "%lf", &e2);
	    sscanf(parm.route->answers[i + 3], "%lf", &n2);
	    /* Get profile info */
	    do_profile(e1, e2, n1, n2, name, fd, data_type);

	    /* Get last coord */
	    if (i == k - 2)
		do_profile(e2, e2, n2, n2, name, fd, data_type);
	}
    }
	/* done with coordinates */


    /* Output final part of script */
    /* generate key-frame script */
    if (key_frames) {
	strcpy(buf, outfile);
	buf[strlen(outfile) - 6] = '\0';	/* skip extension */
	strcat(buf, ".kanim");
	fprintf(fp, "\n## The following saves the animation to a format\n");
	fprintf(fp, "## suitable for editing with the kanimator panel\n");
	fprintf(fp, "SendScriptLine \"Nprint_keys %s\"\n", buf);
	fprintf(fp, "puts \"Saving Key Frame file %s\"\n", buf);
    }

    /* output off-screen option */
    if (off_screen) {
	fprintf(fp, "\n## Off screen rendering enabled \n");
	fprintf(fp, "## Ensure main window is minimized before running\n");
	fprintf(fp, "SendScriptLine \"Noff_screen 1\"\n");
    }

    fprintf(fp, "\n\nset num %d", frame_start);
    fprintf(fp, "\n\nfor {set frame 1} {$frame <= $FRAMES} {incr frame} {");

    if (parm.f->answer) {
	/* Full render and save */
	fprintf(fp, "\nset name %s", img_name);
	fprintf(fp, "\nset num2 [format \"\%%04d\" $num]");
	fprintf(fp, "\nappend name $num2 \".ppm\"");
	fprintf(fp, "\nSendScriptLine \"Ndo_framestep $frame 1\"");
	fprintf(fp, "\nSendScriptLine \"Nwrite_ppm $name \"");
	fprintf(fp, "\nincr num");
    }
    else {
	/* Quick draw wire */
	/* output full variables commented so can be easily changed */
	fprintf(fp, "\nset name %s", img_name);
	fprintf(fp, "\nset num2 [format \"\%%04d\" $num]");
	fprintf(fp, "\nappend name $num2 \".ppm\"");
	fprintf(fp,
		"\n## To render in full set to 1 and uncomment Nwrite_ppm \"");
	fprintf(fp, "\nSendScriptLine \"Ndo_framestep $frame 0\"");
	fprintf(fp, "\n#SendScriptLine \"Nwrite_ppm $name \"");
	fprintf(fp, "\nincr num");
    }

    fprintf(fp, "\n}\n");
    if (off_screen)
	fprintf(fp, "SendScriptLine \"Noff_screen 0\"\n");

    fprintf(fp, "SendScriptLine \"set ScriptPlaying 0\"\n");
    fprintf(fp, "puts \"DONE!\"\n");

    Rast_close(fd);
    fclose(fp);

    G_done_msg(_("Created NVIZ script <%s>."), outfile);

    exit(EXIT_SUCCESS);

}				/* Done with main */



/* ************************************
 * Claculate camera and eye coordinates
 **************************************/
int do_profile(double e1, double e2, double n1, double n2,
	       const char *name, int fd, int data_type)
{
    float rows, cols, LEN;
    double Y, X, AZI;

    cols = e1 - e2;
    rows = n1 - n2;

    LEN = G_distance(e1, n1, e2, n2);

    /* Calculate Azimuth of Line */
    if (rows == 0 && cols == 0) {
	/* Special case for no movement */
	/* do nothing */
	return 0;
    }

    if (rows >= 0 && cols < 0) {
	/* SE Quad or due east */
	AZI = fabs(atan((rows / cols)));
	Y = (double)DIST *sin(AZI);
	X = (double)DIST *cos(AZI);

	if (e != 0.0 && (e != e1 || n != n1)) {
	    dist -= G_distance(e, n, e1, n1);
	}
	read_rast(e2 - X, n2 + Y, LEN, fd, 1, data_type);
	read_rast(e2, n2, LEN, fd, 0, data_type);
    }

    if (rows < 0 && cols <= 0) {
	/* NE Quad  or due north */
	AZI = fabs(atan((cols / rows)));
	X = (double)DIST *sin(AZI);
	Y = (double)DIST *cos(AZI);

	if (e != 0.0 && (e != e1 || n != n1)) {
	    dist -= G_distance(e, n, e1, n1);
	}
	read_rast(e2 - X, n2 - Y, LEN, fd, 1, data_type);
	read_rast(e2, n2, LEN, fd, 0, data_type);
    }

    if (rows > 0 && cols >= 0) {
	/* SW Quad or due south */
	AZI = fabs(atan((rows / cols)));
	X = (double)DIST *cos(AZI);
	Y = (double)DIST *sin(AZI);

	if (e != 0.0 && (e != e1 || n != n1)) {
	    dist -= G_distance(e, n, e1, n1);
	}
	read_rast(e2 + X, n2 + Y, LEN, fd, 1, data_type);
	read_rast(e2, n2, LEN, fd, 0, data_type);
    }

    if (rows <= 0 && cols > 0) {
	/* NW Quad  or due west */
	AZI = fabs(atan((rows / cols)));
	X = (double)DIST *cos(AZI);
	Y = (double)DIST *sin(AZI);

	if (e != 0.0 && (e != e1 || n != n1)) {
	    dist -= G_distance(e, n, e1, n1);
	}
	read_rast(e2 + X, n2 - Y, LEN, fd, 1, data_type);
	read_rast(e2, n2, LEN, fd, 0, data_type);
    }

    return 0;
}				/* done with do_profile */


/*****************************
 * read_rast
 * function to get raster value at set location
 * and output nviz script
*****************************/
int read_rast
    (double east,
     double north,
     double dist, int fd, int out_type, RASTER_MAP_TYPE data_type)
{
    int row, col, nrows, ncols;
    struct Cell_head window;
    CELL *cell;
    char buf[1024] = "";
    char buf2[1024];
    FCELL *fcell;
    DCELL *dcell;
    double camera_height;


    G_get_window(&window);
    nrows = window.rows;
    ncols = window.cols;

    row = (int)(0.5 + D_u_to_a_row(north));
    col = (int)(0.5 + D_u_to_a_col(east));

    if (row < 0 || row > nrows || col < 0 || col > ncols) {
	G_debug(3, "Fail: row=%d  nrows=%d   col=%d  ncols=%d", row, nrows,
		col, ncols);
	G_warning(_("Skipping this point, selected point is outside region. "
		   "Perhaps the camera setback distance puts it beyond the edge?"));
	frame++;
	return 1;
    }


    if (data_type == CELL_TYPE) {
	cell = Rast_allocate_c_buf();
	Rast_get_c_row(fd, cell, row);

	if (Rast_is_c_null_value(&cell[col]))
	    camera_height = (double)9999.;
	else
	    camera_height = (double)cell[col];
    }

    if (data_type == FCELL_TYPE) {
	fcell = Rast_allocate_f_buf();
	Rast_get_f_row(fd, fcell, row);
	if (Rast_is_f_null_value(&fcell[col]))
	    camera_height = (double)9999.;
	else
	    camera_height = (double)fcell[col];
    }

    if (data_type == DCELL_TYPE) {
	dcell = Rast_allocate_d_buf();
	Rast_get_d_row(fd, dcell, row);
	if (Rast_is_d_null_value(&dcell[col]))
	    camera_height = (double)9999.;
	else
	    camera_height = (double)dcell[col];
    }

    /* Output script commands */

/*************************/

    /* Set camera Height value */
    if (camera_height == 9999.)
	camera_height = OLD_DEPTH;

    if (height_flag && out_type)
	camera_height = HT;
    else if (!height_flag && out_type)
	camera_height = camera_height + HT;

    if (out_type) {
	/* Set Camera Position */
	sprintf(buf2, "\nSendScriptLine \"Nmove_to_real %f %f %f\"",
		east, north, camera_height);
	key_time += (dist + fabs(camera_height - OLD_DEPTH)) / 10000.;
    }
    else {

	/* Set Center of View */
	sprintf(buf2, "\nSendScriptLine \"Nset_focus %f %f %f\"",
		east - window.west - (window.ew_res / 2),
		north - window.south - (window.ns_res / 2), camera_height);

	/* Use frame number for now -- TODO figure even increment
	 * based on no. of frames and distance */
	sprintf(buf,
		"\nSendScriptLine \"Nadd_key %f KF_ALL_MASK 1 0.0\"\n",
		key_time);
	strcat(buf2, buf);
	cnt++;
    }

    /* Out to file */
    fprintf(fp, "%s", buf2);
    OLD_DEPTH = camera_height;

    frame++;
    return 0;
}
