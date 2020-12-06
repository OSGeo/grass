
/***************************************************************
 *
 * MODULE:       v.vect.stats
 * 
 * AUTHOR(S):    Markus Metz
 *               
 * PURPOSE:      Counts points per area and calculates aggregate statistics. 
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
#include <grass/stats.h>
#include <grass/dbmi.h>
#include <grass/vector.h>

struct menu
{
    stat_func *method;		/* routine to compute new value */
    int is_int;			/* whether the result is an integer (unused) */
    char *name;			/* method name */
    char *text;			/* menu display - full description */
};

/* modify this table to add new methods */
static struct menu menu[] = {
    {c_sum, 0, "sum", "sum of values"},
    {c_ave, 0, "average", "average value"},
    {c_median, 0, "median", "median value"},
    {c_mode, 0, "mode", "most frequently occurring value"},
    {c_min, 0, "minimum", "lowest value"},
    {c_minx, 1, "min_cat", "category number of lowest value"},
    {c_max, 0, "maximum", "highest value"},
    {c_maxx, 1, "max_cat", "category number of highest value"},
    {c_range, 0, "range", "range of values"},
    {c_stddev, 0, "stddev", "standard deviation"},
    {c_var, 0, "variance", "statistical variance"},
    {c_divr, 1, "diversity", "number of different values"},
    {NULL, 0, NULL, NULL}
};

/* Structure to store info for each area category */
typedef struct
{
    int area_cat;		/* area category */
    int count;			/* number of points in areas with area_cat */
    double *values;
    int *cats;
    int nvalues, nalloc;

} AREA_CAT;

/* compare function for qsort and bsearch */
static int cmp_area(const void *pa, const void *pb)
{
    AREA_CAT *p1 = (AREA_CAT *) pa;
    AREA_CAT *p2 = (AREA_CAT *) pb;

    if (p1->area_cat < p2->area_cat)
	return -1;

    return (p1->area_cat > p2->area_cat);
}

int main(int argc, char *argv[])
{
    char *p;
    int i, j, k;
    int method, use_catno;
    const char *mapset;
    struct GModule *module;
    struct Option *point_opt,	/* point vector */
     *area_opt,			/* area vector */
     *point_type_opt,		/* point type */
     *point_field_opt,		/* point layer */
     *point_cats_opt,		/* point cats */
     *point_where_opt,		/* point where */
     *area_field_opt,		/* area layer */
     *area_cats_opt,		/* area cats */
     *area_where_opt,		/* area where */
     *method_opt,		/* stats method */
     *point_column_opt,		/* point column for stats */
     *count_column_opt,		/* area column for point count */
     *stats_column_opt,		/* area column for stats result */
     *fs_opt;			/* field separator for printed output */
    struct Flag *print_flag;
    char *fs;
    struct Map_info PIn, AIn;
    int point_type, point_field, area_field;
    struct cat_list *acat_list = NULL;
    struct cat_list *pcat_list = NULL;
    struct line_pnts *Points;
    struct line_cats *ACats, *PCats;
    AREA_CAT *Area_cat;
    int pline, ptype, count;
    int area, nareas, nacats, nacatsalloc;
    int ctype, nrec;
    struct field_info *PFi, *AFi;
    dbString stmt;
    dbDriver *Pdriver, *Adriver;
    char buf[2000];
    int update_ok, update_err;
    struct boxlist *List;
    struct bound_box box;
    dbCatValArray cvarr;
    dbColumn *column;
    struct pvalcat
    {
	double dval;
	int catno;
    } *pvalcats;
    int npvalcats, npvalcatsalloc;
    stat_func *statsvalue = NULL;
    double result;

    column = NULL;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("database"));
    G_add_keyword(_("univariate statistics"));
    G_add_keyword(_("zonal statistics"));
    module->description = _("Count points in areas, calculate statistics from point attributes.");

    point_opt = G_define_standard_option(G_OPT_V_INPUT);
    point_opt->key = "points";
    point_opt->label = _("Name of existing vector map with points");
    /* point_opt->guisection = _("Required"); */
      
    area_opt = G_define_standard_option(G_OPT_V_INPUT);
    area_opt->key = "areas";
    area_opt->label = _("Name of existing vector map with areas");
    /* area_opt->guisection = _("Required"); */

    point_type_opt = G_define_standard_option(G_OPT_V_TYPE);
    point_type_opt->key = "type";
    point_type_opt->options = "point,centroid";
    point_type_opt->answer = "point";
    point_type_opt->label = _("Feature type");
    point_type_opt->required = NO;
    point_type_opt->guisection = _("Selection");

    point_field_opt = G_define_standard_option(G_OPT_V_FIELD);
    point_field_opt->key = "points_layer";
    point_field_opt->label = _("Layer number for points map");
    point_field_opt->guisection = _("Selection");

    point_cats_opt = G_define_standard_option(G_OPT_V_CATS);
    point_cats_opt->key = "points_cats";
    point_cats_opt->label = _("Category values for points map");
    point_cats_opt->guisection = _("Selection");
    
    point_where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    point_where_opt->key = "points_where";
    point_where_opt->label = _("WHERE conditions of SQL statement without 'where' keyword for points map");;
    point_where_opt->guisection = _("Selection");

    area_field_opt = G_define_standard_option(G_OPT_V_FIELD);
    area_field_opt->key = "areas_layer";
    area_field_opt->label = _("Layer number for area map");
    area_field_opt->guisection = _("Selection");

    area_cats_opt = G_define_standard_option(G_OPT_V_CATS);
    area_cats_opt->key = "areas_cats";
    area_cats_opt->label = _("Category values for area map");
    area_cats_opt->guisection = _("Selection");
    
    area_where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    area_where_opt->key = "areas_where";
    area_where_opt->label = _("WHERE conditions of SQL statement without 'where' keyword for area map");;
    area_where_opt->guisection = _("Selection");

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = NO;
    method_opt->multiple = NO;
    p = G_malloc(1024);
    for (i = 0; menu[i].name; i++) {
	if (i)
	    strcat(p, ",");
	else
	    *p = 0;
	strcat(p, menu[i].name);
    }
    method_opt->options = p;
    method_opt->description = _("Method for aggregate statistics");
    method_opt->guisection = _("Statistics");

    point_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    point_column_opt->key = "points_column";
    point_column_opt->label =
	_("Column name of points map to use for statistics");
    point_column_opt->description = _("Column of points map must be numeric");
    point_column_opt->guisection = _("Statistics");

    count_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    count_column_opt->key = "count_column";
    count_column_opt->type = TYPE_STRING;
    count_column_opt->label = _("Column name to upload points count");
    count_column_opt->description =
	_("Column to hold points count, must be of type integer, will be created if not existing");
    count_column_opt->guisection = _("Statistics");

    stats_column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    stats_column_opt->key = "stats_column";
    stats_column_opt->label = _("Column name to upload statistics");
    stats_column_opt->description =
	_("Column to hold statistics, must be of type double, will be created if not existing");
    stats_column_opt->guisection = _("Statistics");

    fs_opt = G_define_standard_option(G_OPT_F_SEP);
    fs_opt->guisection = _("Print");
    
    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->label =
	_("Print output to stdout, do not update attribute table");
    print_flag->description = _("First column is always area category");
    print_flag->guisection = _("Print");

    /* GUI dependencies */
    G_asprintf((char **) &(point_opt->guidependency), "%s,%s,%s",
               point_field_opt->key, point_where_opt->key, point_column_opt->key);
    G_asprintf((char **) &(point_field_opt->guidependency), "%s,%s",
               point_where_opt->key, point_column_opt->key);
    G_asprintf((char **) &(area_opt->guidependency), "%s,%s,%s,%s",
               area_field_opt->key, area_where_opt->key,
               count_column_opt->key, stats_column_opt->key);
    G_asprintf((char **) &(area_field_opt->guidependency), "%s,%s,%s",
               area_where_opt->key, count_column_opt->key, stats_column_opt->key);
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    point_type = Vect_option_to_types(point_type_opt);

    fs = NULL;
    if (print_flag->answer) {
	/* get field separator */
	fs = G_option_to_separator(fs_opt);
    }

    /* check for stats */
    if (method_opt->answer) {
	if (!point_column_opt->answer) {
	    G_fatal_error("Method but no point column selected");
	}
	if (!print_flag->answer && !stats_column_opt->answer)
	    G_fatal_error("Name for stats_column is missing");
    }

    if (point_column_opt->answer) {
	if (!method_opt->answer)
	    G_fatal_error("No method for statistics selected");
	if (!print_flag->answer && !stats_column_opt->answer)
	    G_fatal_error("Name for stats_column is missing");
    }
    
    /* Open points vector */
    if ((mapset = G_find_vector2(point_opt->answer, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), point_opt->answer);

    Vect_set_open_level(2);
    if (Vect_open_old(&PIn, point_opt->answer, mapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), point_opt->answer);

    /* Open areas vector */
    if ((mapset = G_find_vector2(area_opt->answer, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), area_opt->answer);
    if (!print_flag->answer && strcmp(mapset, G_mapset()) != 0)
	G_fatal_error(_("Vector map <%s> is not in the current mapset <%s> and cannot be updated"),
		      area_opt->answer, G_mapset());

    Vect_set_open_level(2);
    if (Vect_open_old(&AIn, area_opt->answer, mapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), area_opt->answer);

    point_field = atoi(point_field_opt->answer);
    pcat_list = NULL;
    if (point_field > 0)
	pcat_list = Vect_cats_set_constraint(&PIn, point_field,
	                                     point_where_opt->answer,
                                             point_cats_opt->answer);

    area_field = atoi(area_field_opt->answer);
    acat_list = NULL;
    if (area_field > 0)
	acat_list = Vect_cats_set_constraint(&AIn, area_field,
	                                     area_where_opt->answer,
                                             area_cats_opt->answer);

    method = -1;
    use_catno = 0;
    if (method_opt->answer) {
	/* get the method */
	for (method = 0; (p = menu[method].name); method++)
	    if ((strcmp(p, method_opt->answer) == 0))
		break;
	if (!p) {
	    G_warning(_("<%s=%s> unknown %s"),
		      method_opt->key, method_opt->answer,
		      method_opt->answer);
	    G_usage();
	    exit(EXIT_FAILURE);
	}

	/* establish the statsvalue routine */
	statsvalue = menu[method].method;

	/* category number of lowest/highest value */
	if ((strcmp(menu[method].name, menu[5].name) == 0) ||
	    (strcmp(menu[method].name, menu[7].name) == 0))
	    use_catno = 1;

	G_debug(1, "method: %s, use cat value: %s", menu[method].name,
		(use_catno == 1 ? "yes" : "no"));
    }

    /* Open database driver */
    db_init_string(&stmt);
    Adriver = NULL;
    AFi = NULL;

    if (!print_flag->answer) {

	AFi = Vect_get_field(&AIn, area_field);
	if (AFi == NULL)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  area_field);

	Adriver = db_start_driver_open_database(AFi->driver, AFi->database);
	if (Adriver == NULL)
	    G_fatal_error(_("Unable to open database <%s> with driver <%s>"),
			  AFi->database, AFi->driver);

	if (!count_column_opt->answer)
	    G_fatal_error(_("count_column is required to upload point counts"));

	/* check if count column exists */
	G_debug(1, "check if count column exists");
	db_get_column(Adriver, AFi->table, count_column_opt->answer, &column);
	if (column) {
	    /* check count column type */
	    if (db_column_Ctype(Adriver, AFi->table, count_column_opt->answer)
		!= DB_C_TYPE_INT)
		G_fatal_error(_("count_column must be of type integer"));

	    db_free_column(column);
	    column = NULL;
	}
	else {
	    /* create count column */
	    /* db_add_column() exists but is not implemented,
	     * see lib/db/stubs/add_col.c */
	    sprintf(buf, "alter table %s add column %s integer",
	                    AFi->table, count_column_opt->answer);
	    db_set_string(&stmt, buf);
	    if (db_execute_immediate(Adriver, &stmt) != DB_OK)
		G_fatal_error(_("Unable to add column <%s>"),
			      count_column_opt->answer);
	}

	if (method_opt->answer) {
	    if (!stats_column_opt->answer)
		G_fatal_error(_("stats_column is required to upload point stats"));

	    /* check if stats column exists */
	    G_debug(1, "check if stats column exists");
	    db_get_column(Adriver, AFi->table, stats_column_opt->answer,
			  &column);
	    if (column) {
		/* check stats column type */
		if (db_column_Ctype
		    (Adriver, AFi->table,
		     stats_column_opt->answer) != DB_C_TYPE_DOUBLE)
		    G_fatal_error(_("stats_column must be of type double precision"));

		db_free_column(column);
		column = NULL;
	    }
	    else {
		/* create stats column */
		/* db_add_column() exists but is not implemented,
		 * see lib/db/stubs/add_col.c */
		sprintf(buf, "alter table %s add column %s double",
				AFi->table, stats_column_opt->answer);
		db_set_string(&stmt, buf);
		if (db_execute_immediate(Adriver, &stmt) != DB_OK)
		    G_fatal_error(_("Unable to add column <%s>"),
				  stats_column_opt->answer);
	    }
	}
    }

    Pdriver = NULL;
    if (method_opt->answer) {

	G_verbose_message(_("collecting attributes from points vector..."));

	PFi = Vect_get_field(&PIn, point_field);
	if (PFi == NULL)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  point_field);


	/* Open points map driver and database */
	if (Adriver && strcmp(AFi->driver, PFi->driver) == 0
	    && strcmp(AFi->database, PFi->database) == 0) {
	    G_debug(3, "Use the same driver");
	    Pdriver = Adriver;
	}
	else {
	    Pdriver = db_start_driver_open_database(PFi->driver, PFi->database);
	    if (Pdriver == NULL)
		G_fatal_error(_("Unable to open database <%s> with driver <%s>"),
			      PFi->database, PFi->driver);
	}

	/* check if point column exists */
	db_get_column(Pdriver, PFi->table, point_column_opt->answer, &column);
	if (column) {
	    db_free_column(column);
	    column = NULL;
	}
	else {
	    G_fatal_error(_("Column <%s> not found in table <%s>"),
			  point_column_opt->answer, PFi->table);
	}

	/* Check column type */
	ctype =
	    db_column_Ctype(Pdriver, PFi->table, point_column_opt->answer);

	if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("points_column <%s> of points vector <%s> must be numeric"), point_column_opt->answer, PFi->table);

	db_CatValArray_init(&cvarr);
	nrec = db_select_CatValArray(Pdriver, PFi->table, PFi->key,
				     point_column_opt->answer, NULL, &cvarr);
	G_debug(1, "selected values = %d", nrec);

	if (Pdriver != Adriver)
	    db_close_database_shutdown_driver(Pdriver);
    }

    Points = Vect_new_line_struct();
    ACats = Vect_new_cats_struct();
    PCats = Vect_new_cats_struct();
    List = Vect_new_boxlist(0);

    /* Allocate space ( may be more than needed (duplicate cats and elements without cats) ) */
    if ((nareas = Vect_get_num_areas(&AIn)) <= 0)
	G_fatal_error("No areas in area input vector");

    nacatsalloc = nareas;
    Area_cat = (AREA_CAT *) G_calloc(nacatsalloc, sizeof(AREA_CAT));

    /* Read all cats from 'area' */
    nacats = 0;
    for (area = 1; area <= nareas; area++) {

	Vect_get_area_cats(&AIn, area, ACats);

	if (ACats->n_cats <= 0)
	    continue;

	if (area_field > 0 
	    && !Vect_cats_in_constraint(ACats, area_field, acat_list))
	    continue;

	for (i = 0; i < ACats->n_cats; i++) {

	    if (ACats->field[i] == area_field) {
		if (acat_list && !Vect_cat_in_cat_list(ACats->cat[i], acat_list)) {
		    continue;
		}
		Area_cat[nacats].area_cat = ACats->cat[i];
		Area_cat[nacats].count = 0;
		Area_cat[nacats].nvalues = 0;
		Area_cat[nacats].nalloc = 0;
		Area_cat[nacats].values = NULL;
		Area_cat[nacats].cats = NULL;
		nacats++;
		if (nacats >= nacatsalloc) {
		    nacatsalloc += 100;
		    Area_cat =
			(AREA_CAT *) G_realloc(Area_cat,
					       nacatsalloc *
					       sizeof(AREA_CAT));
		}
	    }

	}
    }

    G_debug(1, "%d cats loaded from vector (including duplicates)", nacats);

    if (nacats == 0)
	G_fatal_error(_("No area categories in vector <%s>, layer <%s>"),
		      area_opt->answer, area_field_opt->answer);

    /* Sort by category */
    qsort((void *)Area_cat, nacats, sizeof(AREA_CAT), cmp_area);

    /* remove duplicate categories */
    j = 1;
    for (i = 1; i < nacats; i++) {
	if (Area_cat[i].area_cat != Area_cat[i - 1].area_cat) {
	    Area_cat[j].area_cat = Area_cat[i].area_cat;
	    j++;
	}
    }
    nacats = j;

    G_debug(1, "%d cats loaded from vector (unique)", nacats);

    /* Go through all areas in area vector and find points in points vector
     * falling into the area */
    npvalcatsalloc = 10;
    npvalcats = 0;
    pvalcats =
	(struct pvalcat *)G_calloc(npvalcatsalloc, sizeof(struct pvalcat));

    G_message(_("Selecting points for each area..."));
    count = 0;
    for (area = 1; area <= nareas; area++) {
	dbCatVal *catval;

	G_debug(3, "area = %d", area);
	G_percent(area, nareas, 2);

	Vect_get_area_cats(&AIn, area, ACats);

	if (ACats->n_cats <= 0)
	    continue;

	if (area_field > 0 
	    && !Vect_cats_in_constraint(ACats, area_field, acat_list))
	    continue;

	/* select points by box */
	Vect_get_area_box(&AIn, area, &box);
	box.T = PORT_DOUBLE_MAX;
	box.B = -PORT_DOUBLE_MAX;

	Vect_select_lines_by_box(&PIn, &box, point_type, List);
	G_debug(4, "%d points selected by box", List->n_values);

	/* For each point in box check if it is in the area */
	for (i = 0; i < List->n_values; i++) {

	    pline = List->id[i];
	    G_debug(4, "%d: point %d", i, pline);

	    ptype = Vect_read_line(&PIn, Points, PCats, pline);
	    if (!(ptype & point_type))
		continue;

	    if (point_field > 0 
		&& !Vect_cats_in_constraint(PCats, point_field, pcat_list))
		continue;

	    /* point in area */
	    if (Vect_point_in_area(Points->x[0], Points->y[0], &AIn, area, &box)) {
		AREA_CAT *area_info, search_ai;

		int tmp_cat;

		/* stats on point column */
		if (method_opt->answer) {
		    npvalcats = 0;
		    tmp_cat = -1;
		    for (j = 0; j < PCats->n_cats; j++) {
			if (PCats->field[j] == point_field) {
			    if (pcat_list && !Vect_cat_in_cat_list(PCats->cat[j], pcat_list)) {
				continue;
			    }

			    if (tmp_cat >= 0)
				G_debug(3,
					"More cats found in point layer (point=%d)",
					pline);
			    tmp_cat = PCats->cat[j];

			    /* find cat in array */
			    db_CatValArray_get_value(&cvarr, tmp_cat,
						     &catval);

			    if (catval) {
				pvalcats[npvalcats].catno = tmp_cat;
				switch (cvarr.ctype) {
				case DB_C_TYPE_INT:
				    pvalcats[npvalcats].dval = catval->val.i;
				    npvalcats++;
				    break;

				case DB_C_TYPE_DOUBLE:
				    pvalcats[npvalcats].dval = catval->val.d;
				    npvalcats++;
				    break;
				}
				if (npvalcats >= npvalcatsalloc) {
				    npvalcatsalloc += 10;
				    pvalcats =
					(struct pvalcat *)G_realloc(pvalcats,
								    npvalcatsalloc
								    *
								    sizeof
								    (struct
								     pvalcat));
				}
			    }
			}
		    }
		}

		/* update count for all area cats of given field */
		search_ai.area_cat = -1;
		for (j = 0; j < ACats->n_cats; j++) {
		    if (ACats->field[j] == area_field) {
			if (acat_list && !Vect_cat_in_cat_list(ACats->cat[j], acat_list)) {
			    continue;
			}


			if (search_ai.area_cat >= 0)
			    G_debug(3,
				    "More cats found in area layer (area=%d)",
				    area);
			search_ai.area_cat = ACats->cat[j];

			/* find cat in array */
			area_info =
			    (AREA_CAT *) bsearch((void *)&search_ai, Area_cat,
						 nacats, sizeof(AREA_CAT),
						 cmp_area);
			if (area_info->area_cat != search_ai.area_cat)
			    G_fatal_error(_("could not find area category %d"),
					  search_ai.area_cat);

			/* each point is counted once, also if it has
			 * more than one category or no category
			 * OK? */
			area_info->count++;

			if (method_opt->answer) {
			    /* ensure enough space */
			    if (area_info->nvalues + npvalcats >=
				area_info->nalloc) {
				if (area_info->nalloc == 0) {
				    area_info->nalloc = npvalcats + 10;
				    area_info->values =
					(double *)G_calloc(area_info->nalloc,
							   sizeof(double));
				    area_info->cats =
					(int *)G_calloc(area_info->nalloc,
							sizeof(int));
				}
				else
				    area_info->nalloc +=
					area_info->nvalues + npvalcats + 10;
				area_info->values =
				    (double *)G_realloc(area_info->values,
							area_info->nalloc *
							sizeof(double));
				area_info->cats =
				    (int *)G_realloc(area_info->cats,
						     area_info->nalloc *
						     sizeof(int));
			    }
			    for (k = 0; k < npvalcats; k++) {
				area_info->cats[area_info->nvalues] =
				    pvalcats[k].catno;
				area_info->values[area_info->nvalues] =
				    pvalcats[k].dval;
				area_info->nvalues++;
			    }
			}
		    }
		}
		count++;
	    }
	}			/* next point in box */
    }				/* next area */

    G_debug(1, "count = %d", count);

    /* release catval array */
    if (method_opt->answer)
	db_CatValArray_free(&cvarr);

    Vect_close(&PIn);

    /* Update table or print to stdout */
    if (print_flag->answer) {	/* print header */
	fprintf(stdout, "area_cat%scount", fs);
	if (method_opt->answer)
	    fprintf(stdout, "%s%s", fs, menu[method].name);
	fprintf(stdout, "\n");
    }
    else {
	G_message("Updating attributes for area vector...");
	update_err = update_ok = 0;
    }
    if (Adriver)
	db_begin_transaction(Adriver);

    for (i = 0; i < nacats; i++) {
	if (!print_flag->answer)
	    G_percent(i, nacats, 2);

	result = 0;

	if (Area_cat[i].count > 0 && method_opt->answer) {
	    /* get stats */
	    statsvalue(&result, Area_cat[i].values, Area_cat[i].nvalues,
			NULL);

	    if (use_catno)
		result = Area_cat[i].cats[(int)result];
	}
	if (print_flag->answer) {
	    fprintf(stdout, "%d%s%d", Area_cat[i].area_cat, fs,
		    Area_cat[i].count);
	    if (method_opt->answer) {
		if (Area_cat[i].count > 0)
		    fprintf(stdout, "%s%.15g", fs, result);
		else
		    fprintf(stdout, "%snull", fs);
	    }
	    fprintf(stdout, "\n");
	}
	else {
	    sprintf(buf, "update %s set %s = %d", AFi->table,
		    count_column_opt->answer, Area_cat[i].count);
	    db_set_string(&stmt, buf);
	    if (method_opt->answer) {
		if (Area_cat[i].count > 0)
		    sprintf(buf, " , %s = %.15g", stats_column_opt->answer,
			    result);
		else
		    sprintf(buf, " , %s = null", stats_column_opt->answer);
		db_append_string(&stmt, buf);
	    }
	    sprintf(buf, " where %s = %d", AFi->key, Area_cat[i].area_cat);
	    db_append_string(&stmt, buf);
	    G_debug(2, "SQL: %s", db_get_string(&stmt));
	    if (db_execute_immediate(Adriver, &stmt) == DB_OK) {
		update_ok++;
	    }
	    else {
		update_err++;
	    }

	}
    }
    if (Adriver)
	db_commit_transaction(Adriver);

    if (!print_flag->answer) {
	G_percent(nacats, nacats, 2);
	db_close_database_shutdown_driver(Adriver);
	db_free_string(&stmt);
	G_message(_("%d records updated"), update_ok);
	if (update_err > 0)
	    G_message(_("%d update errors"), update_err);

	Vect_set_db_updated(&AIn);
    }

    Vect_close(&AIn);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
