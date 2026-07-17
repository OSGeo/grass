/****************************************************************************
 *
 * MODULE:       t.connect
 * AUTHOR(S):    Soeren Gebbert, based on db.connect
 *
 * PURPOSE:      Prints/sets general temporal GIS database connection for
 *               current mapset.
 * SPDX-FileCopyrightText: 2002-2010 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/temporal.h>
#include <grass/glocale.h>
#include <grass/gjson.h>

enum OutputFormat { PLAIN, SHELL, JSON };

static void print_tgis_connection(const char *mapset, G_JSON_Array *json_array)
{
    char *drv = tgis_get_mapset_driver_name(mapset);
    char *db = tgis_get_mapset_database_name(mapset);
    G_JSON_Value *ms_value = G_json_value_init_object();

    if (!ms_value)
        G_fatal_error(_("Failed to initialize JSON object. Out of memory?"));
    G_JSON_Object *ms_object = G_json_object(ms_value);

    G_json_object_set_string(ms_object, "mapset", mapset);
    if (drv)
        G_json_object_set_string(ms_object, "driver", drv);
    else
        G_json_object_set_null(ms_object, "driver");
    if (db)
        G_json_object_set_string(ms_object, "database", db);
    else
        G_json_object_set_null(ms_object, "database");
    G_json_array_append_value(json_array, ms_value);

    G_free(drv);
    G_free(db);
}

int main(int argc, char *argv[])
{
    dbConnection conn;
    struct Flag *print, *check_set_default, *def, *sh;
    struct Option *driver, *database, *format_opt, *mapset_opt;
    struct GModule *module;

    enum OutputFormat format;
    G_JSON_Value *root_value = NULL;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("temporal"));
    G_add_keyword(_("settings"));
    G_add_keyword(_("metadata"));
    module->description = _("Prints/sets general temporal GIS database "
                            "connection for current mapset.");

    print = G_define_flag();
    print->key = 'p';
    print->description = _("Print current connection parameters and exit");
    print->guisection = _("Print");

    format_opt = G_define_standard_option(G_OPT_F_FORMAT);
    format_opt->options = "plain,shell,json";
    format_opt->required = NO;
    format_opt->answer = "plain";
    format_opt->descriptions = _("plain;Plain text output;"
                                 "shell;Shell script style output;"
                                 "json;JSON (JavaScript Object Notation)");
    format_opt->guisection = _("Print");

    check_set_default = G_define_flag();
    check_set_default->key = 'c';
    check_set_default->description =
        _("Check connection parameters, set if uninitialized, and exit");
    check_set_default->guisection = _("Set");

    def = G_define_flag();
    def->key = 'd';
    def->label = _("Set from default settings and exit");
    def->description = _("Overwrite current settings if initialized");
    def->guisection = _("Set");

    sh = G_define_flag();
    sh->key = 'g';
    sh->label = _("Print current connection parameters using shell style "
                  "and exit [deprecated]");
    sh->description =
        _("This flag is deprecated and will be removed in a future "
          "release. Use format=shell instead.");
    sh->guisection = _("Print");

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = "sqlite,pg";
    driver->answer = (char *)tgis_get_default_driver_name();
    driver->guisection = _("Set");

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    database->answer = (char *)tgis_get_default_database_name();
    database->guisection = _("Set");

    mapset_opt = G_define_standard_option(G_OPT_M_MAPSET);
    mapset_opt->multiple = YES;
    mapset_opt->required = NO;
    mapset_opt->label =
        _("Name of the mapset(s) to print the connection info for");
    mapset_opt->description =
        _("'.' for current mapset; '*' for all mapsets in the project. "
          "Requires format=json.");
    mapset_opt->guisection = _("Print");

    G_option_requires(mapset_opt, print, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (format_opt->answer != NULL) {
        if (strcmp(format_opt->answer, "json") == 0) {
            format = JSON;
        }
        else if (strcmp(format_opt->answer, "shell") == 0) {
            format = SHELL;
        }
        else {
            format = PLAIN;
        }
    }
    else {
        format = PLAIN;
    }

    if (sh->answer) {
        G_verbose_message(
            _("Flag 'g' is deprecated and will be removed in a future "
              "release. Please use format=shell instead."));
        if (format == JSON) {
            /* Free memory before failing to prevent memory leaks */
            if (root_value)
                G_json_value_free(root_value);
            G_fatal_error(_("The -g flag cannot be used with format=json. "
                            "Please select only one output format."));
        }
        format = SHELL;
        print->answer = 1;
    }

    if (format != PLAIN && !print->answer) {
        G_fatal_error(
            _("The -p flag is required when using the format option."));
    }

    if (mapset_opt->answer && format != JSON) {
        if (root_value)
            G_json_value_free(root_value);
        G_fatal_error(_("The mapset option requires format=json."));
    }

    if (print->answer) {
        if (format == JSON) {
            /* JSON always outputs an array. When mapset is not specified,
             * report connections for all mapsets on the current search path. */
            G_JSON_Array *json_array = NULL;
            char *json_string;

            root_value = G_json_value_init_array();
            if (!root_value)
                G_fatal_error(
                    _("Failed to initialize JSON array. Out of memory?"));
            json_array = G_json_array(root_value);

            if (mapset_opt->answers) {
                int i;

                for (i = 0; mapset_opt->answers[i]; i++) {
                    const char *ms_arg = mapset_opt->answers[i];

                    if (strcmp(ms_arg, "*") == 0) {
                        char **ms_list = G_get_available_mapsets();
                        int j;

                        for (j = 0; ms_list[j]; j++)
                            print_tgis_connection(ms_list[j], json_array);
                    }
                    else {
                        const char *ms =
                            strcmp(ms_arg, ".") == 0 ? G_mapset() : ms_arg;

                        print_tgis_connection(ms, json_array);
                    }
                }
            }
            else {
                /* default: all mapsets on the current search path */
                const char *ms;
                int j;

                for (j = 0; (ms = G_get_mapset_name(j)); j++)
                    print_tgis_connection(ms, json_array);
            }

            json_string = G_json_serialize_to_string_pretty(root_value);
            if (!json_string) {
                G_json_value_free(root_value);
                G_fatal_error(_("Failed to serialize JSON to pretty format."));
            }
            fputs(json_string, stdout);
            fputc('\n', stdout);
            G_json_free_serialized_string(json_string);
            G_json_value_free(root_value);
        }
        else {
            /* PLAIN / SHELL: report the current mapset's connection */
            if (tgis_get_connection(&conn) == DB_OK) {
                switch (format) {
                case SHELL:
                    fprintf(stdout, "driver=%s\n",
                            conn.driverName ? conn.driverName : "");
                    fprintf(stdout, "database=%s\n",
                            conn.databaseName ? conn.databaseName : "");
                    break;

                case PLAIN:
                    fprintf(stdout, "driver:%s\n",
                            conn.driverName ? conn.driverName : "");
                    fprintf(stdout, "database:%s\n",
                            conn.databaseName ? conn.databaseName : "");
                    break;

                case JSON:
                    /* unreachable: JSON is handled above */
                    break;
                }
            }
            else {
                G_fatal_error(_("Temporal GIS database connection not defined. "
                                "Run t.connect."));
            }
        }

        fflush(stdout);
        exit(EXIT_SUCCESS);
    }

    if (check_set_default->answer) {
        /* check connection and set to system-wide default in required */
        tgis_get_connection(&conn);

        if (!conn.driverName && !conn.databaseName) {

            tgis_set_default_connection();
            tgis_get_connection(&conn);

            G_important_message(_("Default TGIS driver / database set to:\n"
                                  "driver: %s\ndatabase: %s"),
                                conn.driverName, conn.databaseName);
        }
        /* they must be a matched pair, so if one is set but not the other
           then give up and let the user figure it out */
        else if (!conn.driverName) {
            G_fatal_error(_("Default TGIS driver is not set"));
        }
        else if (!conn.databaseName) {
            G_fatal_error(_("Default TGIS database is not set"));
        }

        /* connection either already existed or now exists */
        exit(EXIT_SUCCESS);
    }

    if (def->answer) {
        tgis_set_default_connection();
        tgis_get_connection(&conn);

        G_important_message(_("Default driver / database set to:\n"
                              "driver: %s\ndatabase: %s"),
                            conn.driverName, conn.databaseName);
        exit(EXIT_SUCCESS);
    }

    /* set connection */
    tgis_get_connection(&conn); /* read current */

    if (driver->answer)
        conn.driverName = driver->answer;

    if (database->answer)
        conn.databaseName = database->answer;

    tgis_set_connection(&conn);

    exit(EXIT_SUCCESS);
}
