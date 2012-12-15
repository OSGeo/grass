
/******************************************************************************
 * MODULE:       v.in.db 
 *               
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Create new vector from db table.
 * 	    
 * COPYRIGHT:    (C) 2000-2007, 2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    int i, cat, with_z, more, ctype, ret, nrows;
    char buf[2000];
    int count;
    double coor[3];
    int ncoor;
    int coltype;
    struct Option *driver_opt, *database_opt, *table_opt;
    struct Option *xcol_opt, *ycol_opt, *zcol_opt, *keycol_opt, *where_opt,
	*outvect;
    struct Flag *same_table_flag;
    struct GModule *module;
    struct Map_info Map;
    struct line_pnts *Points;
    struct line_cats *Cats;
    dbString sql;
    dbDriver *driver;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    struct field_info *fi;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword(_("database"));
    G_add_keyword(_("points"));
    module->description =
	_("Creates new vector (points) map from database table containing coordinates.");

    table_opt = G_define_standard_option(G_OPT_DB_TABLE);
    table_opt->required = YES;
    table_opt->description = _("Input table name");

    driver_opt = G_define_standard_option(G_OPT_DB_DRIVER);
    driver_opt->options = db_list_drivers();
    driver_opt->answer = (char *)db_get_default_driver_name();
    driver_opt->guisection = _("Connection");

    database_opt = G_define_standard_option(G_OPT_DB_DATABASE);
    database_opt->answer = (char *)db_get_default_database_name();
    database_opt->guisection = _("Connection");

    xcol_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    xcol_opt->key = "x";
    xcol_opt->required = YES;
    xcol_opt->description = _("Name of column containing x coordinate");

    ycol_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    ycol_opt->key = "y";
    ycol_opt->required = YES;
    ycol_opt->description = _("Name of column containing y coordinate");

    zcol_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    zcol_opt->key = "z";
    zcol_opt->description = _("Name of column containing z coordinate");

    keycol_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    keycol_opt->key = "key";
    keycol_opt->required = YES;
    keycol_opt->label = _("Name of column containing category number");
    keycol_opt->description = _("Must refer to an integer column");

    where_opt = G_define_standard_option(G_OPT_DB_WHERE);

    outvect = G_define_standard_option(G_OPT_V_OUTPUT);

    same_table_flag = G_define_flag();
    same_table_flag->key = 't';
    same_table_flag->guisection = _("Connection");
    same_table_flag->description =
	_("Use imported table as attribute table for new map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (zcol_opt->answer) {
	with_z = WITH_Z;
	ncoor = 3;
    }
    else {
	with_z = WITHOUT_Z;
	ncoor = 2;
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    db_init_string(&sql);

    Vect_open_new(&Map, outvect->answer, with_z);
    Vect_set_error_handler_io(NULL, &Map);
    
    Vect_hist_command(&Map);

    fi = Vect_default_field_info(&Map, 1, NULL, GV_1TABLE);

    /* Open driver */
    driver = db_start_driver_open_database(driver_opt->answer,
					   database_opt->answer);
    if (driver == NULL) {
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      fi->database, fi->driver);
    }

    /* check if target table already exists */
    G_debug(3, "Output vector table <%s>, driver: <%s>, database: <%s>",
	    outvect->answer, db_get_default_driver_name(),
	    db_get_default_database_name());

    if (db_table_exists(db_get_default_driver_name(),
			db_get_default_database_name(), outvect->answer) == 1)
	G_fatal_error(_("Output vector map, table <%s> (driver: <%s>, database: <%s>) "
		       "already exists"), outvect->answer,
		      db_get_default_driver_name(),
		      db_get_default_database_name());

    coltype = db_column_Ctype(driver, table_opt->answer, keycol_opt->answer);

    if (coltype == -1)
	G_fatal_error(_("Missing column <%s> in table <%s>"),
		      keycol_opt->answer, table_opt->answer);
    if (coltype != DB_C_TYPE_INT)
	G_fatal_error(_("Data type of key column must be integer"));

    /* Open select cursor */
    sprintf(buf, "select %s, %s, %s", keycol_opt->answer, xcol_opt->answer,
	    ycol_opt->answer);
    db_set_string(&sql, buf);

    if (with_z) {
	sprintf(buf, ", %s", zcol_opt->answer);
	db_append_string(&sql, buf);
    }

    sprintf(buf, " from %s", table_opt->answer);
    db_append_string(&sql, buf);

    if (where_opt->answer) {
	sprintf(buf, " WHERE %s", where_opt->answer);
	db_append_string(&sql, buf);
    }
    G_debug(2, "SQL: %s", db_get_string(&sql));

    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK) {
	G_fatal_error(_("Unable to open select cursor: '%s'"),
		      db_get_string(&sql));
    }

    table = db_get_cursor_table(&cursor);
    nrows = db_get_num_rows(&cursor);

    G_debug(2, "%d points selected", nrows);

    count = 0;
    G_message(_("Writing features..."));
    while (db_fetch(&cursor, DB_NEXT, &more) == DB_OK && more) {
	G_percent(count, nrows, 2);
	/* key column */
	column = db_get_table_column(table, 0);
	ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
	if (ctype != DB_C_TYPE_INT)
	    G_fatal_error(_("Key column must be integer"));
	value = db_get_column_value(column);
	cat = db_get_value_int(value);

	for (i = 0; i < ncoor; i++) {
	    column = db_get_table_column(table, i + 1);
	    ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
	    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
		G_fatal_error(_("x/y/z column must be integer or double"));
	    value = db_get_column_value(column);
	    if (ctype == DB_C_TYPE_INT)
		coor[i] = (double)db_get_value_int(value);
	    else
		coor[i] = db_get_value_double(value);
	}

	Vect_reset_line(Points);
	Vect_reset_cats(Cats);

	Vect_append_point(Points, coor[0], coor[1], coor[2]);

	Vect_cat_set(Cats, 1, cat);

	Vect_write_line(&Map, GV_POINT, Points, Cats);

	count++;
    }
    G_percent(1, 1, 1);

    db_close_database_shutdown_driver(driver);

    /* Copy table */
    G_message(_("Copying attributes..."));
    if (!same_table_flag->answer) {
	if (where_opt->answer)
	    ret =
		db_copy_table_where(driver_opt->answer, database_opt->answer,
				    table_opt->answer, fi->driver,
				    fi->database, fi->table,
				    where_opt->answer);
	else
	    ret =
		db_copy_table(driver_opt->answer, database_opt->answer,
			      table_opt->answer, fi->driver, fi->database,
			      fi->table);
	if (ret == DB_FAILED) {
	    G_warning(_("Unable to copy table"));
	}
	else {
	    Vect_map_add_dblink(&Map, 1, NULL, fi->table, keycol_opt->answer,
				fi->database, fi->driver);
	}
    }
    else {
	Vect_map_add_dblink(&Map, 1, NULL, table_opt->answer,
			    keycol_opt->answer, database_opt->answer,
			    driver_opt->answer);
    }

    Vect_build(&Map);
    Vect_close(&Map);

    G_done_msg(_("%d points written to vector map."), count);

    return (EXIT_SUCCESS);
}
