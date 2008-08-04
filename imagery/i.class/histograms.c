#include <math.h>
#include "globals.h"
#include <grass/raster.h>
#include "local_proto.h"

#define MU	'l'		/* these are the characters that are printed as  */
#define SIGMA	'r'		/* mu and sigma in the greek simplex font */
#define RANGE_MIN "R1"
#define RANGE_MAX "R2"
#define MIN "min"
#define MAX "max"

#define MEAN(b)		(sum[b]/np)
#define STD_DEV(b)	((float) sqrt ((double) VAR(b,b) / np))
#define VAR(b1,b2)	(product[b1][b2] - sum[b1]*sum[b2]/np)

#define BORDER 10
#define MAX_HISTO_WIDTH 11	/* should be odd number */
#define MIN_HISTO_WIDTH 1	/* should be an odd number */
#define LEGEND_SPACE 3*TEXT_HEIGHT
#define NUM_CHARS 30
#define TEXT_HEIGHT (width/NUM_CHARS)

static int old_range = 1;

int histograms(int nbands, float *sum, float **product, int **histo,
	       int np, int *min, int *max, double in_nstd, int b_or_a)
{
    int h_top, h_bottom, h_left, h_right;
    int b;
    int nrows, ncols;
    int height;
    int width;
    char msg[200];
    float mean;
    float std_dev;
    int x1, y1;
    int x2, y2;
    int nbars, bar;
    int cat;
    float scale;
    int grand_max;
    int max_range, histo_width;

    /*    Get_window (MAP_WINDOW, &w);  */

    Erase_view(VIEW_HISTO);
    Outline_box(VIEW_HISTO->top, VIEW_HISTO->bottom, VIEW_HISTO->left,
		VIEW_HISTO->right);
    R_set_window(VIEW_HISTO->top, VIEW_HISTO->bottom, VIEW_HISTO->left,
		 VIEW_HISTO->right);

    nrows = VIEW_HISTO->nrows;
    ncols = VIEW_HISTO->ncols;

    /* get maximum range */
    if (b_or_a == BEFORE_STD) {
	max_range = 1;
	for (b = 0; b < nbands; b++)
	    if (max[b] - min[b] > max_range)
		max_range = max[b] - min[b];
	old_range = max_range;
    }
    else
	max_range = old_range;

    /* calculate the width (in pixels) for each histogram bar */
    histo_width = (ncols - BORDER * 2) / max_range;
    if (histo_width % 2 == 0)
	histo_width = histo_width - 1;
    if (histo_width < MIN_HISTO_WIDTH)
	histo_width = MIN_HISTO_WIDTH;
    else if (histo_width > MAX_HISTO_WIDTH)
	histo_width = MAX_HISTO_WIDTH;

    height = (nrows - BORDER * 2) / nbands;
    width = (ncols - BORDER * 2) / histo_width * histo_width;
    nbars = width / histo_width;

    /* Set text size based on legend length */
    R_text_size(3 * TEXT_HEIGHT / 4, TEXT_HEIGHT);

    h_top = VIEW_HISTO->top + BORDER;
    h_left = VIEW_HISTO->left + BORDER;
    h_right = h_left + width - 1;

    grand_max = 0;
    for (b = 0; b < nbands; b++)
	for (x1 = 0; x1 < MAX_CATS; x1++)
	    if (histo[b][x1] > grand_max)
		grand_max = histo[b][x1];

    if (grand_max > 0)
	scale = (float)(height - LEGEND_SPACE) / (float)grand_max;
    else
	scale = 0;

    R_standard_color(BLACK);

    /* print header message */
    if (b_or_a == BEFORE_STD)
	sprintf(msg, "Region Sample Size: %d", np);
    else
	sprintf(msg, "Histograms with Range = %5.2f *", in_nstd);
    R_move_abs(VIEW_HISTO->left + 3, VIEW_HISTO->top + TEXT_HEIGHT);
    R_text(msg);
    if (b_or_a == AFTER_STD) {
	R_font(GREEK_FONT);
	sprintf(msg, "                                %c", SIGMA);
	R_move_abs(VIEW_HISTO->left + 3, VIEW_HISTO->top + TEXT_HEIGHT);
	R_text(msg);
	R_font(NORMAL_FONT);
    }

    /* draw the histograms */
    for (b = 0; b < nbands; b++) {
	int bottom_adjusted;

	h_bottom = h_top + height - 1;
	bottom_adjusted = h_bottom - 2 * LEGEND_SPACE / 3;

	mean = MEAN(b);
	std_dev = STD_DEV(b);

	/* print legend info under the histogram */
	if (b_or_a == BEFORE_STD)
	    sprintf(msg, "%-14s                    %s=%-3d  %s=%-3d",
		    Refer.file[b].name, MIN, min[b], MAX, max[b]);
	else
	    sprintf(msg, "%-14s                    %s=%-3d  %s=%-3d",
		    Refer.file[b].name, RANGE_MIN, min[b], RANGE_MAX, max[b]);
	R_move_abs(h_left, h_bottom);
	R_text(msg);
	sprintf(msg, "                %c=%-5.1f  %c=%-5.2f", MU, mean,
		SIGMA, std_dev);
	R_move_abs(h_left, h_bottom);
	R_font(GREEK_FONT);
	R_text(msg);
	R_font(NORMAL_FONT);

	/* draw the actual histogram */
	cat = mean - (nbars - 1) / 2;
	y1 = y2 = bottom_adjusted;
	x1 = h_left;
	x2 = h_left + histo_width - 1;

	R_move_abs(x1, y1);
	for (bar = 0; bar < nbars && cat < MAX_CATS;
	     bar++, cat++, x2 += histo_width) {
	    if (cat >= 0)
		y2 = bottom_adjusted - (histo[b][cat] * scale + .5);
	    R_cont_abs(x1, y2);
	    R_cont_abs(x2, y2);
	    y1 = y2;
	    x1 = x2;
	}
	R_cont_abs(x1, bottom_adjusted);
	R_cont_abs(h_right, bottom_adjusted);

	/* put sigma and mu on the histogram */
	cat = mean - (nbars - 1) / 2;
	x1 = h_left;
	R_font(GREEK_FONT);
	for (bar = 0; bar < nbars && cat < MAX_CATS;
	     bar++, cat++, x1 += histo_width) {
	    float x;
	    int mp;
	    int top_adjusted;
	    float vh;
	    int color;

	    if (cat < 0)
		continue;

	    if (cat == (int)(x = mean + .5)) {
		sprintf(msg, "%c", MU);
		vh = .5;
		color = GREY;
	    }
	    else if (cat == (int)(x = mean + std_dev + .5)) {
		sprintf(msg, "%c", SIGMA);
		vh = .25;
		color = GREY;
	    }
	    else if (cat == (int)(x = mean - std_dev + .5)) {
		sprintf(msg, "%c", SIGMA);
		vh = .25;
		color = GREY;
	    }
	    else if (cat == (int)(x = (float)max[b] + .5)) {
		if (b_or_a == AFTER_STD)
		    sprintf(msg, "%s", RANGE_MAX);
		else
		    sprintf(msg, "%s", MAX);
		vh = .5;
		color = RED;
	    }
	    else if (cat == (int)(x = (float)min[b] + .5)) {
		if (b_or_a == AFTER_STD)
		    sprintf(msg, "%s", RANGE_MIN);
		else
		    sprintf(msg, "%s", MIN);
		vh = .5;
		color = RED;
	    }
	    else
		continue;

	    top_adjusted = bottom_adjusted - (bottom_adjusted - h_top) * vh;
	    mp = x1 + (x - cat) * histo_width;
	    R_move_abs(mp - 2, bottom_adjusted + TEXT_HEIGHT);
	    if (color == RED) {
		R_standard_color(color);
		R_font(NORMAL_FONT);
		R_text(msg);
		R_font(GREEK_FONT);
	    }
	    else {
		R_text(msg);
		R_standard_color(color);
	    }
	    R_move_abs(mp, bottom_adjusted);
	    R_cont_abs(mp, top_adjusted);
	    R_standard_color(BLACK);
	}
	R_font(NORMAL_FONT);

	h_top = h_bottom + 1;
    }
    R_standard_color(BLACK);
    R_text_size(3 * NORMAL_TEXT_SIZE / 4, NORMAL_TEXT_SIZE);
    R_set_window(SCREEN_TOP, SCREEN_BOTTOM, SCREEN_LEFT, SCREEN_RIGHT);

    return 0;
}
