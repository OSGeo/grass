/* plot1() - Level One vector reading */

#include <string.h>
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

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int plot1(struct Map_info *Map, int type, int area, struct cat_list *Clist,
	  const struct color_rgb *color, const struct color_rgb *fcolor,
	  int chcat, SYMBOL * Symb, int size, int id_flag,
	  int table_colors_flag, int cats_color_flag, char *rgb_column,
	  int default_width, char *width_column, double width_scale)
{
    int i, ltype, nlines = 0, line, cat = -1;
    double *x, *y;
    struct line_pnts *Points, *PPoints;
    struct line_cats *Cats;
    double msize;
    int x0, y0;

    struct field_info *fi = NULL;
    dbDriver *driver = NULL;
    dbCatValArray cvarr_rgb, cvarr_width;
    dbCatVal *cv_rgb = NULL, *cv_width = NULL;
    int nrec_rgb = 0, nrec_width = 0;

    int open_db;
    int custom_rgb = FALSE;
    char colorstring[12];	/* RRR:GGG:BBB */
    int red, grn, blu;
    RGBA_Color *line_color, *fill_color, *primary_color;
    unsigned char which;
    int width;

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


    msize = size * (D_d_to_u_col(2.0) - D_d_to_u_col(1.0));	/* do it better */

    Points = Vect_new_line_struct();
    PPoints = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    open_db = table_colors_flag || width_column;

    if (open_db) {
	fi = Vect_get_field(Map, (Clist->field > 0 ? Clist->field : 1));
	if (fi == NULL) {
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  (Clist->field > 0 ? Clist->field : 1));
	}

	driver = db_start_driver_open_database(fi->driver, fi->database);
	if (driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  fi->database, fi->driver);
         db_set_error_handler_driver(driver);
    }

    if (table_colors_flag) {
	/* for reading RRR:GGG:BBB color strings from table */

	if (rgb_column == NULL || *rgb_column == '\0')
	    G_fatal_error(_("Color definition column not specified"));

	db_CatValArray_init(&cvarr_rgb);

	nrec_rgb = db_select_CatValArray(driver, fi->table, fi->key,
					 rgb_column, NULL, &cvarr_rgb);

	G_debug(3, "nrec_rgb (%s) = %d", rgb_column, nrec_rgb);

	if (cvarr_rgb.ctype != DB_C_TYPE_STRING)
	    G_fatal_error(_("Color definition column (%s) not a string. "
			    "Column must be of form RRR:GGG:BBB where RGB values range 0-255."),
			  rgb_column);

	if (nrec_rgb < 0)
	    G_fatal_error(_("Cannot select data (%s) from table"),
			  rgb_column);

	G_debug(2, "\n%d records selected from table", nrec_rgb);

	for (i = 0; i < cvarr_rgb.n_values; i++) {
	    G_debug(4, "cat = %d  %s = %s", cvarr_rgb.value[i].cat,
		    rgb_column, db_get_string(cvarr_rgb.value[i].val.s));
	}
    }

    if (width_column) {
	if (*width_column == '\0')
	    G_fatal_error(_("Line width column not specified."));

	db_CatValArray_init(&cvarr_width);

	nrec_width = db_select_CatValArray(driver, fi->table, fi->key,
					   width_column, NULL, &cvarr_width);

	G_debug(3, "nrec_width (%s) = %d", width_column, nrec_width);

	if (cvarr_width.ctype != DB_C_TYPE_INT &&
	    cvarr_width.ctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Line width column (%s) not a number."),
			  width_column);

	if (nrec_width < 0)
	    G_fatal_error(_("Cannot select data (%s) from table"),
			  width_column);

	G_debug(2, "\n%d records selected from table", nrec_width);

	for (i = 0; i < cvarr_width.n_values; i++) {
	    G_debug(4, "cat = %d  %s = %d", cvarr_width.value[i].cat,
		    width_column,
		    (cvarr_width.ctype ==
		     DB_C_TYPE_INT ? cvarr_width.value[i].val.
		     i : (int)cvarr_width.value[i].val.d));
	}
    }

    if (open_db)
	db_close_database_shutdown_driver(driver);

    Vect_rewind(Map);

    /* Is it necessary to reset line/label color in each loop ? */

    if (color && !table_colors_flag && !cats_color_flag)
	D_RGB_color(color->r, color->g, color->b);

    if (Vect_level(Map) >= 2)
	nlines = Vect_get_num_lines(Map);

    line = 0;
    while (1) {
	if (Vect_level(Map) >= 2) {
	    line++;
	    if (line > nlines)
		return 0;
	    if (!Vect_line_alive(Map, line))
		continue;
	    ltype = Vect_read_line(Map, Points, Cats, line);
	}
	else {
	    ltype = Vect_read_next_line(Map, Points, Cats);
	    switch (ltype) {
	    case -1:
		fprintf(stderr, _("\nERROR: vector map - can't read\n"));
		return -1;
	    case -2:		/* EOF */
		return 0;
	    }
	}

	if (!(type & ltype))
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


	if (table_colors_flag) {

	    /* only first category */
	    cat = Vect_get_line_cat(Map, line,
				    (Clist->field > 0 ? Clist->field :
				     (Cats->n_cats >
				      0 ? Cats->field[0] : 1)));

	    if (cat >= 0) {
		G_debug(3, "display element %d, cat %d", line, cat);

		/* Read RGB colors from db for current area # */
		if (db_CatValArray_get_value(&cvarr_rgb, cat, &cv_rgb) !=
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
			    G_warning(_("Error in color definition column (%s), element %d "
				       "with cat %d: colorstring [%s]"),
				      rgb_column, line, cat, colorstring);
			}
		    }
		    else {
			custom_rgb = FALSE;
			G_warning(_("Error in color definition column (%s), element %d with cat %d"),
				  rgb_column, line, cat);
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
		cat = Vect_get_line_cat(Map, line, Clist->field);
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
	    cat = Vect_get_line_cat(Map, line,
				    (Clist->field > 0 ? Clist->field :
				     (Cats->n_cats >
				      0 ? Cats->field[0] : 1)));

	    if (cat >= 0) {
		G_debug(3, "display element %d, cat %d", line, cat);

		/* Read line width from db for current area # */

		if (db_CatValArray_get_value(&cvarr_width, cat, &cv_width) !=
		    DB_OK) {
		    width = default_width;
		}
		else {
		    width =
			width_scale * (cvarr_width.ctype ==
				       DB_C_TYPE_INT ? cv_width->val.
				       i : (int)cv_width->val.d);
		    if (width < 0) {
			G_warning(_("Error in line width column (%s), element %d "
				   "with cat %d: line width [%d]"),
				  width_column, line, cat, width);
			width = default_width;
		    }
		}
	    }			/* end if cat */
	    else {
		width = default_width;
	    }

	    D_line_width(width);
	}			/* end if nrec_width */


	/* enough of the prep work, lets start plotting stuff */
	x = Points->x;
	y = Points->y;

	if ((ltype & GV_POINTS) && Symb != NULL) {
	    if (!(color || fcolor || custom_rgb))
		continue;

	    x0 = D_u_to_d_col(x[0]);
	    y0 = D_u_to_d_row(y[0]);

	    /* skip if the point is outside of the display window */
	    /*      xy<0 tests make it go ever-so-slightly faster */
	    if (x0 < 0 || y0 < 0 ||
		x0 > D_get_d_east() || x0 < D_get_d_west() ||
		y0 > D_get_d_south() || y0 < D_get_d_north())
		continue;

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


	}
	else if (color || custom_rgb) {
	    if (!table_colors_flag && !cats_color_flag)
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
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return 0;			/* not reached */
}
