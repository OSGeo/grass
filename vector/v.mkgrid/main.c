/****************************************************************************
 *
 * MODULE:       v.mkgrid
 * AUTHOR(S):    Written by GRASS, Fall of 88, Michael Higgins CERL (original contributor)
 *               Updated <02 Jun 1992> by Darrell McCauley <mccauley@ecn.purdue.edu> angle option added.
 *               Upgrade to 5.7 Radim Blazek 10/2004
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>, Markus Neteler <neteler itc.it>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "grid_structs.h"
#include "local_proto.h"

int main(int argc, char *argv[])
{

    /* loop */
    int i, j;

    /* store filename and path  */
    char *dig_file;
    
    char buf[2000];

    /* Other local variables */
    int attCount, nbreaks;

    struct grid_description grid_info;
    struct Cell_head window;
    struct Map_info Map;
    struct Option *vectname, *grid, *coord, *box, *angle, *position_opt, *breaks;
    struct GModule *module;
    struct Flag *points_fl;
    int points_p;
    char *desc;

    struct line_pnts *Points;
    struct line_cats *Cats;

    /* Attributes */
    struct field_info *Fi;
    dbDriver *Driver;
    dbString sql;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    module->description = _("Creates a vector map of a user-defined grid.");

    vectname = G_define_standard_option(G_OPT_V_OUTPUT);
    vectname->key = "map";

    grid = G_define_option();
    grid->key = "grid";
    grid->key_desc = _("rows,columns");
    grid->type = TYPE_INTEGER;
    grid->required = YES;
    grid->multiple = NO;
    grid->description = _("Number of rows and columns in grid");

    position_opt = G_define_option();
    position_opt->key = "position";
    position_opt->type = TYPE_STRING;
    position_opt->required = NO;
    position_opt->multiple = NO;
    position_opt->options = "region,coor";
    position_opt->answer = "region";
    position_opt->description = _("Where to place the grid");
    desc = NULL;
    G_asprintf(&desc,
            "region;%s;coor;%s",
            _("current region"),
            _("use 'coor' and 'box' options"));
    position_opt->descriptions = desc;

    coord = G_define_option();
    coord->key = "coor";
    coord->key_desc = "x,y";
    coord->type = TYPE_DOUBLE;
    coord->required = NO;
    coord->multiple = NO;
    coord->description =
	_("Lower left easting and northing coordinates of map");

    box = G_define_option();
    box->key = "box";
    box->key_desc = _("width,height");
    box->type = TYPE_DOUBLE;
    box->required = NO;
    box->multiple = NO;
    box->description = _("Width and height of boxes in grid");

    angle = G_define_option();
    angle->key = "angle";
    angle->type = TYPE_DOUBLE;
    angle->required = NO;
    angle->description =
	_("Angle of rotation (in degrees counter-clockwise)");
    angle->answer = "0";

    breaks = G_define_option();
    breaks->key = "breaks";
    breaks->type = TYPE_INTEGER;
    breaks->required = NO;
    breaks->description =
	_("Number of horizontal vertex points per grid cell");
    breaks->options = "0-60";
    breaks->answer = "3";

    points_fl = G_define_flag();
    points_fl->key = 'p';
    points_fl->description =
	_("Create grid of points instead of areas and centroids");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    points_p = points_fl->answer;

    /* get the current window  */
    G_get_window(&window);

    /*
     * information we need to collect from user: origin point x and y (lower
     * left), shift in x, shift in y,  number of rows, number of cols
     */
    dig_file = G_store(vectname->answer);

    /* Number of row and cols */
    grid_info.num_rows = atoi(grid->answers[0]);
    grid_info.num_cols = atoi(grid->answers[1]);

    grid_info.angle = M_PI / 180 * atof(angle->answer);

    nbreaks = atoi(breaks->answer);

    /* Position */
    if (position_opt->answer[0] == 'r') {	/* region */
	if (coord->answer)
	    G_fatal_error(_("'coor' and 'position=region' are exclusive options"));

	if (box->answer)
	    G_fatal_error(_("'box' and 'position=region' are exclusive options"));

	if (grid_info.angle != 0.0) {
	    G_fatal_error(_("'angle' and 'position=region' are exclusive options"));
	    grid_info.angle = 0.0;
	}

	grid_info.origin_x = window.west;
	grid_info.origin_y = window.south;

	grid_info.length = (window.east - window.west) / grid_info.num_cols;
	grid_info.width = (window.north - window.south) / grid_info.num_rows;

	G_debug(2, "x = %e y = %e l = %e w = %e", grid_info.origin_x,
		grid_info.origin_y, grid_info.length, grid_info.width);
    }
    else {
	if (!coord->answer)
	    G_fatal_error(_("'coor' option missing"));

	if (!box->answer)
	    G_fatal_error(_("'box' option missing"));

	if (!G_scan_easting
	    (coord->answers[0], &(grid_info.origin_x), window.proj))
	    G_fatal_error(_("Invalid easting"));;
	if (!G_scan_northing
	    (coord->answers[1], &(grid_info.origin_y), window.proj))
	    G_fatal_error(_("Invalid northing"));;

	if (!G_scan_resolution
	    (box->answers[0], &(grid_info.length), window.proj))
	    G_fatal_error(_("Invalid distance"));;
	if (!G_scan_resolution
	    (box->answers[1], &(grid_info.width), window.proj))
	    G_fatal_error(_("Invalid distance"));;

    }

    /*
     * vector rows are the actual number of rows of vectors to make up the
     * entire grid.   ditto for cols.
     */
    grid_info.num_vect_rows = grid_info.num_rows + 1;
    grid_info.num_vect_cols = grid_info.num_cols + 1;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    db_init_string(&sql);

    /* Open output map */
    if (0 > Vect_open_new(&Map, dig_file, 0)) {
	G_fatal_error(_("Unable to create vector map <%s>"), dig_file);
    }

    Vect_hist_command(&Map);

    /* Open database, create table */
    Fi = Vect_default_field_info(&Map, 1, NULL, GV_1TABLE);
    Vect_map_add_dblink(&Map, Fi->number, Fi->name, Fi->table, Fi->key,
			Fi->database, Fi->driver);

    Driver =
	db_start_driver_open_database(Fi->driver,
				      Vect_subst_var(Fi->database, &Map));
    if (Driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    if (grid_info.num_rows < 27 && grid_info.num_cols < 27) {
	sprintf(buf,
		"create table %s ( cat integer, row integer, col integer, "
		"rown varchar(1), coln varchar(1))", Fi->table);
    }
    else {
	sprintf(buf,
		"create table %s ( cat integer, row integer, col integer)",
		Fi->table);
    }
    db_set_string(&sql, buf);

    G_debug(1, "SQL: %s", db_get_string(&sql));

    if (db_execute_immediate(Driver, &sql) != DB_OK) {
	G_fatal_error(_("Unable to create table: %s"), db_get_string(&sql));
    }

    if (db_create_index2(Driver, Fi->table, Fi->key) != DB_OK)
	G_warning(_("Unable to create index"));

    if (db_grant_on_table
	(Driver, Fi->table, DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Unable to grant privileges on table <%s>"),
		      Fi->table);

    if (!points_p) {
	/* create areas */
	write_grid(&grid_info, &Map, nbreaks);
    }

    /* Create a grid of label points at the centres of the grid cells */
    G_verbose_message(_("Creating centroids..."));

    /* Write out centroids and attributes */
    db_begin_transaction(Driver);
    attCount = 0;
    for (i = 0; i < grid_info.num_rows; ++i) {
	for (j = 0; j < grid_info.num_cols; ++j) {
	    double x, y;
	    const int point_type = points_p ? GV_POINT : GV_CENTROID;

	    x = grid_info.origin_x + (0.5 + j) * grid_info.length;
	    y = grid_info.origin_y + (0.5 + i) * grid_info.width;

	    rotate(&x, &y, grid_info.origin_x, grid_info.origin_y,
		   grid_info.angle);

	    Vect_reset_line(Points);
	    Vect_reset_cats(Cats);

	    Vect_append_point(Points, x, y, 0.0);
	    Vect_cat_set(Cats, 1, attCount + 1);
	    Vect_write_line(&Map, point_type, Points, Cats);

	    sprintf(buf, "insert into %s values ", Fi->table);
	    if (db_set_string(&sql, buf) != DB_OK)
		G_fatal_error(_("Unable to fill attribute table"));

            if (grid_info.num_rows < 27 && grid_info.num_cols < 27) {
		sprintf(buf,
			"( %d, %d, %d, '%c', '%c' )",
			attCount + 1, grid_info.num_rows - i,
			j + 1, 'A' + grid_info.num_rows - i - 1, 'A' + j);
	    }
	    else {
		sprintf(buf, "( %d, %d, %d )",
			attCount + 1, i + 1, j + 1);
	    }
            if (db_append_string(&sql, buf) != DB_OK)
                    G_fatal_error(_("Unable to fill attribute table"));

	    G_debug(3, "SQL: %s", db_get_string(&sql));

	    if (db_execute_immediate(Driver, &sql) != DB_OK) {
		G_fatal_error(_("Unable to insert new record: %s"),
			    db_get_string(&sql));
	    }
	    attCount++;
	}
    }
    db_commit_transaction(Driver);

    db_close_database_shutdown_driver(Driver);

    Vect_build(&Map);
    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
