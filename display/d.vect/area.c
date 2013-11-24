/* Author: Radim Blazek
 *
 * added color support: Markus Neteler, Martin Landa
 */
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/colors.h>

#include <grass/glocale.h>
#include "plot.h"
#include "local_proto.h"

int display_area(struct Map_info *Map, struct cat_list *Clist, const struct Cell_head *window,
		 const struct color_rgb *bcolor, const struct color_rgb *fcolor, int chcat,
		 int id_flag, int cats_color_flag, 
		 int default_width, double width_scale,
		 struct Colors *zcolors,
		 dbCatValArray *cvarr_rgb, struct Colors *colors,
		 dbCatValArray *cvarr_width, int nrec_width)
{
    int num, area, isle, n_isles, n_points;
    double xl, yl;
    struct line_pnts *Points, * APoints, **IPoints;
    struct line_cats *Cats;
    int n_ipoints_alloc;
    int cat, centroid;
    int red, grn, blu;

    int i, custom_rgb, found;
    int width;
    struct bound_box box;
    
    if (Vect_level(Map) < 2) {
	G_warning(_("Unable to display areas, topology not available. "
		    "Please try to rebuild topology using "
		    "v.build or v.build.all."));
	return 1;
    }

    G_debug(1, "display areas:");
    
    centroid = 0;
    Points = Vect_new_line_struct();
    APoints = Vect_new_line_struct();
    n_ipoints_alloc = 10;
    IPoints = (struct line_pnts **)G_malloc(n_ipoints_alloc * sizeof(struct line_pnts *));
    for (i = 0; i < n_ipoints_alloc; i++) {
	IPoints[i] = Vect_new_line_struct();
    }
    Cats = Vect_new_cats_struct();
    
    num = Vect_get_num_areas(Map);
    G_debug(2, "\tn_areas = %d", num);

    for (area = 1; area <= num; area++) {
	G_debug(3, "\tarea = %d", area);

	if (!Vect_area_alive(Map, area))
	    continue;

	centroid = Vect_get_area_centroid(Map, area);
	if (!centroid) {
	    continue;
	}

	/* Check box */
	Vect_get_area_box(Map, area, &box);
	if (box.N < window->south || box.S > window->north ||
	    box.E < window->west || box.W > window->east) {
	    if (window->proj != PROJECTION_LL)
		continue;
	    else { /* out of bounds for -180 to 180, try 0 to 360 as well */
		if (box.N < window->south || box.S > window->north)
		    continue;
		if (box.E + 360 < window->west || box.W + 360 > window->east)
		    continue;
	    }
	}

	custom_rgb = FALSE;
		
	found = FALSE;
	if (chcat) {		
	    if (id_flag) {
		if (!(Vect_cat_in_cat_list(area, Clist)))
		    continue;
	    }
	    else {
		G_debug(3, "centroid = %d", centroid);
		if (centroid < 1)
		    continue;
		Vect_read_line(Map, Points, Cats, centroid);

		for (i = 0; i < Cats->n_cats; i++) {
		    G_debug(3, "  centroid = %d, field = %d, cat = %d",
			    centroid, Cats->field[i], Cats->cat[i]);

		    if (Cats->field[i] == Clist->field &&
			Vect_cat_in_cat_list(Cats->cat[i], Clist)) {
			found = TRUE;
			break;
		    }
		}
		
		if (!found)
		    continue;
	    }
	}
	else if (Clist->field > 0) {
	    found = FALSE;
	    G_debug(3, "\tcentroid = %d", centroid);
	    if (centroid < 1)
		continue;
	    Vect_read_line(Map, NULL, Cats, centroid);

	    for (i = 0; i < Cats->n_cats; i++) {
		G_debug(3, "\tcentroid = %d, field = %d, cat = %d", centroid,
			Cats->field[i], Cats->cat[i]);
		if (Cats->field[i] == Clist->field) {
		    found = TRUE;
		    break;
		}
	    }
	    
	    /* lines with no category will be displayed */
	    if (Cats->n_cats > 0 && !found)
		continue;
	}

	/* fill */
	Vect_get_area_points(Map, area, APoints);
	G_debug(3, "\tn_points = %d", APoints->n_points);
	if (APoints->n_points < 3) {
	    G_warning(_("Invalid area %d skipped (not enough points)"), area);
	    continue;
	}
	Vect_reset_line(Points);
	Vect_append_points(Points, APoints, GV_FORWARD);

	n_points = Points->n_points;
	xl = Points->x[n_points - 1];
	yl = Points->y[n_points - 1];
	n_isles = Vect_get_area_num_isles(Map, area);
	if (n_isles >= n_ipoints_alloc) {
	    IPoints = (struct line_pnts **)G_realloc(IPoints, (n_isles + 10) * sizeof(struct line_pnts *));
	    for (i = n_ipoints_alloc; i < n_isles + 10; i++) {
		IPoints[i] = Vect_new_line_struct();
	    }
	    n_ipoints_alloc = n_isles + 10;
	}
	for (i = 0; i < n_isles; i++) {
	    isle = Vect_get_area_isle(Map, area, i);
	    Vect_get_isle_points(Map, isle, IPoints[i]);
	    Vect_append_points(Points, IPoints[i], GV_FORWARD);
	    Vect_append_point(Points, xl, yl, 0.0);	/* ??? */
	}

	cat = Vect_get_area_cat(Map, area,
				(Clist->field > 0 ? Clist->field :
				 (Cats->n_cats > 0 ? Cats->field[0] : 1)));

	if (!centroid && cat == -1) {
	    continue;
	}

	/* z height colors */
	if (zcolors) {
	    if (Rast_get_d_color(&Points->z[0], &red, &grn, &blu, zcolors) == 1)
		custom_rgb = TRUE;
	    else
		custom_rgb = FALSE;
	}

        /* custom colors */
	if (colors || cvarr_rgb) {
	    custom_rgb = get_table_color(cat, area, colors, cvarr_rgb,
					 &red, &grn, &blu);
	}
	
	/* random colors */
	if (cats_color_flag) {
	    custom_rgb = get_cat_color(area, Cats, Clist,
				       &red, &grn, &blu);
	}
	
	/* line width */
	if (nrec_width) {
	    width = (int) get_property(cat, area, cvarr_width,
				       (double) width_scale,
				       (double) default_width);
	    
	    D_line_width(width);
	}
	
	if (fcolor || zcolors) {
	    if (!cvarr_rgb && !cats_color_flag && !zcolors && !colors) {
		D_RGB_color(fcolor->r, fcolor->g, fcolor->b);
		D_polygon_abs(Points->x, Points->y, Points->n_points);
	    }
	    else {
		if (custom_rgb) {
		    D_RGB_color((unsigned char)red, (unsigned char)grn,
				(unsigned char)blu);
		}
		else {
		    D_RGB_color(fcolor->r, fcolor->g, fcolor->b);
		}
		if (cat >= 0) {
		    D_polygon_abs(Points->x, Points->y, Points->n_points);
		}
	    }
	}

	/* boundary */
	if (bcolor) {
	    if (custom_rgb) {
		D_RGB_color((unsigned char)red, (unsigned char)grn,
			    (unsigned char)blu);
	    }
	    else {
		D_RGB_color(bcolor->r, bcolor->g, bcolor->b);
	    }
	    /* use different user defined render methods */
	    D_polyline_abs(APoints->x, APoints->y, APoints->n_points);
	    for (i = 0; i < n_isles; i++) {
		/* use different user defined render methods */
		D_polyline_abs(IPoints[i]->x, IPoints[i]->y, IPoints[i]->n_points);
	    }
	}
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(APoints);
    for (i = 0; i < n_ipoints_alloc; i++) {
	Vect_destroy_line_struct(IPoints[i]);
    }
    G_free(IPoints);
    Vect_destroy_cats_struct(Cats);

    return 0;
}
