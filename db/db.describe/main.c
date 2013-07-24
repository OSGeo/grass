
/****************************************************************************
 *
 * MODULE:       db.describe
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>,
 *               Stephan Holl
 * PURPOSE:      Displays table information
 * COPYRIGHT:    (C) 2002-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "local_proto.h"


struct
{
    char *driver, *database, *table;
    int printcolnames;
} parms;


/* function prototypes */
static void parse_command_line(int, char **);


int main(int argc, char **argv)
{
    dbDriver *driver;
    dbHandle handle;
    dbTable *table;
    dbString table_name;
    int col, ncols, nrows;
    dbColumn *column;
    char buf[1024];
    dbString stmt;

    parse_command_line(argc, argv);
    if (!db_table_exists(parms.driver, parms.database, parms.table)) {
	G_message(_("Table <%s> not found in database <%s> using driver <%s>"),
		   parms.table, parms.database, parms.driver);
	exit(EXIT_SUCCESS);
    }
    driver = db_start_driver(parms.driver);
    if (driver == NULL)
	G_fatal_error(_("Unable to start driver <%s>"), parms.driver);

    db_init_handle(&handle);
    db_set_handle(&handle, parms.database, NULL);
    if (db_open_database(driver, &handle) != DB_OK)
	G_fatal_error(_("Unable to open database <%s>"), parms.database);

    db_init_string(&table_name);
    db_set_string(&table_name, parms.table);

    if (db_describe_table(driver, &table_name, &table) != DB_OK)
	G_fatal_error(_("Unable to describe table <%s>"), db_get_string(&table_name));

    if (!parms.printcolnames)
	print_table_definition(driver, table);
    else {
	ncols = db_get_table_number_of_columns(table);

	db_init_string(&stmt);
	sprintf(buf, "select * from %s", db_get_table_name(table));
	db_set_string(&stmt, buf);
	nrows = db_get_table_number_of_rows(driver, &stmt);
	fprintf(stdout, "ncols: %d\n", ncols);
	fprintf(stdout, "nrows: %d\n", nrows);
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    fprintf(stdout, "Column %d: %s:%s:%d\n", (col + 1),
		    db_get_column_name(column),
		    db_sqltype_name(db_get_column_sqltype(column)),
		    db_get_column_length(column));
	}
    }

    db_close_database(driver);
    db_shutdown_driver(driver);

    exit(EXIT_SUCCESS);
}


static void parse_command_line(int argc, char **argv)
{
    struct Option *driver, *database, *table;
    struct Flag *cols, *tdesc;
    struct GModule *module;
    const char *drv, *db;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    cols = G_define_flag();
    cols->key = 'c';
    cols->description = _("Print column names only instead "
			  "of full column descriptions");

    tdesc = G_define_flag();
    tdesc->key = 't';
    tdesc->description = _("Print table structure");

    table = G_define_standard_option(G_OPT_DB_TABLE);
    table->required = YES;

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    if ((drv = db_get_default_driver_name()))
      driver->answer = (char *) drv;

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    if ((db = db_get_default_database_name()))
	database->answer = (char *) db;

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    module->description = _("Describes a table in detail.");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    parms.driver = driver->answer;
    parms.database = database->answer;
    parms.table = table->answer;
    parms.printcolnames = cols->answer;
}
