/****************************************************************************
 *
 * MODULE:       v.mkgrid
 * AUTHOR(S):    Written by GRASS, Fall of 88, Michael Higgins CERL (original contributor)
 *               Updated <02 Jun 1992> by Darrell McCauley <mccauley@ecn.purdue.edu> angle option added.
 *               Upgrade to 5.7 Radim Blazek 10/2004
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>, Markus Neteler <neteler itc.it>
 *               Ivan Shevlakov: points support -p
 *               Luca Delucchi: lines support -l, vertical breaks
 *               Markus Metz: rewrite, hexagon creation 
 * PURPOSE:
 * COPYRIGHT:    (C) 1999-2014 by the GRASS Development Team
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
    struct Option *vectname, *grid, *coord, *box, *angle, *position_opt,
                  *breaks, *type_opt;
    struct GModule *module;
    struct Flag *hex_flag, *ha_flag, *diag_flag;
    int otype, ptype, ltype, diag;
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
    G_add_keyword(_("grid"));
    G_add_keyword(_("point pattern"));
    G_add_keyword(_("hexagon"));
    module->description = _("Creates a vector map of a user-defined grid.");

    vectname = G_define_standard_option(G_OPT_V_OUTPUT);
    vectname->key = "map";

    grid = G_define_option();
    grid->key = "grid";
    grid->key_desc = _("rows,columns");
    grid->type = TYPE_INTEGER;
    grid->required = NO;
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

    coord = G_define_standard_option(G_OPT_M_COORDS);
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
	_("Number of vertex points per grid cell");
    breaks->options = "0-60";
    breaks->answer = "0";

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,line,area";
    type_opt->answer = "area";
    type_opt->multiple = NO;
    type_opt->description = _("Output feature type");

    hex_flag = G_define_flag();
    hex_flag->key = 'h';
    hex_flag->description =
	_("Create hexagons (default: rectangles)");

    ha_flag = G_define_flag();
    ha_flag->key = 'a';
    ha_flag->description =
	_("Allow asymmetric hexagons");

    diag_flag = G_define_flag();
    diag_flag->key = 'd';
    diag_flag->label =
	_("EXPERIMENTAL: Add diagonals to rectangular lines");
    diag_flag->description =
	_("Applies only to lines for rectangles");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    otype = 0;
    switch (type_opt->answer[0]) {
    case 'p':
	otype |= GV_POINT;
	break;
    case 'l':
	otype |= GV_LINE;
	break;
    case 'a':
	otype |= GV_CENTROID;
	otype |= GV_BOUNDARY;
	break;
    }

    ptype = (otype & GV_POINTS);
    ltype = (otype & GV_LINES);

    diag = (otype == GV_LINE && !hex_flag->answer && diag_flag->answer);
    if (diag) {
	ptype = 0;
	ltype = GV_LINE;
	hex_flag->answer = 0;
    }

    /* get the current window  */
    G_get_window(&window);

    /*
     * information we need to collect from user: origin point x and y (lower
     * left), shift in x, shift in y,  number of rows, number of cols
     */
    dig_file = G_store(vectname->answer);

    grid_info.angle = M_PI / 180 * atof(angle->answer);
    set_angle(grid_info.angle);

    nbreaks = atoi(breaks->answer);

    /* Position */
    if (position_opt->answer[0] == 'r') {	/* region */
	if (coord->answer)
	    G_fatal_error(_("'coor' and 'position=region' are exclusive options"));

	if (box->answer && grid->answer)
	    G_fatal_error(_("'box' and 'grid' are exclusive options for 'position=region'"));

	grid_info.west = window.west;
	grid_info.south = window.south;
	grid_info.east = window.east;
	grid_info.north = window.north;

	grid_info.num_rows = window.rows;
	grid_info.num_cols = window.cols;

	grid_info.width = window.ew_res;
	grid_info.height = window.ns_res;

	if (grid->answer) {
	    grid_info.num_rows = atoi(grid->answers[0]);
	    grid_info.num_cols = atoi(grid->answers[1]);

	    grid_info.width = (grid_info.east - grid_info.west) / grid_info.num_cols;
	    grid_info.height = (grid_info.north - grid_info.south) / grid_info.num_rows;
	}
	else if (box->answer) {
	    if (!G_scan_resolution
		(box->answers[0], &(grid_info.width), window.proj))
		G_fatal_error(_("Invalid width"));
	    if (!G_scan_resolution
		(box->answers[1], &(grid_info.height), window.proj))
		G_fatal_error(_("Invalid height"));

	    /* register to lower left corner as for position=coor */
	    grid_info.num_cols = (grid_info.east - grid_info.west + 
	                         grid_info.width / 2.0) / grid_info.width;
	    grid_info.num_rows = (grid_info.north - grid_info.south + 
	                         grid_info.height / 2.0) / grid_info.height;
	    grid_info.north = grid_info.south + grid_info.num_rows * grid_info.height;
	    grid_info.east = grid_info.west + grid_info.num_cols * grid_info.width;
	}

	G_debug(2, "x = %e y = %e w = %e h = %e", grid_info.west,
		grid_info.south, grid_info.width, grid_info.height);
    }
    else {
	if (!grid->answer)
	    G_fatal_error(_("'grid' option missing"));

	if (!coord->answer)
	    G_fatal_error(_("'coor' option missing"));

	if (!box->answer)
	    G_fatal_error(_("'box' option missing"));

	if (!G_scan_easting
	    (coord->answers[0], &(grid_info.west), window.proj))
	    G_fatal_error(_("Invalid easting"));
	if (!G_scan_northing
	    (coord->answers[1], &(grid_info.south), window.proj))
	    G_fatal_error(_("Invalid northing"));

	if (!G_scan_resolution
	    (box->answers[0], &(grid_info.width), window.proj))
	    G_fatal_error(_("Invalid width"));
	if (!G_scan_resolution
	    (box->answers[1], &(grid_info.height), window.proj))
	    G_fatal_error(_("Invalid height"));

	grid_info.num_rows = atoi(grid->answers[0]);
	grid_info.num_cols = atoi(grid->answers[1]);

	grid_info.east = grid_info.west + grid_info.width * grid_info.num_cols;
	grid_info.north = grid_info.south + grid_info.height * grid_info.num_rows;
    }
    grid_info.xo = (grid_info.east + grid_info.west) / 2.0;
    grid_info.yo = (grid_info.north + grid_info.south) / 2.0;

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
    db_set_error_handler_driver(Driver);
    db_init_string(&sql);

    if (hex_flag->answer) {

	grid_info.num_vect_rows = 2 * grid_info.num_rows + 1;
	grid_info.num_vect_cols = grid_info.num_cols;

	/* figure out hexagon radius */
	grid_info.rstep = (grid_info.north - grid_info.south) /
	                  (grid_info.num_rows * 2);
	grid_info.rrad = grid_info.rstep * 2 / sqrt(3.0);

	grid_info.cstep = (grid_info.east - grid_info.west) / 
	                  (grid_info.num_cols + 1.0 / 3.0);
	grid_info.crad = grid_info.cstep / 1.5;

	if (!ha_flag->answer || grid_info.width == grid_info.height) {
	    if (grid_info.rrad > grid_info.crad) {
		grid_info.rrad = grid_info.crad;
		grid_info.rstep = grid_info.rrad * sqrt(3.0) / 2.0;
	    }
	    else if (grid_info.crad > grid_info.rrad) {
		grid_info.crad = grid_info.rrad;
		grid_info.cstep = grid_info.crad * 1.5;
	    }
	}
	else {
	    if (grid_info.width != grid_info.height) {
		G_important_message(_("The hexagons will be asymmetrical."));
	    }
	}

	grid_info.num_vect_rows = (grid_info.north - grid_info.south) / 
	     grid_info.rstep;
	if (grid_info.north - grid_info.rstep * (grid_info.num_vect_rows + 1) < 
	    grid_info.south)
	    grid_info.num_vect_rows--;
	grid_info.num_vect_cols = (grid_info.east - grid_info.west -
	     grid_info.crad * 0.5) / grid_info.cstep;

	if (grid_info.east - grid_info.west < 3.5 * grid_info.crad) {
	    G_fatal_error(_("Please use a higher resolution or a larger region"));
	}
	if (grid_info.north - grid_info.south < 3 * grid_info.rstep) {
	    G_fatal_error(_("Please use a higher resolution or a larger region"));
	}

	if ((int)(grid_info.num_vect_rows / 2.0 + 0.5) != grid_info.num_rows)
	    G_message(_("The number of rows has been adjusted from %d to %d"),
	              grid_info.num_rows, (int)(grid_info.num_vect_rows / 2.0 + 0.5));
	if (grid_info.num_vect_cols != grid_info.num_cols)
	    G_message(_("The number of columns has been adjusted from %d to %d"),
	              grid_info.num_cols, grid_info.num_vect_cols);

	sprintf(buf, "create table %s ( %s integer)", Fi->table, Fi->key);

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

	attCount = hexgrid(&grid_info, &Map, nbreaks, otype);

	db_begin_transaction(Driver);

	for (i = 1; i <= attCount; ++i) {

	    sprintf(buf, "insert into %s values ( %d )", Fi->table, i);
	    if (db_set_string(&sql, buf) != DB_OK)
		G_fatal_error(_("Unable to fill attribute table"));

	    G_debug(3, "SQL: %s", db_get_string(&sql));
	    if (db_execute_immediate(Driver, &sql) != DB_OK) {
		G_fatal_error(_("Unable to insert new record: %s"),
			      db_get_string(&sql));
	    }
	}
	db_commit_transaction(Driver);
    }
    else {
	if (grid_info.width != grid_info.height) {
	    G_important_message(_("The rectangles will be asymmetrical."));
	}

	/*
	 * vector rows are the actual number of rows of vectors to make up the
	 * entire grid.   ditto for cols.
	 */
	grid_info.num_vect_rows = grid_info.num_rows + 1;
	grid_info.num_vect_cols = grid_info.num_cols + 1;

	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();

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

	if (ltype) {
	    /* create areas */
	    write_grid(&grid_info, &Map, nbreaks, ltype, diag);
	}

	/* Create a grid of label points at the centres of the grid cells */
	G_message(_("Creating centroids..."));

	/* Write out centroids and attributes */
	/* If the output id is lines it skips to add centroids and attributes
	   TODO decide what to write in the attribute table
	 */
	if (ptype) {
            db_begin_transaction(Driver);
	    attCount = 0;
	    for (i = 0; i < grid_info.num_rows; ++i) {
                G_percent(i, grid_info.num_rows, 2);
	        for (j = 0; j < grid_info.num_cols; ++j) {
		    double x, y;

		    x = grid_info.west + (0.5 + j) * grid_info.width;
		    y = grid_info.south + (0.5 + i) * grid_info.height;

		    rotate(&x, &y, grid_info.xo, grid_info.yo,
			   grid_info.angle);

		    Vect_reset_line(Points);
		    Vect_reset_cats(Cats);

		    Vect_append_point(Points, x, y, 0.0);
		    Vect_cat_set(Cats, 1, attCount + 1);
		    Vect_write_line(&Map, ptype, Points, Cats);

		    sprintf(buf, "insert into %s values ", Fi->table);
		    if (db_set_string(&sql, buf) != DB_OK)
		        G_fatal_error(_("Unable to fill attribute table"));

		    if (grid_info.num_rows < 27 && grid_info.num_cols < 27) {
		        sprintf(buf, "( %d, %d, %d, '%c', '%c' )",
			      attCount + 1, grid_info.num_rows - i,
			      j + 1, 'A' + grid_info.num_rows - i - 1, 'A' + j);
		    }
		    else {
		        sprintf(buf, "( %d, %d, %d )",
			        attCount + 1, grid_info.num_rows - i, j + 1);
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
            G_percent(1, 1, 1);
	}
    }

    db_close_database_shutdown_driver(Driver);

    Vect_build(&Map);
    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
