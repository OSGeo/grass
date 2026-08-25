/****************************************************************************
 *
 * MODULE:       db.describe
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>,
 *               Stephan Holl
 * PURPOSE:      Displays table information
 * COPYRIGHT:    (C) 2002-2021 by the GRASS Development Team
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
#include <grass/gjson.h>
#include "local_proto.h"

struct {
    char *driver, *database, *table;
    int printcolnames;
    enum OutputFormat format;
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

    G_JSON_Object *root_object = NULL, *col_object = NULL;
    G_JSON_Value *root_value = NULL, *cols_value = NULL, *col_value = NULL;
    G_JSON_Array *cols_array = NULL;

    parse_command_line(argc, argv);

    if (parms.format == JSON) {
        root_value = G_json_value_init_object();
        if (root_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        root_object = G_json_object(root_value);
        cols_value = G_json_value_init_array();
        if (cols_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        cols_array = G_json_array(cols_value);
    }

    if (!db_table_exists(parms.driver, parms.database, parms.table)) {
        G_warning(_("Table <%s> not found in database <%s> using driver <%s>"),
                  parms.table, parms.database, parms.driver);
        exit(EXIT_FAILURE);
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
        G_fatal_error(_("Unable to describe table <%s>"),
                      db_get_string(&table_name));

    if (!parms.printcolnames)
        print_table_definition(driver, table, parms.format, root_object,
                               cols_array);
    else {
        ncols = db_get_table_number_of_columns(table);

        db_init_string(&stmt);
        snprintf(buf, sizeof(buf), "select * from %s",
                 db_get_table_name(table));
        db_set_string(&stmt, buf);
        nrows = db_get_table_number_of_rows(driver, &stmt);

        switch (parms.format) {
        case PLAIN:
            fprintf(stdout, "ncols: %d\n", ncols);
            fprintf(stdout, "nrows: %d\n", nrows);
            break;
        case JSON:
            G_json_object_set_number(root_object, "ncols", ncols);
            G_json_object_set_number(root_object, "nrows", nrows);
            break;
        }

        for (col = 0; col < ncols; col++) {
            column = db_get_table_column(table, col);

            switch (parms.format) {
            case PLAIN:
                fprintf(stdout, "Column %d: %s:%s:%d\n", (col + 1),
                        db_get_column_name(column),
                        db_sqltype_name(db_get_column_sqltype(column)),
                        db_get_column_length(column));
                break;
            case JSON:
                col_value = G_json_value_init_object();
                col_object = G_json_object(col_value);
                G_json_object_set_number(col_object, "position", col + 1);
                G_json_object_set_string(col_object, "name",
                                         db_get_column_name(column));
                G_json_object_set_string(
                    col_object, "type",
                    db_sqltype_name(db_get_column_sqltype(column)));
                G_json_object_set_number(col_object, "length",
                                         db_get_column_length(column));
                G_json_array_append_value(cols_array, col_value);
                break;
            }
        }
    }

    if (parms.format == JSON) {
        G_json_object_set_value(root_object, "columns", cols_value);
        char *serialized_string = NULL;
        serialized_string = G_json_serialize_to_string_pretty(root_value);
        if (serialized_string == NULL) {
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        G_json_free_serialized_string(serialized_string);
        G_json_value_free(root_value);
    }

    db_close_database(driver);
    db_shutdown_driver(driver);

    exit(EXIT_SUCCESS);
}

static void parse_command_line(int argc, char **argv)
{
    struct Option *driver, *database, *table, *format_opt;
    struct Flag *cols;
    struct GModule *module;
    const char *drv, *db;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    cols = G_define_flag();
    cols->key = 'c';
    cols->description = _("Print column names only instead "
                          "of full column descriptions");

    table = G_define_standard_option(G_OPT_DB_TABLE);
    table->required = YES;

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    if ((drv = db_get_default_driver_name()))
        driver->answer = (char *)drv;

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    if ((db = db_get_default_database_name()))
        database->answer = (char *)db;

    format_opt = G_define_standard_option(G_OPT_F_FORMAT);
    format_opt->guisection = _("Print");

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("json"));
    G_add_keyword(_("attribute table"));
    module->description = _("Describes a table in detail.");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    parms.driver = driver->answer;
    parms.database = database->answer;
    parms.table = table->answer;
    parms.printcolnames = cols->answer;

    if (strcmp(format_opt->answer, "json") == 0) {
        parms.format = JSON;
    }
    else {
        parms.format = PLAIN;
    }
}
