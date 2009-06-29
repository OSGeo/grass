/*
 ************************************************************
 * MODULE: r.le.setup/mv_wind.c                             *
 *         Version 5.0beta            Oct. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To set up sampling areas, which can can then    *
 *         be used to obtain data using the r.le.dist,      *
 *         r.le.patch, and r.le.pixel programs.  The        *
 *         mv_wind.c code queries the user for information  *
 *         needed to setup the moving window                *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <grass/display.h>
#include "setup.h"
#include <grass/config.h>



				/* SETUP THE PARAMETERS FOR THE 
				   MOVING WINDOW */

void mov_wind(int t, int b, int l, int r, char *n1, char *n2, char *n3,
	      double *mx)
{

    int xp = 0, yp = 0, x1, y1, btn = 0, s, l0, t0, xpl, ypt,
	u_w = 0, u_l = 0, u_w0, u_l0, w_w = 0, w_l = 0, initl = 0,
	initt = 0, d, fmask, j, circle = 0;
    register int i;
    double tmp[2], tmp1, radius = 0.0;
    FILE *fp;

    /*
       mx[0]        =       cols of region/width of screen
       mx[1]        =       rows of region/height of screen
       t    =       top row of sampling frame
       b    =       bottom row of sampling frame
       l    =       left col of sampling frame
       r    =       right col of sampling frame
     */

    /* open the moving window parameter file */

    fp = fopen0("r.le.para/move_wind", "w");
    G_sleep_on_error(0);

    initl = l;
    initt = t;

    l = (int)((double)(l * mx[0]) + 0.5);
    r = (int)((double)(r * mx[0]) + 0.5);
    t = (int)((double)(t * mx[1]) + 0.5);
    b = (int)((double)(b * mx[1]) + 0.5);

    /* display sampling frame */

    R_open_driver();
    R_standard_color(D_translate_color("grey"));
    draw_box((int)(l / mx[0] + 0.5), (int)(t / mx[1] + 0.5),
	     (int)(r / mx[0] + 0.5), (int)(b / mx[1] + 0.5), 1);
    R_close_driver();

    /* determine whether the user will use
       the keyboard or mouse to setup the 
       moving window */

  keyboard:
    fprintf(stderr, "\n\n    HOW WILL YOU SPECIFY THE MOVING WINDOW?\n");
    fprintf(stderr,
	    "\n       Use keyboard to enter moving window dimensions   1");
    fprintf(stderr,
	    "\n       Use mouse to draw moving window                  2\n");
    fprintf(stderr,
	    "\n                                            Which Number?  ");

    numtrap(1, &tmp1);
    d = (int)(tmp1);
    if (d < 1 || d > 2) {
	fprintf(stderr, "     You did not enter a 1 or 2, try again\n");

	goto keyboard;
    }

    if (d == 1 || d == 2) {
	/* return a value > 0 to fmask if there is
	   a MASK present */

	fprintf(stderr,
		"\n    If a MASK is not present (see r.mask) a beep may sound\n");
	fprintf(stderr,
		"    and a WARNING may be printed that can be ignored.\n");
	fprintf(stderr,
		"    If a MASK is present there will be no warning.\n");

	fmask = Rast_open_old("MASK", G_mapset());
	fprintf(stderr, "\n");


	/* setup the moving window using keyboard */

	if (d == 1) {
	    /* if sampling using circles */

	    fprintf(stderr, "\n    Do you want to sample using rectangles");

	    if (!G_yes
		("\n       (including squares) (y) or circles (n)?   ", 1)) {
		fprintf(stderr,
			"\n    What radius do you want for the circles?  Radius");
		fprintf(stderr,
			"\n       is in pixels; add 0.5 pixels, for the center");
		fprintf(stderr,
			"\n       pixel, to the number of pixels outside the");
		fprintf(stderr,
			"\n       center pixel.  Type a real number with one");
		fprintf(stderr,
			"\n       decimal place ending in .5 (e.g., 4.5):        ");

		numtrap(1, &radius);
		u_w = (int)(2 * radius);
		u_l = (int)(2 * radius);
		u_w0 = u_w / mx[0];
		u_l0 = u_l / mx[1];
	    }

	    /* if sampling using rectangles/squares */

	    else {
	      back:
		fprintf(stderr,
			"\n    Enter number of COLUMNS & ROWS for the dimensions of");
		fprintf(stderr,
			"\n       the moving window (e.g., 10 10):  ");

		numtrap(2, tmp);
		u_w = fabs(tmp[0]);
		u_l = fabs(tmp[1]);
		u_w0 = fabs(tmp[0]) / mx[0];
		u_l0 = fabs(tmp[1]) / mx[1];

		/* trap possible errors in dimensions */

		if (!u_w0 || !u_l0) {
		    fprintf(stderr,
			    "\n    You entered a dimension as 0; enter dimensions again\n");

		    goto back;
		}
		else if (u_w == 1 && u_l == 1) {
		    fprintf(stderr,
			    "\n    You entered dimensions as 1 1; This will not produce");
		    fprintf(stderr,
			    "\n       meaningful results; enter larger dimensions\n");

		    goto back;
		}
		else if (u_w >= r || u_l >= b) {
		    fprintf(stderr,
			    "\n    Window size you chose allows < 2 windows across each row;");
		    fprintf(stderr,
			    "\n       please make window dimensions smaller\n");

		    goto back;
		}
	    }

	    /* display the user-defined moving
	       window on the map */

	    R_open_driver();
	    R_standard_color(D_translate_color("red"));
	    if (radius) {
		draw_circle(initl, initt, initl + u_w0, initt + u_l0, 3);
	    }
	    else {
		draw_box(initl, initt, initl + u_w0, initt + u_l0, 1);
	    }
	    R_close_driver();

	    /* if all is OK, then set window dimensions */

	    fprintf(stderr,
		    "\n    Is the displayed moving window as you wanted it (y) or");

	    if (G_yes("\n       do you want to redo it? (n)     ", 1)) {
		xp = (int)(u_w0);
		yp = (int)(u_l0);
	    }
	    else {
		paint_map(n1, n2, n3);
		R_open_driver();
		R_standard_color(D_translate_color("grey"));
		draw_box((int)(l / mx[0] + 0.5), (int)(t / mx[1] + 0.5),
			 (int)(r / mx[0] + 0.5), (int)(b / mx[1] + 0.5), 1);
		R_close_driver();
		radius = 0.0;
		goto keyboard;
	    }
	}

	/* setup the moving window using the mouse */

	else if (d == 2) {
	    G_system("clear");

	    /* if sampling using circles */

	    fprintf(stderr,
		    "\n\n    Do you want to use a rectangular (including squares) (y)");

	    if (!G_yes("\n       or circular (n) moving window?   ", 1)) {
		circle = 1;
		fprintf(stderr,
			"\n    Draw a rectangular area to contain a circular moving window.");
		fprintf(stderr,
			"\n    First select upper left corner, then lower right:\n");
		fprintf(stderr, "       Left button:     Check unit size\n");
		fprintf(stderr,
			"       Middle button:   Upper left corner of area here\n");
		fprintf(stderr,
			"       Right button:    Lower right corner of area here\n");

	    }
	    else {
		circle = 0;
		fprintf(stderr,
			"\n    Draw a rectangular (or square) moving window");
		fprintf(stderr,
			"\n    First select upper left corner, then lower right:\n");
		fprintf(stderr,
			"       Left button:     Check moving window size\n");
		fprintf(stderr,
			"       Middle button:   Upper left corner of window here\n");
		fprintf(stderr,
			"       Right: button:   Lower right corner of window here\n");

	    }
	    R_open_driver();
	    while (btn != 3) {
	      back1:
		R_get_location_with_box(l, t, &xp, &yp, &btn);
		u_w = (int)((double)(xp - l) * mx[0] + 0.5);
		u_l = (int)((double)(yp - t) * mx[1] + 0.5);

		if (btn == 1) {	/** show the size and ratio **/
		    fprintf(stderr,
			    "    Window would be %d columns wide by %d rows long\n",
			    u_w, u_l);
		    fprintf(stderr,
			    "    Width/length would be %5.2f and area %d pixels\n",
			    (float)u_w / u_l, u_w * u_l);

		    for (i = 0; i < 120; i++)
			fprintf(stderr, "\b");

		}

		else if (btn == 2) {
		    R_move_abs(xp, yp);
		    l0 = l;
		    t0 = t;
		    l = xp;
		    t = yp;
		}

		else if (btn == 3) {
		    xpl =
			(int)((double)((int)((double)(xp - l) * mx[0] + 0.5))
			      / mx[0]);
		    ypt =
			(int)((double)((int)((double)(yp - t) * mx[1] + 0.5))
			      / mx[1]);
		    if (xpl < 0 || ypt < 0) {
			fprintf(stderr,
				"\n    You did not put lower right corner below and to the");
			fprintf(stderr,
				"\n       of upper left corner. Please select lower right");
			fprintf(stderr, "\n       corner again");

			goto back1;
		    }
		    else if (xpl == 0 || ypt == 0) {
			fprintf(stderr,
				"\n\n    Window would have 0 rows and/or 0 columns;");
			fprintf(stderr, "       try again\n");

			goto back1;
		    }
		    else if (xpl > 0 && ypt > 0) {
			R_standard_color(D_translate_color("red"));
			if (circle) {
			    if (xpl > ypt)
				xpl = ypt;
			    else if (ypt > xpl)
				ypt = xpl;
			    u_w = (int)((double)xpl * mx[0] + 0.5);
			    u_l = (int)((double)ypt * mx[1] + 0.5);
			    draw_circle(initl, initt, initl + xpl,
					initt + ypt, 3);
			}
			else
			    draw_box(initl, initt, initl + xpl, initt + ypt,
				     1);

			G_system("clear");
			if (circle) {
			    radius = (float)u_w / 2.0;
			    fprintf(stderr,
				    "\n\n    Circular moving window has radius = %5.2f pixels\n",
				    radius);

			}
			else {
			    fprintf(stderr,
				    "\n    Rectangular moving window has %d columns and %d rows",
				    u_w, u_l);
			    fprintf(stderr,
				    "\n    with width/length ratio of %5.2f and area of %d pixels\n",
				    (float)u_w / u_l, u_w * u_l);

			}
		    }
		}
	    }
	    R_close_driver();
	    l = l0;
	    t = t0;
	}

	Rast_close(fmask);
    }

    /* if neither, then exit */

    else
	exit(0);
    /* write the moving window parameters
       into the r.le.para/move_wind file */

    fprintf(fp, "%8d%8d  u_w u_l: CELL\n", u_w, u_l);


    w_w = r - l;
    w_l = b - t;

    /* write the radius of circles, if a
       circular moving window is to be used */

    fprintf(fp, "%8.1f          radius of circular moving window\n", radius);


    /* write the search area in the
       r.le.para/move_wind file */

    fprintf(fp, "%8d%8d  w_w w_l\n", w_w, w_l);
    fprintf(fp, "%8d%8d  x0, y0\n", (int)((double)(initl) * mx[0] + 0.5),
	    (int)((double)(initt) * mx[1] + 0.5));


    fclose(fp);
    return;
}
