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
		 const struct color_rgb *bcolor, const struct color_rgb *fcolor, int chcat, int id_flag, 
		 int table_colors_flag, int cats_color_flag, const char *rgb_column,
		 int default_width, const char *width_column, double width_scale,
		 int z_color_flag, const char *z_column,
		 dbCatValArray *cvarr_rgb, dbCatValArray *cvarr_width, int nrec_width)
{
    int num, area, isle, n_isles, n_points;
    double xl, yl;
    struct line_pnts *Points, * APoints, **IPoints;
    struct line_cats *Cats;
    int n_ipoints_alloc;
    int cat, centroid;
    int red, grn, blu;

    dbCatVal *cv_rgb, *cv_width;

    int i, rgb;	     
    char colorstring[12];	/* RRR:GGG:BBB */
    unsigned char which;
    int width;

    centroid = 0;
    cv_rgb = cv_width = NULL;
    rgb = 0;

    if (Vect_level(Map) < 2) {
	G_warning(_("Unable to display areas, topology not available. "
		    "Please try to rebuild topology using "
		    "v.build or v.build.all."));
	return 1;
    }
    
    G_debug(1, "display areas:");
    Points = Vect_new_line_struct();
    APoints = Vect_new_line_struct();
    n_ipoints_alloc = 10;
    IPoints = (struct line_pnts **)G_malloc(n_ipoints_alloc * sizeof(struct line_pnts *));
    for (i = 0; i < n_ipoints_alloc; i++) {
	IPoints[i] = Vect_new_line_struct();
    }
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
	    else { /* out of bounds for -180 to 180, try 0 to 360 as well */
		if (box.N < window->south || box.S > window->north)
		    continue;
		if (box.E + 360 < window->west || box.W + 360 > window->east)
		    continue;
	    }
	}

	/* check category: where_opt or cat_opt used */
	if (chcat) {		
	    if (id_flag) {
		if (!(Vect_cat_in_cat_list(area, Clist)))
		    continue;
	    }
	    else {
		int found;
		
		found = FALSE;
		centroid = Vect_get_area_centroid(Map, area);
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
	    }			/* end else */
	}			/* end if id_flag */
	else if (Clist->field > 0) {
	    int found;

	    found = FALSE;
	    centroid = Vect_get_area_centroid(Map, area);
	    G_debug(3, "centroid = %d", centroid);
	    if (centroid < 1)
		continue;
	    Vect_read_line(Map, NULL, Cats, centroid);

	    for (i = 0; i < Cats->n_cats; i++) {
		G_debug(3, "  centroid = %d, field = %d, cat = %d", centroid,
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

	G_debug(3, "display area %d", area);

	/* fill */
	Vect_get_area_points(Map, area, APoints);
	G_debug(3, "n_points = %d", APoints->n_points);
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

	if (!Vect_get_area_centroid(Map, area) && cat == -1) {
	    continue;
	}

	/* z height colors */
	if (z_color_flag && Vect_is_3d(Map)) {
	    struct bound_box box;
	    double zval;
	    struct Colors colors;

	    Vect_get_map_box(Map, &box);
	    zval = Points->z[0];
	    G_debug(3,
		    "display area %d, centroid %d, cat %d, x: %f, y: %f, z: %f",
		    area, centroid, cat, Points->x[0], Points->y[0],
		    Points->z[0]);
	    rgb = 1;
	    Rast_make_fp_colors(&colors, z_column, box.B, box.T);
	    Rast_get_color(&zval, &red, &grn, &blu, &colors, DCELL_TYPE);
	    G_debug(3, "b %d, g: %d, r %d", blu, grn, red);
	}

	if (table_colors_flag) {
	    centroid = Vect_get_area_centroid(Map, area);
	    if (cat >= 0) {
		G_debug(3, "display area %d, centroid %d, cat %d", area,
			centroid, cat);

		/* Read RGB colors from db for current area # */
		if (db_CatValArray_get_value(cvarr_rgb, cat, &cv_rgb) !=
		    DB_OK) {
		    rgb = 0;
		}
		else {
		    sprintf(colorstring, "%s", db_get_string(cv_rgb->val.s));

		    if (*colorstring != '\0') {

			G_debug(3, "area %d: colorstring: %s", area,
				colorstring);

			if (G_str_to_color(colorstring, &red, &grn, &blu) ==
			    1) {
			    rgb = 1;
			    G_debug(3, "area:%d  cat %d r:%d g:%d b:%d", area,
				    cat, red, grn, blu);
			}
			else {
			    rgb = 0;
			    G_warning(_("Error in color definition column (%s), area %d "
				       "with cat %d: colorstring [%s]"),
				      rgb_column, area, cat, colorstring);
			}
		    }
		    else {
			G_warning(_("Error in color definition column (%s), area %d with cat %d"),
				  rgb_column, area, cat);
			rgb = 0;
		    }
		}
	    }			/* end if cat */
	    else {
		rgb = 0;
	    }
	}			/* end if table_colors_flag */

	/* random colors */
	if (cats_color_flag) {
	    rgb = 0;
	    centroid = Vect_get_area_centroid(Map, area);
	    if (Clist->field > 0) {
		if (cat >= 0) {
		    G_debug(3, "display area %d, centroid %d, cat %d", area,
			    centroid, cat);
		    /* fetch color number from category */
		    which = (cat % palette_ncolors);
		    G_debug(3, "cat:%d which color:%d r:%d g:%d b:%d", cat,
			    which, palette[which].R, palette[which].G,
			    palette[which].B);
		    rgb = 1;
		    red = palette[which].R;
		    grn = palette[which].G;
		    blu = palette[which].B;
		}
	    }
	    else if (Cats->n_cats > 0) {
		/* fetch color number from layer */
		which = (Cats->field[0] % palette_ncolors);
		G_debug(3, "layer:%d which color:%d r:%d g:%d b:%d",
			Cats->field[0], which, palette[which].R,
			palette[which].G, palette[which].B);

		rgb = 1;
		red = palette[which].R;
		grn = palette[which].G;
		blu = palette[which].B;
	    }
	}

	if (nrec_width) {
	    centroid = Vect_get_area_centroid(Map, area);
	    if (cat >= 0) {
		G_debug(3, "display area %d, centroid %d, cat %d", area,
			centroid, cat);

		/* Read line width from db for current area # */
		if (db_CatValArray_get_value(cvarr_width, cat, &cv_width) !=
		    DB_OK) {
		    width = default_width;
		}
		else {
		    width =
			width_scale * (cvarr_width->ctype ==
				       DB_C_TYPE_INT ? cv_width->val.
				       i : (int)cv_width->val.d);
		    if (width < 0) {
			G_warning(_("Error in line width column (%s), element %d "
				   "with cat %d: line width [%d]"),
				  width_column, area, cat, width);
			width = default_width;
		    }
		}
	    }			/* end if cat */
	    else {
		width = default_width;
	    }

	    D_line_width(width);
	}			/* end if nrec_width */

	if (fcolor || (z_color_flag && Vect_is_3d(Map))) {
	    if (!table_colors_flag && !cats_color_flag && !z_color_flag) {
		D_RGB_color(fcolor->r, fcolor->g, fcolor->b);
		D_polygon_abs(Points->x, Points->y, Points->n_points);
	    }
	    else {
		if (rgb) {
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
	    int i;

	    if (rgb) {
		D_RGB_color((unsigned char)red, (unsigned char)grn,
			    (unsigned char)blu);
	    }
	    else {
		D_RGB_color(bcolor->r, bcolor->g, bcolor->b);
	    }
	    /*use different user defined render methods */
	    D_polyline_abs(APoints->x, APoints->y, APoints->n_points);
	    for (i = 0; i < n_isles; i++) {
		/*use different user defined render methods */
		D_polyline_abs(IPoints[i]->x, IPoints[i]->y, IPoints[i]->n_points);
	    }
	}
    }				/* end for */

    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(APoints);
    for (i = 0; i < n_ipoints_alloc; i++) {
	Vect_destroy_line_struct(IPoints[i]);
    }
    G_free(IPoints);
    Vect_destroy_cats_struct(Cats);

    return 0;
}
