#include <unistd.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include <grass/glocale.h>
#include "vectpoints.h"
#include "globals.h"


#define SYM_SIZE 5
#define SYM_NAME "basic/cross1"


int plot(char *name, char *mapset, struct line_pnts *Points)
{
    int line, nlines, ltype;
    struct Cell_head window;
    struct Map_info P_map;
    SYMBOL *Symb;
    RGBA_Color *linecolor_rgb, *fillcolor_rgb;
    struct color_rgb rgb;
    int ix, iy;

    Vect_set_open_level(2);
    Vect_set_fatal_error(GV_FATAL_RETURN);

    if (2 > Vect_open_old(&P_map, name, mapset)) {
	return -1;
    }

    G_get_set_window(&window);

    nlines = Vect_get_num_lines(&P_map);

    /* set line and fill color for vector point symbols */
    linecolor_rgb = G_malloc(sizeof(RGB_Color));
    fillcolor_rgb = G_malloc(sizeof(RGB_Color));

    rgb = G_standard_color_rgb(line_color);
    linecolor_rgb->r = rgb.r;
    linecolor_rgb->g = rgb.g;
    linecolor_rgb->b = rgb.b;
    linecolor_rgb->a = RGBA_COLOR_OPAQUE;
    fillcolor_rgb->a = RGBA_COLOR_NONE;


    for (line = 1; line <= nlines; line++) {
	ltype = Vect_read_line(&P_map, Points, NULL, line);

	if (ltype & GV_POINT) {	/* GV_ singular: don't plot centroids, Points only */

	    Symb = S_read(SYM_NAME);

	    if (Symb == NULL) {
		G_warning(_("Cannot read symbol, cannot display points"));
		return (-1);
	    }
	    else
		S_stroke(Symb, SYM_SIZE, 0, 0);

	    ix = (int)(D_u_to_d_col(Points->x[0]) + 0.5);
	    iy = (int)(D_u_to_d_row(Points->y[0]) + 0.5);

	    D_symbol(Symb, ix, iy, linecolor_rgb, fillcolor_rgb);
	}


	if (ltype & GV_LINES) {	/* GV_ plural: both lines and boundaries */
	    D_polyline(Points->x, Points->y, Points->n_points);
	}
    }

    Vect_close(&P_map);
    G_free(linecolor_rgb);
    G_free(fillcolor_rgb);

    return 0;
}

/* plot inverse coordinate transformation */
int plot_warp(char *name, char *mapset, struct line_pnts *Points,
	      double E[], double N[], int trans_order)
{
    double *x, *y;
    int i, np;
    int line, nlines, ltype;
    struct Cell_head window;
    struct Map_info P_map;
    SYMBOL *Symb;
    RGBA_Color *linecolor_rgb, *fillcolor_rgb;
    struct color_rgb rgb;
    int ix, iy;

    Vect_set_open_level(2);
    Vect_set_fatal_error(GV_FATAL_RETURN);

    if (2 > Vect_open_old(&P_map, name, mapset)) {
	return -1;
    }

    G_get_set_window(&window);

    nlines = Vect_get_num_lines(&P_map);

    /* set line and fill color for vector point symbols */
    linecolor_rgb = G_malloc(sizeof(RGB_Color));
    fillcolor_rgb = G_malloc(sizeof(RGB_Color));

    rgb = G_standard_color_rgb(line_color);
    linecolor_rgb->r = rgb.r;
    linecolor_rgb->g = rgb.g;
    linecolor_rgb->b = rgb.b;
    linecolor_rgb->a = RGBA_COLOR_OPAQUE;
    fillcolor_rgb->a = RGBA_COLOR_NONE;


    for (line = 1; line <= nlines; line++) {
	ltype = Vect_read_line(&P_map, Points, NULL, line);

	if (ltype & GV_POINT) {	/* GV_ singular: don't plot centroids, Points only */

	    Symb = S_read(SYM_NAME);

	    if (Symb == NULL) {
		G_warning(_("Cannot read symbol, cannot display points"));
		return (-1);
	    }
	    else
		S_stroke(Symb, SYM_SIZE, 0, 0);

	    x = Points->x;
	    y = Points->y;

	    CRS_georef(x[0], y[0], &x[0], &y[0], E, N, trans_order);

	    ix = (int)(D_u_to_d_col(x[0]) + 0.5);
	    iy = (int)(D_u_to_d_row(y[0]) + 0.5);

	    D_symbol(Symb, ix, iy, linecolor_rgb, fillcolor_rgb);
	}

	if (ltype & GV_LINES) {	/* GV_ plural: both lines and boundaries */

	    np = Points->n_points;
	    x = Points->x;
	    y = Points->y;

	    for (i = 1; i < np; i++)
		CRS_georef(x[i], y[i], &x[i], &y[i], E, N, trans_order);

	    D_polyline(x, y, np);
	}
    }

    Vect_close(&P_map);
    G_free(linecolor_rgb);
    G_free(fillcolor_rgb);

    return 0;
}
