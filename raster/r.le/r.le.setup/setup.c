/*
 ************************************************************
 * MODULE: r.le.setup/setup.c                               *
 *         Version 5.0beta            Oct. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To set up sampling areas, which can can then    *
 *         be used to obtain data using the r.le.dist,      *
 *         r.le.patch, and r.le.pixel programs.  The        *
 *         setup.c code displays menus on the screen and    *
 *         asks for user input                              *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "setup.h"
#include <grass/config.h>

struct Colors *colors_old;

static void change_draw(void);
static void set_rgn(double *msc, char *name, char *name1, char *name2);
static void set_frame(double *msc, int *t, int *b, int *l, int *r);
static void ppoint(double *m, int x, int y, int num);
static void pbutton(int opt);
static void save_rgn(char *name, char *tempfile, FILE * tmp, int *x, int *y,
		     int pts, int class, int opt);
static void print_hd(FILE * mapfile, struct Cell_head *universe);
static void scr_cell(struct Cell_head *wind, int top, int bot, int left,
		     int right, double *m);
static void change_color(char *name, char *name1, char *name2);

static void change_draw(void)
{

    int method;
    double dtmp;

    G_system("clear");
    fprintf(stderr, "\n\n    CHOOSE THE COLOR FOR DRAWING:\n\n");
    fprintf(stderr, "       Red             1\n");
    fprintf(stderr, "       Orange          2\n");
    fprintf(stderr, "       Yellow          3\n");
    fprintf(stderr, "       Green           4\n");
    fprintf(stderr, "       Blue            5\n");
    fprintf(stderr, "       Indigo          6\n");
    fprintf(stderr, "       White           7\n");
    fprintf(stderr, "       Black           8\n");
    fprintf(stderr, "       Brown           9\n");
    fprintf(stderr, "       Magenta        10\n");
    fprintf(stderr, "       Aqua           11\n");
    fprintf(stderr, "       Gray           12\n\n");

    do {
	fprintf(stderr, "\n                             Which Number?   ");
	numtrap(1, &dtmp);
	if ((method = fabs(dtmp)) > 12 || method < 1) {
	    fprintf(stderr, "\n    Choice must between 1-12; try again");
	}
    }
    while (method > 12 || method < 1);

    if (method == 1)
	R_standard_color(D_translate_color("red"));
    else if (method == 2)
	R_standard_color(D_translate_color("orange"));
    else if (method == 3)
	R_standard_color(D_translate_color("yellow"));
    else if (method == 4)
	R_standard_color(D_translate_color("green"));
    else if (method == 5)
	R_standard_color(D_translate_color("blue"));
    else if (method == 6)
	R_standard_color(D_translate_color("indigo"));
    else if (method == 7)
	R_standard_color(D_translate_color("white"));
    else if (method == 8)
	R_standard_color(D_translate_color("black"));
    else if (method == 9)
	R_standard_color(D_translate_color("brown"));
    else if (method == 10)
	R_standard_color(D_translate_color("magenta"));
    else if (method == 11)
	R_standard_color(D_translate_color("aqua"));
    else if (method == 12)
	R_standard_color(D_translate_color("gray"));

    return;

}


				/* SHOW MAIN MENU AND INVOKE THE SETUP
				   ROUTINES */

void set_map(char *name, char *name1, char *name2, struct Cell_head window,
	     int top, int bot, int left, int right)
{
    char cmd[30], cmd1[30], cmd2[30], **sel;
    int i, j, btn, d, class, top0, bot0, right0, left0, paint = 0, method;
    double msc[2], dtmp;


    /* VARIABLES
       IN:
       name = raster map name to be set up
       name1 = overlay vector map name
       name2 = overlay site map name
     */

    colors_old = (struct Colors *)G_malloc(1 * sizeof(struct Colors));
    G_init_colors(colors_old);
    G_read_colors(name, G_mapset(), colors_old);

    G_system("clear");
    paint_map(name, name1, name2);
    paint = 1;

    /* setup the screen to raster map 
       coordinate conversion system */

    scr_cell(&window, top, bot, left, right, msc);

    top0 = top;
    bot0 = bot;
    left0 = left;
    right0 = right;

    /* display the menu and instructions */
  again:
    if (!paint) {
	if (G_yes
	    ("\n    Refresh the screen before choosing more setup?  ", 1))
	    paint_map(name, name1, name2);
    }
    else
	G_system("clear");

    fprintf(stderr, "\n\n    CHOOSE THE SETUP OPTION:\n\n");
    fprintf(stderr, "       Draw sampling regions                1\n");
    fprintf(stderr, "       Setup a sampling frame               2\n");
    fprintf(stderr, "       Setup sampling units                 3\n");
    fprintf(stderr, "       Setup a moving window                4\n");
    fprintf(stderr, "       Setup group or class limits          5\n");
    fprintf(stderr, "       Change the raster map color table    6\n");
    fprintf(stderr, "       Exit and save setup                  7\n");

    do {
	fprintf(stderr, "\n                                Which Number?   ");
	dtmp = 5.0;
	numtrap(1, &dtmp);
	if ((method = fabs(dtmp)) > 7 || method < 1) {
	    fprintf(stderr, "\n    Choice must between 1-7; try again");
	}
    }
    while (method > 7 || method < 1);

    /* setup regions */
    if (method == 1)
	set_rgn(msc, name, name1, name2);

    /* setup the sampling frame */
    else if (method == 2) {
	top = top0;
	bot = bot0;
	right = right0;
	left = left0;
	set_frame(msc, &top, &bot, &left, &right);
    }

    /* setup sampling units */

    else if (method == 3) {
	sample(top, bot, left, right, name, name1, name2, msc);
    }

    /* setup the moving window */

    else if (method == 4) {
	mov_wind(top, bot, left, right, name, name1, name2, msc);
    }

    /* setup group/class limits */

    else if (method == 5) {

	/* setup the buffer to store the user's input */

	sel = (char **)G_malloc(10 * sizeof(char *));
	for (i = 0; i < 9; i++)
	    sel[i] = (char *)G_calloc(2, sizeof(char));

      back:
	ask_group(sel);

	/* check for no input */

	if (sel[0][0] != 'x' && sel[1][0] != 'x' && sel[2][0] != 'x' &&
	    sel[3][0] != 'x' && sel[4][0] != 'x' && sel[5][0] != 'x' &&
	    sel[6][0] != 'x' && sel[7][0] != 'x' && sel[8][0] != 'x') {
	    G_system("clear");
	    fprintf(stderr,
		    "    Did you mean to not make any attribute group");
	    if (!G_yes("\n    or index class setup choices?   ", 1))
		goto back;
	}

	/* if there is input, then invoke the
	   group/class setup module and then free
	   the memory allocated for selections */

	else {
	    get_group_drv(sel);
	    for (i = 0; i < 9; i++)
		G_free(sel[i]);
	    G_free(sel);
	}
    }

    /* change color tables */

    else if (method == 6)
	change_color(name, name1, name2);

    /* reset the colortable and exit */

    else if (method == 7) {
	G_write_colors(name, G_mapset(), colors_old);
	G_free_colors(colors_old);
	/*     R_close_driver(); */
	G_system("d.frame -e");
	exit(0);
    }
    paint = 0;
    goto again;
    return;
}




				/* REDISPLAY THE RASTER MAP AND THE
				   OVERLAYS */

void paint_map(char *n1, char *n2, char *n3)
{
    char *cmd;

    cmd = G_malloc(120);

    G_system("clear");
    sprintf(cmd, "d.rast %s", n1);
    G_system("d.erase");
    G_system(cmd);
    if (n2) {
	sprintf(cmd, "d.vect %s color=black", n2);
	G_system(cmd);
    }
    if (n3) {
	sprintf(cmd, "d.vect %s color=black", n3);
	G_system(cmd);
    }
    G_free(cmd);
    return;
}






				/* SETUP REGIONS */

static void set_rgn(double *msc, char *name, char *name1, char *name2)
{
    char reg_name[20];
    int x0, y0, xp, yp, *x, *y, xstart, ystart, btn, d, method, meth;
    static int pts, rgn_cnt = 0;
    double dtmp, etmp;
    FILE *tmp;
    char *tempfile;

    /* get the name of the regions map */

    if (!G_ask_cell_new("    ENTER THE NEW REGION MAP NAME:", reg_name))
	return;

    /* allocate memory for storing the
       points along the boundary of each
       region */

    x = (int *)G_malloc(100 * sizeof(int));
    y = (int *)G_malloc(100 * sizeof(int));

    tempfile = G_tempfile();
    tmp = fopen(tempfile, "w");

  back2:
    G_system("clear");
    fprintf(stderr, "\n\n    CHOOSE AN OPTION:\n\n");
    fprintf(stderr, "       Draw a region                     1\n");
    fprintf(stderr, "       Quit drawing regions and return");
    fprintf(stderr, "\n          to setup options menu          2\n");
    fprintf(stderr, "       Change the color for drawing      3\n\n");

    do {
	fprintf(stderr, "                             Which Number?   ");
	numtrap(1, &etmp);
	if ((meth = fabs(etmp)) > 3 || meth < 1) {
	    fprintf(stderr, "\n    Choice must between 1-3; try again");
	}
    }
    while (meth > 3 || meth < 1);

    if (meth == 2)
	return;
    if (meth == 3) {
	R_open_driver();
	change_draw();
    }
    if (meth == 1) {
	R_open_driver();
	rgn_cnt = 0;
    }

    /* ask the user to outline a region */

  back:
    G_system("clear");
    ppoint(NULL, 0, 0, -1);
    fprintf(stderr, "\n    PLEASE OUTLINE REGION # %d\n", (++rgn_cnt));
    pbutton(0);
    pts = 0;
    x0 = 0;
    y0 = 0;

    /* get the points along the boundary
       of the region as they are drawn */

    do {
	btn = 0;
	R_get_location_with_line(x0, y0, &xp, &yp, &btn);
	if (btn == 1)
	    ppoint(msc, xp, yp, 0);
	else if (btn == 2) {
	    if (!pts) {
		pbutton(1);
		R_move_abs(xp, yp);
		xstart = xp;
		ystart = yp;
	    }
	    x[pts] = xp * msc[0];
	    y[pts] = yp * msc[1];
	    ppoint(msc, xp, yp, (++pts));
	    x0 = xp;
	    y0 = yp;
	    R_cont_abs(x0, y0);
	}
	else if (btn == 3 && pts < 3) {
	    fprintf(stderr,
		    "\n\n    Please digitize more than 2 boundary points\n\n");
	}
    }
    while (btn != 3 || pts < 3);

    R_cont_abs(xstart, ystart);
    R_close_driver();
    R_open_driver();
    x[pts] = x[0];
    y[pts] = y[0];
    pts++;

    /* redisplay the menu and find out what
       to do next */
  back1:
    G_system("clear");
    fprintf(stderr, "\n\n    CHOOSE AN OPTION:\n\n");
    fprintf(stderr,
	    "       Draw another region                          1\n");
    fprintf(stderr,
	    "       Start over drawing regions                   2\n");
    fprintf(stderr,
	    "       Quit drawing and save the region map         3\n");
    fprintf(stderr,
	    "       Quit drawing and don't save the region map   4\n");
    fprintf(stderr,
	    "       Change the color for drawing                 5\n\n");
    do {
	fprintf(stderr,
		"                                        Which Number?  ");
	numtrap(1, &dtmp);
	if ((method = fabs(dtmp)) > 5 || method < 1) {
	    fprintf(stderr, "\n    Choice must between 1-5; try again");
	}
    }
    while (method > 5 || method < 1);


    /* save the region and draw another */

    if (method == 1) {
	save_rgn(reg_name, tempfile, tmp, x, y, pts, rgn_cnt, 1);
	goto back;
    }

    /* start over */

    else if (method == 2) {
	fclose(tmp);
	if (!(tmp = fopen(tempfile, "w")))
	    G_fatal_error
		("Can't open temporary file for storing region info\n");
	rgn_cnt = 0;
	R_close_driver();
	paint_map(name, name1, name2);
	goto back2;
    }

    /* save the region and exit */

    else if (method == 3)
	save_rgn(reg_name, tempfile, tmp, x, y, pts, rgn_cnt, 2);


    /* change the color for drawing */

    else if (method == 5) {
	change_draw();
	goto back1;
    }

    R_close_driver();
    G_free(x);
    G_free(y);
    unlink(tempfile);
    return;

}




				/* SETUP THE SAMPLING FRAME */

static void set_frame(double *msc, int *t, int *b, int *l, int *r)
{
    int t0, b0, l0, r0, btn;

    /* record the initial boundaries of the map */

    t0 = *t;
    b0 = *b;
    l0 = *l;
    r0 = *r;

    /* if the total area to be sampled will be the
       whole map */

    G_system("clear");

    if (G_yes
	("\n    Will the sampling frame (total area within which sampling\n      units are distributed) be the whole map?   ",
	 1)) {
	R_open_driver();
	R_standard_color(D_translate_color("grey"));
	draw_box(*l, *t, *r, *b, 1);
	R_close_driver();
	fprintf(stderr, "\n    Sampling frame set to whole map");
    }

    /* if the total area to be sampled is not the
       whole map, then have the user draw the
       area */

    else {
      back:
	G_system("clear");
	fprintf(stderr, " \n    OUTLINE SAMPLING FRAME:\n");
	R_open_driver();
	fprintf(stderr,
		"\n    Please move cursor to the UPPER-LEFT corner of\n");
	fprintf(stderr,
		"       the sampling frame and click any mouse button\n");
	R_get_location_with_line(0, 0, l, t, &btn);

	fprintf(stderr,
		"\n    Please move cursor to the LOWER-RIGHT corner of\n");
	fprintf(stderr,
		"       the sampling frame and click any mouse button again\n");
      back2:
	R_get_location_with_box(*l, *t, r, b, &btn);

	/* check that sampling frame is in map */

	if (*l < l0 || *r > r0 || *t < t0 || *b > b0) {
	    fprintf(stderr,
		    "\n    The cursor is outside of the map, try again\n");
	    goto back;
	}

	/* check that cursor is below & to right */

	if (*r <= *l || *b <= *t) {
	    fprintf(stderr,
		    "\n    Please put the lower right corner below and to the");
	    fprintf(stderr, "\n    right of the upper left corner\n");
	    goto back2;
	}

	R_standard_color(D_translate_color("grey"));
	*l = (int)((double)((int)(*l * msc[0] + 0.5)) / msc[0]);
	*r = (int)((double)((int)(*r * msc[0] + 0.5)) / msc[0]);
	*t = (int)((double)((int)(*t * msc[1] + 0.5)) / msc[1]);
	*b = (int)((double)((int)(*b * msc[1] + 0.5)) / msc[1]);
	draw_box(*l, *t, *r, *b, 1);
	R_close_driver();
	fprintf(stderr,
		"\n    Sampling frame is set to the area you just drew");
    }
    return;
}





				/* SHOW THE CURSOR COORDINATES TO
				   THE USER */

static void ppoint(double *m, int x, int y, int num)
{
    register int i;

    if (num < 0) {
	for (i = 0; i < 80; i++)
	    fprintf(stderr, " ");
    }

    else {
	if (num > 0) {
	    fprintf(stderr, "    Point %d is at Row %5d and Col %5d",
		    num, (int)(y * m[1]), (int)(x * m[0]));
	}
	else if (num == 0) {
	    fprintf(stderr, "    Point is at Row %5d and Col %5d",
		    (int)(y * m[1]), (int)(x * m[0]));
	}

	for (i = 0; i < 80; i++) {
	    fprintf(stderr, "\b");
	}
    }
    return;
}




				/* PRINT THE INSTRUCTIONS FOR USING THE
				   MOUSE BUTTONS */

static void pbutton(int opt)
{
    static char *str[2] = { "start", "next" };
    fprintf(stderr, "\n    Use the mouse to outline the region\n");
    fprintf(stderr,
	    "       Left button:     What are row & column coordinates at this point?\n");
    fprintf(stderr, "       Middle button:   Mark %s point\n", str[opt]);
    fprintf(stderr,
	    "       Right  button:   Finish region-connect to first point\n\n");
    return;
}





/* SAVE THE REGION */
static void save_rgn(char *name, char *tempfile, FILE * tmp, int *x, int *y,
		     int pts, int class, int opt)
{
    char *cmd;
    struct Cell_head wind;
    struct Colors colors;
    static double rx, ry, ofy, ofx;
    register int i;

    /* setup the temporary file to save
       the region boundary pts */

    if (class == 1) {
	G_get_set_window(&wind);
	print_hd(tmp, &wind);
	ofy = wind.north;
	ofx = wind.west;
	ry = (wind.north - wind.south) / wind.rows;
	rx = (wind.east - wind.west) / wind.cols;
    }
    fprintf(tmp, "A %10.2f %10.2f %10d\n", (ofy - *y * ry), (*x * rx + ofx),
	    class);
    for (i = 0; i < pts; i++)
	fprintf(tmp, "  %10.2f %10.2f\n", (ofy - *(y + i) * ry),
		(*(x + i) * rx + ofx));

    /* if the choice was made to draw more
       regions, then return */

    if (opt != 2)
	return;

    fprintf(tmp, "E\n");
    fclose(tmp);
    G_get_set_window(&wind);
    G_put_cellhd(name, &wind);

    /* make a GRASS raster map from the
       region boundary pts, using the
       poly_to_bmif and bmif_to_cell
       programs */

    cmd = G_malloc(200);
    sprintf(cmd,
	    "%s/etc/poly_to_bmif < %s | sort -t: +0n -1 | %s/etc/bmif_to_cell %s",
	    G_gisbase(), tempfile, G_gisbase(), name);
    fprintf(stderr, "    Generating '%s' file... %20c\n\n", name, ' ');
    G_system(cmd);
    G_free(cmd);

    /* set the color table for the regions
       file to color wave */

    G_init_colors(&colors);
    G_make_wave_colors(&colors, 1, class);
    G_write_colors(name, G_mapset(), &colors);

    /* overlay the region file on the
       screen */

    if (!(cmd = G_malloc(20)))
	G_fatal_error("Can't allocate enough memory\n");
    R_close_driver();
    sprintf(cmd, "d.rast -o  %s", name);
    G_system(cmd);
    G_free(cmd);
    G_sleep(4);			/* hold the screen for 4 seconds */
    R_open_driver();
    return;
}






				/* SETUP THE HEADER FOR THE REGION
				   FILE */

static void print_hd(FILE * mapfile, struct Cell_head *universe)
{
    fprintf(mapfile, "TITLE:\n");
    fprintf(mapfile, "	User created region.\n");
    fprintf(mapfile, "ENDT\n");
    fprintf(mapfile, "SIZE      %10d %10d\n", universe->rows, universe->cols);
    fprintf(mapfile, "BOUND     %10.2f %10.2f %10.2f %10.2f\n",
	    universe->ns_res, universe->ew_res,
	    universe->south, universe->west);
    fprintf(mapfile, "VERTI\n");
    return;
}





				/* SETUP THE CONVERSION BETWEEN SCREEN
				   AND RASTER COORDINATES */

static void scr_cell(struct Cell_head *wind, int top, int bot, int left,
		     int right, double *m)
{
    m[0] = (double)wind->cols / (right - left);
    m[1] = (double)wind->rows / (bot - top);
    return;
}





				/* CHANGE THE COLORTABLE OF THE RASTER
				   MAP */

static void change_color(char *name, char *name1, char *name2)
{
    struct Colors colors;
    struct Range range;
    int d, min, max;
    double etmp;


    G_read_range(name, G_mapset(), &range);
    G_get_range_min_max(&range, &min, &max);
    G_system("clear");

  again:
    fprintf(stderr, "\n\n    SELECT NEW COLOR TABLE FOR RASTER MAP:\n\n");
    fprintf(stderr, "       Aspect                           1\n");
    fprintf(stderr, "       Color ramp                       2\n");
    fprintf(stderr, "       Color wave                       3\n");
    fprintf(stderr, "       Linear grey scale                4\n");
    fprintf(stderr, "       Rainbow colors                   5\n");
    fprintf(stderr, "       Random colors                    6\n");
    fprintf(stderr, "       Red-Yellow-Green Sequence        7\n");
    fprintf(stderr, "       Green-Yellow-Red Sequence        8\n");
    fprintf(stderr, "       Set original color table         9\n");
    fprintf(stderr, "       Return to setup options menu    10\n");

    do {
	fprintf(stderr, "\n                             Which Number?  ");
	numtrap(1, &etmp);
	if ((d = fabs(etmp)) > 10 || d < 1) {
	    fprintf(stderr, "\n    Choice must between 1-10; try again");
	}
    }
    while (d > 10 || d < 1);

    if (d == 1) {
	G_make_aspect_colors(&colors, min, max);
	G_write_colors(name, G_mapset(), &colors);
    }
    else if (d == 2) {
	G_make_ramp_colors(&colors, min, max);
	G_write_colors(name, G_mapset(), &colors);
    }
    else if (d == 3) {
	G_make_wave_colors(&colors, min, max);
	G_write_colors(name, G_mapset(), &colors);
    }
    else if (d == 4) {
	G_make_grey_scale_colors(&colors, min, max);
	G_write_colors(name, G_mapset(), &colors);
    }
    else if (d == 5) {
	G_make_rainbow_colors(&colors, min, max);
	G_write_colors(name, G_mapset(), &colors);
    }
    else if (d == 6) {
	G_make_random_colors(&colors, min, max);
	G_write_colors(name, G_mapset(), &colors);
    }
    else if (d == 7) {
	G_make_ryg_colors(&colors, min, max);
	G_write_colors(name, G_mapset(), &colors);
    }
    else if (d == 8) {
	G_make_gyr_colors(&colors, min, max);
	G_write_colors(name, G_mapset(), &colors);
    }
    else if (d == 9) {
	G_write_colors(name, G_mapset(), colors_old);
    }
    else if (d == 10) {
	return;
    }

    paint_map(name, name1, name2);

    fprintf(stderr, "\n    CHOOSE NEXT OPTION:\n\n");
    fprintf(stderr, "       Don't save color table just chosen:");
    fprintf(stderr, "\n         Return to color table menu            1\n");
    fprintf(stderr, "         Return to setup option menu           2\n");
    fprintf(stderr, "         Exit r.le.setup                       3\n\n");
    fprintf(stderr, "       Do save color table just chosen:");
    fprintf(stderr, "\n         Return to setup options menu          4\n");
    fprintf(stderr, "         Exit r.le.setup                       5\n");

    do {
	fprintf(stderr,
		"\n                                   Which Number?  ");
	numtrap(1, &etmp);
	if ((d = fabs(etmp)) > 5 || d < 1) {
	    fprintf(stderr, "\n    Choice must between 1-5; try again");
	}
    }
    while (d > 5 || d < 1);

    if (d == 1)
	goto again;
    else if (d == 2)
	return;
    else if (d == 3) {
	G_write_colors(name, G_mapset(), colors_old);
	G_free_colors(colors_old);
    }
    else if (d == 4)
	*colors_old = colors;
    if (d == 3 || d == 5) {
	G_system("d.frame -e");
	exit(0);
    }
    return;
}
