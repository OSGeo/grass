/* Author: Moritz Lennert, based on d.vect from Radim Blazek
 *
 */
#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "plot.h"
#include "local_proto.h"

int dareatheme(struct Map_info *Map, struct cat_list *Clist,
	       dbCatValArray * cvarr, double *breaks, int nbreaks,
	       const struct color_rgb *colors, const struct color_rgb *bcolor,
	       int chcat, struct Cell_head *window, int default_width)
{

    int num, area, isle, n_isles, n_points;
    double xl, yl;
    struct line_pnts *Points, *IPoints;
    struct line_cats *Cats;
    int cat, centroid = 0;
    double breakval = 0.0;

    dbCatVal *cv = NULL;

    G_debug(1, "display areas:");
    Points = Vect_new_line_struct();
    IPoints = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    num = Vect_get_num_areas(Map);
    G_debug(2, "n_areas = %d", num);


    for (area = 1; area <= num; area++) {
	int i;
	struct bound_box box;

	G_debug(3, "area = %d", area);

	if (!Vect_area_alive(Map, area))
	    continue;

	/* Check box */
	Vect_get_area_box(Map, area, &box);
	if (box.N < window->south || box.S > window->north ||
	    box.E < window->west || box.W > window->east) {

	    if (window->proj != PROJECTION_LL)
		continue;
	    else {
		if (!G_window_overlap(window, box.N, box.S, box.E, box.W))
		    continue;
	    }
	}

	if (chcat) {		/* check category: where_opt used */
	    int found = 0;

	    centroid = Vect_get_area_centroid(Map, area);
	    G_debug(3, "centroid = %d", centroid);
	    if (centroid < 1)
		continue;
	    Vect_read_line(Map, Points, Cats, centroid);

	    for (i = 0; i < Cats->n_cats; i++) {
		G_debug(3, "  centroid = %d, field = %d, cat = %d", centroid,
			Cats->field[i], Cats->cat[i]);

		if (Cats->field[i] == Clist->field &&
		    Vect_cat_in_cat_list(Cats->cat[i], Clist)) {
		    found = 1;
		    break;
		}
	    }
	    if (!found)
		continue;
	}			/* end if chcat */
	else if (Clist->field > 0) {
	    int found = 0;

	    centroid = Vect_get_area_centroid(Map, area);
	    G_debug(3, "centroid = %d", centroid);
	    if (centroid < 1)
		continue;
	    Vect_read_line(Map, Points, Cats, centroid);

	    for (i = 0; i < Cats->n_cats; i++) {
		G_debug(3, "  centroid = %d, field = %d, cat = %d", centroid,
			Cats->field[i], Cats->cat[i]);
		if (Cats->field[i] == Clist->field) {
		    found = 1;
		    break;
		}
	    }
	    /* lines with no category will be displayed */
	    if (Cats->n_cats > 0 && !found)
		continue;
	}

	G_debug(3, "display area %d", area);

	/* fill */
	Vect_get_area_points(Map, area, Points);
	G_debug(3, "n_points = %d", Points->n_points);

	n_points = Points->n_points;
	xl = Points->x[n_points - 1];
	yl = Points->y[n_points - 1];
	n_isles = Vect_get_area_num_isles(Map, area);
	for (i = 0; i < n_isles; i++) {
	    isle = Vect_get_area_isle(Map, area, i);
	    Vect_get_isle_points(Map, isle, IPoints);
	    Vect_append_points(Points, IPoints, GV_FORWARD);
	    Vect_append_point(Points, xl, yl, 0.0);	/* ??? */
	}

	cat = Vect_get_area_cat(Map, area,
				(Clist->field > 0 ? Clist->field :
				 (Cats->n_cats > 0 ? Cats->field[0] : 1)));

	if (!Vect_get_area_centroid(Map, area) && cat == -1) {
	    continue;
	}

	centroid = Vect_get_area_centroid(Map, area);
	if (cat >= 0) {
	    G_debug(3, "display area %d, centroid %d, cat %d", area, centroid,
		    cat);

	    /* Get value of data for this area */
	    if (db_CatValArray_get_value(cvarr, cat, &cv) != DB_OK) {
		G_debug(3, "No value found for cat %i", cat);

	    }
	    else {
		breakval =
		    (cvarr->ctype == DB_C_TYPE_INT ? cv->val.i : cv->val.d);
	    }
	}

        /* find out into which class breakval falls */
        i = 0;
        while (breakval > breaks[i] && i < nbreaks)
        i++;



	/* plot polygon in class color */
	D_RGB_color(colors[i].r, colors[i].g, colors[i].b);
	D_polygon_abs(Points->x, Points->y, Points->n_points);

	/* XXX rewrite boundary */
	if (bcolor) {
	    int i;

	    Vect_get_area_points(Map, area, Points);
	    D_RGB_color(bcolor->r, bcolor->g, bcolor->b);
	    /*use different user defined render methods */
	    D_polyline_abs(Points->x, Points->y, Points->n_points);
	    for (i = 0; i < n_isles; i++) {
		isle = Vect_get_area_isle(Map, area, i);
		Vect_get_isle_points(Map, isle, Points);
		/*use different user defined render methods */
		D_polyline_abs(Points->x, Points->y, Points->n_points);
	    }
	}
    }				/* end for loop over areas */

    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(IPoints);
    Vect_destroy_cats_struct(Cats);

    return 0;
}
