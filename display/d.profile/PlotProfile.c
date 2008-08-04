/* PlotProfile.c
 *
 * function defined:
 *
 * PlotProfile(profile,letter,min,max)
 *
 * struct Profile profile;      - profile structure 
 * int min,                     - min cell-file value
 *     max;                     - max cell-file calue
 * 
 * PURPOSE: To plot a profile in the currently chosen on-screen window.
 * The profile's length is scaled to fit along the x-axis.  The profile
 * is scaled to fit the maximum and minimum cell-file values (instead of
 * the maximum and minimum profile values) on the y-axis. 
 *
 * NOTES: 
 *
 * 1) assumes that R_open_driver has already been called.
 *
 * 2) assumes that the profile structure has been both initialized by
 * a call to InitProfile, and filled with data by a call to ExtractProfile.
 *
 * Dave Johnson
 * DBA Systems, Inc.
 * 10560 Arrowhead Drive
 * Fairfax, Virginia 22030
 *
 */

#include <limits.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "profile.h"

#define ORIGIN_X	0.13
#define ORIGIN_Y	0.07
#define YAXIS_END	0.77
#define XAXIS_END	0.95
#define TEXT_HEIGHT	0.11
#define TEXT_COLUMN    	0.07

double _get_cat(UCAT *, int);

int PlotProfile(struct Profile profile, char *letter, int min, int max)
{
    struct ProfileNode *ptr;
    char txt_buf[512];
    int done;
    int text_width,
	text_height,
	i, t, b, l, r, tt, tb, tl, tr, height, width, x_line[3], y_line[3];
    double yoffset, xoffset, xscale, yscale;

    /* get current graphics window coordinates */
    D_get_screen_window(&t, &b, &l, &r);
    R_set_window(t, b, l, r);

    /* erase current graphics window to black */
    R_standard_color(D_translate_color(DEFAULT_BG_COLOR));
    D_erase_window();

    /* create axis lines */
    height = b - t;
    width = r - l;
    x_line[0] = x_line[1] = l + (int)(ORIGIN_X * width);
    x_line[2] = l + (int)(XAXIS_END * width);
    y_line[0] = b - (int)(YAXIS_END * height);
    y_line[1] = y_line[2] = b - (int)(ORIGIN_Y * height);
    R_standard_color(D_translate_color(DEFAULT_FG_COLOR));

    /* figure scaling factors and offsets for profile line */
    xscale = ((double)(x_line[2] - x_line[1]) / ((double)profile.count));
    yscale = ((double)(y_line[1] - y_line[0])) / ((double)(max - min));
    yoffset = (double)(y_line[1]);
    xoffset = (double)x_line[1];

    /* plot profile */
    ptr = profile.ptr;
    R_move_abs((int)xoffset, (int)yoffset);
    for (i = 0; i <= profile.count; i++) {
	if (ptr == NULL)
	    break;
	if (xscale > 1) {
	    R_cont_abs((int)(xoffset + xscale * i),
		       (int)(yoffset - yscale * _get_cat(&ptr->cat, min)));
	    R_cont_abs((int)(xoffset + xscale * (i + 1.0)),
		       (int)(yoffset - yscale * _get_cat(&ptr->cat, min)));
	}
	else
	    R_cont_abs((int)(xoffset + xscale * i),
		       (int)(yoffset - yscale * _get_cat(&ptr->cat, min)));
	ptr = ptr->next;
    }
    R_standard_color(D_translate_color("red"));
    R_polyline_abs(x_line, y_line, 3);

    /* loop until coordinate text is sized correctly to fit in window */
    text_height = TEXT_HEIGHT * (b - t);
    text_width = text_height * 0.8;
    R_standard_color(D_translate_color(DEFAULT_FG_COLOR));
    sprintf(txt_buf, "%s: From (%10.2f,%10.2f) to (%10.2f,%10.2f)",
	    letter, profile.e1, profile.n1, profile.e2, profile.n2);
    done = 0;
    do {
	R_get_text_box(txt_buf, &tt, &tb, &tl, &tr);
	if ((tr - tl) >= (r - l)) {
	    text_height *= 0.95;
	    text_width *= 0.95;
	    R_text_size(text_width, text_height);
	}
	else
	    done = 1;
    }
    while (!done);
    R_move_abs((int)(l + 0.5 * (r - l) - .5 * (tr - tl)),
	       (int)(t + .12 * (b - t)));
    R_text(txt_buf);

    /* set text size for y-axis labels */
    text_height = TEXT_HEIGHT * (b - t);
    text_width = text_height * 0.8;
    R_text_size(text_width, text_height);

    /* plot y-axis label (bottom) */
    sprintf(txt_buf, "%d", min);
    R_get_text_box(txt_buf, &tt, &tb, &tl, &tr);
    R_move_abs((int)(l + TEXT_COLUMN * (r - l) - .5 * (tr - tl)),
	       (int)(yoffset + .5 * (tb - tt)));
    R_text(txt_buf);

    /* plot y-axis label (top) */
    sprintf(txt_buf, "%d", max);
    R_get_text_box(txt_buf, &tt, &tb, &tl, &tr);
    R_move_abs((int)(l + TEXT_COLUMN * (r - l) - .5 * (tr - tl)),
	       (int)(y_line[0] + .5 * (tb - tt)));
    R_text(txt_buf);
    R_stabilize();

    return 0;
}


double _get_cat(UCAT * theCat, int min)
{
    switch (theCat->type) {
    case CELL_TYPE:
	if (theCat->val.c >= min)
	    return (double)(theCat->val.c - (double)min);
	else
	    return (double)0.0;
    case FCELL_TYPE:
	if (theCat->val.f >= min)
	    return (double)(theCat->val.f - (double)min);
	else
	    return (double)0.0;
    case DCELL_TYPE:
	if (theCat->val.d >= min)
	    return (theCat->val.d - (double)min);
	else
	    return (double)0.0;
    default:			/* Shouldn't happen */
	return (double)0.0;
    }
}

/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
