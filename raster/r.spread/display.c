#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "cmd_line.h"
#include "local_proto.h"

#define SIZE 80

static char buf[SIZE], old_time[SIZE], cur_time[SIZE];
static double f2s_x, f2s_y;	/*file-to-screen factors */
static int old_value = 0;
static int xoffset, yoffset;	/*screen coors of u-l corner of file */
static int x1, y1, x2, y2;	/*diagonal pts of a box */
static int x1_st, y1_st, x2_st, y2_st;	/*pts of the elapsed Spread Time box */
static int x1_ct, y1_ct, x2_ct, y2_ct;	/*pts of the Current Time box */
static struct Colors colors;
static struct tm *t_time;
static time_t c_time;

void display_init(void)
{
    extern struct Cell_head window;
    extern int nrows, ncols;	/*numbers of rows and columns in file */
    double t, b, l, r;		/*top, bottom, left and right of frame */
    int width, height;


    /*set time zone for tracing local time */
    tzset();

    if (R_open_driver() != 0) {
	G_fatal_error("couldn't open display");
    }
    D_setup(1);
    D_get_screen_window(&t, &b, &l, &r);
    /*printf("\nt,b,l,r: %d, %d, %d, %d", t, b, l, r);
     */
    /*setup some graphics boxes */
    R_standard_color(D_translate_color("grey"));
    R_move_abs(l, t + (b - t) / 20);
    R_cont_abs(r, t + (b - t) / 20);
    R_move_abs(r, t + (b - t) / 10);
    R_cont_abs(l, t + (b - t) / 10);
    R_move_abs(l + (r - l) / 2, t);
    R_cont_abs(l + (r - l) / 2, t + (b - t) / 10);

    /*logo box */
    x1 = l;
    y1 = t;
    x2 = l + 0.5 * (r - l);
    y2 = t + 0.05 * (b - t);
    R_standard_color(5);
    R_box_abs(x1, y1, x2 - 1, y2 - 1);
    R_text_size((int)(0.055 * (x2 - x1)), (int)(0.7 * (y2 - y1)));
    R_move_abs((int)(x1 + 0.03 * (x2 - x1)), (int)(y1 + 0.825 * (y2 - y1)));
    R_standard_color(8);
    R_text("Live SPREAD Simulation");

    /*elapsed spread time box */
    x1_st = l + 0.5 * (r - l);
    y1_st = t;
    x2_st = r;
    y2_st = t + 0.05 * (b - t);
    /*printf("\ny1-st y2-st x1st, x2st: %d, %d, %d, %d", y1_st,y2_st,x1_st,x2_st);
     */ R_standard_color(9);
    R_box_abs(x1_st + 1, y1_st, x2_st, y2_st - 1);
    R_text_size((int)(0.049 * (x2_st - x1_st)), (int)(0.5 * (y2_st - y1_st)));
    R_move_abs((int)(x1_st + 0.03 * (x2_st - x1_st)),
	       (int)(y1_st + 0.75 * (y2_st - y1_st)));
    R_standard_color(1);
    R_text("Elapsed Spread Time 00:00");

    /*starting watch-time box */
    x1 = l;
    y1 = t + 0.05 * (b - t);
    x2 = l + 0.5 * (r - l);
    y2 = t + 0.1 * (b - t);
    R_standard_color(9);
    R_box_abs(x1, y1 + 1, x2 - 1, y2 - 1);
    R_text_size((int)(0.05 * (x2 - x1)), (int)(0.5 * (y2 - y1)));
    R_move_abs((int)(x1 + 0.03 * (x2 - x1)), (int)(y1 + 0.75 * (y2 - y1)));
    time(&c_time);
    t_time = localtime(&c_time);
    strftime(cur_time, SIZE, "%H:%M", t_time);
    sprintf(buf, "   Started  At  %s", cur_time);
    R_standard_color(8);
    R_text(buf);

    /*current watch-time box */
    x1_ct = l + 0.5 * (r - l);
    y1_ct = t + 0.05 * (b - t);
    x2_ct = r;
    y2_ct = t + 0.1 * (b - t);
    R_standard_color(9);
    R_box_abs(x1_ct + 1, y1_ct + 1, x2_ct, y2_ct - 1);
    R_text_size((int)(0.05 * (x2_ct - x1_ct)), (int)(0.5 * (y2_ct - y1_ct)));
    R_move_abs((int)(x1_ct + 0.03 * (x2_ct - x1_ct)),
	       (int)(y1_ct + 0.75 * (y2_ct - y1_ct)));
    R_standard_color(8);
    sprintf(buf, "  Current  Time  %s", cur_time);
    R_text(buf);

    /*live image display box */
    D_reset_screen_window(t + (b - t) / 10 + 2, b, l, r);
    R_close_driver();

    /*Set a graster map as a background image */
    if (backdrop_layer) {
	sprintf(buf, "d.rast -o %s", backdrop_layer);
	system(buf);
    }

    /*figure scaling factors of a file cell to a screen unit */
    R_open_driver();
    Rast_make_grey_scale_colors(&colors, 0, 59);
    width = r - l;
    height = 0.9 * (b - t) - 1;
    if (width * nrows > height * ncols) {
	f2s_y = height / (float)nrows;
	f2s_x = (window.ew_res / window.ns_res) * f2s_y;
	yoffset = t + 0.1 * (b - t) + 3;
	xoffset = l + (width - f2s_x * ncols) / 2 + 1;
    }
    else {
	f2s_x = width / (float)ncols;
	f2s_y = (window.ns_res / window.ew_res) * f2s_x;
	xoffset = l + 1;
	yoffset = t + 0.1 * (b - t) + (height - f2s_y * nrows) / 2 + 3;
    }

    D_reset_screen_window(t, b, l, r);
}


void draw_a_cell(int row, int col, int cell_value)
{
    x1 = xoffset + f2s_x * col;
    y1 = yoffset + f2s_y * row;
    x2 = x1 + f2s_x + 0.999;	/*tradeoff:allowing overlaps to avoid gaps */
    y2 = y1 + f2s_y + 0.999;	/*tradeoff:allowing overlaps to avoid gaps */
    D_color(cell_value % 60, &colors);
    R_box_abs(x1, y1, x2, y2);
    R_flush();

    /* if cell_value changes, update it in the elasped-spread-time box, 
     * also update the current watch-time if it changes*/
    if (cell_value > old_value) {
	old_value = cell_value;
	R_standard_color(9);
	R_box_abs(x1_st + 1, y1_st, x2_st, y2_st - 1);
	R_text_size((int)(0.049 * (x2_st - x1_st)),
		    (int)(0.5 * (y2_st - y1_st)));
	R_move_abs((int)(x1_st + 0.03 * (x2_st - x1_st)),
		   (int)(y1_st + 0.75 * (y2_st - y1_st)));
	R_standard_color(1);
	sprintf(buf, "Elapsed Spread Time %d%d:%d%d", (cell_value / 600),
		(cell_value / 60 - cell_value / 600 * 10),
		((cell_value - cell_value / 60 * 60) / 10),
		((cell_value - cell_value / 60 * 60) -
		 (cell_value - cell_value / 60 * 60) / 10 * 10));
	R_text(buf);

	time(&c_time);
	t_time = localtime(&c_time);
	strftime(cur_time, SIZE, "%H:%M", t_time);
	if (strcmp(cur_time, old_time) != 0) {
	    strcpy(old_time, cur_time);
	    R_standard_color(9);
	    R_box_abs(x1_ct + 1, y1_ct + 1, x2_ct, y2_ct);
	    R_text_size((int)(0.05 * (x2_ct - x1_ct)),
			(int)(0.5 * (y2_ct - y1_ct)));
	    R_move_abs((int)(x1_ct + 0.03 * (x2_ct - x1_ct)),
		       (int)(y1_ct + 0.75 * (y2_ct - y1_ct)));
	    R_standard_color(8);
	    sprintf(buf, "  Current  Time  %s", cur_time);
	    R_text(buf);
	}
    }
}

void draw_a_burning_cell(int row, int col)
{
    x1 = xoffset + f2s_x * col;
    y1 = yoffset + f2s_y * row;
    x2 = x1 + f2s_x + 0.999;	/*tradeoff:allowing overlaps to avoid gaps */
    y2 = y1 + f2s_y + 0.999;	/*tradeoff:allowing overlaps to avoid gaps */
    R_standard_color(D_translate_color("red"));
    R_box_abs(x1, y1, x2, y2);
    R_flush();
}

void display_close(void)
{
    R_close_driver();
}
