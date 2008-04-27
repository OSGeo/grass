/*
************************************************************
* MODULE: r.le.setup/sample.c                              *
*         Version 5.0beta            Oct. 1, 2001          *
*				                           *
* AUTHOR: W.L. Baker, University of Wyoming                *
*         BAKERWL@UWYO.EDU                                 *
*                                                          *
* PURPOSE: To set up sampling areas, which can can then    *
*         be used to obtain data using the r.le.dist,      *
*         r.le.patch, and r.le.pixel programs.  The        *
*         sample.c code queries the user for information   *
*         needed to setup sampling units                   *
*				                           *
* COPYRIGHT: (C) 2001 by W.L. Baker                        *
*                                                          *
* This program is free software under the GNU General      *
* Public License(>=v2).  Read the file COPYING that comes  *
* with GRASS for details                                   *
*				                           *
************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/config.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "setup.h"


static int calc_unit_loc (double radius, int top, int bot, int left, int right,
	double ratio, int u_w, int u_l, int method, double intv,
	int num, int h_d, int v_d, double *ux, double *uy,
	int *sites, double startx, int starty, int fmask,
	double nx, double x, double y);
static void draw_grid (int l, int t, int w_w, int w_l, int h_d, int v_d, int starty,
	int startx, double colratio, double rowratio);
static void man_unit (int t, int b, int l, int r, char *n1, char *n2, char *n3,
	double *mx, int fmask);
static void get_rd (int exp1, int exp2, int dx, int dy, int u_w, int u_l, int *l, int *t);
static int overlap (int x1, int y1, int x2, int y2, int dx, int dy);
static int calc_num (int w_w, int w_l, double ratio, int u_w, int u_l, int method,
	double intv, int startx, int starty, int size, int count);
static void graph_unit (int t, int b, int l, int r, char *n1, char *n2, char *n3, double *mx, int fmask);

int  tag = 0;

static RETSIGTYPE f(int);

/* SAMPLING UNIT SETUP DRIVER */
void sample (int t0, int b0, int l0, int r0, char *name, char *name1,
		char *name2, double *msc)
{
  int    btn, d, fmask;
  double tmp;

/*
  IN
    name = name of raster map to be set up
    name1 = name of overlay vector map
    name2 = name of overlay site map
    msc[0]= cols of region/width of screen
    msc[1]= rows of region/height of screen
    t0 = top row of sampling frame
    b0 = bottom row of sampling frame
    l0 = left col of sampling frame
    r0 = right col of sampling frame
*/

				/* determine whether the user will use
				   the keyboard or mouse to setup the 
				   sampling units */

keyboard:
  fprintf(stderr, "\n\n    HOW WILL YOU SPECIFY SAMPLING UNITS?\n");
  fprintf(stderr, "\n       Use keyboard to enter sampling unit dimensions   1");
  fprintf(stderr, "\n       Use mouse to draw sampling units                 2\n");
  fprintf(stderr, "\n                                            Which Number?  ");
  numtrap(1, &tmp);
  d = (int)(tmp);
  if (d < 1 || d > 2) {
     fprintf(stderr, "     You did not enter a 1 or 2, try again\n");
     goto keyboard;
  }

  if (d == 1 || d == 2) {
                                /* return a value > 0 to fmask if there is
				   a MASK present */

     fprintf(stderr, "\n    If a MASK is not present (see r.mask) a beep may sound\n");
     fprintf(stderr, "    and a WARNING may be printed that can be ignored.\n");
     fprintf(stderr, "    If a MASK is present there will be no warning.\n");
     fmask = G_open_cell_old("MASK",G_mapset());
     fprintf(stderr, "\n");


				/* call the routine to setup sampling
				   units manually */

     if (d == 1)
        man_unit(t0, b0, l0, r0, name, name1, name2, msc, fmask);

				/* call the routine to setup sampling
				   units graphically */

     else if (d == 2)
        graph_unit(t0, b0, l0, r0, name, name1, name2, msc, fmask);

     G_close_cell(fmask);
  }
  /* if neither, then exit */
  else exit(0);

  return;
}




/* DEFINE SAMPLING UNITS MANUALLY */
static void man_unit (int t, int b, int l, int r, char *n1, char *n2, char *n3,
	double *mx, int fmask)
{
  int      i, j, dx, dy, w_w, w_l, u_w, u_l,
	   method, l0, t0, randflag=0, unit_num, num=0, scales,
	   h_d=1, v_d=1, itmp, thick, sites, *row_buf, fr, k,
	   count=0, maxsize=0, nx=0, ny=0, numx=0, numy=0,
	   al=0, ar=0, at=0, ab=0, au_w=0, au_l=0;
  double   *ux, *uy;
  FILE     *fp ;
  double   dtmp, ratio, size, intv=0.0, start[2], cnt=0, radius=0.0;
  char     *sites_mapset;
  struct Cell_head   wind;

/*  VARIABLES:
	COORDINATES IN THIS ROUTINE ARE IN CELLS

	t	=	top row of sampling frame
	b	=	bottom row of sampling frame
	l	=	left col of sampling frame
	r	=	right col of sampling frame
	n1	=
	n2	=
	n3	=
	start[0]=	row of UL corner of starting pt for strata
	start[1]= 	col of UL corner of starting pt for strata
	mx[0]	=	cols of region/width of screen
	mx[1]	=	rows of region/height of screen

*/


  start[0] = 0.0;
  start[1] = 0.0;

  l = (int)((double)(l * mx[0]) + 0.5);
  r = (int)((double)(r * mx[0]) + 0.5);
  t = (int)((double)(t * mx[1]) + 0.5);
  b = (int)((double)(b * mx[1]) + 0.5);
  w_w = r-l;
  w_l = b-t;

				/* draw the sampling frame */

  R_open_driver();
  R_standard_color(D_translate_color("grey"));
  draw_box((int)(l/mx[0] + 0.5), (int)(t/mx[1] + 0.5), (int)(r/mx[0] + 0.5),
     (int)(b/mx[1] + 0.5), 1);
  R_close_driver();

				/* open the units file for output */

  fp = fopen0("r.le.para/units", "w");
  G_sleep_on_error(0);

  				/* get the number of scales */

  do {
     fprintf(stderr, "\n    How many different SCALES do you want (1-15)?   ");

     numtrap(1, &dtmp);
     if (dtmp > 15 || dtmp < 1) {
	fprintf(stderr, "\n    Too many (>15) or too few scales; try again");

     }
  }
  while (dtmp < 1 || dtmp > 15 );

  fprintf(fp, "%10d    # of scales\n", (scales = (int)dtmp));


				/* for each scale */

  for(i = 0; i < scales; i++) {
     for(;;) {
        G_system("clear");

        radius = 0.0;

	fprintf(stderr, "\n\n    TYPE IN PARAMETERS FOR SCALE %d:\n", i+1);


  				/* get the distribution method */

        fprintf(stderr, "\n    Choose method of sampling unit DISTRIBUTION  \n");
        fprintf(stderr, "       Random nonoverlapping       1\n");
        fprintf(stderr, "       Systematic contiguous       2\n");
        fprintf(stderr, "       Systematic noncontiguous    3\n");
        fprintf(stderr, "       Stratified random           4\n");
        fprintf(stderr, "       Centered over sites         5\n");
	fprintf(stderr, "       Exit to setup option menu   6\n\n");

        do {
           fprintf(stderr, "                       Which Number?   ");

           numtrap(1, &dtmp);
           if ((method = fabs(dtmp)) > 6 || method < 1) {
              fprintf(stderr, "\n    Choice must between 1-5; try again");

           }
        }
	while(method > 6 || method < 1);

	if (method == 6) return;

				/* for stratified random distribution,
				   determine the number of strata */

        if (method == 4) {
getstrata:
           fprintf(stderr, "\n    Number of strata along the x-axis? (1-60)  ");

           numtrap(1, &dtmp);
	   h_d = fabs(dtmp);

           fprintf(stderr, "\n    Number of strata along the y-axis? (1-60)  ");

           numtrap(1, &dtmp);
           v_d = fabs(dtmp);

           if (h_d < 1 || v_d < 1 || h_d > 60 || v_d > 60) {
 	      fprintf(stderr, "\n    Number must be between 1-60; try again.");

	      goto getstrata;
	   }
        }

				/* for methods with strata */

        if (method == 2 || method == 3 || method == 4) {
strata:
           fprintf(stderr, "\n    Sampling frame row & col for upper left corner of");
           fprintf(stderr, " the strata?\n       Rows are numbered down and columns");
           fprintf(stderr, " are numbered to the right\n       Enter 1 1 to start in");
           fprintf(stderr, " upper left corner of sampling frame:  ");

           numtrap(2, start);
	   start[0] = start[0]-1.0;
	   start[1] = start[1]-1.0;
	   if (start[0] > w_l || start[0] < 0 ||
               start[1] > w_w || start[1] < 0) {
	      fprintf(stderr, "\n    The starting row and col you entered are outside");
	      fprintf(stderr, " the sampling frame\n       Try again\n");

	      goto strata;
	   }
	}


        if (method == 4) {

				/* call draw_grid with the left, top, width,
				   length, the number of horizontal and
				   vertical strata, and the starting row
				   and col for the strata */

  	   draw_grid((int)(l/mx[0] + 0.5), (int)(t/mx[1] + 0.5),
              (int)(w_w/mx[0] + 0.5), (int)(w_l/mx[1] + 0.5), h_d, v_d,
              (int)(start[0]/mx[1] + 0.5), (int)(start[1]/mx[0] + 0.5),
              mx[0], mx[1]);
	   if (!G_yes("    Are these strata OK?   ",1)) {
	      if (G_yes("\n\n    Refresh the screen?   ", 1)) {
	         paint_map(n1, n2, n3);
                 R_open_driver();
   	         R_standard_color(D_translate_color("grey"));
     	         draw_box((int)(l/mx[0] + 0.5), (int)(t/mx[1] + 0.5),
                    (int)(r/mx[0] + 0.5), (int)(b/mx[1] + 0.5), 1);
    	         R_close_driver();
              }
	      goto getstrata;
           }
        }

				/* if sampling using circles */

        fprintf(stderr, "\n    Do you want to sample using rectangles");

        if (!G_yes("\n       (including squares) (y) or circles (n)?   ",1)) {
getradius:
           fprintf(stderr, "\n    What radius do you want for the circles?  Radius");
           fprintf(stderr, "\n       is in pixels; add 0.5 pixels, for the center");
           fprintf(stderr, "\n       pixel, to the number of pixels outside the");
           fprintf(stderr, "\n       center pixel.  Type a real number with one");
           fprintf(stderr, "\n       decimal place ending in .5 (e.g., 4.5):        ");

           numtrap(1, &radius);
           if (radius > 100.0) {
              fprintf(stderr, "\n    Are you sure that you want such a large");

              if (!G_yes("\n       radius (> 100 pixels)?   ",1))
                 goto getradius;
           }

	   ratio = 1.0;
	   u_w = (int)(2 * radius);
	   u_l = (int)(2 * radius);

	   if (fmask > 0) {
	      count = 0;
	      row_buf = G_allocate_raster_buf(CELL_TYPE);
 	      fr = G_open_cell_old(n1, G_mapset());
	      for (j = t; j < b; j++) {
		 G_zero_raster_buf(row_buf, CELL_TYPE);
	         G_get_raster_row(fr, row_buf, j, CELL_TYPE);
	         for (k = l; k < r; k++) {
		    if (*(row_buf + k))
                       count++;
	         }
	      }
	      G_free (row_buf);
	      G_close_cell(fr);
	      cnt = (double)(count);
	      if (cnt)
	         cnt = sqrt(cnt);
	      else
		 cnt = 0;
           }
	   else {
	      count = (w_l-(int)(start[0]))*(w_w-(int)(start[1]));
           }
        }

				/* if sampling using rectangles/squares */

        else {

	  			/* get the width/length ratio */

getratio:
	fprintf(stderr, "\n    Sampling unit SHAPE (aspect ratio, #cols/#rows) "
			"expressed as real number"
			"\n    (e.g., 10 cols/5 rows = 2.0) for sampling units "
			"of scale %d? ", i+1);

	numtrap(1, &ratio);
	if (ratio < 0) ratio = -ratio;
	else if (ratio > 25.0)
	   if (!G_yes("\n    Are you sure you want such a large ratio?   ",1))
		goto getratio;

				/* determine the recommended maximum size
			 	   for sampling units */

getsize:
	   dtmp = (ratio > 1) ? 1/ratio : ratio;
	   dtmp /= (h_d > v_d) ? h_d*h_d : v_d*v_d;

tryagain:
           if (method == 1) {

	      if (fmask > 0) {
	         count = 0;
	         row_buf = G_allocate_raster_buf(CELL_TYPE);
 	         fr = G_open_cell_old(n1, G_mapset());
	         for (j = t; j < b; j++) {
		    G_zero_raster_buf(row_buf, CELL_TYPE);
	            G_get_raster_row(fr, row_buf, j, CELL_TYPE);
	            for (k = l; k < r; k++) {
		       if (*(row_buf + k))
                          count++;
	            }
	         }
	         G_free (row_buf);
	         G_close_cell(fr);
	         cnt = (double)(count);
	         if (cnt)
	            cnt = sqrt(cnt);
	         else
		    cnt = 0;
	         maxsize = ((cnt*dtmp/2)*(cnt*dtmp/2) > 1.0/dtmp) ?
                    (cnt*dtmp/2)*(cnt*dtmp/2) : 1.0/dtmp;
	         fprintf(stderr, "\n    Recommended maximum SIZE is %d in %d cell total",
 	            maxsize, count);
                 fprintf(stderr, " area\n");

	      }

	      else {
	         fprintf(stderr, "\n    Recommended maximum SIZE is");
                 fprintf(stderr, " %d in %d pixel total area\n",
                    (int)((w_l-(int)(start[0]))*(w_w-(int)(start[1]))*dtmp/2),
                    (w_l-(int)(start[0]))*(w_w-(int)(start[1])));

	         count = (w_l-(int)(start[0]))*(w_w-(int)(start[1]));
	         maxsize=(int)((w_l-(int)(start[0]))*(w_w-(int)(start[1]))*dtmp/2);
	      }
	   }

           else if (method == 2 || method == 3 || method == 5) {
	      fprintf(stderr, "\n    Recommended maximum SIZE is %d in %d pixel total",
                 (int)((w_l-(int)(start[0]))*(w_w-(int)(start[1]))*dtmp/2),
                       (w_l-(int)(start[0]))*(w_w-(int)(start[1])));
              fprintf(stderr, " area\n");

           }

           else if (method == 4) {
	      fprintf(stderr, "\n    Recommended maximum SIZE is");
              fprintf(stderr, " %d in %d pixel individual", (int)(w_w*w_l*dtmp/2),
                 ((w_w - (int)(start[1]))/h_d) * ((w_l - (int)(start[0]))/v_d));
	      fprintf(stderr, " stratum area\n");

           }

				/* get the unit size, display the calculated
				   size, and ask if it is OK */

           fprintf(stderr, "    What size (in pixels) for each sampling unit of scale %d?  ",
              i+1);

           numtrap(1, &size);
	   thick = 1;
           if (size < 15 || ratio < 0.2 || ratio > 5) thick = 0;
           u_w = sqrt(size * ratio);
           u_l = sqrt(size / ratio);
           fprintf(stderr, "\n    The nearest size is %d cells wide X %d cells high = %d",
              u_w, u_l, u_w*u_l);
           fprintf(stderr, " cells\n");

           if (!u_w || !u_l) {
              fprintf(stderr, "\n    0 cells wide or high is not acceptable; try again");

              goto tryagain;
           }
	   if (!G_yes("    Is this SIZE OK?   ",1))
	      goto getsize;
        }

				/* for syst. noncontig. distribution, get
				   the interval between units */

	if (method==3) {
	   fprintf(stderr, "\n    The interval, in pixels, between the units of scale");
           fprintf(stderr, " %d?  ",i+1);

           numtrap(1, &intv);
  	}

  				/* if the unit dimension + the interval
				   is too large, print a warning and
				   try getting another size */

  	if (u_w + intv > w_w / h_d || u_l + intv > w_l / v_d ) {
	   fprintf(stderr, "\n    Unit size too large for sampling frame; try again\n");

           if (radius)
              goto getradius;
           else
	      goto getsize;

	}

				/* for stratified random distribution,
				   the number of units is the same as
				   the number of strata */

        if (method == 4)
           num = h_d * v_d;

				/* for the other distributions, calculate the
				   maximum number of units, then get the
				   number of units */

        else if (method == 1 || method == 2 || method == 3) {

	   if (method == 1) {
              if (!(unit_num = calc_num(w_w, w_l, ratio, u_w, u_l, method,
	         intv, (int)(start[1]), (int)(start[0]), u_w*u_l, count))){
 	         fprintf(stderr, "\n    Something wrong with sampling unit size, try again\n");

                 if (radius)
                    goto getradius;
                 else
                    goto getsize;
	      }
              fprintf(stderr, "\n    Maximum NUMBER of units in scale %d is %d\n",
	         i+1, unit_num);
              fprintf(stderr, "    Usually 1/2 of this number can be successfully");
              fprintf(stderr, " distributed\n    More than 1/2 can sometimes be");
              fprintf(stderr, " distributed\n");

	   }

	   else if (method == 2 || method == 3) {
              numx = floor((double)(w_w - start[1]) / (u_w + intv));
              numy = floor((double)(w_l - start[0]) / (u_l + intv));
              if (((w_w - (int)(start[1])) % (numx*(u_w+(int)(intv)))) >= u_w)
	         numx ++;
              if (((w_l - (int)(start[0])) % (numy*(u_l+(int)(intv)))) >= u_l)
		 numy ++;
	      unit_num = numx*numy;
              fprintf(stderr, "\n    Maximum NUMBER of units in scale %d is %d as %d",
	         i+1, unit_num, numy);
              fprintf(stderr, " rows with %d units per row", numx);

	   }

	   do {
	      fprintf(stderr, "\n    What NUMBER of sampling units do you want to try");
              fprintf(stderr, " to use?  ");

	      numtrap(1, &dtmp );

	      if ((num = dtmp) > unit_num || num < 1) {
 	         fprintf(stderr, "\n    %d is greater than the maximum number of", num);
                 fprintf(stderr, " sampling units; try again\n");

              }

	      else if (method == 2 || method == 3) {
	         fprintf(stderr, "\n    How many sampling units do you want per row?  ");

	         numtrap(1, &dtmp );
	         if ((nx = dtmp) > num) {
 	            fprintf(stderr, "\n    Number in each row > number requested; try");
                    fprintf(stderr, " again\n");

                 }
	         else {
		    if (nx > numx) {
	               fprintf(stderr, "\n    Can't fit %d units in each row, try", nx);
                       fprintf(stderr, " again\n");

                    }
		    else {
		       if (num % nx)
			  ny = num/nx + 1;
		       else
			  ny = num/nx;
		       if (ny > numy) {
 	                  fprintf(stderr, "\n    Can't fit the needed %d rows, try", ny);
                          fprintf(stderr, " again\n");

                       }
		    }
		 }
	      }
           }
           while (num > unit_num || num < 1 || nx > num || nx > numx || ny > numy);
	}

				/* dynamically allocate storage for arrays to
				   store the upper left corner of sampling
				   units */

	if (method != 5) {
	    ux = G_calloc(num+1, sizeof(double));
	    uy = G_calloc(num+1, sizeof(double));
	}

	else {
	    ux = G_calloc(250, sizeof(double));
	    uy = G_calloc(250, sizeof(double));
	}

				/* calculate the upper left corner of sampling
				   units and store them in arrays ux and uy */

        if (!calc_unit_loc(radius, t, b, l, r, ratio, u_w, u_l, method, intv,
		num, h_d, v_d, ux, uy, &sites, (int)(start[1]), (int)(start[0]),
		fmask, nx, mx[0], mx[1]))
	   goto last;

        signal (SIGINT, SIG_DFL);
	if (method == 5)
	   num = sites;

 				/* draw the sampling units on the
				   screen */

	if (method == 2 || method == 3 || method == 5) {
	   R_open_driver();
           R_standard_color(D_translate_color("red"));
           for (j = 0; j < num; j++) {
              if (radius) {
	         draw_circle((int)((double)(ux[j])/mx[0]),
                       (int)((double)(uy[j])/mx[1]),
	               (int)((double)(ux[j]+u_w)/mx[0]),
                       (int)((double)(uy[j]+u_l)/mx[1]), 3);
              }
              else {
	         draw_box((int)((double)(ux[j])/mx[0]),
                       (int)((double)(uy[j])/mx[1]),
	               (int)((double)(ux[j]+u_w)/mx[0]),
                       (int)((double)(uy[j]+u_l)/mx[1]), 1);
              }
           }
	   R_close_driver();
        }

        if (G_yes("\n    Is this set of sampling units OK?   ", 1))
	   break;
last:
        signal (SIGINT, SIG_DFL);
        if (G_yes("\n    Refresh the screen?   ", 1)) {
	   paint_map(n1, n2, n3);
           R_open_driver();
   	   R_standard_color(D_translate_color("grey"));
     	   draw_box((int)(l/mx[0]), (int)(t/mx[1]), (int)(r/mx[0]),
              (int)(b/mx[1]), 1);
    	   R_close_driver();
        }
     }

				/* save the sampling unit parameters
				   in r.le.para/units file */

     fprintf(fp, "%10d    # of units of scale %d.\n", num, (i+1));
     fprintf(fp, "%10d%10d   u_w, u_l of units in scale %d\n",  u_w, u_l,
        (i+1));
     fprintf(fp, "%10.1f             radius of circles in scale %d\n",
        radius, (i+1));

     for(j = 0; j < num; j++)
	fprintf(fp, "%10d%10d   left, top of unit[%d]\n", (int)ux[j], (int)uy[j], j+1);

     if (i < scales - 1 && G_yes("\n\n    Refresh the screen?   ", 1)) {
	paint_map(n1, n2, n3);
        R_open_driver();
   	R_standard_color(D_translate_color("grey"));
     	draw_box((int)(l/mx[0]), (int)(t/mx[1]), (int)(r/mx[0]),
           (int)(b/mx[1]), 1);
    	R_close_driver();
     }
  }

				/* free dynamically allocated memory */

  G_free (ux);
  G_free (uy);
  fclose(fp);
  return;
}



/* FOR STRATIFIED RANDOM DISTRIBUTION, DRAW THE STRATA ON THE SCREEN */
static void draw_grid (int l, int t, int w_w, int w_l, int h_d, int v_d, int starty,
	int startx, double colratio, double rowratio)
{
   int j, k, l0, t0, itmp, dx, dy, initl, tmp;

/* VARIABLES:
	k	= the remainder after dividing the screen width/height
		     by the number of strata
	dx	= how many screen cols per stratum
	dy	= how many screen rows per stratum
	l0	= left side of screen + dx
	t0	= top of screen + dy
	w_w	= width of screen
	w_l	= height of screen
	h_d	= number of horizontal strata
	v_d	= number of vertical strata
*/

   R_open_driver();
   R_standard_color(D_translate_color("orange"));
   if (startx > 0) {
      dx = (int)((double)((int)(colratio *
          ((int)((double)(w_w-startx)/(double)(h_d))))/colratio + 0.5));
      l0 = l + startx;
   }
   else {
      dx = (int)((double)((int)(colratio *
          ((int)((double)(w_w)/(double)(h_d))))/colratio + 0.5));
      l0 = l;
   }
   if (starty > 0) {
      dy = (int)((double)((int)(rowratio *
          ((int)((double)(w_l-starty)/(double)(v_d))))/rowratio + 0.5));

      t0 = t + starty;
   }
   else {
      dy = (int)((double)((int)(rowratio *
          ((int)((double)(w_l)/(double)(v_d))))/rowratio + 0.5));
      t0 = t;
   }
   initl = l0;

					/* draw the vertical strata */

   for (j = 1; j <= h_d - 1; j++){
      l0 += dx;
      R_move_abs(l0, t0);
      R_cont_rel(0, w_l - starty);
   }

					/* draw the horizontal strata */

   for (j = 1; j <= v_d - 1; j++){
      t0 += dy;
      R_move_abs(initl, t0);
      R_cont_rel(w_w - startx, 0);
   }

   R_close_driver();
   return;
}



/* CALCULATE THE COORDINATES OF THE TOP LEFT CORNER OF THE SAMPLING UNITS */
static int calc_unit_loc (
	double radius, int top, int bot, int left, int right,
	double ratio, int u_w, int u_l, int method, double intv,
	int num, int h_d, int v_d, double *ux, double *uy,
	int *sites, double startx, int starty, int fmask,
	double nx, double x, double y)
{
  char	  *sites_mapset, sites_file_name[GNAME_MAX], *cmd;
  struct  Map_info Map;
  struct  Cell_head region;
  double  D_u_to_a_col(), D_u_to_a_row();
  int     i, j, k, cnt=0, w_w = right - left, w_l = bot - top, exp1, exp2,
          dx = w_w, dy = w_l, l, t, left1 = left, top1 = top, n, tmp,
	  ulrow, ulcol, *row_buf, lap=0;
  static struct line_pnts *Points;
  struct line_cats *Cats;
  int ltype;

/*   VARIABLES:
	UNITS FOR ALL DIMENSION VARIABLES IN THIS ROUTINE ARE IN PIXELS

	top	=	sampling frame top row in pixels
	bot	=	sampling frame bottom row in pixels
	left	=	sampling frame left in pixels
	right	=	sampling frame right in pixels
	left1	=	col of left side of sampling frame or each stratum
	top1	=	row of top side of sampling frame or each stratum
        l	=	random # cols to be added to left1
	r	=	random # rows to be added to top1
	ratio	=
	u_w	=	sampling unit width in pixels
	u_l	=	sampling unit length in pixels
	method	=	method of sampling unit distribution (1-5)
	intv	=	interval between sampling units when method=3
	num	=	number of sampling units
	h_d	=	number of horizontal strata
	v_d	=	number of vertical strata
	ux	=
	uy	=
	sites	=
	startx	=	col of UL corner starting pt for strata
	starty	=	row of UL corner starting pt for strata
	dx	=	number of cols per stratum
	dy	=	number of rows per stratum
	w_w	=	width of sampling frame in cols
	w_l	=	length of sampling frame in rows
*/


				/* if user hits Ctrl+C, abort this
				   calculation */

  setjmp(jmp);
  if (tag) {
     tag = 0;
     return 0;
  }

				/* for syst. noncontig. distribution */

  if (method==3) {
     u_w += intv;
     u_l += intv;
  }

				/* for stratified random distribution */

  if (method == 4){
     dx = (int)((double)(w_w - startx) / (double)(h_d));
     dy = (int)((double)(w_l - starty) / (double)(v_d));
  }

				/* for syst. contig. and noncontig.
				   distribution */

  else if (method == 2 || method == 3) {
     if (nx >= num)
        dx = (w_w-startx) - (num-1)*u_w;
     else {
        dx = (w_w-startx) - (nx-1)*u_w;
        dy = (w_l-starty) - (num/nx-1)*u_l;
     }
  }

  if (10 > (exp1 = (int)pow(10.0, ceil(log10((double)(dx-u_w+10)))))) exp1 = 10;
  if (10 > (exp2 = (int)pow(10.0, ceil(log10((double)(dy-u_l+10)))))) exp2 = 10;

				/* for random nonoverlapping and stratified
				   random */

  if (method == 1 || method == 4){

     fprintf(stderr, "\n   'Ctrl+C' and choose fewer units if the requested number");
     fprintf(stderr, " is not reached\n");


     for(i = 0; i < num; i++) {

				/* if Cntl+C */

        if (signal(SIGINT, SIG_IGN) != SIG_IGN) signal(SIGINT, f);

				/* for stratified random distribution */

        if (method == 4) {
	   j = 0;
	   if (n = i % h_d)
              left1 += dx;
           else {
	      left1 = left + startx;
	      if (i < h_d)
	       top1 = top + starty;
	      else
	         top1 += dy;
	   }
           get_rd(exp1, exp2, dx, dy, u_w, u_l, &l, &t);

        }

				/* for random nonoverlapping distribution */

        if (method == 1){

 				/* get random numbers */
back:
           get_rd(exp1, exp2, dx, dy, u_w, u_l, &l, &t);

	   if (left1 + l + u_w > right || top1 + t + u_l > bot ||
	      left1 + l < left || top1 + t < top)
	      goto back;

				/* if there is a mask, check to see that
				   the unit will be within the mask area */

	   if (fmask > 0) {
	      row_buf = G_allocate_cell_buf();
	      G_get_map_row_nomask(fmask, row_buf, t+top1);
	      if (!(*(row_buf+l+left1) && *(row_buf+l+left1+u_w-1)))
		 goto back;
	      G_zero_cell_buf (row_buf);
	      G_get_map_row_nomask(fmask, row_buf, t+top1+u_l-1);
	      if (!(*(row_buf+l+left1) && *(row_buf+l+left1+u_w-1)))
		 goto back;
	      G_free (row_buf);
	   }

				/* check for sampling unit overlap */

	   lap = 0;
           for (j = 0; j < cnt; j++) {
	      if (overlap(l+left1, t+top1, (int)ux[j], (int)uy[j], u_w, u_l))
	         lap = 1;
	   }
           if (lap)
	      goto back;

           cnt ++;
        }
				/* fill the array of upper left coordinates
				   for the sampling units */

        *(ux+i) = l + left1;
        *(uy+i) = t + top1;

				/* draw the sampling units on the
				   screen */

	R_open_driver();
        R_standard_color(D_translate_color("red"));
        if (radius)
	   draw_circle((int)((double)(ux[i])/x), (int)((double)(uy[i])/y),
		 (int)((double)(ux[i]+u_w)/x), (int)((double)(uy[i]+u_l)/y),
		 3);
        else
	   draw_box((int)((double)(ux[i])/x), (int)((double)(uy[i])/y),
		 (int)((double)(ux[i]+u_w)/x), (int)((double)(uy[i]+u_l)/y),
		 1);
	R_close_driver();
        fprintf(stderr, "    Distributed unit %4d of %4d requested\r",i+1,num);

     }
  }

				/* for syst. contig. & syst. noncontig. */

  else if (method == 2 || method == 3) {
     for(i = 0; i < num; i++) {
        *(ux + i) = left + startx + u_w * (i - nx*floor((double)i/nx));
        *(uy + i) = top + starty + u_l * floor((double)i/nx);
     }
  }

				/* for centered over sites */

  else if (method==5){
     sites_mapset = G_ask_vector_old("    Enter name of vector points map", sites_file_name);
     if (sites_mapset == NULL) {
        G_system("d.frame -e");
        exit(0);
     }

     Vect_open_old(&Map, sites_file_name, sites_mapset);
/*    fprintf(stderr, "\n    Can't open vector points file %s\n", sites_file_name); */

     *sites = 0;
     i = 0;
     n = 0;

     Points = Vect_new_line_struct();    /* init line_pnts struct */
     Cats = Vect_new_cats_struct();

     while (1) {
	ltype =  Vect_read_next_line (&Map, Points, Cats);
	if ( ltype == -1 ) G_fatal_error(_("Cannot read vector"));
	if ( ltype == -2 ) break;  /* EOF */
	/* point features only. (GV_POINTS is pts AND centroids, GV_POINT is just pts) */
	if (!(ltype & GV_POINT)) continue;

	ulcol = ((int)(D_u_to_a_col(Points->x[0]))) + 1 - u_w/2;
	ulrow = ((int)(D_u_to_a_row(Points->y[0]))) + 1 - u_l/2;
	if (ulcol <= left || ulrow <= top || ulcol+u_w-1 > right || ulrow+u_l-1 > bot) {
	   fprintf(stderr, "    No sampling unit over site %d at east=%8.1f north=%8.1f\n",
	      n+1, Points->x[0], Points->y[0]);
	   fprintf(stderr, "       as it would extend outside the map\n");

	}
	else {
	   *(ux+i) = ulcol-1;
	   *(uy+i) = ulrow-1;
	   i++;
	}
	n++;
	if (n > 250)
           G_fatal_error("There are more than the maximum of 250 sites\n");
     }
     fprintf(stderr, "    Total sites with sampling units = %d\n",i);

     *sites = i;
     cmd = G_malloc(100);
     sprintf(cmd, "d.vect %s color=black",sites_file_name);
     G_system(cmd);
     G_free (cmd);

     Vect_close(&Map);
     G_free(Points);
     G_free(Cats);

  }

  return 1;

}



/* FIND THE CORRECT RANDOM NUMBER */
static void get_rd (int exp1, int exp2, int dx, int dy, int u_w, int u_l, int *l, int *t)
{
   int  rdl,rdt;

   do {
     rdl = rand();
     *l = rdl % exp1;
     rdt = rand();
     *t = rdt % exp2;
   }
   while(dx < *l + u_w || dy < *t + u_l);
   return;
}



/* */
static RETSIGTYPE f(int sig)
{
  tag = 1;
  longjmp(jmp, 1);
  return;
}



/* CHECK IF 2 SAMPLING UNITS OVERLAP */
static int overlap (int x1, int y1, int x2, int y2, int dx, int dy)
{
  if (x1 >= x2+dx || x2 >= x1+dx || y1 >= y2+dy || y2 >= y1+dy)
     return 0;
  else
     return 1;
}



/* CALCULATE MAXIMUM POSSIBLE NUMBER OF SAMPLING UNITS */
static int calc_num (int w_w, int w_l, double ratio, int u_w, int u_l, int method,
	double intv, int startx, int starty, int size, int count)
{
  int        nx, ny, max;

				/* for random nonoverlapping */
  if (method == 1) {
     max = count/size;
  }

				/* for syst. contig. distribution */

  else if (method == 2) {
     nx = floor((double)(w_w - startx) / u_w);
     ny = floor((double)(w_l - starty) / u_l);
     max = nx*ny;
  }

				/* for syst. noncontig. distribution */

  else if (method==3) {
     nx = floor((double)(w_w - startx) / (u_w + intv));
     ny = floor((double)(w_l - starty) / (u_l + intv));
     max = nx*ny;
  }
  return max;
}




				/* USE THE MOUSE TO DEFINE SAMPLING
				   UNITS GRAPHICALLY */

static void graph_unit (int t, int b, int l, int r, char *n1, char *n2, char *n3, double *mx, int fmask)

{
  int  		 x0=0, y0=0, xp, yp, ux[250], uy[250], u_w, u_l, btn=0, k=0,
		 w_w=0, w_l=0, *row_buf, at, ab, al, ar, circle=0,
		 tmpw, tmpl, au_w, au_l, lap=0, l0=0, r0=0, t0=0, b0=0;
  FILE           *fp;
  double	 tmp, radius=0.0;
  register int   i, j;

/*  VARIABLES:
	COORDINATES IN THIS ROUTINE ARE IN CELLS

	t	=	top row of sampling frame
	b	=	bottom row of sampling frame
	l	=	left col of sampling frame
	r	=	right col of sampling frame
	n1	=
	n2	=
	n3	=
	mx[0]	=	cols of region/width of screen
	mx[1]	=	rows of region/height of screen
	xp 	=	mouse x location in screen coordinates (col)
	yp	=	mouse y location in screen coordinates (row)
	ar	=	mouse x location in map coordinates (col)
	al	=	mouse y location in map coordinates (row)

*/


  l0 = l;
  r0 = r;
  t0 = t;
  b0 = b;

  l = (int)((double)(l * mx[0]) + 0.5);
  r = (int)((double)(r * mx[0]) + 0.5);
  t = (int)((double)(t * mx[1]) + 0.5);
  b = (int)((double)(b * mx[1]) + 0.5);
  w_w = r-l;
  w_l = b-t;

				/* draw the sampling frame */

  R_open_driver();
  R_standard_color(D_translate_color("grey"));
  draw_box((int)(l/mx[0]), (int)(t/mx[1]), (int)(r/mx[0]), (int)(b/mx[1]), 1);
  R_close_driver();

  fp = fopen0("r.le.para/units", "w");
  G_sleep_on_error(0);

  				/* get the number of scales */

  do {
     fprintf(stderr, "\n    How many different SCALES do you want? (1-15)  ");

     numtrap(1, &tmp);
     if (tmp <1 || tmp > 15)
	fprintf(stderr, "    Too many (>15) or too few scales, try again.\n");
  }
  while(tmp < 1 || tmp > 15);
  fprintf(fp, "%10d    # of scales\n", (int)(tmp));

  				/* for each scale */

  for(i = 0; i < tmp; i++){
     G_system("clear");

     radius = 0.0;
     circle = 0;

				/* if sampling using circles */

     fprintf(stderr, "\n    SCALE %d\n",i+1);
     fprintf(stderr, "\n    Do you want to sample using rectangles");

     if (!G_yes("\n       (including squares) (y) or circles (n)?   ",1)) {
        circle = 1;
        fprintf(stderr, "\n    Draw a rectangular area to contain a standard circular");
        fprintf(stderr, "\n    sampling unit of scale %d.  First select upper left", i+1);
        fprintf(stderr, "\n    corner, then lower right:\n");
        fprintf(stderr, "       Left button:     Check unit size\n");
        fprintf(stderr, "       Middle button:   Upper left corner of area here\n");
        fprintf(stderr, "       Right button:    Lower right corner of area here\n");

     }

     else {
        fprintf(stderr, "\n    Draw a standard rectangular unit of scale %d.", i+1);
        fprintf(stderr, "\n    First select upper left corner, then lower right:\n");
        fprintf(stderr, "       Left button:     Check unit size\n");
        fprintf(stderr, "       Middle button:   Upper left corner of unit here\n");
        fprintf(stderr, "       Right button:    Lower right corner of unit here\n");

     }

     R_open_driver();

     do {
back1:
        R_get_location_with_box(x0, y0, &xp, &yp, &btn);

				/* convert the upper left screen coordinate
				   (x0, y0) and the mouse position (xp, yp)
				   on the screen to the nearest row and
				   column; do the same for the sampling
				   unit width (u_w) and height (u_l);
				   then convert back */

	ar = (int)((double)(xp)*mx[0] + 0.5);
	xp = (int)((double)(ar)/mx[0] + 0.5);
	al = (int)((double)(x0)*mx[0] + 0.5);
	x0 = (int)((double)(al)/mx[0] + 0.5);
        au_w = ar-al;
        u_w = (int)((double)(au_w)/mx[0] + 0.5);
	ab = (int)((double)(yp)*mx[1] + 0.5);
        yp = (int)((double)(ab)/mx[1] + 0.5);
	at = (int)((double)(y0)*mx[1] + 0.5);
        y0 = (int)((double)(at)/mx[1] + 0.5);
 	au_l = ab-at;
        u_l = (int)((double)(au_l)/mx[1] + 0.5);


 				/* left button, check the size of the rubber
				   box in array system */

        if (btn == 1) {
           if (ar > r || ab > b || ar < l || ab < t) {
	      fprintf(stderr, "\n    This point is not in the sampling frame; try again\n");

	      goto back1;
   	   }
	   if (x0 < l || y0 < t) {
	      fprintf(stderr, "\n    Use the middle button to first put the upper left");
	      fprintf(stderr, "\n    corner inside the sampling frame\n");

	      goto back1;
           }
	   if (ar <= al || ab <= at) {
	      fprintf(stderr, "\n    Please put the lower right corner down and to");
	      fprintf(stderr, "\n    the right of the upper left corner\n");

	      goto back1;
	   }
	   else {
              fprintf(stderr,"\n    Unit would be %d columns wide by %d rows long\n",
	         abs(au_w), abs(au_l));
	      fprintf(stderr,"    Width/length would be %5.2f and size %d pixels\n",
		 (double)abs((au_w))/(double)abs((au_l)),abs(au_w)*abs(au_l));

	   }
	}

				/* mid button, move the start point of the
				   rubber box */

	else if (btn == 2) {
           if (ar > r || ab > b || ar < l || ab < t) {
	      fprintf(stderr, "\n    Point is not in the sampling frame; try again\n");

	      goto back1;
   	   }
	   else {
              R_move_abs(xp, yp);
              x0 = xp;
              y0 = yp;
	   }
        }

				/* right button, outline the unit */

	else if ( btn == 3) {

           if (circle) {
              if (u_w > u_l) {
                 al = al + ((ar - al) - (ab - at))/2;
                 ar = al + (ab - at);
	         x0 = (int)((double)(al)/mx[0] + 0.5);
	         xp = (int)((double)(ar)/mx[0] + 0.5);
                 au_w = ar-al;
                 u_w = u_l = (int)((double)(au_w)/mx[0] + 0.5);
              }
              if (u_l > u_w) {
                 at = at + ((ab - at) - (ar - al))/2;
                 ab = at + (ar - al);
                 y0 = (int)((double)(at)/mx[1] + 0.5);
                 yp = (int)((double)(ab)/mx[1] + 0.5);
 	         au_l = ab-at;
                 u_w = u_l = (int)((double)(au_l)/mx[1] + 0.5);
              }
           }

	   if (ar > r || ab > b || al < l || at < t) {
	      fprintf(stderr, "\n    The unit extends outside the sampling frame or map;");
              fprintf(stderr, "\n       try again\n");

	      goto back1;
   	   }

	   if (au_w > w_w || au_l > w_l) {
	      fprintf(stderr, "\n    The unit is too big for the sampling frame; ");
	      fprintf(stderr, "try again\n");

	      goto back1;
	   }

				/* if there is a mask, check to see that
				   the unit will be within the mask area,
				   by checking to see whether the four
				   corners of the unit are in the mask */

	   if (fmask > 0) {
	      row_buf = G_allocate_cell_buf();
	      G_get_map_row_nomask(fmask, row_buf, at);
	      if (!(*(row_buf + al) && *(row_buf + ar - 1))) {
		 fprintf(stderr, "\n    The unit would be outside the mask; ");
		 fprintf(stderr, "try again\n");

	         G_free (row_buf);
		 goto back1;
 	      }
	      G_zero_cell_buf (row_buf);
	      G_get_map_row_nomask(fmask, row_buf, ab-1);
	      if (!(*(row_buf + al) && *(row_buf + ar - 1))) {
		 fprintf(stderr, "\n    The unit would be outside the mask; ");
		 fprintf(stderr, "try again\n");

	         G_free (row_buf);
		 goto back1;
	      }
	      G_free (row_buf);
	   }

	   if (xp-x0 > 0 && yp-y0 > 0) {
              R_standard_color(D_translate_color("red"));
              if (circle)
                 draw_circle(x0, y0, xp, yp, 3);
              else
	         draw_box(x0, y0, xp, yp, 1);
	      G_system("clear");
              if (circle) {
	         fprintf(stderr, "\n\n    The standard circular sampling unit has:\n");
	         fprintf(stderr, "       radius = %f pixels\n", (double)(ar-al)/2.0);

              }
              else {
	         fprintf(stderr, "\n\n    The standard sampling unit has:\n");
	         fprintf(stderr, "       columns=%d    rows=%d\n", abs(ar-al), abs(ab-at));
 	         fprintf(stderr, "       width/length ratio=%5.2f\n",
                    (double)abs(ar-al)/(double)abs(ab-at));
	         fprintf(stderr, "       size=%d pixels\n",abs(ar-al)*abs(ab-at));

              }
	      k = 0;
              ux[0] = al;
	      uy[0] = at;
	   }
	   else if (xp-x0 == 0 || yp-y0 == 0) {
	      fprintf(stderr, "\n    Unit has 0 rows and/or 0 columns; try again\n");

	      goto back1;
	   }
	   else {
	      fprintf(stderr, "\n    You did not put the lower right corner below");
              fprintf(stderr, "\n       and to the right of the upper left corner. Please try again");

              goto back1;
           }
        }
     }
     while(btn != 3);
     R_close_driver();

     				/* use the size and shape of the
				   standard unit to outline more units
				   in that scale */

     fprintf(stderr, "\n    Outline more sampling units of scale %d?\n", i+1);
     fprintf(stderr, "       Left button:     Exit\n");
     fprintf(stderr, "       Middle button:   Check unit position\n");
     fprintf(stderr, "       Right button:    Lower right corner of next unit here\n");

     R_open_driver();

				 /* if not the left button (to exit) */

back2:
     while(btn != 1) {
        R_get_location_with_box(xp-u_w, yp-u_l, &xp, &yp, &btn);

				/* convert the left (x0), right (y0),
				   top (y0), bottom (yp) coordinates in
				   screen pixels to the nearest row and
				   column; do the same for the sampling
				   unit width (u_w) and height (u_l);
				   then convert back */

	ar = (int)((double)(xp)*mx[0] + 0.5);
	ab = (int)((double)(yp)*mx[1] + 0.5);
	xp = (int)((double)(ar)/mx[0] + 0.5);
        yp = (int)((double)(ab)/mx[1] + 0.5);
	al = (int)((double)(xp-u_w)*mx[0] + 0.5);
	at = (int)((double)(yp-u_l)*mx[0] + 0.5);
	x0 = (int)((double)(al)/mx[0] + 0.5);
        y0 = (int)((double)(at)/mx[1] + 0.5);


				  /* if right button, outline the unit */

        if (btn == 3){

	   if (ar > r || ab > b || al < l || at < t) {
	      fprintf(stderr, "\n    The unit would be outside the map; try again");
	      goto back2;

   	   }

				/* if there is a mask, check to see that
				   the unit will be within the mask area */

	   if (fmask > 0) {
	      row_buf = G_allocate_cell_buf();
	      G_get_map_row_nomask(fmask, row_buf, at);
	      if (!(*(row_buf + al) && *(row_buf + ar - 1))) {
		 fprintf(stderr, "\n    The unit would be outside the mask; ");
		 fprintf(stderr, "try again");

		 G_free (row_buf);
		 goto back2;
 	      }
	      G_zero_cell_buf (row_buf);
	      G_get_map_row_nomask(fmask, row_buf, ab-1);
	      if (!(*(row_buf + al) && *(row_buf + ar - 1))) {
		 fprintf(stderr, "\n    The unit would be outside the mask; ");
		 fprintf(stderr, "try again");
	         G_free (row_buf);
		 goto back2;
	      }
	      G_free (row_buf);
	   }

				/* check for sampling unit overlap */

	   lap = 0;
           for (j = 0; j < k + 1; j++) {
              if (overlap(al, at, ux[j], uy[j], au_w, au_l)) {
	         fprintf(stderr, "\n    The unit would overlap a previously drawn ");
	         fprintf(stderr, "unit; try again");

	         lap = 1;
	      }
           }
	   if (lap)
	      goto back2;

	   k ++;
           fprintf(stderr, "\n    %d sampling units have been placed", (k + 1));

	   ux[k] = al;
	   uy[k] = at;
           R_standard_color(D_translate_color("red"));
           if (circle)
              draw_circle(x0, y0, xp, yp, 3);
           else
              draw_box(x0, y0,  xp, yp, 1);
        } 
     }
     R_close_driver();

     				/* save the sampling units in the
				   r.le.para/units file */

     if (circle)
        radius = (double)(ar-al)/2.0;
     else
        radius = 0.0;
     fprintf(fp, "%10d    # of units of scale %d\n", k+1, i+1);
     fprintf(fp, "%10d%10d   u_w, u_l of units in scale %d\n",
        (int)(u_w*mx[0]), (int)(u_l*mx[1]), i+1);
     fprintf(fp, "%10.1f             radius of circles in scale %d\n",
        radius, (i+1));
     for(j = 0; j < k + 1; j++)
        fprintf(fp, "%10d%10d   left, top of unit[%d]\n", ux[j], uy[j], j+1);

     if (i < tmp-1 && G_yes("\n    Refresh the screen?   ", 1)) {
	paint_map(n1, n2, n3);
        R_open_driver();
        R_standard_color(D_translate_color("red"));
        R_close_driver();
     }
  }
 
  fclose(fp);
  return;
}



/* DRAW A RECTANGULAR BOX WITH THICKNESS OF "THICK" */
void draw_box (int x0, int y0, int xp, int yp, int thick)
{
  int i;

/*PARAMETERS:
  x0 = leftmost position
  y0 = topmost position
  xp = rightmost position
  yp = bottommost position
  thick = thickness
  i = individual screen pixels
*/

  for(i = 0; i <= thick; i++){
     R_move_abs(x0 + i, y0 + i);
     R_cont_abs(x0 + i, yp - i);
     R_cont_abs(xp - i, yp - i);
     R_cont_abs(xp - i, y0 + i);
     R_cont_abs(x0 + i, y0 + i);

     R_move_abs(x0 - i, y0 - i);
     R_cont_abs(x0 - i, yp + i);
     R_cont_abs(xp + i, yp + i);
     R_cont_abs(xp + i, y0 - i);
     R_cont_abs(x0 - i, y0 - i);
   }
   R_flush();

   return;
}	



/* DRAW A CIRCLE WITH THICKNESS OF "THICK" */
void draw_circle (int x0, int y0, int xp, int yp, int thick)
{
  int i, j, xstart, ystart, x2, yr;
  double ang, xinc, yinc;

/*PARAMETERS
  x0 = leftmost position of enclosing square
  y0 = topmost position of enclosing square
  xp = rightmost position of enclosing square
  yp = bottommost position of enclosing square
  thick = thickness in screen pixels for drawing lines
  i = index for incrementing process
  j = individual screen pixels
  x1 = x coordinate of point where
       circle touches enclosing square
  ang = angle in radians that is the
       angle to be moved in connecting
       a straight line segment to the
       previous location
*/

  for (j = 0; j < thick; j++) {

     xstart = x0 + (xp - x0)/2;
     ystart = y0 + j;
     ang = 0.049087385;

     R_move_abs(xstart, ystart);

     for (i = 1; i < 129; i++) {
        xinc = cos((double)i * ang/2.0) * sin((double)i * ang/2.0) * 
            (double)(yp - y0 - 2 * j);
        yinc = sin((double)i * ang/2.0) * sin((double)i * ang/2.0) * 
            (double)(yp - y0 - 2 * j);
        R_cont_abs(xstart + (int)xinc, ystart + (int)yinc);
     }
  }
  R_flush();

  return;
}



/* READ USER DIGITAL INPUT FROM THE SCREEN */
void numtrap (int n, double *a)
{
  char   num[31], *s;
  int    i = 0, j, k = 1, c;

  while (i < n){

     s = num;

                                /* find the first period, minus sign,
				   or digit in the user input */

     while((c = getchar()) != '.' && c != '-' && !isdigit(c));

                                /* if it is a decimal pt, then get
				   characters as long as they are
				   digits and append them to the 
				   end of the num array */


     if (c == '.') {
	*s++ = c;
	while ((c = getchar()) && isdigit(c) && k < 30) {
	   *s++ = c;
	   k++;
	}
     }


				/* if it is not a period, but
				   is a minus sign or digit, then
				   get characters as long as they 
				   are digits and append them to the
				   end of the num array. If a decimal
				   pt is found, then append this also
				   and keep getting digits */

     else if (isdigit(c) || c == '-'){
	*s++ = c;
	while ((c = getchar()) && isdigit(c) && k < 30) {
	   *s++ = c;
	   k++;
	}
        if (c == '.') {
	   *s++ = c;
	   while ((c = getchar()) && isdigit(c) && k < 30) {
	      *s++ = c;
	      k++;
	   }
        }
     }

     *s = '\0';

                                /* once the user's input has been
				   read, convert the digits of the num
				   array into a double and store it
				   in the i++ element of the "a" array */ 

     *(a + i++) = atof(num);

				/* keep getting characters as long as
				   they are not spaces, commas, tabs,
				   or end of line */

     while(c != ' ' && c != ',' && c != '\t' && c != '\n') c = getchar();
  }
  return;
}
