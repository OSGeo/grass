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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/gjson.h>
#include <grass/glocale.h>

enum OutputFormat { PLAIN, JSON, CSV, LIST };

struct {
    char *driver, *database, *table, *separator;
    enum OutputFormat format;
    bool more_info;
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
    G_JSON_Value *root_value = NULL, *column_value = NULL;
    G_JSON_Object *column_object = NULL;
    G_JSON_Array *root_array = NULL;

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

    // In CSV, we need to print the headers too
    if (parms.format == CSV) {
        if (parms.more_info) { /* -e flag */
            fprintf(stdout, "name%ssql_type%sis_number\n", parms.separator,
                    parms.separator);
        }
        else {
            fprintf(stdout, "name\n");
        }
    }
    for (col = 0; col < ncols; col++) {
        const char *column_name =
            db_get_column_name(db_get_table_column(table, col));

        // -e flag is handled here
        if (parms.more_info) {
            int sql_type =
                db_get_column_sqltype(db_get_table_column(table, col));
            int c_type = db_sqltype_to_Ctype(sql_type);
            const char *sql_type_name = db_sqltype_name(sql_type);

            switch (parms.format) {
            case LIST:
                fprintf(stdout, "%s %s", column_name, sql_type_name);

                // handling last separator: it should be `\n` in all cases
                if (col < ncols - 1)
                    fprintf(stdout, "%s", parms.separator);
                else
                    fprintf(stdout, "\n");
                break;
            case CSV:
                fprintf(stdout, "%s%s%s%s", column_name, parms.separator,
                        sql_type_name, parms.separator);
                if (c_type == DB_C_TYPE_INT || c_type == DB_C_TYPE_DOUBLE)
                    fprintf(stdout, "true\n");
                else
                    fprintf(stdout, "false\n");
                break;
            case JSON:
                column_value = G_json_value_init_object();
                column_object = G_json_object(column_value);

                G_json_object_set_string(column_object, "name", column_name);
                G_json_object_set_string(column_object, "sql_type",
                                         sql_type_name);
                G_json_object_set_boolean(
                    column_object, "is_number",
                    (c_type == DB_C_TYPE_INT || c_type == DB_C_TYPE_DOUBLE));

                G_json_array_append_value(root_array, column_value);
                break;
            case PLAIN:
                fprintf(stdout, "%s: %s\n", column_name, sql_type_name);
                break;
            default:
                break;
            }
        }
        else { /* without -e flag */
            switch (parms.format) {
            case LIST:
                fprintf(stdout, "%s", column_name);

                // handling last separator: it should be `\n` in all cases
                if (col < ncols - 1)
                    fprintf(stdout, "%s", parms.separator);
                else
                    fprintf(stdout, "\n");
                break;
            case CSV: // except header, same as plain; header handled above
            case PLAIN:
                fprintf(stdout, "%s\n", column_name);
                break;

            case JSON:
                G_json_array_append_string(root_array, column_name);
                break;
            }
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
    struct Option *driver, *database, *table, *format, *separator;
    struct Flag *more_info;
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

    more_info = G_define_flag();
    more_info->key = 'e';
    more_info->label = _("Print type information about the columns");
    more_info->description =
        _("Print the name and the type of all the columns for a given table.");
    more_info->guisection = _("Print");

    separator = G_define_standard_option(G_OPT_F_SEP);
    separator->answer = NULL;
    separator->guisection = _("Format");

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    module->description = _("List all columns for a given table.");

    format = G_define_standard_option(G_OPT_F_FORMAT);
    format->options = "plain,csv,json,list";
    format->descriptions = "plain;Configurable plain text output;"
                           "csv;CSV (Comma Separated Values);"
                           "json;JSON (JavaScript Object Notation);"
                           "list;Output in list format";
    format->guisection = _("Print");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    parms.driver = driver->answer;
    parms.database = database->answer;
    parms.table = table->answer;

    parms.more_info = more_info->answer ? true : false;

    if (strcmp(format->answer, "json") == 0) {
        parms.format = JSON;
    }
    else if (strcmp(format->answer, "csv") == 0) {
        parms.format = CSV;
    }
    else if (strcmp(format->answer, "list") == 0) {
        parms.format = LIST;
    }
    else {
        parms.format = PLAIN;
    }
    if (separator->answer) {
        switch (parms.format) {
        case CSV:
        case LIST:
            parms.separator = G_option_to_separator(separator);
            break;
        case JSON:
        case PLAIN:
            G_fatal_error(_("Separator is part of the format."));
            break;
        }
    }
    else {
        switch (parms.format) {
        case CSV:
            parms.separator = G_store(",");
            break;
        case LIST:
            parms.separator = G_store("\n");
            break;
        case JSON:
        case PLAIN:
            break;
        }
    }
}
