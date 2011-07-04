
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
#include <grass/dbmi.h>
#include <grass/vector.h>

/* TODO: support all types (lines, boundaries, areas for 'from' (from_type) */

/* define codes for characteristics of relation between two nearest features */
#define CAT        1		/* category of nearest feature */
#define FROM_X     2		/* x coordinate of nearest point on 'from' feature */
#define FROM_Y     3		/* y coordinate of nearest point on 'from' feature */
#define TO_X       4		/* x coordinate of nearest point on 'to' feature */
#define TO_Y       5		/* y coordinate of nearest point on 'to' feature */
#define FROM_ALONG 6		/* distance to nearest point on 'from' along linear feature */
#define TO_ALONG   7		/* distance to nearest point on 'to' along linear feature */
#define DIST       8		/* minimum distance to nearest feature */
#define TO_ANGLE   9		/* angle of linear feature in nearest point */
#define TO_ATTR   10		/* attribute of nearest feature */
#define END       11		/* end of list */

/* Structure to store info about nearest feature for each category */
typedef struct
{
    int from_cat;		/* category (from) */
    int count;			/* number of features already found */
    int to_cat;			/* category (to) */
    double from_x, from_y, from_z, to_x, to_y, to_z;	/* coordinates of nearest point */
    double from_along, to_along;	/* distance along a linear feature to the nearest point */
    double to_angle;		/* angle of linear feature in nearest point */
    double dist;		/* distance to nearest feature */
} NEAR;

/* Upload and column store */
typedef struct
{
    int upload;			/* code */
    char *column;		/* column name */
} UPLOAD;


static int cmp_near(const void *, const void *);
static int cmp_near_to(const void *, const void *);
static int cmp_exist(const void *, const void *);
static int print_upload(NEAR *, UPLOAD *, int, dbCatValArray *, dbCatVal *);

int main(int argc, char *argv[])
{
    int i, j, k;
    int print_as_matrix;	/* only for all */
    int all;			/* calculate from each to each within the threshold */
    struct GModule *module;
    struct Option *from_opt, *to_opt, *from_type_opt, *to_type_opt,
	*from_field_opt, *to_field_opt;
    struct Option *out_opt, *max_opt, *min_opt, *table_opt;
    struct Option *upload_opt, *column_opt, *to_column_opt;
    struct Flag *print_flag, *all_flag;
    struct Map_info From, To, Out, *Outp;
    int from_type, to_type, from_field, to_field;
    double max, min;
    double *max_step;
    int n_max_steps, curr_step;
    struct line_pnts *FPoints, *TPoints;
    struct line_cats *FCats, *TCats;
    NEAR *Near, *near;
    int anear;			/* allocated space, used only for all */
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

    all = 0;
    print_as_matrix = 0;
    column = NULL;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    module->description =
	_("Finds the nearest element in vector map 'to' for elements in vector map 'from'.");

    from_opt = G_define_standard_option(G_OPT_V_INPUT);
    from_opt->key = "from";
    from_opt->description = _("Name of existing vector map (from)");
    from_opt->guisection = _("From");

    from_field_opt = G_define_standard_option(G_OPT_V_FIELD);
    from_field_opt->key = "from_layer";
    from_field_opt->label = _("Layer number or name (from)");
    from_field_opt->guisection = _("From");

    from_type_opt = G_define_standard_option(G_OPT_V_TYPE);
    from_type_opt->key = "from_type";
    from_type_opt->options = "point,centroid";
    from_type_opt->answer = "point";
    from_type_opt->label = _("Feature type (from)");
    from_type_opt->guisection = _("From");

    to_opt = G_define_standard_option(G_OPT_V_INPUT);
    to_opt->key = "to";
    to_opt->description = _("Name of existing vector map (to)");
    to_opt->guisection = _("To");

    to_field_opt = G_define_standard_option(G_OPT_V_FIELD);
    to_field_opt->key = "to_layer";
    to_field_opt->label = _("Layer number or name (to)");
    to_field_opt->guisection = _("To");

    to_type_opt = G_define_standard_option(G_OPT_V_TYPE);
    to_type_opt->key = "to_type";
    to_type_opt->options = "point,line,boundary,centroid,area";
    to_type_opt->answer = "point,line,area";
    to_type_opt->label = _("Feature type (to)");
    to_type_opt->guisection = _("To");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->key = "output";
    out_opt->required = NO;
    out_opt->description = _("Name for output vector map containing lines "
			     "connecting nearest elements");

    max_opt = G_define_option();
    max_opt->key = "dmax";
    max_opt->type = TYPE_DOUBLE;
    max_opt->required = NO;
    max_opt->answer = "-1";
    max_opt->description = _("Maximum distance or -1 for no limit");

    min_opt = G_define_option();
    min_opt->key = "dmin";
    min_opt->type = TYPE_DOUBLE;
    min_opt->required = NO;
    min_opt->answer = "-1";
    min_opt->description = _("Minimum distance or -1 for no limit");

    upload_opt = G_define_option();
    upload_opt->key = "upload";
    upload_opt->type = TYPE_STRING;
    upload_opt->required = YES;
    upload_opt->multiple = YES;
    upload_opt->options = "cat,dist,to_x,to_y,to_along,to_angle,to_attr";
    upload_opt->description =
	_("Values describing the relation between two nearest features");
    upload_opt->descriptions =
	_("cat;category of the nearest feature;"
	  "dist;minimum distance to nearest feature;"
	  "to_x;x coordinate of the nearest point on 'to' feature;"
	  "to_y;y coordinate of the nearest point on 'to' feature;"
	  "to_along;distance between points/centroids in 'from' map and the linear feature's "
	  "start point in 'to' map, along this linear feature;"
	  "to_angle;angle between the linear feature in 'to' map and the positive x axis, at "
	  "the location of point/centroid in 'from' map, counterclockwise, in radians, which "
	  "is between -PI and PI inclusive;"
	  "to_attr;attribute of nearest feature given by to_column option");
    /*  "from_x - x coordinate of the nearest point on 'from' feature;" */
    /*  "from_y - y coordinate of the nearest point on 'from' feature;" */
    /* "from_along - distance to the nearest point on 'from' feature along linear feature;" */

    column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    column_opt->required = YES;
    column_opt->multiple = YES;
    column_opt->description =
	_("Column name(s) where values specified by 'upload' option will be uploaded");
    column_opt->guisection = _("From_map");

    to_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    to_column_opt->key = "to_column";
    to_column_opt->description =
	_("Column name of nearest feature (used with upload=to_attr)");
    to_column_opt->guisection = _("To");
    
    table_opt = G_define_standard_option(G_OPT_DB_TABLE);
    table_opt->gisprompt = "new_dbtable,dbtable,dbtable";
    table_opt->description =
	_("Name of table created for output when the distance to all flag is used");

    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->label =
	_("Print output to stdout, don't update attribute table");
    print_flag->description =
	_("First column is always category of 'from' feature called from_cat");

    all_flag = G_define_flag();
    all_flag->key = 'a';
    all_flag->label =
	_("Calculate distances to all features within the threshold");
    all_flag->description = _("The output is written to stdout but may be uploaded "
                              "to a new table created by this module. "
			      "From categories are may be multiple.");	/* huh? */

    /* GUI dependency */
    from_opt->guidependency = G_store(from_field_opt->key);
    sprintf(buf1, "%s,%s", to_field_opt->key, to_column_opt->key);
    to_opt->guidependency = G_store(buf1);
    to_field_opt->guidependency = G_store(to_column_opt->key);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    from_type = Vect_option_to_types(from_type_opt);
    to_type = Vect_option_to_types(to_type_opt);

    from_field = atoi(from_field_opt->answer);

    max = atof(max_opt->answer);
    min = atof(min_opt->answer);

    if (all_flag->answer)
	all = 1;

    /* Read upload and column options */
    /* count */
    i = 0;
    while (upload_opt->answers[i])
	i++;
    if (strcmp(from_opt->answer, to_opt->answer) == 0 &&
	all && !table_opt->answer && i == 1)
	print_as_matrix = 1;

    /* alloc */
    Upload = (UPLOAD *) G_calloc(i + 1, sizeof(UPLOAD));
    /* read upload */
    i = 0;
    while (upload_opt->answers[i]) {
	if (strcmp(upload_opt->answers[i], "cat") == 0)
	    Upload[i].upload = CAT;
	else if (strcmp(upload_opt->answers[i], "from_x") == 0)
	    Upload[i].upload = FROM_X;
	else if (strcmp(upload_opt->answers[i], "from_y") == 0)
	    Upload[i].upload = FROM_Y;
	else if (strcmp(upload_opt->answers[i], "to_x") == 0)
	    Upload[i].upload = TO_X;
	else if (strcmp(upload_opt->answers[i], "to_y") == 0)
	    Upload[i].upload = TO_Y;
	else if (strcmp(upload_opt->answers[i], "from_along") == 0)
	    Upload[i].upload = FROM_ALONG;
	else if (strcmp(upload_opt->answers[i], "to_along") == 0)
	    Upload[i].upload = TO_ALONG;
	else if (strcmp(upload_opt->answers[i], "dist") == 0)
	    Upload[i].upload = DIST;
	else if (strcmp(upload_opt->answers[i], "to_angle") == 0)
	    Upload[i].upload = TO_ANGLE;
	else if (strcmp(upload_opt->answers[i], "to_attr") == 0) {
	    if (!(to_column_opt->answer)) {
		G_fatal_error(_("to_column option missing"));
	    }
	    Upload[i].upload = TO_ATTR;
	}

	i++;
    }
    Upload[i].upload = END;
    /* read columns */
    i = 0;
    while (column_opt->answers[i]) {
	if (Upload[i].upload == END) {
	    G_warning(_("Too many column names"));
	    break;
	}
	Upload[i].column = G_store(column_opt->answers[i]);
	i++;
    }
    if (Upload[i].upload != END)
	G_fatal_error(_("Not enough column names"));

    /* Open 'from' vector */
    Vect_set_open_level(2);
    Vect_open_old(&From, from_opt->answer, G_mapset());

    /* Open 'to' vector */
    Vect_set_open_level(2);
    Vect_open_old2(&To, to_opt->answer, "", to_field_opt->answer);

    to_field = Vect_get_field_number(&To, to_field_opt->answer);

    /* Open output vector */
    if (out_opt->answer) {
	Vect_open_new(&Out, out_opt->answer, WITHOUT_Z);
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
			    to_opt->answer);
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
    if (!print_flag->answer) {

	if (!all) {
	    Fi = Vect_get_field(&From, from_field);
	    if (Fi == NULL)
		G_fatal_error(_("Database connection not defined for layer %d"),
			      from_field);

	    driver = db_start_driver_open_database(Fi->driver, Fi->database);
	    if (driver == NULL)
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			      Fi->database, Fi->driver);

	    /* check if column exists */
	    i = 0;
	    while (column_opt->answers[i]) {
		db_get_column(driver, Fi->table, column_opt->answers[i],
			      &column);
		if (column) {
		    db_free_column(column);
		    column = NULL;
		}
		else {
		    G_fatal_error(_("Column <%s> not found in table <%s>"),
				  column_opt->answers[i], Fi->table);
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
    if (to_column_opt->answer) {
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
	db_get_column(to_driver, toFi->table, to_column_opt->answer, &column);
	if (column) {
	    db_free_column(column);
	    column = NULL;
	}
	else {
	    G_fatal_error(_("Column <%s> not found in table <%s>"),
			  to_column_opt->answer, toFi->table);
	}

	/* Check column types */
	if (!print_flag->answer && !all) {
	    char *fcname = NULL;
	    int fctype, tctype;

	    i = 0;
	    while (column_opt->answers[i]) {
		if (Upload[i].upload == TO_ATTR) {
		    fcname = column_opt->answers[i];
		    break;
		}
		i++;
	    }

	    if (fcname) {
		fctype = db_column_Ctype(driver, Fi->table, fcname);
		tctype =
		    db_column_Ctype(to_driver, toFi->table,
				    to_column_opt->answer);

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
    List = Vect_new_boxlist(0);

    /* Allocate space ( may be more than needed (duplicate cats and elements without cats) ) */
    nfrom = Vect_get_num_lines(&From);
    nto = Vect_get_num_lines(&To);
    if (all) {
	/* Attention with space for all, it can easily run out of memory */
	anear = 2 * nfrom;
	Near = (NEAR *) G_calloc(anear, sizeof(NEAR));
    }
    else {
	Near = (NEAR *) G_calloc(nfrom, sizeof(NEAR));
    }

    /* Read all cats from 'from' */
    if (!all) {
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

    count = 0;			/* count of distances in 'all' mode */
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
	    int done = 0;

	    curr_step = 0;

	    G_debug(3, "fline = %d", fline);
	    G_percent(fline, nfrom, 2);
	    ftype = Vect_read_line(&From, FPoints, FCats, fline);
	    if (!(ftype & from_type))
		continue;

	    Vect_cat_get(FCats, from_field, &fcat);
	    if (fcat < 0 && !all)
		continue;

	    while (!done) {
		done = 1;

		if (!all) {
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

		    if (all) {
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

		if (curr_step < n_max_steps) {
		    /* enlarging the search box is possible */
		    if (tline > 0 && dist > box_edge) {
			/* line found but distance > search edge:
			 * line bbox overlaps with search box, line itself is outside search box */
			done = 0;
		    }
		    else if (tline == 0) {
			/* no line within max dist, but search box can still be enlarged */
			done = 0;
		    }
		}
		if (done && !all && tline > 0) {
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
	    int done = 0;
	    
	    curr_step = 0;

	    G_debug(3, "fline = %d", fline);
	    G_percent(fline, nfrom, 2);
	    ftype = Vect_read_line(&From, FPoints, FCats, fline);
	    if (!(ftype & from_type))
		continue;

	    Vect_cat_get(FCats, from_field, &fcat);
	    if (fcat < 0 && !all)
		continue;

	    while (!done) {
		done = 1;

		if (!all) {
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
		    if (Vect_point_in_area(&To, area, FPoints->x[0], FPoints->y[0])) {	/* in area */
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

		    if (all) {
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

		if (curr_step < n_max_steps) {
		    /* enlarging the search box is possible */
		    if (tarea > 0 && dist > box_edge) {
			/* area found but distance > search edge:
			 * area bbox overlaps with search box, area itself is outside search box */
			done = 0;
		    }
		    else if (tarea == 0) {
			/* no area within max dist, but search box can still be enlarged */
			done = 0;
		    }
		}
		if (done && !all && tarea > 0) {
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
    if (print_flag->answer) {	/* print header */
	fprintf(stdout, "from_cat");
	i = 0;
	while (Upload[i].upload != END) {
	    fprintf(stdout, "|%s", Upload[i].column);
	    i++;
	}
	fprintf(stdout, "\n");
    }
    else if (all && table_opt->answer) {	/* create new table */
	db_set_string(&stmt, "create table ");
	db_append_string(&stmt, table_opt->answer);
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

	if (db_grant_on_table(driver, table_opt->answer, DB_PRIV_SELECT,
			      DB_GROUP | DB_PUBLIC) != DB_OK)
	    G_fatal_error(_("Unable to grant privileges on table <%s>"),
			  table_opt->answer);

    }
    else if (!all) {		/* read existing cats from table */
	ncatexist =
	    db_select_int(driver, Fi->table, Fi->key, NULL, &catexist);
	G_debug(1, "%d cats selected from the table", ncatexist);
    }
    update_ok = update_err = update_exist = update_notexist = update_dupl =
	update_notfound = 0;

    if (!all) {
	count = nfcats;
    }
    else if (print_as_matrix) {
	qsort((void *)Near, count, sizeof(NEAR), cmp_near_to);
    }

    if (driver)
	db_begin_transaction(driver);

    /* select 'to' attributes */
    if (to_column_opt->answer) {
	int nrec;

	db_CatValArray_init(&cvarr);
	nrec = db_select_CatValArray(to_driver, toFi->table, toFi->key,
				     to_column_opt->answer, NULL, &cvarr);
	G_debug(3, "selected values = %d", nrec);

	if (cvarr.ctype == DB_C_TYPE_DATETIME) {
	    G_warning(_("DATETIME type not yet supported, no attributes will be uploaded"));
	}
	db_close_database_shutdown_driver(to_driver);
    }

    if (!(print_flag->answer || (all && !table_opt->answer))) /* no printing */
	G_message("Update database...");

    for (i = 0; i < count; i++) {
	dbCatVal *catval = 0;

	if (!(print_flag->answer || (all && !table_opt->answer))) /* no printing */
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

	if (to_column_opt->answer && Near[i].count > 0) {
	    db_CatValArray_get_value(&cvarr, Near[i].to_cat, &catval);
	}

	if (print_flag->answer || (all && !table_opt->answer)) {	/* print only */
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
		print_upload(Near, Upload, i, &cvarr, catval);
		fprintf(stdout, "\n");
	    }
	}
	else if (all) {		/* insert new record */
	    sprintf(buf1, "insert into %s values ( %d ", table_opt->answer,
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
		  update_dupl, from_opt->answer);
    if (update_notfound > 0)
	G_message(_("%d categories - no nearest feature found"),
		  update_notfound);

    if (!print_flag->answer) {
	db_close_database_shutdown_driver(driver);
	db_free_string(&stmt);

	/* print stats */
	if (all && table_opt->answer) {
	    G_message(_("%d distances calculated"), count);
	    G_message(_("%d records inserted"), update_ok);
	    if (update_err > 0)
		G_message(_("%d insert errors"), update_err);
	}
	else if (!all) {
	    if (nfcats > 0)
		G_message(_("%d categories read from the map"), nfcats);
	    if (ncatexist > 0)
		G_message(_("%d categories exist in the table"), ncatexist);
	    if (update_exist > 0)
		G_message(_("%d categories read from the map exist in the table"),
			  update_exist);
	    if (update_notexist > 0)
		G_message(_("%d categories read from the map don't exist in the table"),
			  update_notexist);
	    G_message(_("%d records updated"), update_ok);
	    if (update_err > 0)
		G_message(_("%d update errors"), update_err);

	    G_free(catexist);
	}

	Vect_set_db_updated(&From);
    }

    Vect_close(&From);
    if (Outp != NULL) {
	Vect_build(Outp);
	Vect_close(Outp);
    }

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}


static int cmp_near(const void *pa, const void *pb)
{
    NEAR *p1 = (NEAR *) pa;
    NEAR *p2 = (NEAR *) pb;

    if (p1->from_cat < p2->from_cat)
	return -1;
    if (p1->from_cat > p2->from_cat)
	return 1;
    return 0;
}

static int cmp_near_to(const void *pa, const void *pb)
{
    NEAR *p1 = (NEAR *) pa;
    NEAR *p2 = (NEAR *) pb;

    if (p1->from_cat < p2->from_cat)
	return -1;

    if (p1->from_cat > p2->from_cat)
	return 1;

    if (p1->to_cat < p2->to_cat)
	return -1;

    if (p1->to_cat > p2->to_cat)
	return 1;

    return 0;
}


static int cmp_exist(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    int *p2 = (int *)pb;

    if (*p1 < *p2)
	return -1;
    if (*p1 > *p2)
	return 1;
    return 0;
}

/*
   print out upload values 
 */
static int print_upload(NEAR * Near, UPLOAD * Upload, int i,
			dbCatValArray * cvarr, dbCatVal * catval)
{
    int j;

    j = 0;
    while (Upload[j].upload != END) {
	if (Near[i].count == 0) {	/* no nearest found */
	    fprintf(stdout, "|null");
	}
	else {
	    switch (Upload[j].upload) {
	    case CAT:
		if (Near[i].to_cat >= 0)
		    fprintf(stdout, "|%d", Near[i].to_cat);
		else
		    fprintf(stdout, "|null");
		break;
	    case DIST:
		fprintf(stdout, "|%f", Near[i].dist);
		break;
	    case FROM_X:
		fprintf(stdout, "|%f", Near[i].from_x);
		break;
	    case FROM_Y:
		fprintf(stdout, "|%f", Near[i].from_y);
		break;
	    case TO_X:
		fprintf(stdout, "|%f", Near[i].to_x);
		break;
	    case TO_Y:
		fprintf(stdout, "|%f", Near[i].to_y);
		break;
	    case FROM_ALONG:
		fprintf(stdout, "|%f", Near[i].from_along);
		break;
	    case TO_ALONG:
		fprintf(stdout, "|%f", Near[i].to_along);
		break;
	    case TO_ANGLE:
		fprintf(stdout, "|%f", Near[i].to_angle);
		break;
	    case TO_ATTR:
		if (catval) {
		    switch (cvarr->ctype) {
		    case DB_C_TYPE_INT:
			fprintf(stdout, "|%d", catval->val.i);
			break;

		    case DB_C_TYPE_DOUBLE:
			fprintf(stdout, "|%.15e", catval->val.d);
			break;

		    case DB_C_TYPE_STRING:
			fprintf(stdout, "|%s", db_get_string(catval->val.s));
			break;

		    case DB_C_TYPE_DATETIME:
			/* TODO: formating datetime */
			fprintf(stdout, "|");
			break;
		    }
		}
		else {
		    fprintf(stdout, "|null");
		}
		break;
	    }
	}
	j++;
    }

    return 0;
}
