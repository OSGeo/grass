
/****************************************************************
 *
 * MODULE:       v.buffer
 * 
 * AUTHOR(S):    Radim Blazek
 *               Upgraded by Rosen Matev (Google Summer of Code 2008)
 *               OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *               rewrite and GEOS added by Markus Metz
 *               
 * PURPOSE:      Vector buffer
 *               
 * COPYRIGHT:    (C) 2001-2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define PI M_PI
#ifndef MIN
#define MIN(X,Y) ((X<Y)?X:Y)
#endif
#ifndef MAX
#define MAX(X,Y) ((X>Y)?X:Y)
#endif

/* returns 1 if unit_tolerance is adjusted, 0 otherwise */
int adjust_tolerance(double *tolerance)
{
    double t = 0.999 * (1 - cos(PI / 8));

    G_debug(2, "Maximum tolerance = %f", t);
    if (*tolerance > t) {
	*tolerance = t;
	return 1;
    }
    return 0;
}

int db_CatValArray_get_value_di(dbCatValArray * cvarr, int cat, double *value)
{
    int t;
    int ctype = cvarr->ctype;
    int ret;

    if (ctype == DB_C_TYPE_INT) {
	ret = db_CatValArray_get_value_int(cvarr, cat, &t);
	if (ret != DB_OK)
	    return ret;
	*value = (double)t;
	return DB_OK;
    }

    if (ctype == DB_C_TYPE_DOUBLE) {
	ret = db_CatValArray_get_value_double(cvarr, cat, value);
	return ret;
    }

    return DB_FAILED;
}

int point_in_buffer(struct buf_contours *arr_bc, struct spatial_index *si,
		    struct Map_info *Buf, double x, double y)
{
    int i, j, ret, flag;
    struct bound_box bbox;
    static struct ilist *List = NULL;
    static struct line_pnts *Points = NULL;
    static struct line_cats *BCats = NULL;

    if (List == NULL)
	List = Vect_new_list();
    if (Points == NULL)
	Points = Vect_new_line_struct();
    if (BCats == NULL)
	BCats = Vect_new_cats_struct();

    /* select outer contours overlapping with centroid (x, y) */
    bbox.W = bbox.E = x;
    bbox.N = bbox.S = y;
    bbox.T = PORT_DOUBLE_MAX;
    bbox.B = -PORT_DOUBLE_MAX;

    Vect_spatial_index_select(si, &bbox, List);
    
    for (i = 0; i < List->n_values; i++) {
	Vect_read_line(Buf, Points, BCats, arr_bc[List->value[i]].outer);
	ret = Vect_point_in_poly(x, y, Points);
	if (ret == 0)
	    continue;

	flag = 1;
	for (j = 0; j < arr_bc[List->value[i]].inner_count; j++) {
	    if (arr_bc[List->value[i]].inner[j] < 1)
		continue;

	    Vect_read_line(Buf, Points, NULL, arr_bc[List->value[i]].inner[j]);
	    ret = Vect_point_in_poly(x, y, Points);
	    if (ret != 0) {	/* inside inner contour */
		flag = 0;
		break;
	    }
	}

	if (flag) {
	    /* (x,y) is inside outer contour and outside inner contours of arr_bc[i] */
	    return 1;
	}
    }

    return 0;
}

int buffer_cats(struct buf_contours *arr_bc, struct spatial_index *si,
		    struct Map_info *Buf, double x, double y, struct line_cats *Cats)
{
    int i, j, ret, flag, inside;
    struct bound_box bbox;
    static struct ilist *List = NULL;
    static struct line_pnts *Points = NULL;
    static struct line_cats *BCats = NULL;

    if (List == NULL)
	List = Vect_new_list();
    if (Points == NULL)
	Points = Vect_new_line_struct();
    if (BCats == NULL)
	BCats = Vect_new_cats_struct();

    /* select outer contours overlapping with centroid (x, y) */
    bbox.W = bbox.E = x;
    bbox.N = bbox.S = y;
    bbox.T = PORT_DOUBLE_MAX;
    bbox.B = -PORT_DOUBLE_MAX;

    Vect_spatial_index_select(si, &bbox, List);
    
    Vect_reset_cats(Cats);
    
    inside = 0;
    for (i = 0; i < List->n_values; i++) {
	Vect_read_line(Buf, Points, BCats, arr_bc[List->value[i]].outer);
	ret = Vect_point_in_poly(x, y, Points);
	if (ret == 0)
	    continue;

	flag = 1;
	for (j = 0; j < arr_bc[List->value[i]].inner_count; j++) {
	    if (arr_bc[List->value[i]].inner[j] < 1)
		continue;

	    Vect_read_line(Buf, Points, NULL, arr_bc[List->value[i]].inner[j]);
	    ret = Vect_point_in_poly(x, y, Points);
	    if (ret != 0) {	/* inside inner contour */
		flag = 0;
		break;
	    }
	}

	if (flag) {
	    /* (x,y) is inside outer contour and outside inner contours of arr_bc[i] */
	    inside = 1;
	    for (j = 0; j < BCats->n_cats; j++)
		Vect_cat_set(Cats, BCats->field[j], BCats->cat[j]);
	}
    }

    return inside;
}


int main(int argc, char *argv[])
{
    struct Map_info In, Out, Buf;
    struct line_pnts *Points;
    struct line_cats *Cats, *BCats, *CCats;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *type_opt, *dista_opt, *distb_opt,
	          *angle_opt;
    struct Flag *straight_flag, *nocaps_flag, *cats_flag;
    struct Option *tol_opt, *bufcol_opt, *scale_opt, *field_opt,
		  *where_opt, *cats_opt;

    struct cat_list *cat_list = NULL;
    int verbose, use_geos;
    double da, db, dalpha, tolerance, unit_tolerance;
    int type;
    int i, ret, nareas, area, nlines, line;
    char *Areas, *Lines;
    int field;
    struct buf_contours *arr_bc;
    int arr_bc_alloc;
    struct buf_contours_pts arr_bc_pts;
    int line_id, buffers_count = 0;
    struct spatial_index si;
    struct bound_box bbox;

    /* Attributes if sizecol is used */
    int nrec, ctype;
    struct field_info *Fi = NULL;
    dbDriver *Driver;
    dbCatValArray cvarr;
    double size_val, scale;

#ifdef HAVE_GEOS
    /* TODO: use GEOSBufferParams * */
#endif

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("buffer"));
    G_add_keyword(_("geometry"));
    module->description =
	_("Creates a buffer around vector features of given type.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    field_opt->guisection = _("Selection");

    cats_opt = G_define_standard_option(G_OPT_V_CATS);
    cats_opt->guisection = _("Selection");
    
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,line,boundary,centroid,area";
    type_opt->answer = "point,line,area";
    type_opt->guisection = _("Selection");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    
    dista_opt = G_define_option();
    dista_opt->key = "distance";
    dista_opt->type = TYPE_DOUBLE;
    dista_opt->required = NO;
    dista_opt->description =
	_("Buffer distance along major axis in map units");
    dista_opt->guisection = _("Distance");

    distb_opt = G_define_option();
    distb_opt->key = "minordistance";
    distb_opt->type = TYPE_DOUBLE;
    distb_opt->required = NO;
    distb_opt->description =
	_("Buffer distance along minor axis in map units");
    distb_opt->guisection = _("Distance");

    angle_opt = G_define_option();
    angle_opt->key = "angle";
    angle_opt->type = TYPE_DOUBLE;
    angle_opt->required = NO;
    angle_opt->answer = "0";
    angle_opt->description = _("Angle of major axis in degrees");
    angle_opt->guisection = _("Distance");

    bufcol_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    bufcol_opt->key = "bufcolumn";
    bufcol_opt->description =
	_("Name of column to use for buffer distances");
    bufcol_opt->guisection = _("Distance");

    scale_opt = G_define_option();
    scale_opt->key = "scale";
    scale_opt->type = TYPE_DOUBLE;
    scale_opt->required = NO;
    scale_opt->answer = "1.0";
    scale_opt->description = _("Scaling factor for attribute column values");
    scale_opt->guisection = _("Distance");

    tol_opt = G_define_option();
    tol_opt->key = "tolerance";
    tol_opt->type = TYPE_DOUBLE;
    tol_opt->required = NO;
    tol_opt->answer = "0.01";
    tol_opt->description =
	_("Maximum distance between theoretical arc and polygon segments as multiple of buffer");
    tol_opt->guisection = _("Distance");

    straight_flag = G_define_flag();
    straight_flag->key = 's';
    straight_flag->description = _("Make outside corners straight");

    nocaps_flag = G_define_flag();
    nocaps_flag->key = 'c';
    nocaps_flag->description = _("Don't make caps at the ends of polylines");

    cats_flag = G_define_flag();
    cats_flag->key = 't';
    cats_flag->description = _("Transfer categories and attributes");
    cats_flag->guisection = _("Attributes");
    
    G_gisinit(argv[0]);
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

#if !defined HAVE_GEOS
    use_geos = FALSE;
#else
    use_geos = getenv("GRASS_VECTOR_BUFFER") ? FALSE : TRUE;
#endif
    G_debug(1, "use_geos = %d", use_geos);
    
    type = Vect_option_to_types(type_opt);

    if ((dista_opt->answer && bufcol_opt->answer) ||
	(!(dista_opt->answer || bufcol_opt->answer)))
	G_fatal_error(_("Select a buffer distance/minordistance/angle "
			"or column, but not both."));

    Vect_check_input_output_name(in_opt->answer, out_opt->answer, G_FATAL_EXIT);

    Vect_set_open_level(2); /* topology required */
    Vect_open_old2(&In, in_opt->answer, "", field_opt->answer);
    Vect_set_error_handler_io(&In, &Out);

    if (field_opt->answer)
	field = Vect_get_field_number(&In, field_opt->answer);
    else
	field = -1;

    if ((cats_opt->answer || where_opt->answer) && field == -1) {
        G_warning(_("Invalid layer number (%d). Parameter '%s' or '%s' specified, assuming layer '1'."),
                  field, cats_opt->key, where_opt->key);
        field = 1;
    }

    cat_list = NULL;
    if (field > 0)
	cat_list = Vect_cats_set_constraint(&In, field, where_opt->answer,
                                            cats_opt->answer);
	
    if (bufcol_opt->answer && field == -1)
	G_fatal_error(_("The bufcol option requires a valid layer."));

    tolerance = atof(tol_opt->answer);
    if (tolerance <= 0)
	G_fatal_error(_("The tolerance must be > 0."));

    if (adjust_tolerance(&tolerance))
	G_warning(_("The tolerance was reset to %g"), tolerance);

    scale = atof(scale_opt->answer);
    if (scale <= 0.0)
        G_fatal_error(_("Illegal scale value"));

    da = db = dalpha = unit_tolerance = 0;
    if (dista_opt->answer) {
	da = atof(dista_opt->answer);

	if (distb_opt->answer)
	    db = atof(distb_opt->answer);
	else
	    db = da;

	if (angle_opt->answer)
	    dalpha = atof(angle_opt->answer);
	else
	    dalpha = 0;

	unit_tolerance = fabs(tolerance * MIN(da, db));
	G_verbose_message(_("The tolerance in map units = %g"), unit_tolerance);
    }

    Vect_open_new(&Out, out_opt->answer, WITHOUT_Z);
    
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    BCats = Vect_new_cats_struct();
    CCats = Vect_new_cats_struct();
    
    /* open tmp vector for buffers, needed for cleaning */
    if (0 > Vect_open_tmp_new(&Buf, NULL, WITHOUT_Z)) {
        G_fatal_error(_("Unable to create vector map"));
    }
    Vect_build_partial(&Buf, GV_BUILD_BASE); /* switch to level 2 */

    /* check and load attribute column data */
    if (bufcol_opt->answer) {
	db_CatValArray_init(&cvarr);

	Fi = Vect_get_field(&In, field);
	if (Fi == NULL)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  field);

	Driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (Driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);

	/* Note do not check if the column exists in the table because it may be expression */

	/* TODO: only select values we need instead of all in column */
	nrec =
	    db_select_CatValArray(Driver, Fi->table, Fi->key,
				  bufcol_opt->answer, NULL, &cvarr);
	if (nrec < 0)
	    G_fatal_error(_("Unable to select data from table <%s>"),
			  Fi->table);
	G_debug(2, "%d records selected from table", nrec);

	ctype = cvarr.ctype;
	if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Column type not supported"));

	db_close_database_shutdown_driver(Driver);

	/* Output cats/values list */
	for (i = 0; i < cvarr.n_values; i++) {
	    if (ctype == DB_C_TYPE_INT) {
		G_debug(4, "cat = %d val = %d", cvarr.value[i].cat,
			cvarr.value[i].val.i);
	    }
	    else if (ctype == DB_C_TYPE_DOUBLE) {
		G_debug(4, "cat = %d val = %f", cvarr.value[i].cat,
			cvarr.value[i].val.d);
	    }
	}
    }

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);


    /* Create buffers' boundaries */
    nlines = nareas = 0;
    if ((type & GV_POINTS) || (type & GV_LINES))
	nlines = Vect_get_num_primitives(&In, type);
    if (type & GV_AREA)
	nareas = Vect_get_num_areas(&In);
    
    if (nlines + nareas == 0) {
	G_warning(_("No features available for buffering. "
	            "Check type option and features available in the input vector."));
	exit(EXIT_SUCCESS);
    }

    /* init arr_bc */
    buffers_count = 1;
    arr_bc_alloc = nlines + nareas + 1;
    arr_bc = G_calloc(arr_bc_alloc, sizeof(struct buf_contours));

    Vect_spatial_index_init(&si, 0);

#ifdef HAVE_GEOS
    initGEOS(G_message, G_fatal_error);
#endif
    if(!use_geos && (da < 0. || db < 0.)) {
	G_warning(_("Negative distances for internal buffers are not supported "
	            "and converted to positive values."));
	da = fabs(da);
	db = fabs(db);
    }

    /* Areas */
    if (nareas > 0) {
	int centroid;

	G_message(_("Buffering areas..."));
	for (area = 1; area <= nareas; area++) {
	    int cat;

	    G_percent(area, nareas, 2);
	    
	    if (!Vect_area_alive(&In, area))
		continue;
	    
	    centroid = Vect_get_area_centroid(&In, area);
	    if (centroid == 0)
		continue;

	    Vect_read_line(&In, NULL, Cats, centroid);

	    if (field > 0 && !Vect_cats_in_constraint(Cats, field, cat_list))
		continue;

	    Vect_reset_cats(CCats);
	    for (i = 0; i < Cats->n_cats; i++) {
		if (field < 0 || Cats->field[i] == field) {
		    Vect_cat_set(CCats, Cats->field[i], Cats->cat[i]);
		}
	    }

	    if (bufcol_opt->answer) {
		Vect_cat_get(Cats, field, &cat);

		ret = db_CatValArray_get_value_di(&cvarr, cat, &size_val);
		if (ret != DB_OK) {
		    G_warning(_("No record for category %d in table <%s>"),
			      cat, Fi->table);
		    continue;
		}

		if (size_val < 0.0) {
		    G_warning(_("Attribute is of invalid size (%.3f) for category %d"),
			      size_val, cat);
		    continue;
		}

		if (size_val == 0.0)
		    continue;

		da = size_val * scale;
		db = da;
		dalpha = 0;
		unit_tolerance = fabs(tolerance * MIN(da, db));

		G_debug(2, "    dynamic buffer size = %.2f", da);
		G_debug(2, _("The tolerance in map units: %g"),
			unit_tolerance);
	    }

#ifdef HAVE_GEOS
	    if (use_geos)
		geos_buffer(&In, &Out, &Buf, area, GV_AREA, da,
			    &si, CCats, &arr_bc, &buffers_count, &arr_bc_alloc);
#endif
	    if (!use_geos) {
		Vect_area_buffer2(&In, area, da, db, dalpha,
				  !(straight_flag->answer),
				  !(nocaps_flag->answer), unit_tolerance,
				  &(arr_bc_pts.oPoints),
				  &(arr_bc_pts.iPoints),
				  &(arr_bc_pts.inner_count));
		
		Vect_write_line(&Out, GV_BOUNDARY, arr_bc_pts.oPoints, BCats);
		line_id = Vect_write_line(&Buf, GV_BOUNDARY, arr_bc_pts.oPoints, CCats);
		Vect_destroy_line_struct(arr_bc_pts.oPoints);
		/* add buffer to spatial index */
		Vect_get_line_box(&Buf, line_id, &bbox);
		Vect_spatial_index_add_item(&si, buffers_count, &bbox);
		arr_bc[buffers_count].outer = line_id;
		
		arr_bc[buffers_count].inner_count = arr_bc_pts.inner_count;
		if (arr_bc_pts.inner_count > 0) {
		    arr_bc[buffers_count].inner = G_malloc(arr_bc_pts.inner_count * sizeof(int));
		    for (i = 0; i < arr_bc_pts.inner_count; i++) {
			Vect_write_line(&Out, GV_BOUNDARY, arr_bc_pts.iPoints[i], BCats);
			line_id = Vect_write_line(&Buf, GV_BOUNDARY, arr_bc_pts.iPoints[i], BCats);
			Vect_destroy_line_struct(arr_bc_pts.iPoints[i]);
			arr_bc[buffers_count].inner[i] = line_id;
		    }
		    G_free(arr_bc_pts.iPoints);
		}
		buffers_count++;
	    } /* native buffer end */
	}
    }

    /* Lines (and Points) */
    if (nlines > 0) {
	int ltype;

	G_message(_("Buffering features..."));
	
	if (da < 0 || db < 0) {
	    G_warning(_("Negative distances are only supported for areas"));
	    da = fabs(da);
	    db = fabs(db);
	}

	nlines = Vect_get_num_lines(&In);
	for (line = 1; line <= nlines; line++) {
	    int cat;

	    G_debug(2, "line = %d", line);
	    G_percent(line, nlines, 2);
	    
	    if (!Vect_line_alive(&In, line))
		continue;

	    ltype = Vect_read_line(&In, Points, Cats, line);
	    if (!(ltype & type))
		continue;

	    if (field > 0 && !Vect_cats_in_constraint(Cats, field, cat_list))
		continue;

	    Vect_reset_cats(CCats);
	    for (i = 0; i < Cats->n_cats; i++) {
		if (field < 0 || Cats->field[i] == field) {
		    Vect_cat_set(CCats, Cats->field[i], Cats->cat[i]);
		}
	    }

	    if (bufcol_opt->answer) {
		Vect_cat_get(Cats, field, &cat);
		ret = db_CatValArray_get_value_di(&cvarr, cat, &size_val);
		if (ret != DB_OK) {
		    G_warning(_("No record for category %d in table <%s>"),
			      cat, Fi->table);
		    continue;
		}

		if (size_val < 0.0) {
		    G_warning(_("Attribute is of invalid size (%.3f) for category %d"),
			      size_val, cat);
		    continue;
		}

		if (size_val == 0.0)
		    continue;

		da = size_val * scale;
		if (da < 0) {
		    G_warning(_("Negative distances are only supported for areas"));
		    da = fabs(da);
		}
		db = da;
		dalpha = 0;
		unit_tolerance = tolerance * MIN(da, db);

		G_debug(2, "    dynamic buffer size = %.2f", da);
		G_debug(2, _("The tolerance in map units: %g"),
			unit_tolerance);
	    }
	    
	    Vect_line_prune(Points);
	    if (ltype & GV_POINTS || Points->n_points == 1) {
		Vect_point_buffer2(Points->x[0], Points->y[0], da, db, dalpha,
				   !(straight_flag->answer), unit_tolerance,
				   &(arr_bc_pts.oPoints));

		Vect_write_line(&Out, GV_BOUNDARY, arr_bc_pts.oPoints, BCats);
		line_id = Vect_write_line(&Buf, GV_BOUNDARY, arr_bc_pts.oPoints, CCats);
		Vect_destroy_line_struct(arr_bc_pts.oPoints);
		/* add buffer to spatial index */
		Vect_get_line_box(&Buf, line_id, &bbox);
		Vect_spatial_index_add_item(&si, buffers_count, &bbox);
		arr_bc[buffers_count].outer = line_id;
		arr_bc[buffers_count].inner_count = 0;
		arr_bc[buffers_count].inner = NULL;
		buffers_count++;

	    }
	    else {
#ifdef HAVE_GEOS
		if (use_geos)
		    geos_buffer(&In, &Out, &Buf, line, type, da,
				&si, CCats, &arr_bc, &buffers_count, &arr_bc_alloc);
#endif
		if (!use_geos) {
		    Vect_line_buffer2(Points, da, db, dalpha,
				      !(straight_flag->answer),
				      !(nocaps_flag->answer), unit_tolerance,
				      &(arr_bc_pts.oPoints),
				      &(arr_bc_pts.iPoints),
				      &(arr_bc_pts.inner_count));
		    
		    Vect_write_line(&Out, GV_BOUNDARY, arr_bc_pts.oPoints, BCats);
		    line_id = Vect_write_line(&Buf, GV_BOUNDARY, arr_bc_pts.oPoints, CCats);
		    Vect_destroy_line_struct(arr_bc_pts.oPoints);
		    /* add buffer to spatial index */
		    Vect_get_line_box(&Buf, line_id, &bbox);
		    Vect_spatial_index_add_item(&si, buffers_count, &bbox);
		    arr_bc[buffers_count].outer = line_id;
		    
		    arr_bc[buffers_count].inner_count = arr_bc_pts.inner_count;
		    if (arr_bc_pts.inner_count > 0) {
			arr_bc[buffers_count].inner = G_malloc(arr_bc_pts.inner_count * sizeof(int));
			for (i = 0; i < arr_bc_pts.inner_count; i++) {
			    Vect_write_line(&Out, GV_BOUNDARY, arr_bc_pts.iPoints[i], BCats);
			    line_id = Vect_write_line(&Buf, GV_BOUNDARY, arr_bc_pts.iPoints[i], BCats);
			    Vect_destroy_line_struct(arr_bc_pts.iPoints[i]);
			    arr_bc[buffers_count].inner[i] = line_id;
			}
			G_free(arr_bc_pts.iPoints);
		    }
		    buffers_count++;
		} /* native buffer end */
	    }
	}
    }

#ifdef HAVE_GEOS
    finishGEOS();
#endif

    verbose = G_verbose();

    G_message(_("Cleaning buffers..."));
    
    /* Break lines */
    G_message(_("Building parts of topology..."));
    Vect_build_partial(&Out, GV_BUILD_BASE);

    /* Warning: snapping must be done, otherwise colinear boundaries are not broken and 
     * topology cannot be built (the same angle). But snapping distance must be very, very 
     * small, otherwise counterclockwise boundaries can appear in areas outside the buffer.
     * I have done some tests on real data (projected) and threshold 1e-8 was not enough,
     * Snapping threshold 1e-7 seems to work. Don't increase until we find example 
     * where it is not sufficient. RB */

    /* TODO: look at snapping threshold better, calculate some theoretical value to avoid
     * the same angles of lines at nodes, don't forget about LongLat data, probably
     * calculate different threshold for each map, depending on map's bounding box 
     * and/or distance and tolerance */
    G_message(_("Snapping boundaries..."));
    Vect_snap_lines(&Out, GV_BOUNDARY, 1e-7, NULL);

    G_message(_("Breaking polygons..."));
    Vect_break_polygons(&Out, GV_BOUNDARY, NULL);

    G_message(_("Removing duplicates..."));
    Vect_remove_duplicates(&Out, GV_BOUNDARY, NULL);

    do {
	G_message(_("Breaking boundaries..."));
	Vect_break_lines(&Out, GV_BOUNDARY, NULL);

	G_message(_("Removing duplicates..."));
	Vect_remove_duplicates(&Out, GV_BOUNDARY, NULL);

	G_message(_("Cleaning boundaries at nodes"));

    } while (Vect_clean_small_angles_at_nodes(&Out, GV_BOUNDARY, NULL) > 0);

    /* Dangles and bridges don't seem to be necessary if snapping is small enough. */
    /* Still needed for larger buffer distances ? */

    Vect_build_partial(&Out, GV_BUILD_AREAS);
    G_message(_("Removing dangles..."));
    Vect_remove_dangles(&Out, GV_BOUNDARY, -1, NULL);

    G_message (_("Removing bridges..."));
    Vect_remove_bridges(&Out, NULL, NULL, NULL);

    G_message(_("Attaching islands..."));
    Vect_build_partial(&Out, GV_BUILD_ATTACH_ISLES);

    if (!cats_flag->answer) {
	/* Calculate new centroids for all areas */
	nareas = Vect_get_num_areas(&Out);
	Areas = (char *)G_calloc(nareas + 1, sizeof(char));
	G_message(_("Calculating centroids for all areas..."));
	G_percent(0, nareas, 2);
	for (area = 1; area <= nareas; area++) {
	    double x, y;

	    G_percent(area, nareas, 2);

	    G_debug(3, "area = %d", area);

	    if (!Vect_area_alive(&Out, area))
		continue;

	    ret = Vect_get_point_in_area(&Out, area, &x, &y);
	    if (ret < 0) {
		G_warning(_("Cannot calculate area centroid"));
		continue;
	    }

	    ret = point_in_buffer(arr_bc, &si, &Buf, x, y);

	    if (ret) {
		G_debug(3, "  -> in buffer");
		Areas[area] = 1;
	    }
	}

	/* Make a list of boundaries to be deleted (both sides inside) */
	nlines = Vect_get_num_lines(&Out);
	G_debug(3, "nlines = %d", nlines);
	Lines = (char *)G_calloc(nlines + 1, sizeof(char));

	G_message(_("Generating list of boundaries to be deleted..."));
	for (line = 1; line <= nlines; line++) {
	    int j, side[2], areas[2];

	    G_percent(line, nlines, 2);

	    G_debug(3, "line = %d", line);

	    if (!Vect_line_alive(&Out, line))
		continue;

	    Vect_get_line_areas(&Out, line, &side[0], &side[1]);

	    for (j = 0; j < 2; j++) {
		if (side[j] == 0) {	/* area/isle not build */
		    areas[j] = 0;
		}
		else if (side[j] > 0) {	/* area */
		    areas[j] = side[j];
		}
		else {		/* < 0 -> island */
		    areas[j] = Vect_get_isle_area(&Out, abs(side[j]));
		}
	    }

	    G_debug(3, " areas = %d , %d -> Areas = %d, %d", areas[0], areas[1],
		    Areas[areas[0]], Areas[areas[1]]);
	    if (Areas[areas[0]] && Areas[areas[1]])
		Lines[line] = 1;
	}
	G_free(Areas);

	/* Delete boundaries */
	G_message(_("Deleting boundaries..."));
	for (line = 1; line <= nlines; line++) {
	    G_percent(line, nlines, 2);
	    
	    if (!Vect_line_alive(&Out, line))
		continue;

	    if (Lines[line]) {
		G_debug(3, " delete line %d", line);
		Vect_delete_line(&Out, line);
	    }
	    else {
		/* delete incorrect boundaries */
		int side[2];

		Vect_get_line_areas(&Out, line, &side[0], &side[1]);

		if (!side[0] && !side[1]) {
                    G_debug(3, " delete line %d", line);
		    Vect_delete_line(&Out, line);
                }
	    }
	}

	G_free(Lines);
    }

    /* Create new centroids */
    Vect_reset_cats(Cats);
    Vect_cat_set(Cats, 1, 1);
    nareas = Vect_get_num_areas(&Out);

    G_message(_("Calculating centroids for areas..."));    
    for (area = 1; area <= nareas; area++) {
	double x, y;

	G_percent(area, nareas, 2);

	G_debug(3, "area = %d", area);

	if (!Vect_area_alive(&Out, area))
	    continue;

	ret = Vect_get_point_in_area(&Out, area, &x, &y);
	if (ret < 0) {
            G_warning(_("Unable to calculate centroid for area %d"), area);
	    continue;
	}

	if (cats_flag->answer)
	    ret = buffer_cats(arr_bc, &si, &Buf, x, y, Cats);
	else
	    ret = point_in_buffer(arr_bc, &si, &Buf, x, y);

	if (ret) {
	    Vect_reset_line(Points);
	    Vect_append_point(Points, x, y, 0.);
	    Vect_write_line(&Out, GV_CENTROID, Points, Cats);
	}
    }

    Vect_spatial_index_destroy(&si);
    Vect_close(&Buf);
    
    G_set_verbose(verbose);

    if (cats_flag->answer)
	Vect_copy_tables(&In, &Out, field);

    Vect_close(&In);

    Vect_build_partial(&Out, GV_BUILD_NONE);
    Vect_build(&Out);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
