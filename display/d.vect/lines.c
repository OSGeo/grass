/* plot1() - Level One vector reading */

#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/display.h>
#include "plot.h"
#include "local_proto.h"
#include <grass/symbol.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>

#define RENDER_POLYLINE 0
#define RENDER_POLYGON  1

int palette_ncolors = 16;

struct rgb_color palette[16] = {
    {198, 198, 198},		/*  1: light gray */
    {127, 127, 127},		/*  2: medium/dark gray */
    {255, 0, 0},		/*  3: bright red */
    {139, 0, 0},		/*  4: dark red */
    {0, 255, 0},		/*  5: bright green */
    {0, 139, 0},		/*  6: dark green */
    {0, 0, 255},		/*  7: bright blue */
    {0, 0, 139},		/*  8: dark blue   */
    {255, 255, 0},		/*  9: yellow */
    {139, 126, 10},		/* 10: olivey brown */
    {255, 165, 0},		/* 11: orange */
    {255, 192, 203},		/* 12: pink   */
    {255, 0, 255},		/* 13: magenta */
    {139, 0, 139},		/* 14: dark magenta */
    {0, 255, 255},		/* 15: cyan */
    {0, 139, 139}		/* 16: dark cyan */
};

int display_lines(struct Map_info *Map, int type, struct cat_list *Clist,
		  const struct color_rgb *color, const struct color_rgb *fcolor, int chcat,
		  const char *symbol_name,  double size, const char *size_column, int sqrt_flag, const char *rot_column,
		  int id_flag, int table_colors_flag, int cats_color_flag, const char *rgb_column,
		  int default_width, const char *width_column, double width_scale,
		  int z_color_flag, const char *z_style,
		  dbCatValArray *cvarr_rgb, dbCatValArray *cvarr_width, int nrec_width,
		  dbCatValArray *cvarr_size, int nrec_size,
		  dbCatValArray *cvarr_rot, int nrec_rot)

{
    int i, ltype, nlines, line, cat;
    double *x, *y;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double x0, y0;

    dbCatVal *cv_rgb, *cv_width, *cv_size, *cv_rot;
    int nerror_rgb;
    int n_points, n_lines, n_centroids, n_boundaries, n_faces;
    
    int custom_rgb;
    char colorstring[12];	/* RRR:GGG:BBB */
    int red, grn, blu;
    RGBA_Color *line_color, *fill_color, *primary_color;
    unsigned char which;
    int width;
    SYMBOL *Symb;
    double var_size, rotation;

    Symb = NULL;
    custom_rgb = FALSE;
    cv_rgb = cv_width = cv_size = cv_rot = NULL;
    cat = -1;
    nlines = 0;
    
    if (id_flag && Vect_level(Map) < 2) {
	G_warning(_("Unable to display lines by id, topology not available. "
		    "Please try to rebuild topology using "
		    "v.build or v.build.all."));
	return 1;
    }

    var_size = size;
    rotation = 0.0;
    nerror_rgb = 0;

    line_color = G_malloc(sizeof(RGBA_Color));
    fill_color = G_malloc(sizeof(RGBA_Color));
    primary_color = G_malloc(sizeof(RGBA_Color));
    primary_color->a = RGBA_COLOR_OPAQUE;

    /* change function prototype to pass RGBA_Color instead of color_rgb? */
    if (color) {
	line_color->r = color->r;
	line_color->g = color->g;
	line_color->b = color->b;
	line_color->a = RGBA_COLOR_OPAQUE;
    }
    else
	line_color->a = RGBA_COLOR_NONE;

    if (fcolor) {
	fill_color->r = fcolor->r;
	fill_color->g = fcolor->g;
	fill_color->b = fcolor->b;
	fill_color->a = RGBA_COLOR_OPAQUE;
    }
    else
	fill_color->a = RGBA_COLOR_NONE;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    if (!(nrec_size || nrec_rot) ) {
	Symb = S_read(symbol_name);
	if (!Symb)
	    G_warning(_("Unable to read symbol, unable to display points"));
	else
	    S_stroke(Symb, size, 0.0, 0);
    }
    
    Vect_rewind(Map);

    /* Is it necessary to reset line/label color in each loop ? */

    if (color && !table_colors_flag && !cats_color_flag)
	D_RGB_color(color->r, color->g, color->b);

    if (Vect_level(Map) >= 2)
	nlines = Vect_get_num_lines(Map);

    line = 0;
    n_points = n_lines = 0;
    n_centroids = n_boundaries = 0;
    n_faces = 0;
    while (1) {
	line++;
	if (Vect_level(Map) >= 2) {
	    if (line > nlines)
		break;
	    if (!Vect_line_alive(Map, line))
		continue;
	    ltype = Vect_read_line(Map, Points, Cats, line);
	}
	else {
	    ltype = Vect_read_next_line(Map, Points, Cats);
	    if (ltype == -1) {
		G_fatal_error(_("Unable to read vector map"));
	    }
	    else if (ltype == -2) { /* EOF */
		break;
	    }
	}

	if (!(type & ltype))
	    continue;

	if (Points->n_points == 0)
	    continue;

	if (chcat) {
	    int found = 0;

	    if (id_flag) {	/* use line id */
		if (!(Vect_cat_in_cat_list(line, Clist)))
		    continue;
	    }
	    else {
		for (i = 0; i < Cats->n_cats; i++) {
		    if (Cats->field[i] == Clist->field &&
			Vect_cat_in_cat_list(Cats->cat[i], Clist)) {
			found = 1;
			break;
		    }
		}
		if (!found)
		    continue;
	    }
	}
	else if (Clist->field > 0) {
	    int found = 0;

	    for (i = 0; i < Cats->n_cats; i++) {
		if (Cats->field[i] == Clist->field) {
		    found = 1;
		    break;
		}
	    }
	    /* lines with no category will be displayed */
	    if (Cats->n_cats > 0 && !found)
		continue;
	}

	/* z height colors */
	if (z_color_flag && Vect_is_3d(Map)) {
	    struct bound_box box;
	    double zval;
	    struct Colors colors;

	    Vect_get_map_box(Map, &box);
	    zval = Points->z[0];
	    G_debug(3, "display line %d, cat %d, x: %f, y: %f, z: %f", line,
		    cat, Points->x[0], Points->y[0], Points->z[0]);
	    custom_rgb = TRUE;
	    Rast_make_fp_colors(&colors, z_style, box.B, box.T);
	    Rast_get_color(&zval, &red, &grn, &blu, &colors, DCELL_TYPE);
	    G_debug(3, "b %d, g: %d, r %d", blu, grn, red);
	}

	if (table_colors_flag) {
	    /* only first category */
	    Vect_cat_get(Cats, 
			 (Clist->field > 0 ? Clist->field :
			  (Cats->n_cats >
			   0 ? Cats->field[0] : 1)), &cat);

	    if (cat >= 0) {
		G_debug(3, "display element %d, cat %d", line, cat);

		/* Read RGB colors from db for current area # */
		if (db_CatValArray_get_value(cvarr_rgb, cat, &cv_rgb) !=
		    DB_OK) {
		    custom_rgb = FALSE;
		}
		else {
		    sprintf(colorstring, "%s", db_get_string(cv_rgb->val.s));

		    if (*colorstring != '\0') {
			G_debug(3, "element %d: colorstring: %s", line,
				colorstring);

			if (G_str_to_color(colorstring, &red, &grn, &blu) ==
			    1) {
			    custom_rgb = TRUE;
			    G_debug(3, "element:%d  cat %d r:%d g:%d b:%d",
				    line, cat, red, grn, blu);
			}
			else {
			    custom_rgb = FALSE;
			    G_important_message(_("Error in color definition column '%s', feature id %d "
						  "with cat %d: colorstring '%s'"),
						rgb_column, line, cat, colorstring);
			    nerror_rgb++;
			}
		    }
		    else {
			custom_rgb = FALSE;
			G_important_message(_("Error in color definition column '%s', feature id %d "
					      "with cat %d"),
					    rgb_column, line, cat);
			nerror_rgb++;
		    }
		}
	    }			/* end if cat */
	    else {
		custom_rgb = FALSE;
	    }
	}			/* end if table_colors_flag */


	/* random colors */
	if (cats_color_flag) {
	    custom_rgb = FALSE;
	    if (Clist->field > 0) {
		Vect_cat_get(Cats, Clist->field, &cat);
		if (cat >= 0) {
		    G_debug(3, "display element %d, cat %d", line, cat);
		    /* fetch color number from category */
		    which = (cat % palette_ncolors);
		    G_debug(3, "cat:%d which color:%d r:%d g:%d b:%d", cat,
			    which, palette[which].R, palette[which].G,
			    palette[which].B);

		    custom_rgb = TRUE;
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

		custom_rgb = TRUE;
		red = palette[which].R;
		grn = palette[which].G;
		blu = palette[which].B;
	    }
	}


	if (nrec_width) {
	    /* only first category */
	    Vect_cat_get(Cats,
			 (Clist->field > 0 ? Clist->field :
			  (Cats->n_cats >
			   0 ? Cats->field[0] : 1)), &cat);

	    if (cat >= 0) {
		G_debug(3, "display element %d, cat %d", line, cat);

		/* Read line width from db for current area # */
		if (db_CatValArray_get_value(cvarr_width, cat, &cv_width) !=
		    DB_OK) {
		    width = default_width;
		}
		else {
		    width =
			width_scale * (cvarr_width->ctype ==
				       DB_C_TYPE_INT ? cv_width->val.i
				       : (int)cv_width->val.d);
		    if (width < 0) {
			G_warning(_("Error in line width column (%s), element %d "
				   "with cat %d: line width [%d]"),
				  width_column, line, cat, width);
			width = default_width;
		    }
		}
	    }		/* end if cat */
	    else {
		width = default_width;
	    }

	    D_line_width(width);
	}		/* end if nrec_width */


	/* enough of the prep work, lets start plotting stuff */
	x = Points->x;
	y = Points->y;

	if ((ltype & GV_POINTS) && (Symb != NULL || (nrec_size || nrec_rot)) ) {
	    if (!(color || fcolor || custom_rgb))
		continue;

	    x0 = x[0];
	    y0 = y[0];

	    /* skip if the point is outside of the display window */
	    /*      xy<0 tests make it go ever-so-slightly faster */
	    if (x0 > D_get_u_east() || x0 < D_get_u_west() ||
		y0 < D_get_u_south() || y0 > D_get_u_north())
		continue;

	    /* dynamic symbol size */
	    if (nrec_size) {
		/* only first category */
		Vect_cat_get(Cats,
			     (Clist->field > 0 ? Clist->field :
			      (Cats->n_cats > 0 ?
			       Cats->field[0] : 1)), &cat);

		if (cat >= 0) {
		    G_debug(3, "display element %d, cat %d", line, cat);

		    /* Read symbol size from db for current symbol # */
		    if (db_CatValArray_get_value(cvarr_size, cat, &cv_size) !=
			DB_OK) {
			var_size = size;
		    }
		    else {
			var_size = size *
			    (cvarr_size->ctype == DB_C_TYPE_INT ?
			     (double)cv_size->val.i : cv_size->val.d);

			if (var_size < 0.0) {
			    G_warning(_("Error in symbol size column (%s), element %d "
					"with cat %d: symbol size [%f]"),
				      size_column, line, cat, var_size);
			    var_size = size;
			}
		    }
		}		/* end if cat */
		else {
		    var_size = size;
		}
	    }		/* end if nrec_size */

            if (sqrt_flag)
                var_size = sqrt(var_size);

	    /* dynamic symbol rotation */
	    if (nrec_rot) {
		/* only first category */
		Vect_cat_get(Cats,
			     (Clist->field > 0 ? Clist->field :
			      (Cats->n_cats > 0 ?
			       Cats->field[0] : 1)), &cat);

		if (cat >= 0) {
		    G_debug(3, "display element %d, cat %d", line, cat);

		    /* Read symbol rotation from db for current symbol # */
		    if (db_CatValArray_get_value(cvarr_rot, cat, &cv_rot) !=
			DB_OK) {
			rotation = 0.0;
		    }
		    else {
			rotation =
			    (cvarr_rot->ctype == DB_C_TYPE_INT ?
			     (double)cv_rot->val.i : cv_rot->val.d);
		    }
		}		/* end if cat */
		else {
		    rotation = 0.0;
		}
	    }		/* end if nrec_rot */

	    if(nrec_size || nrec_rot) {
		G_debug(3, ". dynamic symbol: cat=%d  size=%.2f  rotation=%.2f",
			cat, var_size, rotation);

		/* symbol stroking is cumulative, so we need to reread it each time */
		if(Symb) /* unclean free() on first iteration if variables are not init'd to NULL? */
		    G_free(Symb);
		Symb = S_read(symbol_name);
		if (Symb == NULL)
		    G_warning(_("Unable to read symbol, unable to display points"));
		else
		    S_stroke(Symb, var_size, rotation, 0);
	    }

	    /* use random or RGB column color if given, otherwise reset */
	    /* centroids always use default color to stand out from underlying area */
	    if (custom_rgb && (ltype != GV_CENTROID)) {
		primary_color->r = (unsigned char)red;
		primary_color->g = (unsigned char)grn;
		primary_color->b = (unsigned char)blu;
		D_symbol2(Symb, x0, y0, primary_color, line_color);
	    }
	    else
		D_symbol(Symb, x0, y0, line_color, fill_color);

	    /* reset to defaults */
	    var_size = size;
	    rotation = 0.0;
	}
	else if (color || custom_rgb || (z_color_flag && Vect_is_3d(Map))) {
	    if (!table_colors_flag && !cats_color_flag && !z_color_flag)
		D_RGB_color(color->r, color->g, color->b);
	    else {
		if (custom_rgb)
		    D_RGB_color((unsigned char)red, (unsigned char)grn,
				(unsigned char)blu);
		else
		    D_RGB_color(color->r, color->g, color->b);
	    }

	    /* Plot the lines */
	    if (Points->n_points == 1)	/* line with one coor */
		D_polydots_abs(x, y, Points->n_points);
	    else		/*use different user defined render methods */
		D_polyline_abs(x, y, Points->n_points);
	}
	
	switch (ltype) {
	case GV_POINT:
	    n_points++;
	    break;
	case GV_LINE:
	    n_lines++;
	    break;
	case GV_CENTROID:
	    n_centroids++;
	    break;
	case GV_BOUNDARY:
	    n_boundaries++;
	    break;
	case GV_FACE:
	    n_faces++;
	    break;
	default:
	    break;
	}
    }
    
    if (nerror_rgb > 0) {
	G_warning(_("Error in color definition column '%s': %d features affected"),
		  rgb_column, nerror_rgb);
    }
    
    if (n_points > 0) 
	G_verbose_message(_("%d points plotted"), n_points);
    if (n_lines > 0) 
	G_verbose_message(_("%d lines plotted"), n_lines);
    if (n_centroids > 0) 
	G_verbose_message(_("%d centroids plotted"), n_centroids);
    if (n_boundaries > 0) 
	G_verbose_message(_("%d boundaries plotted"), n_boundaries);
    if (n_faces > 0) 
	G_verbose_message(_("%d faces plotted"), n_faces);
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    G_free(line_color);
    G_free(fill_color);
    G_free(primary_color);

    return 0;			/* not reached */
}
