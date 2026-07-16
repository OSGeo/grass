/***************************************************************
 *
 * MODULE:       v.db.connect
 *
 * AUTHOR(S):    Markus Neteler
 *
 * PURPOSE:      sets/prints DB connection for a given vector map
 *
 * COPYRIGHT:    (C) 2002-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 * TODO: - fix -o flag (needs fix in Vect lib)
 *
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

enum OutputFormat { PLAIN, CSV, JSON };

int main(int argc, char **argv)
{
    char *input;
    const char *driver_default, *database_default;

    struct GModule *module;
    struct Option *inopt, *dbdriver, *dbdatabase, *dbtable, *field_opt, *dbkey,
        *sep_opt, *format_opt;
    struct Flag *print, *columns, *delete, *csv_print;
    dbDriver *driver;
    dbString table_name;
    dbTable *table;
    dbHandle handle;
    struct field_info *fi;
    int field, ret, num_dblinks, i, ncols, col;
    char *fieldname;
    struct Map_info Map;
    char *sep;
    enum OutputFormat format;
    G_JSON_Value *root_value = NULL, *conn_value = NULL;
    G_JSON_Array *root_array = NULL;
    G_JSON_Object *conn_object = NULL;
    int skip_header = 0;

    /* set up the options and flags for the command line parser */

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("database"));
    G_add_keyword(_("layer"));
    module->description =
        _("Prints/sets DB connection for a vector map to attribute table.");

    inopt = G_define_standard_option(G_OPT_V_MAP);

    dbdriver = G_define_standard_option(G_OPT_DB_DRIVER);
    dbdriver->options = db_list_drivers();
    driver_default = db_get_default_driver_name();
    if (driver_default)
        dbdriver->answer = G_store(driver_default);
    dbdriver->guisection = _("Settings");

    dbdatabase = G_define_standard_option(G_OPT_DB_DATABASE);
    database_default = db_get_default_database_name();
    if (database_default)
        dbdatabase->answer = G_store(database_default);
    dbdatabase->guisection = _("Settings");

    dbtable = G_define_standard_option(G_OPT_DB_TABLE);

    dbkey = G_define_standard_option(G_OPT_DB_KEYCOLUMN);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    field_opt->description = _("Format: layer number[/layer name]");
    field_opt->gisprompt = "new,layer,layer";

    sep_opt = G_define_standard_option(G_OPT_F_SEP);
    sep_opt->answer = NULL;
    sep_opt->label = _("Field separator for printing output");
    sep_opt->guisection = _("Print");

    format_opt = G_define_standard_option(G_OPT_F_FORMAT);
    format_opt->options = "plain,csv,json";
    format_opt->required = NO;
    format_opt->answer = NULL;
    format_opt->descriptions = ("plain;Human readable text output;"
                                "csv;CSV (Comma Separated Values);"
                                "json;JSON (JavaScript Object Notation);");

    print = G_define_flag();
    print->key = 'p';
    print->description = _("Print all map connection parameters and exit");
    print->guisection = _("Print");

    csv_print = G_define_flag();
    csv_print->key = 'g';
    csv_print->label = _(
        "Print all map connection parameters in a legacy format [deprecated]");
    csv_print->description = _(
        "Order: layer[/layer name] table key database driver"
        "This flag is deprecated and will be removed in a future release. Use "
        "format=csv instead.");
    csv_print->guisection = _("Print");

    columns = G_define_flag();
    columns->key = 'c';
    columns->description = _("Print types/names of table columns for specified "
                             "layer and exit");
    columns->guisection = _("Print");

    delete = G_define_flag();
    delete->key = 'd';
    delete->description =
        _("Delete connection for certain layer (not the table)");

    G_gisinit(argv[0]);
    G_option_exclusive(print, columns, NULL); // -p and -c exclusive

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    // If no format option is specified, preserve backward compatibility
    if (format_opt->answer == NULL || format_opt->answer[0] == '\0') {
        if (csv_print->answer || columns->answer) {
            format_opt->answer = "csv";
            skip_header = 1;
        }
        else
            format_opt->answer = "plain";
    }

    if (strcmp(format_opt->answer, "json") == 0) {
        format = JSON;
        root_value = G_json_value_init_array();
        if (root_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        root_array = G_json_array(root_value);
    }
    else if (strcmp(format_opt->answer, "csv") == 0) {
        format = CSV;
    }
    else {
        format = PLAIN;
    }

    /* For backward compatibility */
    if (!sep_opt->answer) {
        if (!skip_header && format == CSV)
            sep_opt->answer = "comma";
        else
            sep_opt->answer = "pipe";
    }

    if (format != PLAIN && !print->answer && !csv_print->answer &&
        !columns->answer) {
        G_fatal_error(
            _("The -p or -c flag is required when using the format option."));
    }

    if (csv_print->answer) {
        G_verbose_message(
            _("Flag 'g' is deprecated and will be removed in a future "
              "release. Please use format=csv instead."));
        if (format == JSON) {
            G_fatal_error(_("The -g flag cannot be used with format=json. "
                            "Please select only one output format."));
        }
    }

    /* The check must allow '.' in the name (schema.table) */
    /*
       if (dbtable->answer) {
       if ( db_legal_tablename(dbtable->answer) == DB_FAILED )
       G_fatal_error ( _("Table name '%s' is not valid."), dbtable->answer );
       }
     */

    /* set input vector map name */
    input = inopt->answer;

    if (field_opt->answer) {
        fieldname = strchr(field_opt->answer, '/');
        if (fieldname != NULL) { /* field has name */
            fieldname[0] = 0;
            fieldname++;
        }

        field = atoi(field_opt->answer);
    }
    else {
        field = 1;
        fieldname = NULL;
    }
    G_debug(1, "layer number %d, layer name %s", field, fieldname);

    sep = G_option_to_separator(sep_opt);

    if (print->answer && csv_print->answer)
        G_fatal_error(_("Please choose only one print style"));

    Vect_set_open_level(1); /* no topology needed */
    if (print->answer || csv_print->answer || columns->answer) {
        if (Vect_open_old2(&Map, inopt->answer, "", field_opt->answer) < 0)
            G_fatal_error(_("Unable to open vector map <%s>"), inopt->answer);
    }
    else {
        if (Vect_open_update_head(&Map, inopt->answer, G_mapset()) < 1)
            G_fatal_error(
                _("Unable to modify vector map stored in other mapset"));
        Vect_hist_command(&Map);
    }

    if (print->answer || csv_print->answer || columns->answer) {
        num_dblinks = Vect_get_num_dblinks(&Map);
        if (num_dblinks <= 0) {
            /* it is ok if a vector map is not connected o an attribute table */
            G_message(_("Map <%s> is not connected to a database"), input);
            Vect_close(&Map);
            exit(EXIT_SUCCESS);
        }
        else { /* num_dblinks > 0 */

            if (print->answer || csv_print->answer) {
                if (format == PLAIN) {
                    fprintf(stdout, _("Vector map <%s> is connected by:\n"),
                            input);
                }
                if (!skip_header && format == CSV) {
                    /* CSV Header */
                    fprintf(stdout, "%s%s%s%s%s%s%s%s%s%s%s\n", "layer", sep,
                            "layer_name", sep, "table", sep, "key", sep,
                            "database", sep, "driver");
                }
                for (i = 0; i < num_dblinks; i++) {
                    if ((fi = Vect_get_dblink(&Map, i)) == NULL)
                        G_fatal_error(_("Database connection not defined"));

                    switch (format) {
                    case CSV:
                        if (skip_header) {
                            /* For Backward compatibility */
                            if (fi->name)
                                fprintf(stdout, "%d/%s%s%s%s%s%s%s%s%s\n",
                                        fi->number, fi->name, sep, fi->table,
                                        sep, fi->key, sep, fi->database, sep,
                                        fi->driver);
                            else
                                fprintf(stdout, "%d%s%s%s%s%s%s%s%s\n",
                                        fi->number, sep, fi->table, sep,
                                        fi->key, sep, fi->database, sep,
                                        fi->driver);
                        }
                        else {
                            if (fi->name)
                                fprintf(stdout, "%d%s%s%s%s%s%s%s%s%s%s\n",
                                        fi->number, sep, fi->name, sep,
                                        fi->table, sep, fi->key, sep,
                                        fi->database, sep, fi->driver);
                            else
                                fprintf(stdout, "%d%s%s%s%s%s%s%s%s%s%s\n",
                                        fi->number, sep, "", sep, fi->table,
                                        sep, fi->key, sep, fi->database, sep,
                                        fi->driver);
                        }
                        break;

                    case PLAIN:
                        if (fi->name) {
                            fprintf(stdout,
                                    _("layer <%d/%s> table <%s> in database "
                                      "<%s> through driver "
                                      "<%s> with key <%s>\n"),
                                    fi->number, fi->name, fi->table,
                                    fi->database, fi->driver, fi->key);
                        }
                        else {
                            fprintf(stdout,
                                    _("layer <%d> table <%s> in database <%s> "
                                      "through driver "
                                      "<%s> with key <%s>\n"),
                                    fi->number, fi->table, fi->database,
                                    fi->driver, fi->key);
                        }
                        break;

                    case JSON:
                        conn_value = G_json_value_init_object();
                        if (conn_value == NULL) {
                            G_fatal_error(_("Failed to initialize JSON object. "
                                            "Out of memory?"));
                        }
                        conn_object = G_json_object(conn_value);

                        G_json_object_set_number(conn_object, "layer",
                                                 fi->number);
                        if (fi->name)
                            G_json_object_set_string(conn_object, "layer_name",
                                                     fi->name);
                        else
                            G_json_object_set_null(conn_object, "layer_name");

                        G_json_object_set_string(conn_object, "table",
                                                 fi->table);
                        G_json_object_set_string(conn_object, "key", fi->key);
                        G_json_object_set_string(conn_object, "database",
                                                 fi->database);
                        G_json_object_set_string(conn_object, "driver",
                                                 fi->driver);

                        G_json_array_append_value(root_array, conn_value);
                        break;
                    }
                }
            } /* end print */
            else { /* columns */
                char *database_novar;

                if ((fi = Vect_get_field2(&Map, field_opt->answer)) == NULL)
                    G_fatal_error(
                        _("Database connection not defined for layer <%s>"),
                        field_opt->answer);
                driver = db_start_driver(fi->driver);
                if (driver == NULL)
                    G_fatal_error(_("Unable to start driver <%s>"), fi->driver);

                database_novar = Vect_subst_var(fi->database, &Map);

                db_init_handle(&handle);
                db_set_handle(&handle, database_novar, NULL);
                if (db_open_database(driver, &handle) != DB_OK)
                    G_fatal_error(
                        _("Unable to open database <%s> by driver <%s>"),
                        fi->database, fi->driver);
                db_init_string(&table_name);
                db_set_string(&table_name, fi->table);
                if (db_describe_table(driver, &table_name, &table) != DB_OK)
                    G_fatal_error(_("Unable to describe table <%s>"),
                                  fi->table);

                if (!skip_header) {
                    /* CSV Header */
                    if (format == PLAIN)
                        fprintf(stdout, "%s|%s\n", "name", "sql_type");
                    else if (format == CSV)
                        fprintf(stdout, "%s%s%s\n", "name", sep, "sql_type");
                }

                ncols = db_get_table_number_of_columns(table);
                for (col = 0; col < ncols; col++) {
                    switch (format) {
                    case PLAIN:
                        fprintf(
                            stdout, "%s|%s\n",
                            db_get_column_name(db_get_table_column(table, col)),
                            db_sqltype_name(db_get_column_sqltype(
                                db_get_table_column(table, col))));

                        break;
                    case CSV:
                        if (skip_header) {
                            /* For Backward Compatibility */
                            fprintf(stdout, "%s%s%s\n",
                                    db_sqltype_name(db_get_column_sqltype(
                                        db_get_table_column(table, col))),
                                    sep,
                                    db_get_column_name(
                                        db_get_table_column(table, col)));
                        }
                        else {
                            fprintf(stdout, "%s%s%s\n",
                                    db_get_column_name(
                                        db_get_table_column(table, col)),
                                    sep,
                                    db_sqltype_name(db_get_column_sqltype(
                                        db_get_table_column(table, col))));
                        }
                        break;

                    case JSON:
                        conn_value = G_json_value_init_object();
                        if (conn_value == NULL) {
                            G_fatal_error(_("Failed to initialize JSON object. "
                                            "Out of memory?"));
                        }
                        conn_object = G_json_object(conn_value);

                        G_json_object_set_string(
                            conn_object, "name",
                            db_get_column_name(
                                db_get_table_column(table, col)));

                        int sql_type = db_get_column_sqltype(
                            db_get_table_column(table, col));
                        G_json_object_set_string(conn_object, "sql_type",
                                                 db_sqltype_name(sql_type));

                        int c_type = db_sqltype_to_Ctype(sql_type);
                        G_json_object_set_boolean(conn_object, "is_number",
                                                  (c_type == DB_C_TYPE_INT ||
                                                   c_type == DB_C_TYPE_DOUBLE));

                        G_json_array_append_value(root_array, conn_value);
                        break;
                    }
                }

                db_close_database(driver);
                db_shutdown_driver(driver);
            }

            if (format == JSON) {
                char *json_string =
                    G_json_serialize_to_string_pretty(root_value);
                if (!json_string) {
                    G_json_value_free(root_value);
                    G_fatal_error(
                        _("Failed to serialize JSON to pretty format."));
                }

                puts(json_string);

                G_json_free_serialized_string(json_string);
                G_json_value_free(root_value);
            }

        } /* end else num_dblinks */
    } /* end print/columns */
    else { /* define new dbln settings or delete */

        if (delete->answer) {
            Vect_map_del_dblink(&Map, field);
        }
        else {
            if (field_opt->answer && dbtable->answer && dbkey->answer &&
                dbdatabase->answer && dbdriver->answer) {
                char *database_novar;

                fi = (struct field_info *)G_malloc(sizeof(struct field_info));
                fi->name = fieldname;
                fi->table = dbtable->answer;
                fi->key = dbkey->answer;
                fi->database = dbdatabase->answer;
                fi->driver = dbdriver->answer;

                database_novar = Vect_subst_var(fi->database, &Map);

                ret = Vect_map_check_dblink(&Map, field, fieldname);
                G_debug(3, "Vect_map_check_dblink = %d", ret);
                if (ret == 1) {
                    /* field already defined */
                    if (!G_get_overwrite())
                        G_fatal_error(_("Use --overwrite to overwrite "
                                        "existing link for layer <%d>"),
                                      field);
                    else {
                        dbColumn *column;

                        if (db_table_exists(dbdriver->answer, database_novar,
                                            dbtable->answer) < 1)
                            G_fatal_error(
                                _("Table <%s> does not exist in database <%s>"),
                                dbtable->answer, database_novar);

                        driver = db_start_driver_open_database(fi->driver,
                                                               database_novar);
                        if (!driver)
                            G_fatal_error(_("Unable to start driver <%s>"),
                                          dbdriver->answer);

                        db_get_column(driver, dbtable->answer, dbkey->answer,
                                      &column);
                        if (!column)
                            G_fatal_error(
                                _("Column <%s> not found in table <%s>"),
                                dbkey->answer, dbtable->answer);

                        if (db_column_Ctype(driver, dbtable->answer,
                                            dbkey->answer) != DB_C_TYPE_INT)
                            G_fatal_error(
                                _("Data type of key column must be integer"));

                        ret = Vect_map_del_dblink(&Map, field);
                        if (Vect_map_add_dblink(
                                &Map, field, fi->name, fi->table, fi->key,
                                fi->database, fi->driver) == 0) {
                            G_important_message(
                                _("The table <%s> is now part of vector map "
                                  "<%s> "
                                  "and may be deleted "
                                  "or overwritten by GRASS modules"),
                                dbtable->answer, input);
                        }
                        db_close_database_shutdown_driver(driver);
                    }
                }
                else { /* field not yet defined, add new field */
                    if (db_table_exists(dbdriver->answer, database_novar,
                                        dbtable->answer) < 1)
                        G_warning(
                            _("Table <%s> does not exist in database <%s>"),
                            dbtable->answer, dbdatabase->answer);

                    if (Vect_map_add_dblink(&Map, field, fi->name, fi->table,
                                            fi->key, fi->database,
                                            fi->driver) == 0) {
                        G_important_message(
                            _("The table <%s> is now part of vector map <%s> "
                              "and may be deleted "
                              "or overwritten by GRASS modules"),
                            dbtable->answer, input);

                        driver = db_start_driver_open_database(fi->driver,
                                                               database_novar);

                        if (!driver)
                            G_fatal_error(_("Unable to open database <%s> by "
                                            "driver <%s>"),
                                          fi->database, fi->driver);

                        if (db_create_index2(driver, fi->table, fi->key) !=
                            DB_OK)
                            G_warning(_("Cannot create index"));

                        if (db_grant_on_table(driver, fi->table, DB_PRIV_SELECT,
                                              DB_GROUP | DB_PUBLIC) != DB_OK)
                            G_warning(_("Cannot grant privileges on table %s"),
                                      fi->table);

                        G_important_message(
                            _("Select privileges were granted on the table"));

                        db_close_database_shutdown_driver(driver);
                    }
                }
            }
            else /* incomplete parameters given */
                G_fatal_error(_("For defining a new connection you have "
                                "to specify these parameters: "
                                "driver, database, table [, key [, layer]]"));
        }
    } /* end define new dbln settings */

    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
