/*      flash.c
   - to draw areas and lines in toggled color just to be redrawn 
   in the calling routine back to the previous color 
   making quick 'flash' --alex, nov/02
 */
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/Vect.h>
#include "what.h"

void
flash_area(struct Map_info *Map, plus_t area, struct line_pnts *Points,
	   int flash_colr)
{
    struct line_pnts *Points_i;

    int i, j, isle, n_isles, n_points;
    double xl, yl;

    Points_i = Vect_new_line_struct();

    /* fill */
    Vect_get_area_points(Map, area, Points);

    n_points = Points->n_points;
    xl = Points->x[n_points - 1];
    yl = Points->y[n_points - 1];
    n_isles = Vect_get_area_num_isles(Map, area);
    for (i = 0; i < n_isles; i++) {
	isle = Vect_get_area_isle(Map, area, i);
	Vect_get_isle_points(Map, isle, Points_i);
	Vect_append_points(Points, Points_i, GV_FORWARD);
	Vect_append_point(Points, xl, yl, 0);	/* ??? */
    }

    R_standard_color(flash_colr);
    G_plot_polygon(Points->x, Points->y, Points->n_points);

    /* boundary */
    Vect_get_area_points(Map, area, Points);

    for (i = 0; i < Points->n_points - 1; i++) {
	G_plot_line(Points->x[i], Points->y[i], Points->x[i + 1],
		    Points->y[i + 1]);
    }
    for (i = 0; i < n_isles; i++) {
	isle = Vect_get_area_isle(Map, area, i);
	Vect_get_isle_points(Map, isle, Points);
	for (j = 0; j < Points->n_points - 1; j++) {
	    G_plot_line(Points->x[j], Points->y[j], Points->x[j + 1],
			Points->y[j + 1]);
	}
    }

    Vect_destroy_line_struct(Points_i);
    R_flush();


}

void
flash_line(struct Map_info *Map, plus_t line, struct line_pnts *Points,
	   int flash_colr)
{
    double *x, *y;
    int j, np;

    np = Points->n_points;
    x = Points->x;
    y = Points->y;

    R_standard_color(flash_colr);

    for (j = 1; j < np; j++) {

	G_plot_line(x[0], y[0], x[1], y[1]);
	x++;
	y++;
    }
    R_flush();
}
