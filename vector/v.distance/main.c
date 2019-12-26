
/***************************************************************
 *
 * MODULE:       v.distance
 *
 * AUTHOR(S):    - J.Soimasuo 15.9.1994, University of Joensuu,
 *                 Faculty of Forestry, Finland
 *               - some additions by Markus Neteler (2002)
 *               - updated to 5.7 by Radim Blazek (2003)
 *               - OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *               - speed-up for dmax != 0 by Markus Metz (2010)
 *               - support all features by Markus Metz (2012)
 *               - create a new table for non -a runs by Huidae Cho (2017)
 *
 * PURPOSE:      Calculates distance from a point to nearest feature in vector
 *               layer.
 *
 * COPYRIGHT:    (C) 2002-2017 by the GRASS Development Team
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

/* Supported command lines:
 * from= to= upload= -p			# print
 * from= to= upload= column=		# update the "from" table
 * from= to= output=			# create map
 * from= to= output= upload= -p		# create map & print
 * from= to= output= upload= column=	# create map & update the "from" table
 * from= to= output= upload= column= table=
 *					# create map & table
 * -a from= to= upload= -p		# print for all
 * -a from= to= output=			# create map for all
 * -a from= to= output=	upload= -p	# create map for all & print
 * -a from= to= output= upload= column= table=
 *					# create map for all & create table
 *
 * Unsupported command lines:
 * from= to=				# nothing to do
 * from= to= upload= column= table=	# cannot create table without output
 * -a from= to=				# nothing to do
 *X-a from= to= upload= column=		# cannot update the "from" table
 *					# because #new != #"from"
 * -a from= to= upload= column= table=	# cannot create table without output
 *X-a from= to= output= upload= column=	# cannot update the "from" table
 *					# because #new != #"from"
 *
 * X: manual dependency checks
 */

/* TODO: support all types (lines, boundaries, areas for 'from' (from_type) */

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct {
	struct Option *from, *to, *from_type, *to_type,
	    *from_field, *to_field;
	struct Option *out, *max, *min, *table;
	struct Option *upload, *column, *to_column;
	struct Option *sep;
    } opt;
    struct {
	struct Flag *print, *all;
    } flag;

    int print;			/* -p: print to stdout */
    int create_map;		/* output=: create a new map */
    int create_table;		/* table=: create a new table */
    int update_table;		/* column= !table=: update the "from" table */
    int do_all;			/* -a: calculate from each to each within the threshold */
    int print_as_matrix;	/* only for do_all=TRUE */

    int i, j;
    char *desc;
    struct Map_info From, To, Out, *Outp;
    int from_type, to_type, from_field, to_field, with_z;
    double max, min;
    double *max_step, max_map;
    int n_max_steps, curr_step;
    struct line_pnts *FPoints, *TPoints;
    struct line_cats *FCats, *TCats;
    NEAR *Near, *near;
    int anear;			/* allocated space, used only for do_all */
    UPLOAD *Upload;		/* zero terminated */
    int ftype, ttype, fcat, nfcats, tcat, count, fline, tfeature;
    int nfrom, nfromlines, nfromareas, nto, ntolines, ntoareas;
    int tarea, area, isle, nisles, nlines;
    double fx, fy, fz, falong, fangle, tx, ty, tz, talong, tangle, dist;
    double tmp_fx, tmp_fy, tmp_fz, tmp_falong, tmp_fangle, tmp_dist;
    double tmp_tx, tmp_ty, tmp_tz, tmp_talong, tmp_tangle;
    int geodesic;
    struct field_info *Fi, *toFi;
    dbString stmt, dbstr;
    dbDriver *driver, *to_driver;
    int *catexist, ncatexist, *cex;
    char buf1[2000], buf2[2000], to_attr_sqltype[256];
    int update_ok, update_err, update_exist, update_notexist, update_dupl,
	update_notfound, sqltype;
    struct boxlist *lList, *aList;
    struct bound_box fbox, box;
    dbCatValArray cvarr;
    dbColumn *column;
    char *sep;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("distance"));
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    module->description =
	_("Finds the nearest element in vector map 'to' for elements in vector map 'from'.");

    opt.from = G_define_standard_option(G_OPT_V_INPUT);
    opt.from->key = "from";
    opt.from->label = _("Name of existing vector map (from)");
    opt.from->guisection = _("From");

    opt.from_field = G_define_standard_option(G_OPT_V_FIELD);
    opt.from_field->key = "from_layer";
    opt.from_field->label = _("Layer number or name (from)");
    opt.from_field->guisection = _("From");

    opt.from_type = G_define_standard_option(G_OPT_V_TYPE);
    opt.from_type->key = "from_type";
    opt.from_type->options = "point,line,boundary,centroid,area";
    opt.from_type->answer = "point,line,area";
    opt.from_type->label = _("Feature type (from)");
    opt.from_type->guisection = _("From");

    opt.to = G_define_standard_option(G_OPT_V_INPUT);
    opt.to->key = "to";
    opt.to->label = _("Name of existing vector map (to)");
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
    opt.max->label = _("Maximum distance or -1 for no limit");
    opt.max->description = _("Map units, meters for ll");

    opt.min = G_define_option();
    opt.min->key = "dmin";
    opt.min->type = TYPE_DOUBLE;
    opt.min->required = NO;
    opt.min->answer = "-1";
    opt.min->label = _("Minimum distance or -1 for no limit");
    opt.min->description = _("Map units, meters for ll");

    opt.upload = G_define_option();
    opt.upload->key = "upload";
    opt.upload->type = TYPE_STRING;
    opt.upload->required = NO;
    opt.upload->multiple = YES;
    opt.upload->guisection = _("From");
    opt.upload->options = "cat,dist,to_x,to_y,to_along,to_angle,to_attr";
    opt.upload->description =
	_("Values describing the relation between two nearest features");
    desc = NULL;
    G_asprintf(&desc,
	       "cat;%s;"
	       "dist;%s;"
	       "to_x;%s;"
	       "to_y;%s;"
	       "to_along;%s;"
	       "to_angle;%s;"
	       "to_attr;%s",
	       _("category of the nearest feature"),
	       _("minimum distance to nearest feature"),
	       _("x coordinate of the nearest point on the 'to' feature"),
	       _("y coordinate of the nearest point on the 'to' feature"),
	       _("distance to the nearest point on the 'to' feature along "
		 "that linear feature"),
	       _("angle along the nearest linear feature in the 'to' map, "
		 "measured CCW from the +x axis, in radians, between -Pi and Pi "
		 "inclusive"),
	       _("attribute of nearest feature given by to_column option"));
    /* "from_x - x coordinate of the nearest point on 'from' feature;"
     * "from_y - y coordinate of the nearest point on 'from' feature;"
     * "from_along - distance to the nearest point on 'from' feature along linear feature;"
     * "from_angle - angle between the linear feature in 'to' map and the +x "
     * "axis, at the location of point/centroid in 'from' map, CCW, in "
     * "radians, between -Pi and Pi inclusive;"
     */
    opt.upload->descriptions = desc;

    opt.column = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.column->required = NO;
    opt.column->multiple = YES;
    opt.column->description =
	_("Column name(s) where values specified by 'upload' option will be uploaded");
    opt.column->guisection = _("From");

    opt.to_column = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.to_column->key = "to_column";
    opt.to_column->description =
	_("Column name of nearest feature (used with upload=to_attr)");
    opt.to_column->guisection = _("To");

    opt.table = G_define_standard_option(G_OPT_DB_TABLE);
    opt.table->gisprompt = "new_dbtable,dbtable,dbtable";
    opt.table->description = _("Name for new attribute table");

    opt.sep = G_define_standard_option(G_OPT_F_SEP);
    opt.sep->label = _("Field separator for printing output to stdout");

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
	_("Output may be written to stdout using the '-p' flag "
	  "or uploaded to a new table created by the 'table' option; "
	  "multiple 'upload' options may be used.");

    /* GUI dependency */
    opt.from->guidependency = G_store(opt.from_field->key);
    sprintf(buf1, "%s,%s", opt.to_field->key, opt.to_column->key);
    opt.to->guidependency = G_store(buf1);
    opt.to_field->guidependency = G_store(opt.to_column->key);

    G_option_required(opt.upload, opt.out, NULL);
    G_option_exclusive(opt.column, flag.print, NULL);
    G_option_exclusive(opt.table, flag.print, NULL);
    G_option_requires(opt.upload, flag.print, opt.column, NULL);
    G_option_requires(opt.column, opt.upload, NULL);
    G_option_requires(flag.print, opt.upload, NULL);
    G_option_requires_all(opt.table, opt.out, opt.upload, NULL);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    geodesic = G_projection() == PROJECTION_LL;
    if (geodesic)
	line_distance = Vect_line_geodesic_distance;
    else
	line_distance = Vect_line_distance;

    from_type = Vect_option_to_types(opt.from_type);
    to_type = Vect_option_to_types(opt.to_type);

    max = atof(opt.max->answer);
    min = atof(opt.min->answer);

    print = flag.print->answer;
    create_map = opt.out->answer != NULL;
    create_table = opt.table->answer != NULL;
    update_table = !create_table && opt.column->answer;
    do_all = flag.all->answer;

    if (do_all && update_table)
	G_fatal_error(_("Updating the from= table is not supported with -a"));

    /* Read upload and column options */
    /* count */
    i = 0;
    while (opt.upload->answer && opt.upload->answers[i])
	i++;

    /* -a !table= upload=one_value from=map to=map: print as matrix */
    print_as_matrix = do_all && !create_table && i == 1 &&
		      strcmp(opt.from->answer, opt.to->answer) == 0;

    /* TODO: Known issue. Segmentation fault on print_as_matrix with dmin= or
     * dmax= because count may not be nfrom^2. Needs to populate Near[] fully
     * even if some near features are not found */
    if (print_as_matrix && (min >= 0 || max >= 0))
	G_fatal_error(_("Printing distance matrix is not supported with dmin= or dmax="));

    /* alloc */
    Upload = (UPLOAD *) G_calloc(i + 1, sizeof(UPLOAD));
    /* read upload */
    i = 0;
    while (opt.upload->answer && opt.upload->answers[i]) {
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

	/* store default column names if column= is not given */
	if (!opt.column->answer)
	    Upload[i].column = G_store(opt.upload->answers[i]);

	i++;
    }
    Upload[i].upload = END;
    /* read columns */
    i = 0;
    while (opt.column->answer && opt.column->answers[i]) {
	if (Upload[i].upload == END) {
	    G_warning(_("Too many column names"));
	    break;
	}
	Upload[i].column = G_store(opt.column->answers[i]);
	i++;
    }
    if (opt.column->answer && Upload[i].upload != END)
	G_fatal_error(_("Not enough column names"));

    sep = G_option_to_separator(opt.sep);

    /* Open 'from' vector */
    Vect_set_open_level(2);
    if (Vect_open_old2(&From, opt.from->answer, "", opt.from_field->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), opt.from->answer);

    from_field = Vect_get_field_number(&From, opt.from_field->answer);
    /* in 3D features may not be found */
    if (Vect_is_3d(&From))
        G_warning(_("Input vector map <%s> is 3D"), opt.from->answer);

    nfromlines = Vect_get_num_primitives(&From, from_type);
    nfromareas = 0;
    if (from_type & GV_AREA)
	nfromareas = Vect_get_num_areas(&From);

    nfrom = nfromlines + nfromareas;
    if (nfrom < 1) {
	const char *name = Vect_get_full_name(&From);
	Vect_close(&From);
	G_fatal_error(_("No features of selected type found in <%s>"), name);
    }

    /* Open 'to' vector */
    Vect_set_open_level(2);
    if (Vect_open_old2(&To, opt.to->answer, "", opt.to_field->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), opt.to->answer);
    /* in 3D features may not be found */
    if (Vect_is_3d(&To))
        G_warning(_("Input vector map <%s> is 3D"), opt.to->answer);

    ntolines = Vect_get_num_primitives(&To, to_type);
    ntoareas = 0;
    if (to_type & GV_AREA)
	ntoareas = Vect_get_num_areas(&To);

    nto = ntolines + ntoareas;
    if (nto < 1) {
	const char *name = Vect_get_full_name(&To);
	Vect_close(&From);
	Vect_close(&To);
	G_fatal_error(_("No features of selected type found in <%s>"), name);
    }
    with_z = (Vect_is_3d(&From) && Vect_is_3d(&To));

    to_field = Vect_get_field_number(&To, opt.to_field->answer);

    /* Open output vector */
    if (create_map) {
	if (Vect_open_new(&Out, opt.out->answer, WITHOUT_Z) < 0)
	    G_fatal_error(_("Unable to create vector map <%s>"),
			    opt.out->answer);

	Vect_hist_command(&Out);
	Outp = &Out;
    }
    else {
	Outp = NULL;
    }

    /* TODO: add maxdist = -1 to Vect_select_ !!! */
    /* Calc maxdist */
    n_max_steps = 1;
    max_map = max;
    if (max != 0) {
	double dx, dy, dz;

	Vect_get_map_box(&From, &fbox);
	Vect_get_map_box(&To, &box);

	Vect_box_extend(&fbox, &box);

	dx = fbox.E - fbox.W;
	dy = fbox.N - fbox.S;

	if (Vect_is_3d(&From))
	    dz = fbox.T - fbox.B;
	else
	    dz = 0.0;

	max_map = sqrt(dx * dx + dy * dy + dz * dz);
	if (max < 0) {
	    if (geodesic)
		max = PORT_DOUBLE_MAX;
	    else
		max = max_map;
	}

	/* how to determine a reasonable number of steps to increase the search
	 * box? */
	/* with max > 0 but max <<< tmp_max, 2 steps are sufficient, first 0
	 * then max a reasonable number of steps also depends on the number of
	 * features in To e.g. only one area in To, no need to step */

	if (geodesic)
	    n_max_steps = sqrt(nto);
	else
	    n_max_steps = sqrt(nto) * max / max_map;
	/* max 9 steps from testing */
	if (n_max_steps > 9)
	    n_max_steps = 9;
	if (n_max_steps < 3)
	    n_max_steps = 3;
	if (n_max_steps > nto)
	    n_max_steps = nto;

	G_debug(2, "max = %g", max);
	G_debug(2, "maximum reasonable search distance = %g", max_map);
	G_debug(2, "n 'to' features = %d", nto);
	G_debug(2, "n_max_steps = %d", n_max_steps);

	if (!geodesic)
	    max_map = max;
    }

    if (min > max)
	G_fatal_error(_("dmin can not be larger than dmax"));

    if (n_max_steps > 1) {
	/* set up steps to increase search box */
	max_step = G_malloc(n_max_steps * sizeof(double));
	/* first step always 0 */
	if (geodesic)
	    max_step[0] = 0;
	else
	    max_step[0] = (min < 0 ? 0 : min);

	for (curr_step = 1; curr_step < n_max_steps - 1; curr_step++) {
	    /* for 9 steps, this would be max / [128, 64, 32, 16, 8, 4, 2] */
	    max_step[curr_step] = max_map / (2 << (n_max_steps - 1 - curr_step));
	}
	/* last step always max_map */
	max_step[n_max_steps - 1] = max_map;
	j = 1;
	for (i = 1; i < n_max_steps; i++) {
	    if (max_step[j - 1] < max_step[i]) {
		max_step[j] = max_step[i];
		j++;
	    }
	}
	n_max_steps = j;
	for (i = 0; i < n_max_steps; i++) {
	    G_debug(2, "max step %d: %g", i, max_step[i]);
	}
    }
    else {
	max_step = G_malloc(sizeof(double));
	max_step[0] = max_map;
    }

    /* Open database driver */
    db_init_string(&stmt);
    db_init_string(&dbstr);
    driver = NULL;
    Fi = NULL;
    column = NULL;

    if (update_table) {
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
	/* close db connection */
	db_close_database_shutdown_driver(driver);
	driver = NULL;
    }

    to_driver = NULL;
    toFi = NULL;
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

	/* check if to_column exists and get its SQL type */
	db_get_column(to_driver, toFi->table, opt.to_column->answer, &column);
	if (column) {
            sqltype = db_get_column_sqltype(column);
	    switch(sqltype) {
		case DB_SQL_TYPE_CHARACTER:
		    sprintf(to_attr_sqltype, "VARCHAR(%d)", db_get_column_length(column));
		    break;
		default:
		    sprintf(to_attr_sqltype, "%s", db_sqltype_name(sqltype));
	    }

	    db_free_column(column);
	    column = NULL;
	}
	else {
	    G_fatal_error(_("Column <%s> not found in table <%s>"),
			  opt.to_column->answer, toFi->table);
	}

	/* Check column types */
	if (update_table) {
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

		Fi = Vect_get_field(&From, from_field);
		if (Fi == NULL)
		    G_fatal_error(_("Database connection not defined for layer <%s>"),
				  opt.from_field->answer);

		driver = db_start_driver_open_database(Fi->driver, Fi->database);
		if (driver == NULL)
		    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
				  Fi->database, Fi->driver);

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
		/* close db connection */
		db_close_database_shutdown_driver(driver);
		driver = NULL;
	    }
	}
	/* close db connection */
	db_close_database_shutdown_driver(to_driver);
	to_driver = NULL;
    }

    FPoints = Vect_new_line_struct();
    TPoints = Vect_new_line_struct();
    FCats = Vect_new_cats_struct();
    TCats = Vect_new_cats_struct();
    lList = Vect_new_boxlist(1); /* line list */
    aList = Vect_new_boxlist(1); /* area list */

    /* Allocate space ( may be more than needed (duplicate cats and elements
     * without cats) ) */
    /* Be careful with do_all, it can easily run out of memory */
    anear = nfrom;
    Near = (NEAR *) G_calloc(nfrom, sizeof(NEAR));

    /* Read all cats from 'from' */
    nfcats = 0;
    if (!do_all) {
	nlines = Vect_get_num_lines(&From);
	for (i = 1; i <= nlines; i++) {
	    ftype = Vect_read_line(&From, NULL, FCats, i);

	    /* This keeps also categories of areas for future (if area s in
	     * from_type) */
	    if (!(ftype & from_type) &&
		(ftype != GV_CENTROID || !(from_type & GV_AREA)))
		continue;

	    Vect_cat_get(FCats, from_field, &fcat);
	    if (fcat < 0)
		continue;
	    Near[nfcats].from_cat = fcat;
	    Near[nfcats].dist = -1;
	    Near[nfcats].count = 0;
	    nfcats++;
	}
	G_debug(1, "%d cats loaded from vector (including duplicates)",
		nfcats);

	if (nfcats == 0)
	    G_fatal_error(_("No categories for 'from' for selected type and layer"));

	/* Sort by cats and remove duplicates */
	qsort((void *)Near, nfcats, sizeof(NEAR), cmp_near);

	/* remove duplicates */
	j = 1;
	for (i = 1; i < nfcats; i++) {
	    if (Near[i].from_cat != Near[j - 1].from_cat) {
		Near[j].from_cat = Near[i].from_cat;
		j++;
	    }
	}
	nfcats = j;

	G_debug(1, "%d cats loaded from vector (unique)", nfcats);
    }

    /* Go through all lines in 'from' and find nearest in 'to' for each */
    /* Note: as from_type is restricted to GV_POINTS (for now) everything is
     * simple */

    /* suppress compiler warnings */
    tx = ty = tz = fx = fy = fz = 0;

    count = 0;			/* count of distances in 'do_all' mode */
    /* Find nearest features for 'from' lines */
    if (nfromlines) {
	G_message(_("Finding nearest features..."));

	near = NULL;
	nlines = Vect_get_num_lines(&From);

	G_percent(0, nlines, 4);
	for (fline = 1; fline <= nlines; fline++) {
	    int tmp_tcat;
	    double tmp_min = (min < 0 ? 0 : min);
	    double box_edge = 0;
	    int done = FALSE;

	    if (geodesic)
		tmp_min = 0;

	    curr_step = 0;

	    G_debug(3, "fline = %d", fline);
	    G_percent(fline, nlines, 4);
	    ftype = Vect_get_line_type(&From, fline);
	    if (!(ftype & from_type))
		continue;

	    Vect_read_line(&From, FPoints, FCats, fline);
	    Vect_cat_get(FCats, from_field, &fcat);
	    if (fcat < 0 && !do_all)
		continue;

	    get_line_box(FPoints, &fbox);

	    if (!do_all) {
		/* find near by 'from' cat */
		near = (NEAR *) bsearch((void *)&fcat, Near, nfcats,
				        sizeof(NEAR), cmp_near);
	    }

	    dist = PORT_DOUBLE_MAX; /* distance to nearest 'to' feature */

	    while (!done) {
		done = TRUE;

		tfeature = 0;  /* id of nearest 'to' feature */

		if (!do_all) {
		    /* enlarge search box until we get a hit */
		    /* the objective is to enlarge the search box
		     * in the first iterations just a little bit
		     * to keep the number of hits low */
		    while (curr_step < n_max_steps) {
			box_edge = max_step[curr_step];
			curr_step++;

			if (box_edge < tmp_min)
			    continue;

			box.E = fbox.E + box_edge;
			box.W = fbox.W - box_edge;
			box.N = fbox.N + box_edge;
			box.S = fbox.S - box_edge;
			box.T = PORT_DOUBLE_MAX;
			box.B = -PORT_DOUBLE_MAX;

			if (ntolines)
			    Vect_select_lines_by_box(&To, &box, to_type, lList);
			if (ntoareas)
			    Vect_select_areas_by_box(&To, &box, aList);

			if (lList->n_values > 0 || aList->n_values > 0)
			    break;
		    }
		}
		else {
		    box.E = fbox.E + max_map;
		    box.W = fbox.W - max_map;
		    box.N = fbox.N + max_map;
		    box.S = fbox.S - max_map;
		    box.T = PORT_DOUBLE_MAX;
		    box.B = -PORT_DOUBLE_MAX;

		    if (ntolines)
			Vect_select_lines_by_box(&To, &box, to_type, lList);
		    if (ntoareas)
			Vect_select_areas_by_box(&To, &box, aList);
		}

		for (i = 0; i < lList->n_values; i++) {
		    ttype = Vect_read_line(&To, TPoints, TCats, lList->id[i]);

		    line2line(FPoints, ftype, TPoints, ttype,
		              &tmp_fx, &tmp_fy, &tmp_fz, &tmp_falong, &tmp_fangle,
		              &tmp_tx, &tmp_ty, &tmp_tz, &tmp_talong, &tmp_tangle,
			      &tmp_dist, with_z);

		    if (tmp_dist > max || tmp_dist < min)
			continue;	/* not in threshold */

		    tmp_tcat = -1;
		    /* TODO: all cats of given field ? */
		    for (j = 0; j < TCats->n_cats; j++) {
			if (TCats->field[j] == to_field) {
			    if (tmp_tcat >= 0)
				G_warning(_("More cats found in to_layer (line=%d)"),
					  lList->id[i]);
			    tmp_tcat = TCats->cat[j];
			}
		    }

		    G_debug(4, "  tmp_dist = %f tmp_tcat = %d", tmp_dist,
			    tmp_tcat);

		    if (do_all) {
			if (anear <= count) {
			    anear += 10 + nto / 10;
			    Near = (NEAR *) G_realloc(Near, anear * sizeof(NEAR));
			    if (!Near)
				G_fatal_error(_("Out of memory!"));
			}
			near = &(Near[count]);

			/* store info about relation */
			near->from_cat = fcat;
			near->to_cat = tmp_tcat;	/* -1 is OK */
			near->dist = tmp_dist;
			near->from_x = tmp_fx;
			near->from_y = tmp_fy;
			near->from_z = tmp_fz;
			near->from_along = tmp_falong;	/* 0 for points */
			near->from_angle = tmp_fangle;
			near->to_x = tmp_tx;
			near->to_y = tmp_ty;
			near->to_z = tmp_tz;
			near->to_along = tmp_talong;	/* 0 for points */
			near->to_angle = tmp_tangle;
			near->count++;
			count++;
		    }
		    else {
			if (tfeature == 0 || (tmp_dist < dist)) {
			    tfeature = lList->id[i];
			    tcat = tmp_tcat;
			    dist = tmp_dist;
			    fx = tmp_fx;
			    fy = tmp_fy;
			    fz = tmp_fz;
			    falong = tmp_falong;
			    fangle = tmp_fangle;
			    tx = tmp_tx;
			    ty = tmp_ty;
			    tz = tmp_tz;
			    talong = tmp_talong;
			    tangle = tmp_tangle;
			}
		    }
		}

		G_debug(3, "  %d areas in box", aList->n_values);

		for (i = 0; i < aList->n_values; i++) {
		    /* ignore isles OK ? */
		    if (Vect_get_area_centroid(&To, aList->id[i]) == 0)
			continue;

		    line2area(&To, FPoints, ftype, aList->id[i], &aList->box[i],
		              &tmp_fx, &tmp_fy, &tmp_fz, &tmp_falong, &tmp_fangle,
		              &tmp_tx, &tmp_ty, &tmp_tz, &tmp_talong, &tmp_tangle,
			      &tmp_dist, with_z);

		    if (tmp_dist > max || tmp_dist < min)
			continue;	/* not in threshold */

		    /* TODO: more cats of the same field */
		    Vect_get_area_cats(&To, aList->id[i], TCats);
		    tmp_tcat = -1;

		    /* TODO: all cats of given field ? */
		    for (j = 0; j < TCats->n_cats; j++) {
			if (TCats->field[j] == to_field) {
			    if (tmp_tcat >= 0)
				G_warning(_("More cats found in to_layer (area=%d)"),
					  aList->id[i]);
			    tmp_tcat = TCats->cat[j];
			}
		    }

		    G_debug(4, "  tmp_dist = %f tmp_tcat = %d", tmp_dist,
			    tmp_tcat);

		    if (do_all) {
			if (anear <= count) {
			    anear += 10 + nto / 10;
			    Near = (NEAR *) G_realloc(Near, anear * sizeof(NEAR));
			    if (!Near)
				G_fatal_error(_("Out of memory!"));
			}
			near = &(Near[count]);

			/* store info about relation */
			near->from_cat = fcat;
			near->to_cat = tmp_tcat;	/* -1 is OK */
			near->dist = tmp_dist;
			near->from_x = tmp_fx;
			near->from_y = tmp_fy;
			near->from_z = tmp_fz;
			near->from_along = tmp_falong;	/* 0 for points */
			near->from_angle = tmp_fangle;
			near->to_x = tmp_tx;
			near->to_y = tmp_ty;
			near->to_z = tmp_tz;
			near->to_along = tmp_talong;	/* 0 for points */
			near->to_angle = tmp_tangle;
			near->count++;
			count++;
		    }
		    else {
			if (tfeature == 0 || (tmp_dist < dist)) {
			    tfeature = aList->id[i];
			    tcat = tmp_tcat;
			    dist = tmp_dist;
			    fx = tmp_fx;
			    fy = tmp_fy;
			    fz = tmp_fz;
			    falong = tmp_falong;
			    fangle = tmp_fangle;
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
		    double dist_map = dist;

		    if (geodesic && tfeature > 0) {
			double dx = fx - tx;
			double dy = fy - ty;
			double dz = fz - tz;

			dist_map = sqrt(dx * dx + dy * dy + dz * dz);
		    }
		    /* enlarging the search box is possible */
		    if (tfeature > 0 && dist_map > box_edge) {
			/* line found but distance > search edge:
			 * line bbox overlaps with search box, line itself is
			 * outside search box */
			done = FALSE;
		    }
		    else if (tfeature == 0) {
			/* no line within max dist, but search box can still be
			 * enlarged */
			done = FALSE;
		    }
		}
		if (done && !do_all && tfeature > 0) {

		    G_debug(4, "  near->from_cat = %d near->count = %d",
			    near->from_cat, near->count);
		    /* store info about relation */
		    if (near->count == 0 || near->dist > dist) {
			near->to_cat = tcat;	/* -1 is OK */
			near->dist = dist;
			near->from_x = fx;
			near->from_y = fy;
			near->from_z = fz;
			near->from_along = falong;	/* 0 for points */
			near->from_angle = fangle;
			near->to_x = tx;
			near->to_y = ty;
			near->to_z = tz;
			near->to_along = talong;	/* 0 for points */
			near->to_angle = tangle;
		    }
		    near->count++;
		}
	    } /* done searching 'to' */
	} /* next from feature */
    }

    /* Find nearest features for 'from' areas */
    /* the code is pretty much identical to the one for lines */
    if (nfromareas) {

	near = NULL;

	G_message(_("Finding nearest features for areas..."));
	G_percent(0, nfromareas, 2);
	for (area = 1; area <= nfromareas; area++) {
	    int tmp_tcat;
	    double tmp_min = (min < 0 ? 0 : min);
	    double box_edge = 0;
	    int done = FALSE;

	    if (geodesic)
		tmp_min = 0;

	    curr_step = 0;

	    G_debug(3, "farea = %d", area);
	    G_percent(area, nfromareas, 2);

	    if (Vect_get_area_cats(&From, area, FCats) == 1)
		/* ignore isles OK ? */
		continue;

	    Vect_cat_get(FCats, from_field, &fcat);
	    if (fcat < 0 && !do_all)
		continue;

	    Vect_get_area_box(&From, area, &fbox);
	    Vect_reset_line(FPoints);

	    if (!do_all) {
		/* find near by 'from' cat */
		near = (NEAR *) bsearch((void *)&fcat, Near, nfcats,
				        sizeof(NEAR), cmp_near);
	    }

	    dist = PORT_DOUBLE_MAX; /* distance to nearest 'to' feature */

	    while (!done) {
		done = TRUE;

		tfeature = 0;  /* id of nearest 'to' feature */

		if (!do_all) {
		    /* enlarge search box until we get a hit */
		    /* the objective is to enlarge the search box
		     * in the first iterations just a little bit
		     * to keep the number of hits low */
		    while (curr_step < n_max_steps) {
			box_edge = max_step[curr_step];
			curr_step++;

			if (box_edge < tmp_min)
			    continue;

			box.E = fbox.E + box_edge;
			box.W = fbox.W - box_edge;
			box.N = fbox.N + box_edge;
			box.S = fbox.S - box_edge;
			box.T = PORT_DOUBLE_MAX;
			box.B = -PORT_DOUBLE_MAX;

			if (ntolines)
			    Vect_select_lines_by_box(&To, &box, to_type, lList);
			if (ntoareas)
			    Vect_select_areas_by_box(&To, &box, aList);

			if (lList->n_values > 0 || aList->n_values > 0)
			    break;
		    }
		}
		else {
		    box.E = fbox.E + max_map;
		    box.W = fbox.W - max_map;
		    box.N = fbox.N + max_map;
		    box.S = fbox.S - max_map;
		    box.T = PORT_DOUBLE_MAX;
		    box.B = -PORT_DOUBLE_MAX;

		    if (ntolines)
			Vect_select_lines_by_box(&To, &box, to_type, lList);
		    if (ntoareas)
			Vect_select_areas_by_box(&To, &box, aList);
		}

		G_debug(3, "  %d lines in box", lList->n_values);

		for (i = 0; i < lList->n_values; i++) {
		    ttype = Vect_read_line(&To, TPoints, TCats, lList->id[i]);

		    /* area to line */
		    line2area(&From, TPoints, ttype, area, &fbox,
		              &tmp_tx, &tmp_ty, &tmp_tz, &tmp_talong, &tmp_tangle,
		              &tmp_fx, &tmp_fy, &tmp_fz, &tmp_falong, &tmp_fangle,
			      &tmp_dist, with_z);

		    if (tmp_dist > max || tmp_dist < min)
			continue;	/* not in threshold */

		    tmp_tcat = -1;
		    /* TODO: all cats of given field ? */
		    for (j = 0; j < TCats->n_cats; j++) {
			if (TCats->field[j] == to_field) {
			    if (tmp_tcat >= 0)
				G_warning(_("More cats found in to_layer (line=%d)"),
					  lList->id[i]);
			    tmp_tcat = TCats->cat[j];
			}
		    }

		    G_debug(4, "  tmp_dist = %f tmp_tcat = %d", tmp_dist,
			    tmp_tcat);

		    if (do_all) {
			if (anear <= count) {
			    anear += 10 + nto / 10;
			    Near = (NEAR *) G_realloc(Near, anear * sizeof(NEAR));
			    if (!Near)
				G_fatal_error(_("Out of memory!"));
			}
			near = &(Near[count]);

			/* store info about relation */
			near->from_cat = fcat;
			near->to_cat = tmp_tcat;	/* -1 is OK */
			near->dist = tmp_dist;
			near->from_x = tmp_fx;
			near->from_y = tmp_fy;
			near->from_z = tmp_fz;
			near->from_along = tmp_falong;	/* 0 for points */
			near->from_angle = tmp_fangle;
			near->to_x = tmp_tx;
			near->to_y = tmp_ty;
			near->to_z = tmp_tz;
			near->to_along = tmp_talong;	/* 0 for points */
			near->to_angle = tmp_tangle;
			near->count++;
			count++;
		    }
		    else {
			if (tfeature == 0 || (tmp_dist < dist)) {
			    tfeature = lList->id[i];
			    tcat = tmp_tcat;
			    dist = tmp_dist;
			    fx = tmp_fx;
			    fy = tmp_fy;
			    fz = tmp_fz;
			    falong = tmp_falong;
			    fangle = tmp_fangle;
			    tx = tmp_tx;
			    ty = tmp_ty;
			    tz = tmp_tz;
			    talong = tmp_talong;
			    tangle = tmp_tangle;
			}
		    }
		}

		G_debug(3, "  %d areas in box", aList->n_values);

		/* For each area in box check the distance */
		for (i = 0; i < aList->n_values; i++) {
		    int poly;

		    tarea = aList->id[i];
		    G_debug(4, "%d: 'to' area id %d", i, tarea);

		    /* ignore isles OK ? */
		    if (Vect_get_area_centroid(&To, tarea) == 0)
			continue;

		    Vect_get_area_points(&To, tarea, TPoints);

		    ttype = GV_BOUNDARY;

		    /* Find the distance of the outer ring of 'to' area
		     * to 'from' area */
		    poly = line2area(&From, TPoints, ttype, area, &fbox,
		              &tmp_tx, &tmp_ty, &tmp_tz, &tmp_talong, &tmp_tangle,
		              &tmp_fx, &tmp_fy, &tmp_fz, &tmp_falong, &tmp_fangle,
			      &tmp_dist, with_z);

		    if (poly == 3) {
			/* 'to' outer ring is outside 'from' area,
			 * check if 'from' area is inside 'to' area */
			poly = 0;
			/* boxes must overlap */
			if (Vect_box_overlap(&fbox, &aList->box[i])) {
			    if (FPoints->n_points == 0)
				Vect_get_area_points(&From, area, FPoints);
			    for (j = 0; j < FPoints->n_points; j++) {
				poly = Vect_point_in_poly(FPoints->x[j], FPoints->y[j], TPoints);
				if (poly)
				    break;
			    }
			}
			if (poly) {
			    /* 'from' area is (partially) inside 'to' area,
			     * get distance to 'to' area */
			    if (FPoints->n_points == 0)
				Vect_get_area_points(&From, area, FPoints);
			    poly = line2area(&To, FPoints, ttype, tarea, &aList->box[i],
				      &tmp_fx, &tmp_fy, &tmp_fz, &tmp_falong, &tmp_fangle,
				      &tmp_tx, &tmp_ty, &tmp_tz, &tmp_talong, &tmp_tangle,
				      &tmp_dist, with_z);

			    /* inside isle ? */
			    poly = poly == 2;
			}
			if (poly == 1) {
			    double tmp2_tx, tmp2_ty, tmp2_tz, tmp2_talong, tmp2_tangle;
			    double tmp2_fx, tmp2_fy, tmp2_fz, tmp2_falong, tmp2_fangle;
			    double tmp2_dist;

			    /* 'from' area is (partially) inside 'to' area,
			     * get distance to 'to' isles */
			    nisles = Vect_get_area_num_isles(&To, tarea);
			    for (j = 0; j < nisles; j++) {
				isle = Vect_get_area_isle(&To, tarea, j);
				Vect_get_isle_points(&To, isle, TPoints);

				line2area(&From, TPoints, ttype, area, &fbox,
					  &tmp2_tx, &tmp2_ty, &tmp2_tz, &tmp2_talong, &tmp2_tangle,
					  &tmp2_fx, &tmp2_fy, &tmp2_fz, &tmp2_falong, &tmp2_fangle,
					  &tmp2_dist, with_z);

				if (tmp2_dist < tmp_dist) {
				    tmp_dist = tmp2_dist;
				    tmp_fx = tmp2_fx;
				    tmp_fy = tmp2_fy;
				    tmp_fz = tmp2_fz;
				    tmp_falong = tmp2_falong;
				    tmp_fangle = tmp2_fangle;
				    tmp_tx = tmp2_tx;
				    tmp_ty = tmp2_ty;
				    tmp_tz = tmp2_tz;
				    tmp_talong = tmp2_talong;
				    tmp_tangle = tmp2_tangle;
				}
			    }
			}
		    }

		    if (tmp_dist > max || tmp_dist < min)
			continue;	/* not in threshold */
		    Vect_get_area_cats(&To, tarea, TCats);
		    tmp_tcat = -1;
		    /* TODO: all cats of given field ? */
		    for (j = 0; j < TCats->n_cats; j++) {
			if (TCats->field[j] == to_field) {
			    if (tmp_tcat >= 0)
				G_warning(_("More cats found in to_layer (area=%d)"),
					  tarea);
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
			near->from_x = tmp_fx;
			near->from_y = tmp_fy;
			near->from_z = tmp_fz;
			near->from_along = tmp_falong;	/* 0 for points */
			near->from_angle = tmp_fangle;
			near->to_x = tmp_tx;
			near->to_y = tmp_ty;
			near->to_z = tmp_tz;
			near->to_along = tmp_talong;	/* 0 for points */
			near->to_angle = tmp_tangle;
			near->count++;
			count++;
		    }
		    else {
			if (tfeature == 0 || tmp_dist < dist) {
			    tfeature = tarea;
			    tcat = tmp_tcat;
			    dist = tmp_dist;
			    fx = tmp_fx;
			    fy = tmp_fy;
			    fz = tmp_fz;
			    falong = tmp_falong;
			    fangle = tmp_fangle;
			    tx = tmp_tx;
			    ty = tmp_ty;
			    tz = tmp_tz;
			    talong = tmp_talong;
			    tangle = tmp_tangle;
			}
		    }
		}

		if (!do_all && curr_step < n_max_steps) {
		    double dist_map = dist;

		    if (geodesic && tfeature > 0) {
			double dx = fx - tx;
			double dy = fy - ty;
			double dz = fz - tz;

			dist_map = sqrt(dx * dx + dy * dy + dz * dz);
		    }
		    /* enlarging the search box is possible */
		    if (tfeature > 0 && dist_map > box_edge) {
			/* area found but distance > search edge:
			 * area bbox overlaps with search box, area itself is
			 * outside search box */
			done = FALSE;
		    }
		    else if (tfeature == 0) {
			/* no area within max dist, but search box can still be
			 * enlarged */
			done = FALSE;
		    }
		}
		if (done && !do_all && tfeature > 0) {

		    G_debug(4, "near.from_cat = %d near.count = %d dist = %f",
			    near->from_cat, near->count, near->dist);

		    /* store info about relation */
		    if (near->count == 0 || near->dist > dist) {
			near->to_cat = tcat;	/* -1 is OK */
			near->dist = dist;
			near->from_x = fx;
			near->from_y = fy;
			near->from_z = fz;
			near->from_along = falong;
			near->from_angle = fangle;
			near->to_x = tx;
			near->to_y = ty;
			near->to_z = tz;
			near->to_along = talong;
			near->to_angle = tangle;
		    }
		    near->count++;
		}
	    } /* done */
	} /* next feature */
    }

    G_debug(3, "count = %d", count);

    /* select 'to' attributes */
    if (opt.to_column->answer) {
	int nrec;

	to_driver =
	    db_start_driver_open_database(toFi->driver, toFi->database);
	if (to_driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  toFi->database, toFi->driver);

	db_CatValArray_init(&cvarr);
	nrec = db_select_CatValArray(to_driver, toFi->table, toFi->key,
				     opt.to_column->answer, NULL, &cvarr);
	G_debug(3, "selected values = %d", nrec);

	if (cvarr.ctype == DB_C_TYPE_DATETIME) {
	    G_warning(_("DATETIME type not yet supported, no attributes will be uploaded"));
	}
	db_close_database_shutdown_driver(to_driver);
	to_driver = NULL;
    }

    /* open from driver */
    if (update_table) {
	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
    }
    else if (create_table) {
	driver = db_start_driver_open_database(NULL, NULL);
	if (driver == NULL)
	    G_fatal_error(_("Unable to open default database"));
    }

    update_ok = update_err = update_exist = update_notexist = update_dupl =
	update_notfound = ncatexist = 0;

    /* Update database / print to stdout / create output map */
    if (print) {	/* print header */
	fprintf(stdout, "from_cat");
	if (do_all)
	    fprintf(stdout, "%sto_cat", sep);
	i = 0;
	while (Upload[i].upload != END) {
	    fprintf(stdout, "%s%s", sep, Upload[i].column);
	    i++;
	}
	fprintf(stdout, "\n");
    }
    else if (create_table) {	/* create new table */
	db_set_string(&stmt, "create table ");
	db_append_string(&stmt, opt.table->answer);
	if (Outp)
	    db_append_string(&stmt, " (cat integer,from_cat integer");
	else
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
                break;
	    case TO_ATTR:
		sprintf(buf2, "%s %s", Upload[j].column, to_attr_sqltype);
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
    else if (update_table) {	/* read existing cats from table */
	ncatexist =
	    db_select_int(driver, Fi->table, Fi->key, NULL, &catexist);
	G_debug(1, "%d cats selected from the table", ncatexist);
    }

    if (!do_all) {
	count = nfcats;
    }
    else {
	qsort((void *)Near, count, sizeof(NEAR), cmp_near_to);
    }

    if (driver)
	db_begin_transaction(driver);

    if (!print) /* no printing */
	G_message("Update vector attributes...");

    for (i = 0; i < count; i++) {
	dbCatVal *catval = 0;

	if (!print) /* no printing */
	    G_percent(i, count, 1);

	/* Write line connecting nearest points */
	if (Near[i].count > 0 && Outp != NULL) {
	    Vect_reset_line(FPoints);
	    Vect_reset_cats(FCats);

	    Vect_cat_set(FCats, 1, i);

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

	if (print) {	/* print only */
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
			fprintf(stdout, "%s%d", sep, Near[j].to_cat);
		    }
		    fprintf(stdout, "\n");
		}
		if (i % nfrom == 0) {
		    fprintf(stdout, "%d", Near[i].from_cat);
		    for (j = 0; j < nfrom; j++) {
			print_upload(Near, Upload, i + j, &cvarr, catval, sep);
		    }
		    fprintf(stdout, "\n");
		}
	    }
	    else {
		fprintf(stdout, "%d", Near[i].from_cat);
		if (do_all)
		    fprintf(stdout, "%s%d", sep, Near[i].to_cat);
		print_upload(Near, Upload, i, &cvarr, catval, sep);
		fprintf(stdout, "\n");
	    }
	}
	else if (create_table) {	/* insert new record */
	    if (Near[i].count == 0)	/* no nearest found */
		continue;

	    if (!Outp)
		sprintf(buf1, "insert into %s values ( %d ", opt.table->answer,
			Near[i].from_cat);
	    else
		sprintf(buf1, "insert into %s values ( %d, %d ", opt.table->answer,
			i, Near[i].from_cat);

	    db_set_string(&stmt, buf1);

	    j = 0;
	    while (Upload[j].upload != END) {
		db_append_string(&stmt, ",");

		switch (Upload[j].upload) {
		case CAT:
		    sprintf(buf2, " %d", Near[i].to_cat);
		    break;
		case DIST:
		    sprintf(buf2, " %.17g", Near[i].dist);
		    break;
		case FROM_X:
		    sprintf(buf2, " %.17g", Near[i].from_x);
		    break;
		case FROM_Y:
		    sprintf(buf2, " %.17g", Near[i].from_y);
		    break;
		case TO_X:
		    sprintf(buf2, " %.17g", Near[i].to_x);
		    break;
		case TO_Y:
		    sprintf(buf2, " %.17g", Near[i].to_y);
		    break;
		case FROM_ALONG:
		    sprintf(buf2, " %.17g", Near[i].from_along);
		    break;
		case TO_ALONG:
		    sprintf(buf2, " %.17g", Near[i].to_along);
		    break;
		case TO_ANGLE:
		    sprintf(buf2, " %.17g", Near[i].to_angle);
		    break;
		case TO_ATTR:
		    if (catval) {
			switch (cvarr.ctype) {
			case DB_C_TYPE_INT:
			    sprintf(buf2, " %d", catval->val.i);
			    break;

			case DB_C_TYPE_DOUBLE:
			    sprintf(buf2, " %.17g", catval->val.d);
			    break;

			case DB_C_TYPE_STRING:
			    db_set_string(&dbstr,
					  db_get_string(catval->val.s));
			    db_double_quote_string(&dbstr);
			    sprintf(buf2, " '%s'", db_get_string(&dbstr));
			    break;

			case DB_C_TYPE_DATETIME:
			    /* TODO: formatting datetime */
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
	else if (update_table) {	/* update table */
	    int do_update = 0;

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
		    /* really clear existing records if no nearest found ? */
		    db_append_string(&stmt, " null");
		}
		else {
		    do_update = 1;
		    switch (Upload[j].upload) {
		    case CAT:
			if (Near[i].to_cat > 0)
			    sprintf(buf2, " %d", Near[i].to_cat);
			else
			    sprintf(buf2, " null");
			break;
		    case DIST:
			sprintf(buf2, " %.17g", Near[i].dist);
			break;
		    case FROM_X:
			sprintf(buf2, " %.17g", Near[i].from_x);
			break;
		    case FROM_Y:
			sprintf(buf2, " %.17g", Near[i].from_y);
			break;
		    case TO_X:
			sprintf(buf2, " %.17g", Near[i].to_x);
			break;
		    case TO_Y:
			sprintf(buf2, " %.17g", Near[i].to_y);
			break;
		    case FROM_ALONG:
			sprintf(buf2, " %.17g", Near[i].from_along);
			break;
		    case TO_ALONG:
			sprintf(buf2, " %.17g", Near[i].to_along);
			break;
		    case TO_ANGLE:
			sprintf(buf2, " %.17g", Near[i].to_angle);
			break;
		    case TO_ATTR:
			if (catval) {
			    switch (cvarr.ctype) {
			    case DB_C_TYPE_INT:
				sprintf(buf2, " %d", catval->val.i);
				break;

			    case DB_C_TYPE_DOUBLE:
				sprintf(buf2, " %.17g", catval->val.d);
				break;

			    case DB_C_TYPE_STRING:
				db_set_string(&dbstr,
					      db_get_string(catval->val.s));
				db_double_quote_string(&dbstr);
				sprintf(buf2, " '%s'", db_get_string(&dbstr));
				break;

			    case DB_C_TYPE_DATETIME:
				/* TODO: formatting datetime */
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
	    if (do_update) {
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

    if (update_table || create_table) {
	db_close_database_shutdown_driver(driver);
	db_free_string(&stmt);

	/* print stats */
	if (create_table) {
	    G_message(_("%d distances calculated"), count);
	    G_message(_("%d records inserted"), update_ok);
	    if (update_err > 0)
		G_message(_("%d insert errors"), update_err);
	}
	else {
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
	    Vect_set_db_updated(&From);
	}

    }

    Vect_close(&From);
    if (Outp != NULL) {
	if (create_table) {
	    dbConnection connection;

	    db_get_connection(&connection);
	    Vect_map_add_dblink(Outp, 1, NULL, opt.table->answer, "cat",
				connection.databaseName,
				connection.driverName);
	}
	Vect_build(Outp);
	Vect_close(Outp);
    }

    exit(EXIT_SUCCESS);
}
