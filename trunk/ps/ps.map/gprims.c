/* These functions provide the graphics primitives.
 **
 ** Author: Paul W. Carlson     March 1992
 ** Changed to be used with gis libes plot routines Olga Waupotitsch, Dec,1993
 */

#include "ps_info.h"
static int cur_x, cur_y;

int draw_line(double x1, double y1, double x2, double y2)
{
    fprintf(PS.fp, "%.1f %.1f %.1f %.1f L\n", x2, y2, x1, y1);

    return 0;
}

/* in the following functions x, y coordinates are calculated
   by calling G_plot_where_xy() and therefore they are integers which need to
   be divided by 10. to get double with precision until first decimal place
 */

int start_line(double east, double north)
{
    int x, y;

    G_plot_where_xy(east, north, &x, &y);
    fprintf(PS.fp, "%.1f %.1f NM ", (double)x / 10., (double)y / 10.);

    return 0;
}

int move_local(int x, int y)
{
    cur_x = x;
    cur_y = y;

    return 0;
}

int cont_local(int x2, int y2)
{
    if (((double)cur_x / 10. > PS.map_right &&
	 (double)x2 / 10. > PS.map_right)
	|| ((double)cur_x / 10. < PS.map_left &&
	    (double)x2 / 10. < PS.map_left)
	|| ((double)cur_y / 10. < PS.map_bot && (double)y2 / 10. < PS.map_bot)
	|| ((double)cur_y / 10. > PS.map_top && (double)y2 / 10. > PS.map_top)
	) {
	if (sec_draw)
	    return -2;
	/* when both coordinates are outside window draw is called twice with the
	   same line segment by plot_line in gis_libes if proj=Lat-Lon */

	else {
	    fprintf(PS.fp, "%.1f %.1f M", (double)x2 / 10., (double)y2 / 10.);
	    sec_draw = 1;
	    return -1;
	}

    }
    if (sec_draw) {
	/* need to break cont. draw and move to new current point */
	/* L is moveto lineto stroke */
	fprintf(PS.fp, " D");	/* end line */
	/* now start new path */
	fprintf(PS.fp, " %.1f %.1f NM ", (double)cur_x / 10.,
		(double)cur_y / 10.);
    }
    fprintf(PS.fp, "%.1f %.1f LN", (double)x2 / 10., (double)y2 / 10.);
    /* LN is lineto */
    move_local(x2, y2);
    sec_draw = 1;

    return 0;
}

int set_line_width(double width)
{
    fprintf(PS.fp, "%.8f W\n", width);

    return 0;
}

int set_font_name(char *name)
{
    fprintf(PS.fp, "(%s) FN\n", name);

    return 0;
}

int set_font_size(int fontsize)
{
    fprintf(PS.fp, "%d SF\n", fontsize);

    return 0;
}

int show_text(double x, double y, char *text)
{
    fprintf(PS.fp, "(%s)\n", text);
    fprintf(PS.fp, "%.1f %.1f MS\n", x, y);

    return 0;
}
