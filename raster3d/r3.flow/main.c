
/****************************************************************************
 * 
 * MODULE:       r3.flow     
 * AUTHOR(S):    Anna Petrasova kratochanna <at> gmail <dot> com
 * PURPOSE:      Computes 3D flow lines and flow accumulation based on 3D raster map(s)
 * COPYRIGHT:    (C) 2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/raster3d.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "r3flow_structs.h"
#include "flowline.h"

static void create_table(struct Map_info *flowline_vec,
			 struct field_info **f_info, dbDriver ** driver,
			 int write_scalar, int use_sampled_map)
{
    dbString sql;
    char buf[200];
    dbDriver *drvr;
    struct field_info *fi;

    db_init_string(&sql);
    fi = Vect_default_field_info(flowline_vec, 1, NULL, GV_1TABLE);
    *f_info = fi;
    Vect_map_add_dblink(flowline_vec, 1, NULL, fi->table, GV_KEY_COLUMN,
			fi->database, fi->driver);
    drvr = db_start_driver_open_database(fi->driver,
					 Vect_subst_var(fi->database,
							flowline_vec));
    if (drvr == NULL) {
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Vect_subst_var(fi->database, flowline_vec), fi->driver);
    }
    db_set_error_handler_driver(drvr);

    *driver = drvr;
    sprintf(buf, "create table %s (cat integer, velocity double precision",
	    fi->table);
    db_set_string(&sql, buf);
    if (write_scalar)
	db_append_string(&sql, ", input double precision");
    if (use_sampled_map)
	db_append_string(&sql, ", sampled double precision");
    db_append_string(&sql, ")");

    db_begin_transaction(drvr);
    /* Create table */
    if (db_execute_immediate(drvr, &sql) != DB_OK) {
	G_fatal_error(_("Unable to create table: %s"), db_get_string(&sql));
    }
    if (db_create_index2(drvr, fi->table, fi->key) != DB_OK)
	G_warning(_("Unable to create index for table <%s>, key <%s>"),
		  fi->table, fi->key);
    /* Grant */
    if (db_grant_on_table
	(drvr, fi->table, DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK) {
	G_fatal_error(_("Unable to grant privileges on table <%s>"),
		      fi->table);
    }
}

static void check_vector_input_maps(struct Option *vector_opt,
				    struct Option *seed_opt)
{
    int i;

    /* Check for velocity components maps. */
    if (vector_opt->answers != NULL) {
	for (i = 0; i < 3; i++) {
	    if (vector_opt->answers[i] != NULL) {
		if (NULL == G_find_raster3d(vector_opt->answers[i], ""))
		    Rast3d_fatal_error(_("3D raster map <%s> not found"),
				       vector_opt->answers[i]);
	    }
	    else {
		Rast3d_fatal_error(_("Please provide three 3D raster maps"));
	    }
	}
    }

    if (seed_opt->answer != NULL) {
	if (NULL == G_find_vector2(seed_opt->answer, ""))
	    G_fatal_error(_("Vector seed map <%s> not found"),
			  seed_opt->answer);
    }

}

static void load_input_raster3d_maps(struct Option *scalar_opt,
				     struct Option *vector_opt,
				     struct Gradient_info *gradient_info,
				     RASTER3D_Region * region)
{
    int i;

    if (scalar_opt->answer) {
	gradient_info->scalar_map =
	    Rast3d_open_cell_old(scalar_opt->answer,
				 G_find_raster3d(scalar_opt->answer, ""),
				 region, RASTER3D_TILE_SAME_AS_FILE,
				 RASTER3D_USE_CACHE_DEFAULT);
	if (!gradient_info->scalar_map)
	    Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
			       scalar_opt->answer);
	gradient_info->compute_gradient = TRUE;
    }
    else {
	for (i = 0; i < 3; i++) {
	    gradient_info->velocity_maps[i] =
		Rast3d_open_cell_old(vector_opt->answers[i],
				     G_find_raster3d(vector_opt->answers[i],
						     ""), region,
				     RASTER3D_TILE_SAME_AS_FILE,
				     RASTER3D_USE_CACHE_DEFAULT);

	    if (!gradient_info->velocity_maps[i])
		Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
				   vector_opt->answers[i]);
	}
	gradient_info->compute_gradient = FALSE;
    }
}

static void init_flowaccum(RASTER3D_Region * region, RASTER3D_Map * flowacc)
{
    int c, r, d;

    for (d = 0; d < region->depths; d++)
	for (r = 0; r < region->rows; r++)
	    for (c = 0; c < region->cols; c++)
		if (Rast3d_put_float(flowacc, c, r, d, 0) != 1)
		    Rast3d_fatal_error(_("init_flowaccum: error in Rast3d_put_float"));
}

int main(int argc, char *argv[])
{
    struct Option *vector_opt, *seed_opt, *flowlines_opt, *flowacc_opt, *sampled_opt,
	*scalar_opt, *unit_opt, *step_opt, *limit_opt, *skip_opt, *dir_opt,
	*error_opt;
    struct Flag *table_fl;
    struct GModule *module;
    RASTER3D_Region region;
    RASTER3D_Map *flowacc, *sampled;
    struct Integration integration;
    struct Seed seed;
    struct Gradient_info gradient_info;
    struct Map_info seed_Map;
    struct line_pnts *seed_points;
    struct line_cats *seed_cats;
    struct Map_info fl_map;
    struct line_cats *fl_cats;	/* for flowlines */
    struct line_pnts *fl_points;	/* for flowlines */
    struct field_info *finfo;
    dbDriver *driver;
    int cat;			/* cat of flowlines */
    int if_table;
    int i, r, c, d;
    char *desc;
    int n_seeds, seed_count, ltype;
    int skip[3];

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("voxel"));
    module->description =
	_("Computes 3D flow lines and 3D flow accumulation.");


    scalar_opt = G_define_standard_option(G_OPT_R3_INPUT);
    scalar_opt->required = NO;
    scalar_opt->guisection = _("Input");

    vector_opt = G_define_standard_option(G_OPT_R3_INPUTS);
    vector_opt->key = "vector_field";
    vector_opt->required = NO;
    vector_opt->description = _("Names of three 3D raster maps describing "
				"x, y, z components of vector field");
    vector_opt->guisection = _("Input");

    seed_opt = G_define_standard_option(G_OPT_V_INPUT);
    seed_opt->required = NO;
    seed_opt->key = "seed_points";
    seed_opt->description = _("If no map is provided, "
			      "flow lines are generated "
			      "from each cell of the input 3D raster");
    seed_opt->label = _("Name of vector map with points "
			"from which flow lines are generated");
    seed_opt->guisection = _("Input");

    flowlines_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    flowlines_opt->key = "flowline";
    flowlines_opt->required = NO;
    flowlines_opt->description = _("Name for vector map of flow lines");
    flowlines_opt->guisection = _("Output");

    flowacc_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    flowacc_opt->key = "flowaccumulation";
    flowacc_opt->required = NO;
    flowacc_opt->description =
	_("Name for output flowaccumulation 3D raster");
    flowacc_opt->guisection = _("Output");

    sampled_opt = G_define_standard_option(G_OPT_R3_INPUT);
    sampled_opt->key = "sampled";
    sampled_opt->required = NO;
    sampled_opt->label =
            _("Name for 3D raster sampled by flowlines");
    sampled_opt->description =
            _("Values of this 3D raster will be stored "
              "as attributes of flowlines segments");

    unit_opt = G_define_option();
    unit_opt->key = "unit";
    unit_opt->type = TYPE_STRING;
    unit_opt->required = NO;
    unit_opt->answer = "cell";
    unit_opt->options = "time,length,cell";
    desc = NULL;
    G_asprintf(&desc,
	       "time;%s;"
	       "length;%s;"
	       "cell;%s",
	       _("elapsed time"),
	       _("length in map units"), _("length in cells (voxels)"));
    unit_opt->descriptions = desc;
    unit_opt->label = _("Unit of integration step");
    unit_opt->description = _("Default unit is cell");
    unit_opt->guisection = _("Integration");

    step_opt = G_define_option();
    step_opt->key = "step";
    step_opt->type = TYPE_DOUBLE;
    step_opt->required = NO;
    step_opt->answer = "0.25";
    step_opt->label = _("Integration step in selected unit");
    step_opt->description = _("Default step is 0.25 cell");
    step_opt->guisection = _("Integration");

    limit_opt = G_define_option();
    limit_opt->key = "limit";
    limit_opt->type = TYPE_INTEGER;
    limit_opt->required = NO;
    limit_opt->answer = "2000";
    limit_opt->description = _("Maximum number of steps");
    limit_opt->guisection = _("Integration");

    error_opt = G_define_option();
    error_opt->key = "max_error";
    error_opt->type = TYPE_DOUBLE;
    error_opt->required = NO;
    error_opt->answer = "1e-5";
    error_opt->label = _("Maximum error of integration");
    error_opt->description = _("Influences step, increase maximum error "
			       "to allow bigger steps");
    error_opt->guisection = _("Integration");

    skip_opt = G_define_option();
    skip_opt->key = "skip";
    skip_opt->type = TYPE_INTEGER;
    skip_opt->required = NO;
    skip_opt->multiple = YES;
    skip_opt->description =
	_("Number of cells between flow lines in x, y and z direction");

    dir_opt = G_define_option();
    dir_opt->key = "direction";
    dir_opt->type = TYPE_STRING;
    dir_opt->required = NO;
    dir_opt->multiple = NO;
    dir_opt->options = "up,down,both";
    dir_opt->answer = "down";
    dir_opt->description = _("Compute flowlines upstream, "
			     "downstream or in both direction.");

    table_fl = G_define_flag();
    table_fl->key = 'a';
    table_fl->description = _("Create and fill attribute table");

    G_option_required(scalar_opt, vector_opt, NULL);
    G_option_exclusive(scalar_opt, vector_opt, NULL);
    G_option_required(flowlines_opt, flowacc_opt, NULL);
    G_option_requires(seed_opt, flowlines_opt, NULL);
    G_option_requires(table_fl, flowlines_opt, NULL);
    G_option_requires(sampled_opt, table_fl, NULL);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    driver = NULL;
    finfo = NULL;

    if_table = table_fl->answer ? TRUE : FALSE;

    check_vector_input_maps(vector_opt, seed_opt);

    Rast3d_init_defaults();
    Rast3d_get_window(&region);

    /* set up integration variables */
    if (step_opt->answer) {
	integration.step = atof(step_opt->answer);
	integration.unit = unit_opt->answer;
    }
    else {
	integration.unit = "cell";
	integration.step = 0.25;
    }
    integration.max_error = atof(error_opt->answer);
    integration.max_step = 5 * integration.step;
    integration.min_step = integration.step / 5;
    integration.limit = atof(limit_opt->answer);
    if (strcmp(dir_opt->answer, "up") == 0)
	integration.direction_type = FLOWDIR_UP;
    else if (strcmp(dir_opt->answer, "down") == 0)
	integration.direction_type = FLOWDIR_DOWN;
    else
	integration.direction_type = FLOWDIR_BOTH;


    /* cell size is the diagonal */
    integration.cell_size = sqrt(region.ns_res * region.ns_res +
				 region.ew_res * region.ew_res +
				 region.tb_res * region.tb_res);

    /* set default skip if needed */
    if (skip_opt->answers) {
	for (i = 0; i < 3; i++) {
	    if (skip_opt->answers[i] != NULL) {
		skip[i] = atoi(skip_opt->answers[i]);
	    }
	    else {
		G_fatal_error(_("Please provide 3 integer values for skip option."));
	    }
	}
    }
    else {
	skip[0] = fmax(1, region.cols / 10);
	skip[1] = fmax(1, region.rows / 10);
	skip[2] = fmax(1, region.depths / 10);

    }

    /* open raster 3D maps of velocity components */
    gradient_info.initialized = FALSE;
    load_input_raster3d_maps(scalar_opt, vector_opt, &gradient_info, &region);


    /* open new 3D raster map of flowacumulation */
    if (flowacc_opt->answer) {
	flowacc = Rast3d_open_new_opt_tile_size(flowacc_opt->answer,
						RASTER3D_USE_CACHE_DEFAULT,
						&region, FCELL_TYPE, 32);


	if (!flowacc)
	    Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
			       flowacc_opt->answer);
	init_flowaccum(&region, flowacc);
    }

    /* open 3D raster map used for sampling */
    if (sampled_opt->answer) {
	sampled = Rast3d_open_cell_old(sampled_opt->answer,
				       G_find_raster3d(sampled_opt->answer, ""),
				       &region, RASTER3D_TILE_SAME_AS_FILE,
				       RASTER3D_USE_CACHE_DEFAULT);
	if (!sampled)
	    Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
			       sampled_opt->answer);
    }
    else
	sampled = NULL;

    /* open new vector map of flowlines */
    if (flowlines_opt->answer) {
	fl_cats = Vect_new_cats_struct();
	fl_points = Vect_new_line_struct();
	if (Vect_open_new(&fl_map, flowlines_opt->answer, TRUE) < 0)
	    G_fatal_error(_("Unable to create vector map <%s>"),
			  flowlines_opt->answer);

	Vect_hist_command(&fl_map);

	if (if_table) {
	    create_table(&fl_map, &finfo, &driver,
			 gradient_info.compute_gradient, sampled ? 1 : 0);
	}
    }

    n_seeds = 0;
    /* open vector map of seeds */
    if (seed_opt->answer) {
	if (Vect_open_old2(&seed_Map, seed_opt->answer, "", "1") < 0)
	    G_fatal_error(_("Unable to open vector map <%s>"),
			  seed_opt->answer);
	if (!Vect_is_3d(&seed_Map))
	    G_fatal_error(_("Vector map <%s> is not 3D"), seed_opt->answer);

	n_seeds = Vect_get_num_primitives(&seed_Map, GV_POINT);
    }
    if (flowacc_opt->answer || (!seed_opt->answer && flowlines_opt->answer)) {
	if (flowacc_opt->answer)
	    n_seeds += region.cols * region.rows * region.depths;
	else {
	    n_seeds += ceil(region.cols / (double)skip[0]) *
		ceil(region.rows / (double)skip[1]) *
		ceil(region.depths / (double)skip[2]);
	}
    }
    G_debug(1, "Number of seeds is %d", n_seeds);

    seed_count = 0;
    cat = 1;
    if (seed_opt->answer) {

	seed_points = Vect_new_line_struct();
	seed_cats = Vect_new_cats_struct();

	/* compute flowlines from vector seed map */
	while (TRUE) {
	    ltype = Vect_read_next_line(&seed_Map, seed_points, seed_cats);
	    if (ltype == -1) {
		Vect_close(&seed_Map);
		G_fatal_error(_("Error during reading seed vector map"));
	    }
	    else if (ltype == -2) {
		break;
	    }
	    else if (ltype == GV_POINT) {
		seed.x = seed_points->x[0];
		seed.y = seed_points->y[0];
		seed.z = seed_points->z[0];
		seed.flowline = TRUE;
		seed.flowaccum = FALSE;
	    }
	    G_percent(seed_count, n_seeds, 1);
	    if (integration.direction_type == FLOWDIR_UP ||
		integration.direction_type == FLOWDIR_BOTH) {
		integration.actual_direction = FLOWDIR_UP;
		compute_flowline(&region, &seed, &gradient_info, flowacc, sampled,
				 &integration, &fl_map, fl_cats, fl_points,
				 &cat, if_table, finfo, driver);
	    }
	    if (integration.direction_type == FLOWDIR_DOWN ||
		integration.direction_type == FLOWDIR_BOTH) {
		integration.actual_direction = FLOWDIR_DOWN;
		compute_flowline(&region, &seed, &gradient_info, flowacc, sampled,
				 &integration, &fl_map, fl_cats, fl_points,
				 &cat, if_table, finfo, driver);
	    }
	    seed_count++;
	}

	Vect_destroy_line_struct(seed_points);
	Vect_destroy_cats_struct(seed_cats);
	Vect_close(&seed_Map);
    }
    if (flowacc_opt->answer || (!seed_opt->answer && flowlines_opt->answer)) {
	/* compute flowlines from points on grid */
	for (r = region.rows; r > 0; r--) {
	    for (c = 0; c < region.cols; c++) {
		for (d = 0; d < region.depths; d++) {
		    seed.x =
			region.west + c * region.ew_res + region.ew_res / 2;
		    seed.y =
			region.south + r * region.ns_res - region.ns_res / 2;
		    seed.z =
			region.bottom + d * region.tb_res + region.tb_res / 2;
		    seed.flowline = FALSE;
		    seed.flowaccum = FALSE;
		    if (flowacc_opt->answer)
			seed.flowaccum = TRUE;

		    if (flowlines_opt->answer && !seed_opt->answer &&
		       (c % skip[0] == 0) && (r % skip[1] == 0) && (d % skip[2] == 0))
			seed.flowline = TRUE;

		    if (seed.flowaccum || seed.flowline) {
			G_percent(seed_count, n_seeds, 1);

			if (integration.direction_type == FLOWDIR_UP ||
			    integration.direction_type == FLOWDIR_BOTH) {
			    integration.actual_direction = FLOWDIR_UP;
			    compute_flowline(&region, &seed, &gradient_info,
					     flowacc, sampled, &integration, &fl_map,
					     fl_cats, fl_points, &cat,
					     if_table, finfo, driver);
			}
			if (integration.direction_type == FLOWDIR_DOWN ||
			    integration.direction_type == FLOWDIR_BOTH) {
			    integration.actual_direction = FLOWDIR_DOWN;
			    compute_flowline(&region, &seed, &gradient_info,
					     flowacc, sampled, &integration, &fl_map,
					     fl_cats, fl_points, &cat,
					     if_table, finfo, driver);
			}
			seed_count++;
		    }
		}
	    }
	}
    }
    G_percent(1, 1, 1);
    if (flowlines_opt->answer) {
	if (if_table) {
	    db_commit_transaction(driver);
	    db_close_database_shutdown_driver(driver);
	}
	Vect_destroy_line_struct(fl_points);
	Vect_destroy_cats_struct(fl_cats);
	Vect_build(&fl_map);
	Vect_close(&fl_map);
    }

    if (flowacc_opt->answer)
	Rast3d_close(flowacc);


    return EXIT_SUCCESS;
}
