
/***************************************************************
 *
 * MODULE:       v.distance
 * 
 * AUTHOR(S):    - J.Soimasuo 15.9.1994, University of Joensuu,
 *                 Faculty of Forestry, Finland
 *               - some additions 2002 Markus Neteler
 *               - updated to 5.7 by Radim Blazek 2003
 *               - OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *               - speed-up for dmax != 0 Markus Metz 2010
 *               
 * PURPOSE:      Calculates distance from a point to nearest feature in vector layer. 
 *               
 * COPYRIGHT:    (C) 2002-2010 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>
#include "local_proto.h"

/* TODO: support all types (lines, boundaries, areas for 'from' (from_type) */

int main(int argc, char *argv[])
{
    int i, j, k;
    int print_as_matrix;	/* only for do_all=TRUE */
    int do_all;			/* calculate from each to each within the threshold */
    struct GModule *module;
    struct {
	struct Option *from, *to, *from_type, *to_type,
	    *from_field, *to_field;
	struct Option *out, *max, *min, *table;
	struct Option *upload, *column, *to_column;
    } opt;
    struct {
	struct Flag *print, *all;
    } flag;
    struct Map_info From, To, Out, *Outp;
    int from_type, to_type, from_field, to_field;
    double max, min;
    double *max_step;
    int n_max_steps, curr_step;
    struct line_pnts *FPoints, *TPoints;
    struct line_cats *FCats, *TCats;
    NEAR *Near, *near;
    int anear;			/* allocated space, used only for do_all */
    UPLOAD *Upload;		/* zero terminated */
    int ftype, fcat, tcat, count;
    int nfrom, nto, nfcats, fline, tline, tseg, tarea, area, isle, nisles;
    double tx, ty, tz, dist, talong, tmp_tx, tmp_ty, tmp_tz, tmp_dist,
	tmp_talong;
    struct field_info *Fi, *toFi;
    dbString stmt, dbstr;
    dbDriver *driver, *to_driver;
    int *catexist, ncatexist, *cex;
    char buf1[2000], buf2[2000];
    int update_ok, update_err, update_exist, update_notexist, update_dupl,
	update_notfound;
    struct boxlist *List;
    struct bound_box box;
    dbCatValArray cvarr;
    dbColumn *column;

    do_all = FALSE;
    print_as_matrix = FALSE;
    column = NULL;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    module->description =
	_("Finds the nearest element in vector map 'to' for elements in vector map 'from'.");

    opt.from = G_define_standard_option(G_OPT_V_INPUT);
    opt.from->key = "from";
    opt.from->description = _("Name of existing vector map (from)");
    opt.from->guisection = _("From");

    opt.from_field = G_define_standard_option(G_OPT_V_FIELD);
    opt.from_field->key = "from_layer";
    opt.from_field->label = _("Layer number or name (from)");
    opt.from_field->guisection = _("From");

    opt.from_type = G_define_standard_option(G_OPT_V_TYPE);
    opt.from_type->key = "from_type";
    opt.from_type->options = "point,centroid";
    opt.from_type->answer = "point";
    opt.from_type->label = _("Feature type (from)");
    opt.from_type->guisection = _("From");

    opt.to = G_define_standard_option(G_OPT_V_INPUT);
    opt.to->key = "to";
    opt.to->description = _("Name of existing vector map (to)");
    opt.to->guisection = _("To");

    opt.to_field = G_define_standard_option(G_OPT_V_FIELD);
    opt.to_field->key = "to_layer";
    opt.to_field->label = _("Layer number or name (to)");
    opt.to_field->guisection = _("To");

    opt.to_type = G_define_standard_option(G_OPT_V_TYPE);
    opt.to_type->key = "to_type";
    opt.to_type->options = "point,line,boundary,centroid,area";
    opt.to_type->answer = "point,line,area";
    opt.to_type->label = _("Feature type (to)");
    opt.to_type->guisection = _("To");

    opt.out = G_define_standard_option(G_OPT_V_OUTPUT);
    opt.out->key = "output";
    opt.out->required = NO;
    opt.out->description = _("Name for output vector map containing lines "
			     "connecting nearest elements");

    opt.max = G_define_option();
    opt.max->key = "dmax";
    opt.max->type = TYPE_DOUBLE;
    opt.max->required = NO;
    opt.max->answer = "-1";
    opt.max->description = _("Maximum distance or -1 for no limit");

    opt.min = G_define_option();
    opt.min->key = "dmin";
    opt.min->type = TYPE_DOUBLE;
    opt.min->required = NO;
    opt.min->answer = "-1";
    opt.min->description = _("Minimum distance or -1 for no limit");

    opt.upload = G_define_option();
    opt.upload->key = "upload";
    opt.upload->type = TYPE_STRING;
    opt.upload->required = YES;
    opt.upload->multiple = YES;
    opt.upload->options = "cat,dist,to_x,to_y,to_along,to_angle,to_attr";
    opt.upload->description =
	_("Values describing the relation between two nearest features");
    opt.upload->descriptions =
	_("cat;category of the nearest feature;"
	  "dist;minimum distance to nearest feature;"
	  "to_x;x coordinate of the nearest point on the 'to' feature;"
	  "to_y;y coordinate of the nearest point on the 'to' feature;"
	  "to_along;distance to the nearest point on the 'to' feature along "
	     "that linear feature;"
	  "to_angle;angle along the nearest linear feature in the 'to' map, "
	     "measured CCW from the +x axis, in radians, between -Pi and Pi "
	     "inclusive;"
	  "to_attr;attribute of nearest feature given by to_column option");
    /*  "from_x - x coordinate of the nearest point on 'from' feature;" */
    /*  "from_y - y coordinate of the nearest point on 'from' feature;" */
    /* "from_along - distance to the nearest point on 'from' feature along linear feature;" */
    /* "from_angle - angle between the linear feature in 'to' map and the +x "
	"axis, at the location of point/centroid in 'from' map, CCW, in "
	"radians, between -Pi and Pi inclusive;" */

    opt.column = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.column->required = YES;
    opt.column->multiple = YES;
    opt.column->description =
	_("Column name(s) where values specified by 'upload' option will be uploaded");
    opt.column->guisection = _("From_map");

    opt.to_column = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.to_column->key = "to_column";
    opt.to_column->description =
	_("Column name of nearest feature (used with upload=to_attr)");
    opt.to_column->guisection = _("To");
    
    opt.table = G_define_standard_option(G_OPT_DB_TABLE);
    opt.table->gisprompt = "new_dbtable,dbtable,dbtable";
    opt.table->description =
	_("Name of table created when the 'distance to all' flag is used");

    flag.print = G_define_flag();
    flag.print->key = 'p';
    flag.print->label =
	_("Print output to stdout, don't update attribute table");
    flag.print->description =
	_("First column is always category of 'from' feature called from_cat");

    flag.all = G_define_flag();
    flag.all->key = 'a';
    flag.all->label =
	_("Calculate distances to all features within the threshold");
    flag.all->description =
	_("Output is written to stdout but may be uploaded to a new table "
	  "created by this module; multiple 'upload' options may be used.");

    /* GUI dependency */
    opt.from->guidependency = G_store(opt.from_field->key);
    sprintf(buf1, "%s,%s", opt.to_field->key, opt.to_column->key);
    opt.to->guidependency = G_store(buf1);
    opt.to_field->guidependency = G_store(opt.to_column->key);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    from_type = Vect_option_to_types(opt.from_type);
    to_type = Vect_option_to_types(opt.to_type);

    max = atof(opt.max->answer);
    min = atof(opt.min->answer);

    if (flag.all->answer)
	do_all = TRUE;

    /* Read upload and column options */
    /* count */
    i = 0;
    while (opt.upload->answers[i])
	i++;
    if (strcmp(opt.from->answer, opt.to->answer) == 0 &&
	do_all && !opt.table->answer && i == 1)
	print_as_matrix = TRUE;

    /* alloc */
    Upload = (UPLOAD *) G_calloc(i + 1, sizeof(UPLOAD));
    /* read upload */
    i = 0;
    while (opt.upload->answers[i]) {
	if (strcmp(opt.upload->answers[i], "cat") == 0)
	    Upload[i].upload = CAT;
	else if (strcmp(opt.upload->answers[i], "from_x") == 0)
	    Upload[i].upload = FROM_X;
	else if (strcmp(opt.upload->answers[i], "from_y") == 0)
	    Upload[i].upload = FROM_Y;
	else if (strcmp(opt.upload->answers[i], "to_x") == 0)
	    Upload[i].upload = TO_X;
	else if (strcmp(opt.upload->answers[i], "to_y") == 0)
	    Upload[i].upload = TO_Y;
	else if (strcmp(opt.upload->answers[i], "from_along") == 0)
	    Upload[i].upload = FROM_ALONG;
	else if (strcmp(opt.upload->answers[i], "to_along") == 0)
	    Upload[i].upload = TO_ALONG;
	else if (strcmp(opt.upload->answers[i], "dist") == 0)
	    Upload[i].upload = DIST;
	else if (strcmp(opt.upload->answers[i], "to_angle") == 0)
	    Upload[i].upload = TO_ANGLE;
	else if (strcmp(opt.upload->answers[i], "to_attr") == 0) {
	    if (!(opt.to_column->answer)) {
		G_fatal_error(_("to_column option missing"));
	    }
	    Upload[i].upload = TO_ATTR;
	}

	i++;
    }
    Upload[i].upload = END;
    /* read columns */
    i = 0;
    while (opt.column->answers[i]) {
	if (Upload[i].upload == END) {
	    G_warning(_("Too many column names"));
	    break;
	}
	Upload[i].column = G_store(opt.column->answers[i]);
	i++;
    }
    if (Upload[i].upload != END)
	G_fatal_error(_("Not enough column names"));

    /* Open 'from' vector */
    Vect_set_open_level(2);
    Vect_open_old2(&From, opt.from->answer, G_mapset(), opt.from_field->answer);

    from_field = Vect_get_field_number(&From, opt.from_field->answer);

    if (Vect_get_num_primitives(&From, GV_POINTS) < 1) {
	const char *name = Vect_get_full_name(&From);
	Vect_close(&From);
	G_fatal_error(_("No points/centroids found in <%s>"), name);
    }
    
    /* Open 'to' vector */
    Vect_set_open_level(2);
    Vect_open_old2(&To, opt.to->answer, "", opt.to_field->answer);

    to_field = Vect_get_field_number(&To, opt.to_field->answer);

    /* Open output vector */
    if (opt.out->answer) {
	Vect_open_new(&Out, opt.out->answer, WITHOUT_Z);
	Vect_hist_command(&Out);
	Outp = &Out;
    }
    else {
	Outp = NULL;
    }

    /* TODO: add maxdist = -1 to Vect_select_ !!! */
    /* Calc maxdist */
    n_max_steps = 1;
    if (max != 0) {
	struct bound_box fbox, tbox;
	double dx, dy, dz, tmp_max;
	int n_features = 0;

	Vect_get_map_box(&From, &fbox);
	Vect_get_map_box(&To, &tbox);

	Vect_box_extend(&fbox, &tbox);

	dx = fbox.E - fbox.W;
	dy = fbox.N - fbox.S;
	if (Vect_is_3d(&From))
	    dz = fbox.T - fbox.B;
	else
	    dz = 0.0;

	tmp_max = sqrt(dx * dx + dy * dy + dz * dz);
	if (max < 0)
	    max = tmp_max;

	/* how to determine a reasonable number of steps to increase the search box? */
	/* with max > 0 but max <<< tmp_max, 2 steps are sufficient, first 0 then max
	 * a reasonable number of steps also depends on the number of features in To
	 * e.g. only one area in To, no need to step */
	nto = Vect_get_num_lines(&To);
	for (tline = 1; tline <= nto; tline++) {
	    /* TODO: Vect_get_line_type() */
	    n_features += ((to_type & To.plus.Line[tline]->type) != 0);
	}
	if (to_type & GV_AREA) {
	    if (Vect_get_num_areas(&To) > n_features)
		n_features = Vect_get_num_areas(&To);
	}
	if (n_features == 0)
	    G_fatal_error(_("No features of selected type in To vector <%s>"),
			    opt.to->answer);
	n_max_steps = sqrt(n_features) * max / tmp_max;
	/* max 9 steps from testing */
	if (n_max_steps > 9)
	    n_max_steps = 9;
	if (n_max_steps < 2)
	    n_max_steps = 2;
	if (n_max_steps > n_features)
	    n_max_steps = n_features;

	G_debug(2, "max = %f", max);
	G_debug(2, "maximum reasonable search distance = %f", tmp_max);
	G_debug(2, "n_features = %d", n_features);
	G_debug(2, "n_max_steps = %d", n_max_steps);
    }

    if (min > max)
	G_fatal_error("dmin can not be larger than dmax");

    if (n_max_steps > 1) {
	/* set up steps to increase search box */
	max_step = G_malloc(n_max_steps * sizeof(double));
	/* first step always 0 */
	max_step[0] = 0;

	for (curr_step = 1; curr_step < n_max_steps - 1; curr_step++) {
	    /* for 9 steps, this would be max / [128, 64, 32, 16, 8, 4, 2] */
	    max_step[curr_step] = max / (2 << (n_max_steps - 1 - curr_step));
	}
	/* last step always max */
	max_step[n_max_steps - 1] = max;
    }
    else {
	max_step = G_malloc(sizeof(double));
	max_step[0] = max;
    }

    /* Open database driver */
    db_init_string(&stmt);
    db_init_string(&dbstr);
    driver = NULL;
    if (!flag.print->answer) {

	if (!do_all) {
	    Fi = Vect_get_field(&From, from_field);
	    if (Fi == NULL)
		G_fatal_error(_("Database connection not defined for layer <%s>"),
			      opt.from_field->answer);

	    driver = db_start_driver_open_database(Fi->driver, Fi->database);
	    if (driver == NULL)
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			      Fi->database, Fi->driver);

	    /* check if column exists */
	    i = 0;
	    while (opt.column->answers[i]) {
		db_get_column(driver, Fi->table, opt.column->answers[i],
			      &column);
		if (column) {
		    db_free_column(column);
		    column = NULL;
		}
		else {
		    G_fatal_error(_("Column <%s> not found in table <%s>"),
				  opt.column->answers[i], Fi->table);
		}
		i++;
	    }
	}
	else {
	    driver = db_start_driver_open_database(NULL, NULL);
	    if (driver == NULL)
		G_fatal_error(_("Unable to open default database"));
	}
    }

    to_driver = NULL;
    if (opt.to_column->answer) {
	toFi = Vect_get_field(&To, to_field);
	if (toFi == NULL)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  to_field);

	to_driver =
	    db_start_driver_open_database(toFi->driver, toFi->database);
	if (to_driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  toFi->database, toFi->driver);

	/* check if to_column exists */
	db_get_column(to_driver, toFi->table, opt.to_column->answer, &column);
	if (column) {
	    db_free_column(column);
	    column = NULL;
	}
	else {
	    G_fatal_error(_("Column <%s> not found in table <%s>"),
			  opt.to_column->answer, toFi->table);
	}

	/* Check column types */
	if (!flag.print->answer && !do_all) {
	    char *fcname = NULL;
	    int fctype, tctype;

	    i = 0;
	    while (opt.column->answers[i]) {
		if (Upload[i].upload == TO_ATTR) {
		    fcname = opt.column->answers[i];
		    break;
		}
		i++;
	    }

	    if (fcname) {
		fctype = db_column_Ctype(driver, Fi->table, fcname);
		tctype =
		    db_column_Ctype(to_driver, toFi->table,
				    opt.to_column->answer);

		if (((tctype == DB_C_TYPE_STRING ||
		      tctype == DB_C_TYPE_DATETIME)
		     && (fctype == DB_C_TYPE_INT ||
			 fctype == DB_C_TYPE_DOUBLE)) ||
		    ((tctype == DB_C_TYPE_INT || tctype == DB_C_TYPE_DOUBLE)
		     && (fctype == DB_C_TYPE_STRING ||
			 fctype == DB_C_TYPE_DATETIME))
		    ) {
		    G_fatal_error(_("Incompatible column types"));
		}
	    }
	}
    }

    FPoints = Vect_new_line_struct();
    TPoints = Vect_new_line_struct();
    FCats = Vect_new_cats_struct();
    TCats = Vect_new_cats_struct();
    List = Vect_new_boxlist(1);

    /* Allocate space ( may be more than needed (duplicate cats and elements without cats) ) */
    nfrom = Vect_get_num_lines(&From);
    nto = Vect_get_num_lines(&To);
    if (do_all) {
	/* Be careful with do_all, it can easily run out of memory */
	anear = 2 * nfrom;
	Near = (NEAR *) G_calloc(anear, sizeof(NEAR));
    }
    else {
	Near = (NEAR *) G_calloc(nfrom, sizeof(NEAR));
    }

    /* Read all cats from 'from' */
    if (!do_all) {
	nfcats = 0;
	for (i = 1; i <= nfrom; i++) {
	    ftype = Vect_read_line(&From, NULL, FCats, i);

	    /* This keeps also categories of areas for future (if area s in from_type) */
	    if (!(ftype & from_type) &&
		(ftype != GV_CENTROID || !(from_type & GV_AREA)))
		continue;

	    Vect_cat_get(FCats, from_field, &fcat);
	    if (fcat < 0)
		continue;
	    Near[nfcats].from_cat = fcat;
	    nfcats++;
	}
	G_debug(1, "%d cats loaded from vector (including duplicates)",
		nfcats);
	/* Sort by cats and remove duplicates */
	qsort((void *)Near, nfcats, sizeof(NEAR), cmp_near);

	/* remove duplicates */
	for (i = 1; i < nfcats; i++) {
	    if (Near[i].from_cat == Near[i - 1].from_cat) {
		for (j = i; j < nfcats - 1; j++) {
		    Near[j].from_cat = Near[j + 1].from_cat;
		}
		nfcats--;
	    }
	}

	G_debug(1, "%d cats loaded from vector (unique)", nfcats);
    }

    /* Go through all lines in 'from' and find nearest in 'to' for each */
    /* Note: as from_type is restricted to GV_POINTS (for now) everything is simple */

    count = 0;			/* count of distances in 'do_all' mode */
    /* Find nearest lines */
    if (to_type & (GV_POINTS | GV_LINES)) {
	struct line_pnts *LLPoints;

	if (G_projection() == PROJECTION_LL) {
	    LLPoints = Vect_new_line_struct();
	}
	else {
	    LLPoints = NULL;
	}
	G_message(_("Finding nearest feature..."));
	for (fline = 1; fline <= nfrom; fline++) {
	    int tmp_tcat;
	    double tmp_tangle, tangle;
	    double tmp_min = (min < 0 ? 0 : min);
	    double box_edge = 0;
	    int done = FALSE;

	    curr_step = 0;

	    G_debug(3, "fline = %d", fline);
	    G_percent(fline, nfrom, 2);
	    ftype = Vect_read_line(&From, FPoints, FCats, fline);
	    if (!(ftype & from_type))
		continue;

	    Vect_cat_get(FCats, from_field, &fcat);
	    if (fcat < 0 && !do_all)
		continue;

	    while (!done) {
		done = TRUE;

		if (!do_all) {
		    /* enlarge search box until we get a hit */
		    /* the objective is to enlarge the search box
		     * in the first iterations just a little bit
		     * to keep the number of hits low */
		    Vect_reset_boxlist(List);
		    while (curr_step < n_max_steps) {
			box_edge = max_step[curr_step];

			if (box_edge < tmp_min)
			    continue;
			
			box.E = FPoints->x[0] + box_edge;
			box.W = FPoints->x[0] - box_edge;
			box.N = FPoints->y[0] + box_edge;
			box.S = FPoints->y[0] - box_edge;
			box.T = PORT_DOUBLE_MAX;
			box.B = -PORT_DOUBLE_MAX;

			Vect_select_lines_by_box(&To, &box, to_type, List);

			curr_step++;
			if (List->n_values > 0)
			    break;
		    }
		}
		else {
		    box.E = FPoints->x[0] + max;
		    box.W = FPoints->x[0] - max;
		    box.N = FPoints->y[0] + max;
		    box.S = FPoints->y[0] - max;
		    box.T = PORT_DOUBLE_MAX;
		    box.B = -PORT_DOUBLE_MAX;

		    Vect_select_lines_by_box(&To, &box, to_type, List);
		}

		G_debug(3, "  %d lines in box", List->n_values);

		tline = 0;
		dist = PORT_DOUBLE_MAX;
		for (i = 0; i < List->n_values; i++) {
		    tmp_tcat = -1;
		    Vect_read_line(&To, TPoints, TCats, List->id[i]);

		    tseg =
			Vect_line_distance(TPoints, FPoints->x[0], FPoints->y[0],
					   FPoints->z[0], (Vect_is_3d(&From) &&
							   Vect_is_3d(&To)) ?
					   WITH_Z : WITHOUT_Z, &tmp_tx, &tmp_ty,
					   &tmp_tz, &tmp_dist, NULL, &tmp_talong);

		    Vect_point_on_line(TPoints, tmp_talong, NULL, NULL, NULL,
				       &tmp_tangle, NULL);

		    if (tmp_dist > max || tmp_dist < min)
			continue;	/* not in threshold */

		    /* TODO: more cats of the same field */
		    Vect_cat_get(TCats, to_field, &tmp_tcat);
		    if (G_projection() == PROJECTION_LL) {
			/* calculate distances in meters not degrees (only 2D) */
			Vect_reset_line(LLPoints);
			Vect_append_point(LLPoints, FPoints->x[0], FPoints->y[0],
					  FPoints->z[0]);
			Vect_append_point(LLPoints, tmp_tx, tmp_ty, tmp_tz);
			tmp_dist = Vect_line_geodesic_length(LLPoints);
			Vect_reset_line(LLPoints);
			for (k = 0; k < tseg; k++)
			    Vect_append_point(LLPoints, TPoints->x[k],
					      TPoints->y[k], TPoints->z[k]);
			Vect_append_point(LLPoints, tmp_tx, tmp_ty, tmp_tz);
			tmp_talong = Vect_line_geodesic_length(LLPoints);
		    }

		    G_debug(4, "  tmp_dist = %f tmp_tcat = %d", tmp_dist,
			    tmp_tcat);

		    if (do_all) {
			if (anear <= count) {
			    anear += 10 + nfrom / 10;
			    Near = (NEAR *) G_realloc(Near, anear * sizeof(NEAR));
			}
			near = &(Near[count]);

			/* store info about relation */
			near->from_cat = fcat;
			near->to_cat = tmp_tcat;	/* -1 is OK */
			near->dist = tmp_dist;
			near->from_x = FPoints->x[0];
			near->from_y = FPoints->y[0];
			near->from_z = FPoints->z[0];
			near->to_x = tmp_tx;
			near->to_y = tmp_ty;
			near->to_z = tmp_tz;
			near->to_along = tmp_talong;	/* 0 for points */
			near->to_angle = tmp_tangle;
			near->count++;
			count++;
		    }
		    else {
			if (tline == 0 || (tmp_dist < dist)) {
			    tline = List->id[i];
			    tcat = tmp_tcat;
			    dist = tmp_dist;
			    tx = tmp_tx;
			    ty = tmp_ty;
			    tz = tmp_tz;
			    talong = tmp_talong;
			    tangle = tmp_tangle;
			}
		    }
		}

		G_debug(4, "  dist = %f", dist);

		if (!do_all && curr_step < n_max_steps) {
		    /* enlarging the search box is possible */
		    if (tline > 0 && dist > box_edge) {
			/* line found but distance > search edge:
			 * line bbox overlaps with search box, line itself is outside search box */
			done = FALSE;
		    }
		    else if (tline == 0) {
			/* no line within max dist, but search box can still be enlarged */
			done = FALSE;
		    }
		}
		if (done && !do_all && tline > 0) {
		    /* find near by cat */
		    near =
			(NEAR *) bsearch((void *)&fcat, Near, nfcats,
					 sizeof(NEAR), cmp_near);

		    G_debug(4, "  near.from_cat = %d near.count = %d",
			    near->from_cat, near->count);
		    /* store info about relation */
		    if (near->count == 0 || near->dist > dist) {
			near->to_cat = tcat;	/* -1 is OK */
			near->dist = dist;
			near->from_x = FPoints->x[0];
			near->from_y = FPoints->y[0];
			near->from_z = FPoints->z[0];
			near->to_x = tx;
			near->to_y = ty;
			near->to_z = tz;
			near->to_along = talong;	/* 0 for points */
			near->to_angle = tangle;
		    }
		    near->count++;
		}
	    } /* done */
	} /* next feature */
	if (LLPoints) {
	    Vect_destroy_line_struct(LLPoints);
	}
    }

    /* Find nearest areas */
    if (to_type & GV_AREA) {
	
	G_message(_("Finding nearest areas..."));
	for (fline = 1; fline <= nfrom; fline++) {
	    double tmp_min = (min < 0 ? 0 : min);
	    double box_edge = 0;
	    int done = FALSE;
	    
	    curr_step = 0;

	    G_debug(3, "fline = %d", fline);
	    G_percent(fline, nfrom, 2);
	    ftype = Vect_read_line(&From, FPoints, FCats, fline);
	    if (!(ftype & from_type))
		continue;

	    Vect_cat_get(FCats, from_field, &fcat);
	    if (fcat < 0 && !do_all)
		continue;

	    while (!done) {
		done = TRUE;

		if (!do_all) {
		    /* enlarge search box until we get a hit */
		    /* the objective is to enlarge the search box
		     * in the first iterations just a little bit
		     * to keep the number of hits low */
		    Vect_reset_boxlist(List);
		    while (curr_step < n_max_steps) {
			box_edge = max_step[curr_step];

			if (box_edge < tmp_min)
			    continue;
			
			box.E = FPoints->x[0] + box_edge;
			box.W = FPoints->x[0] - box_edge;
			box.N = FPoints->y[0] + box_edge;
			box.S = FPoints->y[0] - box_edge;
			box.T = PORT_DOUBLE_MAX;
			box.B = -PORT_DOUBLE_MAX;

			Vect_select_areas_by_box(&To, &box, List);

			curr_step++;
			if (List->n_values > 0)
			    break;
		    }
		}
		else {
		    box.E = FPoints->x[0] + max;
		    box.W = FPoints->x[0] - max;
		    box.N = FPoints->y[0] + max;
		    box.S = FPoints->y[0] - max;
		    box.T = PORT_DOUBLE_MAX;
		    box.B = -PORT_DOUBLE_MAX;

		    Vect_select_areas_by_box(&To, &box, List);
		}

		G_debug(4, "%d areas selected by box", List->n_values);

		/* For each area in box check the distance */
		tarea = 0;
		dist = PORT_DOUBLE_MAX;
		for (i = 0; i < List->n_values; i++) {
		    int tmp_tcat;

		    area = List->id[i];
		    G_debug(4, "%d: area %d", i, area);
		    Vect_get_area_points(&To, area, TPoints);

		    /* Find the distance to this area */
		    if (Vect_point_in_area(FPoints->x[0], FPoints->y[0], &To, area, &List->box[i])) {	/* in area */
			tmp_dist = 0;
			tmp_tx = FPoints->x[0];
			tmp_ty = FPoints->y[0];
		    }
		    else if (Vect_point_in_poly(FPoints->x[0], FPoints->y[0], TPoints) > 0) {	/* in isle */
			nisles = Vect_get_area_num_isles(&To, area);
			for (j = 0; j < nisles; j++) {
			    double tmp2_dist, tmp2_tx, tmp2_ty;

			    isle = Vect_get_area_isle(&To, area, j);
			    Vect_get_isle_points(&To, isle, TPoints);
			    Vect_line_distance(TPoints, FPoints->x[0],
					       FPoints->y[0], FPoints->z[0],
					       WITHOUT_Z, &tmp2_tx, &tmp2_ty,
					       NULL, &tmp2_dist, NULL, NULL);

			    if (j == 0 || tmp2_dist < tmp_dist) {
				tmp_dist = tmp2_dist;
				tmp_tx = tmp2_tx;
				tmp_ty = tmp2_ty;
			    }
			}
		    }
		    else {		/* outside area */
			Vect_line_distance(TPoints, FPoints->x[0], FPoints->y[0],
					   FPoints->z[0], WITHOUT_Z, &tmp_tx,
					   &tmp_ty, NULL, &tmp_dist, NULL, NULL);

		    }
		    if (tmp_dist > max || tmp_dist < min)
			continue;	/* not in threshold */
		    Vect_get_area_cats(&To, area, TCats);
		    tmp_tcat = -1;
		    /* TODO: all cats of given field ? */
		    for (j = 0; j < TCats->n_cats; j++) {
			if (TCats->field[j] == to_field) {
			    if (tmp_tcat >= 0)
				G_warning(_("More cats found in to_layer (area=%d)"),
					  area);
			    tmp_tcat = TCats->cat[j];
			}
		    }

		    G_debug(4, "  tmp_dist = %f tmp_tcat = %d", tmp_dist,
			    tmp_tcat);

		    if (do_all) {
			if (anear <= count) {
			    anear += 10 + nfrom / 10;
			    Near = (NEAR *) G_realloc(Near, anear * sizeof(NEAR));
			}
			near = &(Near[count]);

			/* store info about relation */
			near->from_cat = fcat;
			near->to_cat = tmp_tcat;	/* -1 is OK */
			near->dist = tmp_dist;
			near->from_x = FPoints->x[0];
			near->from_y = FPoints->y[0];
			near->to_x = tmp_tx;
			near->to_y = tmp_ty;
			near->to_along = 0;	/* nonsense for areas */
			near->to_angle = 0;	/* not supported for areas */
			near->count++;
			count++;
		    }
		    else if (tarea == 0 || tmp_dist < dist) {
			tarea = area;
			tcat = tmp_tcat;
			dist = tmp_dist;
			tx = tmp_tx;
			ty = tmp_ty;
		    }
		}

		if (!do_all && curr_step < n_max_steps) {
		    /* enlarging the search box is possible */
		    if (tarea > 0 && dist > box_edge) {
			/* area found but distance > search edge:
			 * area bbox overlaps with search box, area itself is outside search box */
			done = FALSE;
		    }
		    else if (tarea == 0) {
			/* no area within max dist, but search box can still be enlarged */
			done = FALSE;
		    }
		}
		if (done && !do_all && tarea > 0) {
		    /* find near by cat */
		    near =
			(NEAR *) bsearch((void *)&fcat, Near, nfcats,
					 sizeof(NEAR), cmp_near);

		    G_debug(4, "near.from_cat = %d near.count = %d dist = %f",
			    near->from_cat, near->count, near->dist);

		    /* store info about relation */
		    if (near->count == 0 || near->dist > dist) {
			near->to_cat = tcat;	/* -1 is OK */
			near->dist = dist;
			near->from_x = FPoints->x[0];
			near->from_y = FPoints->y[0];
			near->to_x = tx;
			near->to_y = ty;
			near->to_along = 0;	/* nonsense for areas */
			near->to_angle = 0;	/* not supported for areas */
		    }
		    near->count++;
		}
	    } /* done */
	} /* next feature */
    }

    G_debug(3, "count = %d", count);

    /* Update database / print to stdout / create output map */
    if (flag.print->answer) {	/* print header */
	fprintf(stdout, "from_cat");
	if (do_all)
	    fprintf(stdout, "|to_cat");
	i = 0;
	while (Upload[i].upload != END) {
	    fprintf(stdout, "|%s", Upload[i].column);
	    i++;
	}
	fprintf(stdout, "\n");
    }
    else if (do_all && opt.table->answer) {	/* create new table */
	db_set_string(&stmt, "create table ");
	db_append_string(&stmt, opt.table->answer);
	db_append_string(&stmt, " (from_cat integer");

	j = 0;
	while (Upload[j].upload != END) {
	    db_append_string(&stmt, ", ");

	    switch (Upload[j].upload) {
	    case CAT:
		sprintf(buf2, "%s integer", Upload[j].column);
		break;
	    case DIST:
	    case FROM_X:
	    case FROM_Y:
	    case TO_X:
	    case TO_Y:
	    case FROM_ALONG:
	    case TO_ALONG:
	    case TO_ANGLE:
		sprintf(buf2, "%s double precision", Upload[j].column);
	    }
	    db_append_string(&stmt, buf2);
	    j++;
	}
	db_append_string(&stmt, " )");
	G_debug(3, "SQL: %s", db_get_string(&stmt));

	if (db_execute_immediate(driver, &stmt) != DB_OK)
	    G_fatal_error(_("Unable to create table: '%s'"),
			  db_get_string(&stmt));

	if (db_grant_on_table(driver, opt.table->answer, DB_PRIV_SELECT,
			      DB_GROUP | DB_PUBLIC) != DB_OK)
	    G_fatal_error(_("Unable to grant privileges on table <%s>"),
			  opt.table->answer);

    }
    else if (!do_all) {		/* read existing cats from table */
	ncatexist =
	    db_select_int(driver, Fi->table, Fi->key, NULL, &catexist);
	G_debug(1, "%d cats selected from the table", ncatexist);
    }
    update_ok = update_err = update_exist = update_notexist = update_dupl =
	update_notfound = 0;

    if (!do_all) {
	count = nfcats;
    }
    else {
	qsort((void *)Near, count, sizeof(NEAR), cmp_near_to);
    }

    if (driver)
	db_begin_transaction(driver);

    /* select 'to' attributes */
    if (opt.to_column->answer) {
	int nrec;

	db_CatValArray_init(&cvarr);
	nrec = db_select_CatValArray(to_driver, toFi->table, toFi->key,
				     opt.to_column->answer, NULL, &cvarr);
	G_debug(3, "selected values = %d", nrec);

	if (cvarr.ctype == DB_C_TYPE_DATETIME) {
	    G_warning(_("DATETIME type not yet supported, no attributes will be uploaded"));
	}
	db_close_database_shutdown_driver(to_driver);
    }

    if (!(flag.print->answer || (do_all && !opt.table->answer))) /* no printing */
	G_message("Update vector attributes...");

    for (i = 0; i < count; i++) {
	dbCatVal *catval = 0;

	if (!(flag.print->answer || (do_all && !opt.table->answer))) /* no printing */
	    G_percent(i, count, 1);

	/* Write line connecting nearest points */
	if (Outp != NULL) {
	    Vect_reset_line(FPoints);
	    Vect_reset_cats(FCats);

	    Vect_append_point(FPoints, Near[i].from_x, Near[i].from_y, 0);

	    if (Near[i].dist == 0) {
		Vect_write_line(Outp, GV_POINT, FPoints, FCats);
	    }
	    else {
		Vect_append_point(FPoints, Near[i].to_x, Near[i].to_y, 0);
		Vect_write_line(Outp, GV_LINE, FPoints, FCats);
	    }

	}

	if (Near[i].count > 1)
	    update_dupl++;
	if (Near[i].count == 0)
	    update_notfound++;

	if (opt.to_column->answer && Near[i].count > 0) {
	    db_CatValArray_get_value(&cvarr, Near[i].to_cat, &catval);
	}

	if (flag.print->answer || (do_all && !opt.table->answer)) {	/* print only */
	    /*
	       input and output is the same &&
	       calculate distances &&
	       only one upload option given ->
	       print as a matrix
	     */
	    if (print_as_matrix) {
		if (i == 0) {
		    for (j = 0; j < nfrom; j++) {
			if (j == 0)
			    fprintf(stdout, " ");
			fprintf(stdout, "|%d", Near[j].to_cat);
		    }
		    fprintf(stdout, "\n");
		}
		if (i % nfrom == 0) {
		    fprintf(stdout, "%d", Near[i].from_cat);
		    for (j = 0; j < nfrom; j++) {
			print_upload(Near, Upload, i + j, &cvarr, catval);
		    }
		    fprintf(stdout, "\n");
		}
	    }
	    else {
		fprintf(stdout, "%d", Near[i].from_cat);
		if (do_all)
		    fprintf(stdout, "|%d", Near[i].to_cat);
		print_upload(Near, Upload, i, &cvarr, catval);
		fprintf(stdout, "\n");
	    }
	}
	else if (do_all) {		/* insert new record */
	    sprintf(buf1, "insert into %s values ( %d ", opt.table->answer,
		    Near[i].from_cat);
	    db_set_string(&stmt, buf1);

	    j = 0;
	    while (Upload[j].upload != END) {
		db_append_string(&stmt, ",");

		switch (Upload[j].upload) {
		case CAT:
		    sprintf(buf2, " %d", Near[i].to_cat);
		    break;
		case DIST:
		    sprintf(buf2, " %f", Near[i].dist);
		    break;
		case FROM_X:
		    sprintf(buf2, " %f", Near[i].from_x);
		    break;
		case FROM_Y:
		    sprintf(buf2, " %f", Near[i].from_y);
		    break;
		case TO_X:
		    sprintf(buf2, " %f", Near[i].to_x);
		    break;
		case TO_Y:
		    sprintf(buf2, " %f", Near[i].to_y);
		    break;
		case FROM_ALONG:
		    sprintf(buf2, " %f", Near[i].from_along);
		    break;
		case TO_ALONG:
		    sprintf(buf2, " %f", Near[i].to_along);
		    break;
		case TO_ANGLE:
		    sprintf(buf2, " %f", Near[i].to_angle);
		    break;
		case TO_ATTR:
		    if (catval) {
			switch (cvarr.ctype) {
			case DB_C_TYPE_INT:
			    sprintf(buf2, " %d", catval->val.i);
			    break;

			case DB_C_TYPE_DOUBLE:
			    sprintf(buf2, " %.15e", catval->val.d);
			    break;

			case DB_C_TYPE_STRING:
			    db_set_string(&dbstr,
					  db_get_string(catval->val.s));
			    db_double_quote_string(&dbstr);
			    sprintf(buf2, " '%s'", db_get_string(&dbstr));
			    break;

			case DB_C_TYPE_DATETIME:
			    /* TODO: formating datetime */
			    sprintf(buf2, " null");
			    break;
			}
		    }
		    else {
			sprintf(buf2, " null");
		    }
		    break;
		}
		db_append_string(&stmt, buf2);
		j++;
	    }
	    db_append_string(&stmt, " )");
	    G_debug(3, "SQL: %s", db_get_string(&stmt));
	    if (db_execute_immediate(driver, &stmt) == DB_OK) {
		update_ok++;
	    }
	    else {
		update_err++;
	    }
	}
	else {			/* update table */
	    /* check if exists in table */
	    cex =
		(int *)bsearch((void *)&(Near[i].from_cat), catexist,
			       ncatexist, sizeof(int), cmp_exist);
	    if (cex == NULL) {	/* cat does not exist in DB */
		update_notexist++;
		continue;
	    }
	    update_exist++;

	    sprintf(buf1, "update %s set", Fi->table);
	    db_set_string(&stmt, buf1);

	    j = 0;
	    while (Upload[j].upload != END) {
		if (j > 0)
		    db_append_string(&stmt, ",");

		sprintf(buf2, " %s =", Upload[j].column);
		db_append_string(&stmt, buf2);

		if (Near[i].count == 0) {	/* no nearest found */
		    db_append_string(&stmt, " null");
		}
		else {
		    switch (Upload[j].upload) {
		    case CAT:
			if (Near[i].to_cat > 0)
			    sprintf(buf2, " %d", Near[i].to_cat);
			else
			    sprintf(buf2, " null");
			break;
		    case DIST:
			sprintf(buf2, " %f", Near[i].dist);
			break;
		    case FROM_X:
			sprintf(buf2, " %f", Near[i].from_x);
			break;
		    case FROM_Y:
			sprintf(buf2, " %f", Near[i].from_y);
			break;
		    case TO_X:
			sprintf(buf2, " %f", Near[i].to_x);
			break;
		    case TO_Y:
			sprintf(buf2, " %f", Near[i].to_y);
			break;
		    case FROM_ALONG:
			sprintf(buf2, " %f", Near[i].from_along);
			break;
		    case TO_ALONG:
			sprintf(buf2, " %f", Near[i].to_along);
			break;
		    case TO_ANGLE:
			sprintf(buf2, " %f", Near[i].to_angle);
			break;
		    case TO_ATTR:
			if (catval) {
			    switch (cvarr.ctype) {
			    case DB_C_TYPE_INT:
				sprintf(buf2, " %d", catval->val.i);
				break;

			    case DB_C_TYPE_DOUBLE:
				sprintf(buf2, " %.15e", catval->val.d);
				break;

			    case DB_C_TYPE_STRING:
				db_set_string(&dbstr,
					      db_get_string(catval->val.s));
				db_double_quote_string(&dbstr);
				sprintf(buf2, " '%s'", db_get_string(&dbstr));
				break;

			    case DB_C_TYPE_DATETIME:
				/* TODO: formating datetime */
				sprintf(buf2, " null");
				break;
			    }
			}
			else {
			    sprintf(buf2, " null");
			}
			break;
		    }
		    db_append_string(&stmt, buf2);
		}
		j++;
	    }
	    sprintf(buf2, " where %s = %d", Fi->key, Near[i].from_cat);
	    db_append_string(&stmt, buf2);
	    G_debug(2, "SQL: %s", db_get_string(&stmt));
	    if (db_execute_immediate(driver, &stmt) == DB_OK) {
		update_ok++;
	    }
	    else {
		update_err++;
	    }
	}
    }
    G_percent(count, count, 1);

    if (driver)
	db_commit_transaction(driver);

    /* print stats */
    if (update_dupl > 0)
	G_message(_("%d categories with more than 1 feature in vector map <%s>"),
		  update_dupl, opt.from->answer);
    if (update_notfound > 0)
	G_message(_("%d categories - no nearest feature found"),
		  update_notfound);

    if (!flag.print->answer) {
	db_close_database_shutdown_driver(driver);
	db_free_string(&stmt);

	/* print stats */
	if (do_all && opt.table->answer) {
	    G_message(_("%d distances calculated"), count);
	    G_message(_("%d records inserted"), update_ok);
	    if (update_err > 0)
		G_message(_("%d insert errors"), update_err);
	}
	else if (!do_all) {
	    if (nfcats > 0)
		G_verbose_message(_("%d categories read from the map"), nfcats);
	    if (ncatexist > 0)
		G_verbose_message(_("%d categories exist in the table"), ncatexist);
	    if (update_exist > 0)
		G_verbose_message(_("%d categories read from the map exist in the table"),
			  update_exist);
	    if (update_notexist > 0)
		G_verbose_message(_("%d categories read from the map don't exist in the table"),
			  update_notexist);
	    if (update_err > 0)
		G_warning(_("%d update errors"), update_err);

	    G_done_msg(_("%d records updated."), update_ok);

	    G_free(catexist);
	}

	Vect_set_db_updated(&From);
    }

    Vect_close(&From);
    if (Outp != NULL) {
	Vect_build(Outp);
	Vect_close(Outp);
    }

    exit(EXIT_SUCCESS);
}
