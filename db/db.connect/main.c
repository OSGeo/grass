/****************************************************************************
 *
 * MODULE:       db.connect
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Alex Shevlakov <sixote yahoo.com>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>,
 *               Hamish Bowman <hamish_b yahoo com>
 *               Martin Landa <landa.martin gmail.com> ('d' flag)
 * PURPOSE:      set parameters for connection to database
 * COPYRIGHT:    (C) 2002-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

static char *substitute_variables(dbConnection *);

enum OutputFormat { PLAIN, SHELL, JSON };

int main(int argc, char *argv[])
{
    char *databaseName;
    dbConnection conn;
    struct Flag *print, *shell, *check_set_default, *def;

    /*    struct Option *driver, *database, *user, *password, *keycol; */
    struct Option *driver, *database, *schema, *group, *frmt;
    struct GModule *module;
    enum OutputFormat format;
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("connection settings"));
    module->description =
        _("Prints/sets general DB connection for current mapset.");

    print = G_define_flag();
    print->key = 'p';
    print->label = _("Print current connection parameters and exit");
    print->guisection = _("Print");

    shell = G_define_flag();
    shell->key = 'g';
    shell->label = _("Print current connection parameters using shell style "
                     "and exit [deprecated]");
    shell->description = _(
        "This flag is deprecated and will be removed in a future release. Use "
        "format=shell instead.");
    shell->guisection = _("Print");

    check_set_default = G_define_flag();
    check_set_default->key = 'c';
    check_set_default->description =
        _("Check connection parameters, set if uninitialized, and exit");
    check_set_default->guisection = _("Set");

    def = G_define_flag();
    def->key = 'd';
    def->label = _("Set from default settings and exit");
    def->description = _("Overwrite current settings if already initialized");
    def->guisection = _("Set");

    /* the default answers below are GRASS default settings,
     * not current settings */
    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    if (strcmp(DB_DEFAULT_DRIVER, "sqlite") == 0) {
        driver->answer = "sqlite";
    }
    else {
        driver->answer = "dbf";
    }
    driver->guisection = _("Set");

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    if (strcmp(DB_DEFAULT_DRIVER, "sqlite") == 0) {
        database->answer = "$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db";
    }
    else {
        database->answer = "$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/";
    }
    database->guisection = _("Set");

    schema = G_define_standard_option(G_OPT_DB_SCHEMA);
    schema->guisection = _("Set");

    group = G_define_option();
    group->key = "group";
    group->type = TYPE_STRING;
    group->required = NO;
    group->multiple = NO;
    group->description = _("Default group of database users to which "
                           "select privilege is granted");
    group->guisection = _("Set");

    frmt = G_define_standard_option(G_OPT_F_FORMAT);
    frmt->options = "plain,shell,json";
    frmt->descriptions = _("plain;Plain text output;"
                           "shell;shell script style output;"
                           "json;JSON (JavaScript Object Notation);");
    frmt->guisection = _("Print");

    /* commented due to new mechanism - see db.login
       user = G_define_option() ;
       user->key        = "user" ;
       user->type       = TYPE_STRING ;
       user->required   = NO  ;
       user->multiple   = NO ;
       user->description= "User:" ;

       password = G_define_option() ;
       password->key        = "password" ;
       password->type       = TYPE_STRING ;
       password->required   = NO  ;
       password->multiple   = NO ;
       password->description= "Password:" ;
     */

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (strcmp(frmt->answer, "json") == 0) {
        format = JSON;
        root_value = G_json_value_init_object();
        if (root_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        root_object = G_json_object(root_value);
    }
    else if (strcmp(frmt->answer, "shell") == 0) {
        format = SHELL;
    }
    else {
        format = PLAIN;
    }

    if (shell->answer) {
        G_verbose_message(
            _("Flag 'g' is deprecated and will be removed in a future "
              "release. Please use format=shell instead."));
        if (format == JSON) {
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

    if (print->answer) {
        /* get and print connection */
        if (db_get_connection(&conn) == DB_OK) {
            switch (format) {
            case SHELL:
                fprintf(stdout, "driver=%s\n",
                        conn.driverName ? conn.driverName : "");
                fprintf(stdout, "database=%s\n",
                        conn.databaseName ? conn.databaseName : "");
                fprintf(stdout, "schema=%s\n",
                        conn.schemaName ? conn.schemaName : "");
                fprintf(stdout, "group=%s\n", conn.group ? conn.group : "");
                break;

            case PLAIN:
                databaseName = substitute_variables(&conn);

                fprintf(stdout, "driver: %s\n",
                        conn.driverName ? conn.driverName : "");
                /* substitute variables */
                fprintf(stdout, "database: %s\n", databaseName);
                G_free(database);

                fprintf(stdout, "schema: %s\n",
                        conn.schemaName ? conn.schemaName : "");
                fprintf(stdout, "group: %s\n", conn.group ? conn.group : "");
                break;

            case JSON:
                databaseName = substitute_variables(&conn);

                if (conn.driverName)
                    G_json_object_set_string(root_object, "driver",
                                             conn.driverName);
                else
                    G_json_object_set_null(root_object, "driver");

                if (conn.databaseName)
                    G_json_object_set_string(root_object, "database_template",
                                             conn.databaseName);
                else
                    G_json_object_set_null(root_object, "database_template");

                /* substitute variables */
                if (databaseName)
                    G_json_object_set_string(root_object, "database",
                                             databaseName);
                else
                    G_json_object_set_null(root_object, "database");

                if (conn.schemaName)
                    G_json_object_set_string(root_object, "schema",
                                             conn.schemaName);
                else
                    G_json_object_set_null(root_object, "schema");

                if (conn.group)
                    G_json_object_set_string(root_object, "group", conn.group);
                else
                    G_json_object_set_null(root_object, "group");
                break;
            }
        }
        else
            G_fatal_error(_("Database connection not defined. "
                            "Run db.connect."));

        if (format == JSON) {
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

    if (check_set_default->answer) {
        /* check connection and set to system-wide default in required */
        /*
         * TODO: improve db_{get,set}_connection() to not return DB_OK on error
         *       (thus currently there is no point in checking for that here)
         */
        db_get_connection(&conn);

        if (!conn.driverName && !conn.databaseName) {

            db_set_default_connection();
            db_get_connection(&conn);

            databaseName = substitute_variables(&conn);
            G_important_message(_("Default driver / database set to:\n"
                                  "driver: %s\ndatabase: %s"),
                                conn.driverName, databaseName);
        }
        else {
            G_important_message(
                _("DB settings already defined, nothing to do"));
        }

        /* they must be a matched pair, so if one is set but not the other
           then give up and let the user figure it out */
        if (!conn.driverName) {
            G_fatal_error(_("Default driver is not set"));
        }
        if (!conn.databaseName) {
            G_fatal_error(_("Default database is not set"));
        }

        /* connection either already existed or now exists */
        exit(EXIT_SUCCESS);
    }

    if (def->answer) {
        db_set_default_connection();
        db_get_connection(&conn);

        databaseName = substitute_variables(&conn);
        G_important_message(_("Default driver / database set to:\n"
                              "driver: %s\ndatabase: %s"),
                            conn.driverName, databaseName);
        exit(EXIT_SUCCESS);
    }

    /* do not read current, set new connection from options */
    G_zero(&conn, sizeof(dbConnection));

    if (driver->answer)
        conn.driverName = driver->answer;

    if (database->answer)
        conn.databaseName = database->answer;

    if (schema->answer)
        conn.schemaName = schema->answer;

    if (group->answer)
        conn.group = group->answer;

    db_set_connection(&conn);
    /* check */
    if (db_get_connection(&conn) != DB_OK) {
        G_fatal_error(_("Unable to set default database connection"));

        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

char *substitute_variables(dbConnection *conn)
{
    char *database, *c, buf[GPATH_MAX];

    if (!conn->databaseName)
        return NULL;

    database = (char *)G_malloc(GPATH_MAX);
    strcpy(database, conn->databaseName);

    strcpy(buf, database);
    c = (char *)strstr(buf, "$GISDBASE");
    if (c != NULL) {
        *c = '\0';
        snprintf(database, GPATH_MAX, "%s%s%s", buf, G_gisdbase(), c + 9);
    }

    strcpy(buf, database);
    c = (char *)strstr(buf, "$LOCATION_NAME");
    if (c != NULL) {
        *c = '\0';
        snprintf(database, GPATH_MAX, "%s%s%s", buf, G_location(), c + 14);
    }

    strcpy(buf, database);
    c = (char *)strstr(buf, "$MAPSET");
    if (c != NULL) {
        *c = '\0';
        snprintf(database, GPATH_MAX, "%s%s%s", buf, G_mapset(), c + 7);
    }
#ifdef __MINGW32__
    if (strcmp(conn->driverName, "sqlite") == 0 ||
        strcmp(conn->driverName, "dbf") == 0) {
        char *p;

        p = database;
        while (*p) {
            if (*p == '/')
                *p = HOST_DIRSEP;
            p++;
        }
    }
#endif

    return database;
}
