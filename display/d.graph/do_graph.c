#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/symbol.h>
#include <grass/glocale.h>

#include "options.h"
#include "local_proto.h"

#define CHUNK	128

static int coors_allocated = 0;
static int *xarray;
static int *yarray;

static float xincr;
static float yincr;

static double rotation;		/* degrees counter-clockwise from east */

static RGBA_Color last_color;

int set_graph_stuff(void)
{
    xincr = (float)(r - l) / 100.;
    if (xincr < 0.0)
	xincr = -xincr;		/* mod: shapiro 13 jun 1991 */
    yincr = (float)(b - t) / 100.;
    if (yincr < 0.0)
	yincr = -yincr;		/* mod: shapiro 13 jun 1991 */

    rotation = 0.0;		/* init */

    return 0;
}

int set_text_size(void)
{
    if (hsize >= 0. && vsize >= 0. && hsize <= 100. && vsize <= 100.) {
	R_text_size((int)(hsize * xincr), (int)(vsize * yincr));
	G_debug(3, "text size initialized to [%d,%d] pixels",
		(int)(hsize * xincr), (int)(vsize * yincr));
    }
    return (0);
}

int do_draw(char *buff)
{
    float xper, yper;

    if (2 != sscanf(buff, "%*s %f %f", &xper, &yper)) {
	G_warning(_("Problem parsing coordinates [%s]"), buff);
	return (-1);
    }

    if (mapunits) {
	/* skip check: clips segments if map coordinate is out of region.
	   if( xper < D_get_u_west() ||
	   yper < D_get_u_south() ||
	   xper > D_get_u_east() ||
	   yper > D_get_u_north() )
	   return(-1);
	 */
	R_cont_abs((int)(D_u_to_d_col(xper) + 0.5),
		   (int)(D_u_to_d_row(yper) + 0.5));
    }
    else {
	if (xper < 0. || yper < 0. || xper > 100. || yper > 100.)
	    return (-1);
	R_cont_abs(l + (int)(xper * xincr), b - (int)(yper * yincr));
    }

    return (0);
}

int do_move(char *buff)
{
    float xper, yper;

    if (2 != sscanf(buff, "%*s %f %f", &xper, &yper)) {
	G_warning(_("Problem parsing coordinates [%s]"), buff);
	return (-1);
    }

    if (mapunits)
	R_move_abs((int)(D_u_to_d_col(xper) + 0.5),
		   (int)(D_u_to_d_row(yper) + 0.5));
    else {
	if (xper < 0. || yper < 0. || xper > 100. || yper > 100.)
	    return (-1);
	R_move_abs(l + (int)(xper * xincr), b - (int)(yper * yincr));
    }

    return (0);
}

int do_color(char *buff)
{
    char in_color[64];
    int R, G, B, color = 0;

    if (1 != sscanf(buff, "%*s %s", in_color)) {
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
	R_RGB_color(R, G, B);
	/* store for backup */
	set_last_color(R, G, B, RGBA_COLOR_OPAQUE);
    }
    if (color == 2) {		/* color == 'none' */
	R = D_translate_color(DEFAULT_BG_COLOR);
	R_standard_color(R);
	/* store for backup */
	set_last_color(0, 0, 0, RGBA_COLOR_NONE);
    }
    return (0);
}

int do_linewidth(char *buff)
{
    int width;			/* in pixels */

    if (1 != sscanf(buff, "%*s %d", &width)) {
	G_warning(_("Problem parsing command [%s]"), buff);
	return (-1);
    }

    D_line_width(width);
    G_debug(3, "line width set to %d pixels", width);

    return (0);
}


int do_poly(char *buff, FILE * infile)
{
    int num;
    char origcmd[64];
    float xper, yper;
    char *fgets();
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

	if (!mapunits) {
	    if (xper < 0. || yper < 0. || xper > 100. || yper > 100.)
		break;
	}
	check_alloc(num + 1);

	if (mapunits) {
	    xarray[num] = (int)(D_u_to_d_col(xper) + 0.5);
	    yarray[num] = (int)(D_u_to_d_row(yper) + 0.5);
	}
	else {
	    xarray[num] = l + (int)(xper * xincr);
	    yarray[num] = b - (int)(yper * yincr);
	}

	num++;
    }

    if (num) {
	/* this check is here so you can use the "polyline" command 
	   to make an unfilled polygon */
	if (!strcmp(origcmd, "polygon"))
	    R_polygon_abs(xarray, yarray, num);
	else
	    R_polyline_abs(xarray, yarray, num);
    }

    return (to_return);
}

int do_size(char *buff)
{
    float xper, yper;
    int ret;

    ret = sscanf(buff, "%*s %f %f", &xper, &yper);

    if (ret != 2 && ret != 1) {
	G_warning(_("Problem parsing command [%s]"), buff);
	return (-1);
    }

    /* if only one size is given assume same value in both axes */
    if (ret == 1)
	yper = xper;

    if (xper < 0. || yper < 0. || xper > 100. || yper > 100.)
	return (-1);

    R_text_size((int)(xper * xincr), (int)(yper * yincr));
    G_debug(3, "text size set to [%d,%d] pixels",
	    (int)(xper * xincr), (int)(yper * yincr));

    return (0);
}

int do_rotate(char *buff)
{
    if (1 != sscanf(buff, "%*s %lf", &rotation)) {
	G_warning(_("Problem parsing command [%s]"), buff);
	return (-1);
    }

    R_text_rotation((float)rotation);
    G_debug(3, "rotation set to %.1f degrees", rotation);

    return (0);
}

int do_text(char *buff)
{
    char *ptr;

    ptr = buff;
    /* skip to beginning of actual text */
    for (; *ptr != ' '; ptr++) ;
    for (; *ptr == ' '; ptr++) ;
    R_text(ptr);

    return 0;
}

int check_alloc(int num)
{
    int to_alloc;

    if (num < coors_allocated)
	return 0;

    to_alloc = coors_allocated;
    while (num >= to_alloc)
	to_alloc += CHUNK;

    if (coors_allocated == 0) {
	xarray = (int *)falloc(to_alloc, sizeof(int));
	yarray = (int *)falloc(to_alloc, sizeof(int));
    }
    else {
	xarray = (int *)frealloc((char *)xarray,
				 to_alloc, sizeof(int), coors_allocated);
	yarray = (int *)frealloc((char *)yarray,
				 to_alloc, sizeof(int), coors_allocated);
    }

    coors_allocated = to_alloc;

    return 0;
}

int do_icon(char *buff)
{
    double xper, yper;
    char type;
    int size;
    int ix, iy;

    if (4 != sscanf(buff, "%*s %c %d %lf %lf", &type, &size, &xper, &yper)) {
	G_warning(_("Problem parsing command [%s]"), buff);
	return (-1);
    }

    if (mapunits) {
	ix = (int)(D_u_to_d_col(xper) + 0.5);
	iy = (int)(D_u_to_d_row(yper) + 0.5);
	/* size in map units too? currently in percentage.
	   use "size * D_get_u_to_d_yconv()" to convert? */
    }
    else {
	if (xper < 0. || yper < 0. || xper > 100. || yper > 100.)
	    return (-1);

	ix = l + (int)(xper * xincr);
	iy = b - (int)(yper * yincr);
    }

    switch (type & 0177) {
    case 'o':
	R_move_abs(ix - size, iy - size);
	R_cont_abs(ix - size, iy + size);
	R_cont_abs(ix + size, iy + size);
	R_cont_abs(ix + size, iy - size);
	R_cont_abs(ix - size, iy - size);
	break;
    case 'x':
	R_move_abs(ix - size, iy - size);
	R_cont_abs(ix + size, iy + size);
	R_move_abs(ix - size, iy + size);
	R_cont_abs(ix + size, iy - size);
	break;
    case '+':
    default:
	R_move_abs(ix, iy - size);
	R_cont_abs(ix, iy + size);
	R_move_abs(ix - size, iy);
	R_cont_abs(ix + size, iy);
	break;
    }
    return (0);
}

int do_symbol(char *buff)
{
    double xper, yper;
    int size;
    int ix, iy;
    char *symb_name;
    SYMBOL *Symb;
    char *line_color_str, *fill_color_str;
    RGBA_Color *line_color, *fill_color;
    int R, G, B, ret;


    line_color = G_malloc(sizeof(RGBA_Color));
    fill_color = G_malloc(sizeof(RGBA_Color));

    symb_name = G_malloc(sizeof(char) * strlen(buff) + 1);	/* well, it won't be any bigger than this */
    line_color_str = G_malloc(sizeof(char) * strlen(buff) + 1);
    fill_color_str = G_malloc(sizeof(char) * strlen(buff) + 1);

    G_debug(3, "do_symbol() [%s]", buff);

    /* set default colors so colors are optional */
    strcpy(line_color_str, DEFAULT_FG_COLOR);
    strcpy(fill_color_str, "grey");

    if (sscanf
	(buff, "%*s %s %d %lf %lf %s %s", symb_name, &size, &xper, &yper,
	 line_color_str, fill_color_str) < 4) {
	G_warning(_("Problem parsing command [%s]"), buff);
	return (-1);
    }

    if (mapunits) {
	ix = (int)(D_u_to_d_col(xper) + 0.5);
	iy = (int)(D_u_to_d_row(yper) + 0.5);
	/* consider size in map units too? maybe as percentage of display?
	   perhaps use "size * D_get_u_to_d_yconv()" to convert */
    }
    else {
	if (xper < 0. || yper < 0. || xper > 100. || yper > 100.)
	    return (-1);
	ix = l + (int)(xper * xincr);
	iy = b - (int)(yper * yincr);
    }

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
	R_RGB_color(last_color.r, last_color.g, last_color.b);
    else if (last_color.a == RGBA_COLOR_NONE)
	D_raster_use_color(D_parse_color(DEFAULT_BG_COLOR, 0));
    else			/* unset or bad */
	R_RGB_color(line_color->r, line_color->g, line_color->b);

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
