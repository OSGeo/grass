/* Functions: do_grid, do_grid_numbers, format_northing, format_easting
 **
 ** These functions are much modified versions of the p.map functions.
 **
 ** Author: Paul W. Carlson     April 1992
 */
#include <string.h>
#include <math.h>
#include "local_proto.h"

#define LEFT 0
#define RIGHT 1
#define LOWER 0
#define UPPER 1
#define CENTER 2

static char *format_northing(double, int);
static char *format_easting(double, int);

int do_grid(void)
{
    double g, e1, e2;
    int j;

    if (PS.grid <= 0)
	return 1;

    /* set color and set line width to 1 */
    set_ps_color(&PS.grid_color);
    set_line_width(PS.grid_width);

    /* draw horizontal lines in 3 pieces -- lat-lon lines must not
     * extend more than half the globe
     * start with first grid line just south of the window north
     */
    e1 = (PS.w.east * 2 + PS.w.west) / 3;
    e2 = (PS.w.west * 2 + PS.w.east) / 3;

    g = floor(PS.w.north / PS.grid) * PS.grid;
    for (j = 0; g >= PS.w.south; j++, g -= PS.grid) {
	if (g == PS.w.north || g == PS.w.south)
	    continue;

	start_line(PS.w.east, g);
	sec_draw = 0;
	G_plot_line(PS.w.east, g, e1, g);
	fprintf(PS.fp, " D ");

	start_line(e1, g);
	sec_draw = 0;
	G_plot_line(e1, g, e2, g);
	fprintf(PS.fp, " D ");

	start_line(e2, g);
	sec_draw = 0;
	G_plot_line(e2, g, PS.w.west, g);
	fprintf(PS.fp, " D\n");
    }

    /* vertical lines */
    /* start with first grid line just west of the window east */
    g = floor(PS.w.east / PS.grid) * PS.grid;
    for (j = 0; g > PS.w.west; j++, g -= PS.grid) {
	if (g == PS.w.east || g == PS.w.west)
	    continue;
	start_line(g, PS.w.north);
	sec_draw = 0;
	G_plot_line(g, PS.w.north, g, PS.w.south);
	if (j & 1)
	    fprintf(PS.fp, " D\n");
	else
	    fprintf(PS.fp, " D ");
    }

    return 0;
}

int do_grid_cross(void)
{
    double g_north, g_east;
    int j, k;

    if (PS.grid <= 0)
	return 1;

    /* set color and set line width to 1 */
    set_ps_color(&PS.grid_color);
    set_line_width(PS.grid_width);

    g_north = floor(PS.w.north / PS.grid) * PS.grid;
    g_east = floor(PS.w.east / PS.grid) * PS.grid;
    for (j = 0; g_north >= PS.w.south; j++, g_north -= PS.grid) {
	for (k = 0; g_east > PS.w.west; k++, g_east -= PS.grid) {

	    if (g_north == PS.w.north || g_north == PS.w.south)
		continue;
	    if (g_east == PS.w.east || g_east == PS.w.west)
		continue;

	    start_line(g_east - PS.grid_cross, g_north);
	    G_plot_line(g_east - PS.grid_cross, g_north,
			g_east + PS.grid_cross, g_north);
	    fprintf(PS.fp, " D ");
	    start_line(g_east, g_north - PS.grid_cross);
	    G_plot_line(g_east, g_north - PS.grid_cross, g_east,
			g_north + PS.grid_cross);
	    fprintf(PS.fp, " D ");
	}
	g_east = floor(PS.w.east / PS.grid) * PS.grid;
    }

    return 0;
}

int do_grid_numbers(void)
{
    double g;
    char num_text[50];
    int grid, vy, vx, hy = 0, hx = 0;
    int first, len, x, y, last_bottom, last_right;
    int rounded_grid, margin;

    if (PS.grid <= 0 || PS.grid_numbers <= 0)
	return 1;
    grid = PS.grid * PS.grid_numbers;

    /* round grid to multiple of 10 */
    rounded_grid = 1;
    if (PS.w.proj != PROJECTION_LL) {
	sprintf(num_text, "%d", PS.grid);
	len = strlen(num_text);
	while (len-- && num_text[len] == '0')
	    rounded_grid *= 10;
	if (rounded_grid == 10)
	    rounded_grid = 1;
    }

    /* initialize */
    set_font_name(PS.grid_font);
    set_font_size(PS.grid_fontsize);
    set_ps_color(&PS.grid_numbers_color);
    first = 1;

    /* horizontal grid numbers
     * these numbers only appear on the left edge of the first panel.
     * center the numbers on each grid line.
     * suppress number if it falls off the map or would overlay the previous
     *  label
     */
    margin = (int)(0.2 * (double)PS.grid_fontsize + 0.5);
    if (margin < 2)
	margin = 2;
    fprintf(PS.fp, "/mg %d def\n", margin);
    g = floor(PS.w.north / grid) * grid;
    last_bottom = (int)PS.map_top;
    first = 1;
    /* x = XCONV(PS.w.west); */

    for (; g > PS.w.south; g -= grid) {
	/*y = YCONV(g); */

	G_plot_where_xy(PS.w.west, g, &vx, &vy);
	x = (double)vx / 10.;
	y = (double)vy / 10.;

	if (y + PS.grid_fontsize > last_bottom)
	    continue;
	if (y - PS.grid_fontsize < (int)PS.map_bot)
	    continue;
	sprintf(num_text, "%s", format_northing(g, rounded_grid));
	text_box_path(x, y, LEFT, CENTER, num_text, 0);
	set_rgb_color(WHITE);
	fprintf(PS.fp, "F ");
	set_ps_color(&PS.grid_numbers_color);
	fprintf(PS.fp, "TIB\n");
	last_bottom = y - PS.grid_fontsize;
	if (first) {
	    first = 0;
	    hy = y + (int)(0.5 * (double)PS.grid_fontsize + 0.5) + margin;
	    hx = x + 0.7 * strlen(num_text) * PS.grid_fontsize + 2 * margin;
	}
    }

    /* vertical grid numbers 
     * center the numbers on each grid line.
     * suppress number if it falls of the map or would overlay the previous
     *  label
     */
    g = floor(PS.w.west / grid) * grid;
    last_right = (int)PS.map_left;
    /* y = YCONV(PS.w.north); */
    for (; g < PS.w.east; g += grid) {
	/* x = XCONV(g); */

	G_plot_where_xy(g, PS.w.north, &vx, &vy);
	x = (double)vx / 10.;
	y = (double)vy / 10.;

	if (x - PS.grid_fontsize < last_right)
	    continue;
	if (x + PS.grid_fontsize > (int)PS.map_right)
	    continue;
	sprintf(num_text, "%s", format_easting(g, rounded_grid));
	vy = y - 0.7 * strlen(num_text) * PS.grid_fontsize - 2 * margin;
	vx = x - (int)(0.5 * (double)PS.grid_fontsize + 0.5) - margin;
	if (vx < hx && vy < hy)
	    continue;
	fprintf(PS.fp, "ZB (%s) PB 90 rotate\n", num_text);
	fprintf(PS.fp, "%d br sub bl add mg add\n", y);
	fprintf(PS.fp, "%d bt bb sub D2 add mg sub neg TR TB\n", x);
	set_rgb_color(WHITE);
	fprintf(PS.fp, "F ");
	set_ps_color(&PS.grid_numbers_color);
	fprintf(PS.fp, "TIB\n");
	last_right = x + PS.grid_fontsize;
    }

    return 0;
}

static char *format_northing(double n, int round)
{
    static char text[50];

    if (PS.w.proj == PROJECTION_LL)
	G_format_northing(n, text, PS.w.proj);
    else {
	n = floor(n / round);
	sprintf(text, "%.0f", n);
    }
    return text;
}

static char *format_easting(double e, int round)
{
    static char text[50];

    if (PS.w.proj == PROJECTION_LL)
	G_format_easting(e, text, PS.w.proj);
    else {
	e = floor(e / round);
	sprintf(text, "%.0f", e);
    }
    return text;
}
