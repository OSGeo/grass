/* Functions: ps_outline, outlinefile
 **
 ** Author: Paul W. Carlson     May 1992
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "clr.h"
#include "local_proto.h"

static int k, col, row, top, bottom;
static void *tl, *tr, *bl, *br;
static void *buffer[2];
static int scan_length;
static int draw_boundaries(void);
static int read_next(void);
double e1, e2, e3, n1, n2, n3;

/*
   e1 e2 e3
   *--*--* n1
   |  |  |
   *--*--* n2
   |  |  |
   *--*--* n3
 */
/* the ps_outline function creates a vector map called "tmp.outl" in
 ** the current location.  This file is removed after it has been
 ** plotted.
 */

extern RASTER_MAP_TYPE o_open_file();
static RASTER_MAP_TYPE map_type;

int ps_outline(void)
{
    /* let user know what's happenning */
    G_message(_("Outlining areas in raster map <%s in %s> ..."),
	      PS.cell_name, PS.cell_mapset);

    /* set the outline color and width */
    set_ps_color(&PS.outline_color);
    set_line_width(PS.outline_width);

    /* create temporary vector map containing outlines */
    o_io_init();
    map_type = o_open_file(PS.cell_name);
    draw_outline();
    o_close_file();

    return 0;
}


/* The outlinefile function is just slightly modified p.map code. */
#define KEY(x) (strcmp(key,x)==0)
static char *help[] = {
    "color  color",
    "width  #",
    ""
};
int read_outline(void)
{
    char buf[1024];
    char ch, *key, *data;
    PSCOLOR color;
    int ret, r, g, b;

    PS.outline_width = 1.;
    set_color(&color, 0, 0, 0);

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1)
		set_color(&color, r, g, b);
	    else if (ret == 2)  /* i.e. "none" */
		/* unset_color(&color); */
		error(key, data, _("Unsupported color request"));
	    else
		error(key, data, _("illegal color request"));

	    continue;
	}

	if (KEY("width")) {
	    PS.outline_width = -1.;
	    ch = ' ';
	    if (sscanf(data, "%lf%c", &(PS.outline_width), &ch) < 1
		|| PS.outline_width < 0.) {
		PS.outline_width = 1.;
		error(key, data, _("illegal width request"));
	    }
	    if (ch == 'i')
		PS.outline_width = PS.outline_width * 72.;
	    continue;
	}

	error(key, data, _("illegal outline sub-request"));
    }

    PS.outline_color = color;
    PS.do_outline = 1;

    return 0;
}


/* draw_outline - draw boundaries of polygons in file */

int draw_outline(void)
{
    int raster_size;

    row = col = top = 0;	/* get started for read of first */
    bottom = 1;			/*   line from raster map */
    scan_length = read_next();
    k = 0;
    raster_size = Rast_cell_size(map_type);
    while (read_next()) {	/* read rest of file, one row at *//*   a time */
	n1 = Rast_row_to_northing((double)row - 1., &(PS.w));
	n2 = Rast_row_to_northing((double)row, &(PS.w));
	n3 = Rast_row_to_northing((double)row + 1., &(PS.w));

	for (col = 0; col < scan_length - 1; col++) {
	    e1 = Rast_col_to_easting((double)col - 1., &(PS.w));
	    e2 = Rast_col_to_easting((double)col, &(PS.w));
	    e3 = Rast_col_to_easting((double)col + 1., &(PS.w));
	    tl = G_incr_void_ptr(buffer[top], col * raster_size);
	    /* top left in window */
	    tr = G_incr_void_ptr(buffer[top], (col + 1) * raster_size);
	    /* top right in window */
	    bl = G_incr_void_ptr(buffer[bottom], col * raster_size);
	    /* bottom left in window */
	    br = G_incr_void_ptr(buffer[bottom], (col + 1) * raster_size);
	    /* bottom right in window */
	    draw_boundaries();
	    if (k == 3)
		k = 0;
	}
	row++;
    }

    return 0;
}				/* draw_outlines */


static int draw_boundaries(void)
{
    if (Rast_raster_cmp(bl, br, map_type) != 0)
	draw_bot();
    if (Rast_raster_cmp(tr, br, map_type) != 0)
	draw_rite();

    return 0;
}				/* draw_boundaries */

/* read_next - read another line from input file */

static int read_next(void)
{
    int n;

    top = bottom++;		/* switch top and bottom, */
    bottom = 1 & bottom;	/*   which are always 0 or 1 */
    n = o_read_row(buffer[bottom]);
    return (n);
}

/* alloc_bufs - allocate buffers we will need for storing raster map */
/* data, pointers to extracted lines, area number information */

int o_alloc_bufs(int ncols, int size)
{
    buffer[0] = (void *)G_calloc(ncols, size);
    buffer[1] = (void *)G_calloc(ncols, size);

    return 0;
}

int draw_top(void)
/*    *--*--*    */
/*    |  |  |    */
/*    *  |  *    */
/*    |     |    */
/*    *--*--*    */
{
    start_line(e2, n2);
    sec_draw = 0;
    G_plot_line(e2, n2, e2, n1);
    if (++k == 3)
	fprintf(PS.fp, " D\n");
    else
	fprintf(PS.fp, " D ");

    return 0;
}

int draw_rite(void)
/*    *--*--*    */
/*    |     |    */
/*    *  ---*    */
/*    |     |    */
/*    *--*--*    */
{
    start_line(e2, n2);
    sec_draw = 0;
    G_plot_line(e2, n2, e3, n2);
    if (++k == 3)
	fprintf(PS.fp, " D\n");
    else
	fprintf(PS.fp, " D ");

    return 0;
}

int draw_left(void)
/*    *--*--*    */
/*    |     |    */
/*    *---  *    */
/*    |     |    */
/*    *--*--*    */
{
    start_line(e2, n2);
    sec_draw = 0;
    G_plot_line(e2, n2, e1, n2);
    if (++k == 3)
	fprintf(PS.fp, " D\n");
    else
	fprintf(PS.fp, " D ");

    return 0;
}

int draw_bot(void)
/*    *--*--*    */
/*    |     |    */
/*    *  |  *    */
/*    |  |  |    */
/*    *--*--*    */
{
    start_line(e2, n2);
    sec_draw = 0;
    G_plot_line(e2, n2, e2, n3);
    if (++k == 3)
	fprintf(PS.fp, " D\n");
    else
	fprintf(PS.fp, " D ");

    return 0;
}
