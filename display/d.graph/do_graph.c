#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include <grass/glocale.h>

#include "options.h"
#include "local_proto.h"

#define CHUNK	128

static int coors_allocated = 0;
static double *xarray;
static double *yarray;

static float xincr;
static float yincr;

static double rotation;		/* degrees counter-clockwise from east */

static RGBA_Color last_color;

static double t, b, l, r;

static double cur_x, cur_y;

int set_graph_stuff(void)
{
    D_get_dst(&t, &b, &l, &r);

    if (mapunits) {
	xincr = (r - l) / 100.;
	if (xincr < 0.0)
	    xincr = -xincr;		/* mod: shapiro 13 jun 1991 */
	yincr = (b - t) / 100.;
	if (yincr < 0.0)
	    yincr = -yincr;		/* mod: shapiro 13 jun 1991 */
    }
    else
	xincr = yincr = 1;

    rotation = 0.0;		/* init */

    return 0;
}

int set_text_size(void)
{
    if (hsize >= 0. && vsize >= 0. && hsize <= 100. && vsize <= 100.) {
	D_text_size(hsize * xincr, vsize * yincr);
	G_debug(3, "text size initialized to [%.1f,%.1f]",
		hsize * xincr, vsize * yincr);
    }
    return (0);
}

int do_draw(const char *str)
{
    float xper, yper;

    if (2 != sscanf(str, "%*s %f %f", &xper, &yper)) {
	G_warning(_("Problem parsing coordinates [%s]"), str);
	return (-1);
    }

    D_line_abs(cur_x, cur_y, xper, yper);
    cur_x = xper;
    cur_y = yper;

    return (0);
}

int do_move(const char *str)
{
    float xper, yper;

    if (2 != sscanf(str, "%*s %f %f", &xper, &yper)) {
	G_warning(_("Problem parsing coordinates [%s]"), str);
	return (-1);
    }

    D_pos_abs(xper, yper);
    cur_x = xper;
    cur_y = yper;

    return (0);
}

int do_color(const char *str)
{
    char in_color[64];
    int R, G, B, color = 0;

    if (1 != sscanf(str, "%*s %s", in_color)) {
	G_warning(_("Unable to read color"));
	return (-1);
    }

    /* Parse and select color */
    color = G_str_to_color(in_color, &R, &G, &B);
    if (color == 0) {
	G_warning(_("[%s]: No such color"), in_color);
	/* store for backup */
	last_color.a = RGBA_COLOR_NONE;
	return (-1);
    }
    if (color == 1) {
	D_RGB_color(R, G, B);
	/* store for backup */
	set_last_color(R, G, B, RGBA_COLOR_OPAQUE);
    }
    if (color == 2) {		/* color == 'none' */
	R = D_translate_color(DEFAULT_BG_COLOR);
	D_use_color(R);
	/* store for backup */
	set_last_color(0, 0, 0, RGBA_COLOR_NONE);
    }
    return (0);
}

int do_linewidth(const char *str)
{
    double width;

    if (1 != sscanf(str, "%*s %lf", &width)) {
	G_warning(_("Problem parsing command [%s]"), str);
	return (-1);
    }

    D_line_width(width);
    G_debug(3, "line width set to %.1f", width);

    return (0);
}


int do_poly(char *buff, FILE * infile)
{
    int num;
    char origcmd[64];
    float xper, yper;
    int to_return;

    sscanf(buff, "%s", origcmd);

    num = 0;

    for (;;) {
	if ((to_return = G_getl2(buff, 128, infile)) != 1)
	    break;

	if (2 != sscanf(buff, "%f %f", &xper, &yper)) {

	    if ('#' == buff[0]) {
		G_debug(3, " skipping comment line [%s]", buff);
		continue;
	    }

	    G_debug(3, "coordinate pair not found. ending polygon. [%s]",
		    buff);
	    break;
	}

	check_alloc(num + 1);

	xarray[num] = xper;
	yarray[num] = yper;

	num++;
    }

    if (num) {
	/* this check is here so you can use the "polyline" command 
	   to make an unfilled polygon */
	if (!strcmp(origcmd, "polygon"))
	    D_polygon_abs(xarray, yarray, num);
	else
	    D_polyline_abs(xarray, yarray, num);
    }

    return (to_return);
}

int do_size(const char *str)
{
    float xper, yper;
    int ret;

    ret = sscanf(str, "%*s %f %f", &xper, &yper);

    if (ret != 2 && ret != 1) {
	G_warning(_("Problem parsing command [%s]"), str);
	return (-1);
    }

    /* if only one size is given assume same value in both axes */
    if (ret == 1)
	yper = xper;

    if (xper < 0. || yper < 0. || xper > 100. || yper > 100.)
	return (-1);

    D_text_size(xper * xincr, yper * yincr);
    G_debug(3, "text size set to [%.1f,%.1f]",
	    xper * xincr, yper * yincr);

    return (0);
}

int do_rotate(const char *str)
{
    if (1 != sscanf(str, "%*s %lf", &rotation)) {
	G_warning(_("Problem parsing command [%s]"), str);
	return (-1);
    }

    D_text_rotation(rotation);
    G_debug(3, "rotation set to %.1f degrees", rotation);

    return (0);
}

int do_text(const char *str)
{
    const char *ptr = str;

    /* skip to beginning of actual text */
    for (; *ptr != ' '; ptr++)
	;
    for (; *ptr == ' '; ptr++)
	;
    D_text(ptr);

    return 0;
}

int check_alloc(int num)
{
    int to_alloc;

    if (num < coors_allocated)
	return 0;

    to_alloc = coors_allocated;
    if (num >= to_alloc)
	to_alloc = num + CHUNK;

    xarray = G_realloc(xarray, to_alloc * sizeof(double));
    yarray = G_realloc(yarray, to_alloc * sizeof(double));

    coors_allocated = to_alloc;

    return 0;
}

int do_icon(const char *str)
{
    double xper, yper;
    char type;
    double size;
    double ix, iy;

    if (4 != sscanf(str, "%*s %c %lf %lf %lf", &type, &size, &xper, &yper)) {
	G_warning(_("Problem parsing command [%s]"), str);
	return (-1);
    }

    ix = xper;
    iy = yper;
    size *= yincr;

    D_begin();

    switch (type & 0x7F) {
    case 'o':
	D_move_abs(ix - size, iy - size);
	D_cont_abs(ix - size, iy + size);
	D_cont_abs(ix + size, iy + size);
	D_cont_abs(ix + size, iy - size);
	D_cont_abs(ix - size, iy - size);
	break;
    case 'x':
	D_move_abs(ix - size, iy - size);
	D_cont_abs(ix + size, iy + size);
	D_move_abs(ix - size, iy + size);
	D_cont_abs(ix + size, iy - size);
	break;
    case '+':
    default:
	D_move_abs(ix, iy - size);
	D_cont_abs(ix, iy + size);
	D_move_abs(ix - size, iy);
	D_cont_abs(ix + size, iy);
	break;
    }

    D_end();
    D_stroke();

    return (0);
}

int do_symbol(const char *str)
{
    double xper, yper;
    double size;
    double ix, iy;
    char *symb_name;
    SYMBOL *Symb;
    char *line_color_str, *fill_color_str;
    RGBA_Color *line_color, *fill_color;
    int R, G, B, ret;


    line_color = G_malloc(sizeof(RGBA_Color));
    fill_color = G_malloc(sizeof(RGBA_Color));

    symb_name = G_malloc(strlen(str) + 1);	/* well, it won't be any bigger than this */
    line_color_str = G_malloc(strlen(str) + 1);
    fill_color_str = G_malloc(strlen(str) + 1);

    G_debug(3, "do_symbol() [%s]", str);

    /* set default colors so colors are optional */
    strcpy(line_color_str, DEFAULT_FG_COLOR);
    strcpy(fill_color_str, "grey");

    if (sscanf
	(str, "%*s %s %lf %lf %lf %s %s", symb_name, &size, &xper, &yper,
	 line_color_str, fill_color_str) < 4) {
	G_warning(_("Problem parsing command [%s]"), str);
	return (-1);
    }

    ix = xper;
    iy = yper;
    size *= yincr;

    /* parse line color */
    ret = G_str_to_color(line_color_str, &R, &G, &B);
    line_color->r = (unsigned char)R;
    line_color->g = (unsigned char)G;
    line_color->b = (unsigned char)B;

    if (ret == 1) {
	/* here alpha is only used as an on/off switch, otherwise unused by the display drivers */
	line_color->a = RGBA_COLOR_OPAQUE;
    }
    else if (ret == 2)
	line_color->a = RGBA_COLOR_NONE;
    else {
	G_warning(_("[%s]: No such color"), line_color_str);
	return (-1);
    }

    /* parse fill color */
    ret = G_str_to_color(fill_color_str, &R, &G, &B);
    fill_color->r = (unsigned char)R;
    fill_color->g = (unsigned char)G;
    fill_color->b = (unsigned char)B;

    if (ret == 1)
	fill_color->a = RGBA_COLOR_OPAQUE;
    else if (ret == 2)
	fill_color->a = RGBA_COLOR_NONE;
    else {
	G_warning(_("[%s]: No such color"), fill_color_str);
	return (-1);
    }

    Symb = S_read(symb_name);

    if (Symb == NULL) {
	G_warning(_("Cannot read symbol, cannot display points"));
	return (-1);
    }
    else
	S_stroke(Symb, size, rotation, 0);

    D_symbol(Symb, ix, iy, line_color, fill_color);

    /* restore previous d.graph draw color */
    if (last_color.a == RGBA_COLOR_OPAQUE)
	D_RGB_color(last_color.r, last_color.g, last_color.b);
    else if (last_color.a == RGBA_COLOR_NONE)
	D_use_color(D_parse_color(DEFAULT_BG_COLOR, 0));
    else			/* unset or bad */
	D_RGB_color(line_color->r, line_color->g, line_color->b);

    G_free(symb_name);
    G_free(line_color_str);
    G_free(fill_color_str);
    G_free(line_color);
    G_free(fill_color);

    return (0);
}

/* RGBA are 0-255; alpha is only used as an on/off switch. maybe test a<127<a ? */
void set_last_color(int R, int G, int B, int alpha)
{
    if (alpha == RGBA_COLOR_OPAQUE) {
	last_color.r = (unsigned char)R;
	last_color.g = (unsigned char)G;
	last_color.b = (unsigned char)B;
	last_color.a = RGBA_COLOR_OPAQUE;
    }
    else if (alpha == RGBA_COLOR_NONE) {
	last_color.a = RGBA_COLOR_NONE;
    }
    else
	last_color.a = RGBA_COLOR_NONE;
}
