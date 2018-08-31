/* Function: map_setup
 **
 ** Author: Paul W. Carlson     3/92
 */

#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "distance.h"


int map_setup(void)
{
    double w, h;

    /* set top of map */
    if (PS.set_y < PS.min_y)
	PS.min_y = PS.set_y;
    PS.map_y_orig = PS.min_y / 72.0;

    if (!PS.do_raster && !grp.do_group) {
	/* if scale has been set... */
	if (PS.scaletext[0]) {
	    /* if scaled map will fit in map limits... */
	    w = scale(PS.scaletext);
	    h = w * (PS.w.north - PS.w.south) / (PS.w.east - PS.w.west);
	    if (w <= PS.map_width && h <= PS.map_height) {
		PS.map_width = w;
		PS.map_height = h;
		PS.map_pix_wide = 72.0 * PS.map_width;
		PS.map_pix_high = 72.0 * PS.map_height;
	    }
	    else    /* kill the scale */
		PS.scaletext[0] = 0;
	}

	/* fit map to bounding box */
	fit_map_to_box();
    }

    else {
	if (PS.scaletext[0]) {
	    /* if scaled map will fit in map limits... */
	    w = scale(PS.scaletext);
	    h = w * PS.w.ns_res * (double)PS.w.rows /
		(PS.w.ew_res * (double)PS.w.cols);
	    if (w <= PS.map_width && h <= PS.map_height) {
		PS.map_width = w;
		PS.map_height = h;
		PS.map_pix_wide = 72.0 * PS.map_width;
		PS.map_pix_high = 72.0 * PS.map_height;
	    }
	    else    /* kill the scale */
		PS.scaletext[0] = 0;
	}

	/* fit map to bounding box */
	fit_map_to_box();

	PS.cells_high = PS.w.rows;
	PS.cells_wide = PS.w.cols;
	PS.ew_res = PS.w.ew_res;
	PS.ns_res = PS.w.ns_res;

	PS.row_delta = 1;
	PS.col_delta = 1;

	/* compute conversion factors */
	PS.ew_to_x = PS.map_pix_wide / (PS.w.east - PS.w.west);
	PS.ns_to_y = PS.map_pix_high / (PS.w.north - PS.w.south);
    }

    /* set the scale */
    /*   work from height not width to minimize lat/lon curvature problems?? */
    if (!PS.scaletext[0]) {
	sprintf(PS.scaletext, "1 : %.0f",
		METERS_TO_INCHES * distance(PS.w.east,
					    PS.w.west) * 72.0 /
		PS.map_pix_wide);
    }

    G_message(_("Scale set to %s"), PS.scaletext);

    /* compute map edges */
    PS.map_left = 72.0 * PS.map_x_orig;
    PS.map_top = 72.0 * PS.map_y_orig;
    PS.map_bot = PS.map_top - PS.map_pix_high;
    PS.map_right = PS.map_left + PS.map_pix_wide;
    PS.min_y = PS.map_bot;

    /* we want the size to be 10 times biger, because G_plot_where_xy()
       returns integer values (pixels) for x and y, and we want doubles
       until the first decimal point. Then in move() and cont() we will
       divide x and y by 10. to get double coordinates */
    G_setup_plot(PS.map_top * 10., PS.map_bot * 10., PS.map_left * 10.,
		 PS.map_right * 10., move_local, cont_local);

    /* debug fprintf (stdout,"t %.1f b %.1f l %.1f r %.1f\n", PS.map_top,
       PS.map_bot, PS.map_left, PS.map_right);
     */

    /* no need to go on if we're just here for a look-see. (-b flag) */
    if (!PS.fp)
	return 0;


    /* save original graphics state */
    fprintf(PS.fp, "gsave ");

    /* compute conversion factor from meters to PostScript window coordinates */
    /*
       G_begin_distance_calculations();
       meters_to_PS = (PS.map_top - PS.map_bot) / G_distance(0., PS.w.south, 0., PS.w.north);
     */

    /* clip to edge of border */
    box_clip(PS.map_top - 1.0, PS.map_bot + 1.0,
	     PS.map_left + 1.0, PS.map_right - 1.0);

    return 0;
}
