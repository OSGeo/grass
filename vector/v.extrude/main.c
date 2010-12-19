
/****************************************************************
 *
 * MODULE:     v.extrude
 *
 * AUTHOR(S):  Jachym Cepicky <jachym.cepicky gmail.com>
 *             Based on v.example by Radim Blazek
 *             Inspired by d.vect and v.drape
 *             Coding help and code cleaning by Markus Neteler
 *             Support for points & OGR support added by Martin Landa (08/2007 / 2009)
 *
 * PURPOSE:    "Extrudes" flat polygons and lines to 3D with defined height
 *              Useful for creating buildings for displaying with NVIZ
 *
 * COPYRIGHT:  (C) 2005-2010 by the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2). Read the file COPYING that comes
 *             with GRASS for details.
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>

static int extrude(struct Map_info *, struct Map_info *,
		   struct line_cats *, struct line_pnts *,
		   int, int, double, double, struct Cell_head, int, int);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *old, *new, *zshift, *height, *elevation, *hcolumn,
	*type_opt, *field_opt;
    struct Flag *t_flag;

    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct bound_box map_box;

    struct Cell_head window;
    struct cat_list *Clist;

    int field;
    int i, only_type, cat, ctype, fdrast = 0, areanum = 0;
    int nelements;
    int line, type;
    int area, trace, centroid;
    double objheight, voffset;

    /* dbmi */
    struct field_info *Fi;
    dbDriver *driver = NULL;
    char query[1024];
    dbString sql;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    int more;
    char *comment;

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("3D"));
    module->description =
	_("Extrudes flat vector features to 3D with defined height.");

    t_flag = G_define_flag();
    t_flag->key = 't';
    t_flag->description = _("Trace elevation");

    old = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->answer = "point,line,boundary,area";
    type_opt->options = "point,line,boundary,area";

    new = G_define_standard_option(G_OPT_V_OUTPUT);

    zshift = G_define_option();
    zshift->key = "zshift";
    zshift->description = _("Shifting value for z coordinates");
    zshift->type = TYPE_DOUBLE;
    zshift->required = NO;
    zshift->answer = "0";

    /* raster sampling */
    elevation = G_define_standard_option(G_OPT_R_ELEV);
    elevation->required = NO;
    elevation->description = _("Elevation raster for height extraction");

    height = G_define_option();
    height->key = "height";
    height->type = TYPE_DOUBLE;
    height->required = NO;
    height->multiple = NO;
    height->description = _("Fixed height for 3D vector objects");

    hcolumn = G_define_standard_option(G_OPT_DB_COLUMN);
    hcolumn->key = "hcolumn";
    hcolumn->multiple = NO;
    hcolumn->description = _("Name of attribute column with object heights");

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!height->answer && !hcolumn->answer) {
	G_fatal_error(_("One of '%s' or '%s' parameters must be set"),
		      height->key, hcolumn->key);
    }

    sscanf(zshift->answer, "%lf", &voffset);

    if (height->answer)
	sscanf(height->answer, "%lf", &objheight);
    else
	objheight = 0.;

    only_type = Vect_option_to_types(type_opt);

    trace = (t_flag->answer) ? 1 : 0;
    area = (only_type & GV_AREA) ? 1 : 0;
    if (area) {
	if (only_type & GV_BOUNDARY) {
	    /* do not wrire wall twice -> disable boundary type */
	    only_type &= ~GV_BOUNDARY;
	}
	only_type &= ~GV_AREA;
    }

    centroid = 0;

    /* set input vector map name and mapset */
    Vect_check_input_output_name(old->answer, new->answer, GV_FATAL_EXIT);

    /* vector setup */
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_set_open_level(2);
    Vect_open_new(&Out, new->answer, WITH_Z);

    /* opening old vector */
    Vect_open_old2(&In, old->answer, "", field_opt->answer);
    field = Vect_get_field_number(&In, field_opt->answer);

    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* opening database connection, if required */
    if (hcolumn->answer) {
	Clist = Vect_new_cat_list();
	Clist->field = atoi(field_opt->answer);
	if ((Fi = Vect_get_field(&In, Clist->field)) == NULL)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  Clist->field);

	if ((driver =
	     db_start_driver_open_database(Fi->driver, Fi->database)) == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);

    }

    /* do we work with elevation raster? */
    if (elevation->answer) {
	/* raster setup */
	G_get_window(&window);

	/* open the elev raster, and check for error condition */
	fdrast = Rast_open_old(elevation->answer, "");
    }

    /* if area */
    if (area) {
	G_debug(1, "drawing areas");
	nelements = Vect_get_num_areas(&In);
	G_debug(2, "n_areas = %d", nelements);
	if (nelements > 0)
	    G_message(_("Extruding areas..."));
	for (areanum = 1; areanum <= nelements; areanum++) {

	    G_debug(3, "area = %d", areanum);

	    if (!Vect_area_alive(&In, areanum))
		continue;

	    centroid = Vect_get_area_centroid(&In, areanum);
	    if (!centroid) {
		G_warning(_("Skipping area %d without centroid"), areanum);
		continue;
	    }

	    /* height attribute */
	    if (hcolumn->answer) {
		cat = Vect_get_area_cat(&In, areanum, Clist->field);
		db_init_string(&sql);
		sprintf(query, "SELECT %s FROM %s WHERE %s = %d",
			hcolumn->answer, Fi->table, Fi->key, cat);
		G_debug(3, "SQL: %s", query);
		db_append_string(&sql, query);
		if (db_open_select_cursor
		    (driver, &sql, &cursor, DB_SEQUENTIAL)
		    != DB_OK)
		    G_fatal_error(_("Cannot select attributes for area %d"),
				  areanum);
		table = db_get_cursor_table(&cursor);
		column = db_get_table_column(table, 0);	/* first column */

		if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
		    continue;

		value = db_get_column_value(column);

		objheight = db_get_value_as_double(value,
						   db_get_column_host_type
						   (column));

		/* only draw if hcolumn was defined */
		if (objheight != 0) {
		    G_debug(3, "area centroid %d: object height: %f",
			    centroid, objheight);
		}

	    }			/* if hcolumn->answer */

	    Vect_get_area_points(&In, areanum, Points);

	    extrude(&In, &Out, Cats, Points,
		    fdrast, trace, objheight, voffset, window, GV_AREA,
		    centroid);

	    G_percent(areanum, nelements, 2);

	}			/* foreach area */

    }

    if (only_type > 0) {
	G_debug(1, "drawing other than areas");
	i = 1;
	/* loop through each line in the dataset */
	nelements = Vect_get_num_lines(&In);
	G_message(_("Extruding features..."));
	for (line = 1; line <= nelements; line++) {

	    /* read line */
	    type = Vect_read_line(&In, Points, Cats, line);

	    if (!Vect_line_alive(&In, line))
		continue;

	    if (!(type & only_type))
		continue;

	    /* fetch categories */
	    if (field != -1 && !Vect_cat_get(Cats, field, &cat)) {
		Vect_cat_set(Cats, 1, i);
		i++;
	    }

	    /* height attribute */
	    if (hcolumn->answer) {
		cat = Vect_get_line_cat(&In, line, Clist->field);

		/* sql init */
		db_init_string(&sql);
		sprintf(query, "SELECT %s FROM %s WHERE %s = %d",
			hcolumn->answer, Fi->table, Fi->key, cat);
		G_debug(3, "SQL: %s", query);
		db_append_string(&sql, query);

		/* cursor init */
		if (db_open_select_cursor
		    (driver, &sql, &cursor, DB_SEQUENTIAL)
		    != DB_OK)
		    G_fatal_error(_("Cannot select attributes for area #%d"),
				  areanum);
		table = db_get_cursor_table(&cursor);
		column = db_get_table_column(table, 0);	/* first column */

		if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
		    continue;

		/* objheight value */
		value = db_get_column_value(column);
		/* host_type -> ctype ? 
		   objheight = 
		   db_get_value_as_double(value,
		   db_get_column_host_type(column));
		 */
		ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
		if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_STRING &&
		    ctype != DB_C_TYPE_DOUBLE) {
		    G_fatal_error(_("Column <%s>: invalid data type"),
				  db_get_column_name(column));
		}
		objheight = db_get_value_as_double(value, ctype);

		/* only draw if hcolumn was defined */
		if (objheight != 0) {
		    G_debug(3, "area centroid %d: object height: %f",
			    centroid, objheight);
		}

	    }

	    extrude(&In, &Out, Cats, Points,
		    fdrast, trace, objheight, voffset, window, type, -1);

	    /* progress feedback */
	    G_percent(line, nelements, 2);

	}			/* for each line */
    }				/* else if area */

    if (driver) {
	db_close_database(driver);
	db_shutdown_driver(driver);
    }

    Vect_build(&Out);

    /* header */
    G_asprintf(&comment, "Generated by %s from vector map <%s>",
	       G_program_name(), old->answer);
    Vect_set_comment(&Out, comment);
    G_free(comment);

    Vect_get_map_box(&Out, &map_box);

    Vect_close(&In);
    Vect_close(&Out);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    G_done_msg("T: %f B: %f.", map_box.T, map_box.B);
    
    exit(EXIT_SUCCESS);
}


/**
  \brief Extrude vector object

  - point -> 3d line (vertical)
  - line  -> 3d line
  - boundary -> face
  - area -> face + kernel

  \param[in] In input vector map
  \param[out] Out output vector map
  \param[in] Cats categories
  \param[in] Points points
  \param[in] fdrast background raster map
  \param[in] trace trace raster map values
  \param[in] objheight object height
  \param[in] voffset vertical offset
  \param[in] window raster region
  \param[in] type feature type
  \param[in] centroid number of centroid for area
  \return number of writen objects
*/
static int extrude(struct Map_info *In, struct Map_info *Out,
		   struct line_cats *Cats, struct line_pnts *Points,
		   int fdrast, int trace, double objheight, double voffset,
		   struct Cell_head window, int type, int centroid)
{
    int k;			/* Points->n_points */
    int nlines;
    struct line_pnts *Points_wall, *Points_roof;

    double voffset_dem;		/* minimal offset */
    double voffset_curr;	/* offset of current point */
    double voffset_next;	/* offset of next point */

    nlines = 0;

    if (type != GV_POINT && Points->n_points < 2)
	return nlines;

    Points_wall = Vect_new_line_struct();
    Points_roof = Vect_new_line_struct();

    voffset_dem = 0.0;
    /* do not trace -> calculate minumum dem offset */
    if (fdrast && !trace) {
	for (k = 0; k < Points->n_points; k++) {
	    voffset_curr = Rast_get_sample(fdrast, &window, NULL,
					       Points->y[k], Points->x[k], 0,
					       NEAREST);
	    if (Rast_is_d_null_value(&voffset_curr))
		continue;

	    if (k == 0) {
		voffset_dem = voffset_curr;
	    }
	    else {
		if (voffset_curr < voffset_dem)
		    voffset_dem = voffset_curr;
	    }
	}
    }

    
    /* walls */
    for (k = 0; ; k++) {
	voffset_curr = voffset_next = 0.0;

	/* trace */
	if (fdrast && trace) {
	    voffset_curr = Rast_get_sample(fdrast, &window, NULL,
					       Points->y[k], Points->x[k], 0,
					       NEAREST);

	    if (type != GV_POINT) {
		voffset_next = Rast_get_sample(fdrast, &window, NULL,
						   Points->y[k + 1],
						   Points->x[k + 1], 0,
						   NEAREST);
	    }
	}

	if (Rast_is_d_null_value(&voffset_curr) ||
	    Rast_is_d_null_value(&voffset_next)) {
	    if (type == GV_POINT)
		break;
	    else if (type == GV_LINE) {
		if (k >= Points->n_points - 1)
		    break;
	    }
	    else if (type & (GV_BOUNDARY | GV_AREA)) {
		if (k >= Points->n_points - 2)
		    break;
	    }
	    continue;
	}

	if (trace) {
	    voffset_curr += voffset;
	    voffset_next += voffset;
	}
	else {
	    voffset_curr = voffset_dem + voffset;
	    voffset_next = voffset_dem + voffset;
	}

	if (type == GV_POINT) {
	    /* point -> 3d line (vertical) */
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + voffset_curr);
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + objheight + voffset_curr);
	    break;
	}
	else if (type == GV_LINE) {
	    /* line -> 3d line */
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + objheight + voffset_curr);
	    if (k >= Points->n_points - 1)
		break;
	}
	else if (type & (GV_BOUNDARY | GV_AREA)) {
	    /* reset */
	    Vect_reset_line(Points_wall);

	    /* boudary -> face */
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + voffset_curr);
	    Vect_append_point(Points_wall, Points->x[k + 1], Points->y[k + 1],
			      Points->z[k + 1] + voffset_next);
	    Vect_append_point(Points_wall, Points->x[k + 1], Points->y[k + 1],
			      Points->z[k + 1] + objheight + voffset_next);
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + objheight + voffset_curr);
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + voffset_curr);

	    Vect_write_line(Out, GV_FACE, Points_wall, Cats);
	    nlines++;

	    if (type == GV_AREA) {
		/* roof */
		Vect_append_point(Points_roof, Points->x[k], Points->y[k],
				  Points->z[k] + objheight + voffset_curr);
	    }

	    if (k >= Points->n_points - 2)
		break;
	}
    }

    if (type & (GV_POINT | GV_LINE)) {
	Vect_write_line(Out, GV_LINE, Points_wall, Cats);
	nlines++;
    }
    else if (type == GV_AREA && Points_roof->n_points > 3) {
	Vect_append_point(Points_roof,
			  Points_roof->x[0], Points_roof->y[0],
			  Points_roof->z[0]);
	Vect_write_line(Out, GV_FACE, Points_roof, Cats);
	nlines++;

	if (centroid > 0) {
	    /* centroid -> kernel */
	    Vect_read_line(In, Points, Cats, centroid);
	    Points->z[0] = Points_roof->z[0] / 2.0;
	    Vect_write_line(Out, GV_KERNEL, Points, Cats);
	    nlines++;
	}
    }

    Vect_destroy_line_struct(Points_wall);
    Vect_destroy_line_struct(Points_roof);

    return nlines;
}
