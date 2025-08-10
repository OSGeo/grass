/****************************************************************************
 *
 * MODULE:       db.columns
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      list the column names for a table
 * COPYRIGHT:    (C) 2002-2006 by the GRASS Development Team
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
#include <grass/gjson.h>
#include <grass/glocale.h>

enum OutputFormat { PLAIN, JSON };

struct {
    char *driver, *database, *table;
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
    int col, ncols;
    JSON_Value *root_value = NULL;
    JSON_Array *root_array = NULL;

    parse_command_line(argc, argv);

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
    db_set_error_handler_driver(driver);

    db_init_string(&table_name);
    db_set_string(&table_name, parms.table);
    if (db_describe_table(driver, &table_name, &table) != DB_OK)
        G_fatal_error(_("Unable to describe table <%s>"), parms.table);

    db_close_database(driver);
    db_shutdown_driver(driver);

    if (parms.format == JSON) {
        root_value = G_json_value_init_array();
        if (root_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        root_array = G_json_array(root_value);
    }

    ncols = db_get_table_number_of_columns(table);
    for (col = 0; col < ncols; col++) {
        switch (parms.format) {
        case PLAIN:
            fprintf(stdout, "%s\n",
                    db_get_column_name(db_get_table_column(table, col)));
            break;

        case JSON:
            G_json_array_append_string(
                root_array,
                db_get_column_name(db_get_table_column(table, col)));
            break;
        }
    }

    if (parms.format == JSON) {
        char *json_string = G_json_serialize_to_string_pretty(root_value);
        if (!json_string) {
            G_json_value_free(root_value);
            G_fatal_error(_("Failed to serialize JSON to pretty format."));
        }

        puts(json_string);

        G_json_free_serialized_string(json_string);
        G_json_value_free(root_value);
    }

    exit(EXIT_SUCCESS);
}

static void parse_command_line(int argc, char **argv)
{
    struct Option *driver, *database, *table, *format;
    struct GModule *module;
    const char *drv, *db;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    table = G_define_standard_option(G_OPT_DB_TABLE);
    table->required = YES;

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    if ((drv = db_get_default_driver_name()))
        driver->answer = (char *)drv;

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    if ((db = db_get_default_database_name()))
        database->answer = (char *)db;

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    module->description = _("List all columns for a given table.");

    format = G_define_standard_option(G_OPT_F_FORMAT);
    format->guisection = _("Print");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    parms.driver = driver->answer;
    parms.database = database->answer;
    parms.table = table->answer;

    if (strcmp(format->answer, "json") == 0) {
        parms.format = JSON;
    }
    else {
        parms.format = PLAIN;
    }
}
