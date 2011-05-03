/* Function: map_info
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include "map_info.h"
#include "local_proto.h"

int map_info(void)
{
    char buf[400];
    char east[50], west[50], north[50], south[50];
    double x, y, k, fontsize, dy, margin;
    static char *region = "REGION:   ";

    /* get north, south, east and west strings */
    G_format_northing(PS.w.north, north, PS.w.proj);
    G_format_northing(PS.w.south, south, PS.w.proj);
    G_format_easting(PS.w.east, east, PS.w.proj);
    G_format_easting(PS.w.west, west, PS.w.proj);

    /* set font */
    fontsize = (double)m_info.fontsize;
    fprintf(PS.fp, "(%s) FN %.1f SF\n", m_info.font, fontsize);

    /* get text location */
    dy = fontsize;

    if (m_info.x > 0.0)
	x = 72.0 * m_info.x;
    else
	x = PS.map_left;
    if (m_info.y > 0.0)
	y = 72.0 * (PS.page_height - m_info.y);
    else
	y = PS.min_y;

    margin = 0.2 * fontsize;

    if (x < PS.map_left + margin)
	x = PS.map_left + margin;

    /* get offset for text to line up */
    fprintf(PS.fp, "%.1f (%s) SW pop add /sx XD\n", x, region);

    /* if map ifo is on map... */
    if (y > PS.map_bot && y <= PS.map_top && x < PS.map_right) {
	fprintf(PS.fp, "/mg %.1f def\n", margin);

	/* get max. length of text */
	fprintf(PS.fp, "(%s) SW pop /t1 XD\n", PS.scaletext);
	if (PS.grid) {
	    k = 5.5;
	    sprintf(buf,
		    "%d %s", PS.grid, G_database_unit_name(PS.grid != 1));
	    fprintf(PS.fp, "(%s) SW pop /t2 XD\n", buf);
	    fprintf(PS.fp, "t1 t2 lt {/t1 t2 def} if \n");
	}
	else
	    k = 4.5;
	fprintf(PS.fp, "(%s        %s) SW pop /t3 XD\n", west, east);
	fprintf(PS.fp, "t1 t3 lt {/t1 t3 def} if \n");
	fprintf(PS.fp, "/t1 t1 sx mg add add def\n");

	/* draw the background box */
	if (! color_none(&m_info.bgcolor)) {
	    set_ps_color(&m_info.bgcolor);
	    fprintf(PS.fp, "%.1f %.1f t1 %.1f B fill \n",
		    x - margin, y - k * dy - margin, y);
	}
	/* draw the border, if set */
	if (! color_none(&m_info.border)) {
	    set_ps_color(&m_info.border);
	    fprintf(PS.fp, "%.1f %.1f t1 %.1f B\n",
		    x - margin, y - k * dy - margin, y);
	    fprintf(PS.fp, "D\n");
	}
    }

    /* set text color */
    set_ps_color(&m_info.color);

    /* show map scale */
    show_text(x, y - dy, "SCALE:");
    fprintf(PS.fp, "(%s) sx %.1f MS\n", PS.scaletext, y - dy);
    y -= dy;

    /* show size of grid, if any */
    if (PS.grid) {
	sprintf(buf, "%d %s", PS.grid, G_database_unit_name(PS.grid != 1));
	show_text(x, y - dy, "GRID:");
	fprintf(PS.fp, "(%s) sx %.1f MS\n", buf, y - dy);
	y -= dy;
    }

    /* show region */
    dy = 2.5 * fontsize;
    y -= dy;
    sprintf(buf, "%s    ", west);
    show_text(x, y, region);
    fprintf(PS.fp, "(%s) sx %.1f MS\n", buf, y);
    fprintf(PS.fp, "currentpoint pop ");
    fprintf(PS.fp, "(%s) SW pop D2 sub /xo XD\n", north);
    fprintf(PS.fp, "(    %s) show\n", east);
    dy = fontsize;
    fprintf(PS.fp, "(%s) xo %.1f MS\n", north, y + dy);
    fprintf(PS.fp, "(%s) xo %.1f MS\n", south, y - dy);
    y -= dy;
    if (PS.min_y > y)
	PS.min_y = y;

    return 0;
}
