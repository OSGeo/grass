/****************************************************************************
 *
 * MODULE:       db.select
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Huidae Cho <grass4u gmail.com>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>,
 *               Markus Neteler <neteler itc.it>
 *               Stephan Holl
 * PURPOSE:      Process one sql select statement
 * COPYRIGHT:    (C) 2002-2010, 2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/gjson.h>
#include "local_proto.h"

enum OutputFormat { PLAIN, JSON, CSV, VERTICAL };

struct {
    char *driver, *database, *table, *sql, *fs, *vs, *nv, *input, *output;
    int c, d, h, test_only;
    enum OutputFormat format;
} parms;

/* function prototypes */
static void parse_command_line(int, char **);
static int sel(dbDriver *, dbString *, G_JSON_Array *results_array);
static int get_stmt(FILE *, dbString *);
static int stmt_is_empty(dbString *);
void fatal_error_option_value_excludes_option(struct Option *option,
                                              struct Option *excluded,
                                              const char *because)
{
    if (!excluded->answer)
        return;
    G_fatal_error(_("The option %s is not allowed with %s=%s. %s"),
                  excluded->key, option->key, option->answer, because);
}

void fatal_error_option_value_excludes_flag(struct Option *option,
                                            struct Flag *excluded,
                                            const char *because)
{
    if (!excluded->answer)
        return;
    G_fatal_error(_("The flag -%c is not allowed with %s=%s. %s"),
                  excluded->key, option->key, option->answer, because);
}

int main(int argc, char **argv)
{
    dbString stmt;
    dbDriver *driver;
    dbHandle handle;
    int stat;
    FILE *fd;
    G_JSON_Value *root_json_value = NULL;
    G_JSON_Array *results_array = NULL;

    parse_command_line(argc, argv);

    if (parms.table) {
        if (!db_table_exists(parms.driver, parms.database, parms.table)) {
            G_warning(
                _("Table <%s> not found in database <%s> using driver <%s>"),
                parms.table, parms.database, parms.driver);
            exit(EXIT_FAILURE);
        }
    }

    /* read from file or stdin ? */
    if (parms.input && strcmp(parms.input, "-") != 0) {
        fd = fopen(parms.input, "r");
        if (fd == NULL) {
            G_fatal_error(_("Unable to open file <%s>: %s"), parms.input,
                          strerror(errno));
        }
    }
    else
        fd = stdin;

    if (parms.output && strcmp(parms.output, "-") != 0) {
        if (NULL == freopen(parms.output, "w", stdout)) {
            G_fatal_error(_("Unable to open file <%s> for writing"),
                          parms.output);
        }
    }

    /* open DB connection */
    db_init_string(&stmt);

    driver = db_start_driver(parms.driver);
    if (driver == NULL) {
        G_fatal_error(_("Unable to start driver <%s>"), parms.driver);
    }

    db_init_handle(&handle);
    db_set_handle(&handle, parms.database, NULL);
    if (db_open_database(driver, &handle) != DB_OK)
        G_fatal_error(_("Unable to open database <%s>"), parms.database);
    db_set_error_handler_driver(driver);

    if (parms.format == JSON) {
        root_json_value = G_json_value_init_array();
        if (root_json_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        results_array = G_json_array(root_json_value);
    }

    /* check for sql, table, and input */
    if (parms.sql) {
        /* parms.sql */
        db_set_string(&stmt, parms.sql);
        stat = sel(driver, &stmt, results_array);
    }
    else if (parms.table) {
        /* parms.table */
        db_set_string(&stmt, "SELECT * FROM ");
        db_append_string(&stmt, parms.table);
        stat = sel(driver, &stmt, results_array);
    }
    else { /* -> parms.input */
        stat = DB_OK;
        while (stat == DB_OK && get_stmt(fd, &stmt)) {
            if (!stmt_is_empty(&stmt))
                stat = sel(driver, &stmt, results_array);
        }
    }

    if (parms.format == JSON && root_json_value) {
        G_JSON_Value *output_value = root_json_value;

        if (!parms.input) { // if single sql statement, jsonify the only element
            G_JSON_Value *first_child =
                G_json_array_get_value(results_array, 0);
            if (first_child) {
                output_value = first_child;
            }
        }
        char *json_string = G_json_serialize_to_string_pretty(output_value);

        if (json_string) {
            fputs(json_string, stdout);
            fputc('\n', stdout);
            G_json_free_serialized_string(json_string);
        }
        G_json_value_free(root_json_value);
    }

    if (parms.test_only)
        G_verbose_message(stat ? _("Test failed.") : _("Test succeeded."));

    db_close_database(driver);
    db_shutdown_driver(driver);

    exit(stat == DB_OK ? EXIT_SUCCESS : EXIT_FAILURE);
}

int sel(dbDriver *driver, dbString *stmt, G_JSON_Array *results_array)
{
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    dbString value_string;
    int col, ncols, nrows;
    int more;
    G_JSON_Value *root_value = NULL;
    G_JSON_Object *root_object = NULL;
    G_JSON_Object *info_object = NULL;
    G_JSON_Array *columns_array = NULL;
    G_JSON_Array *records_array = NULL;

    if (db_open_select_cursor(driver, stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
        return DB_FAILED;
    if (parms.test_only)
        return DB_OK;

    table = db_get_cursor_table(&cursor);
    ncols = db_get_table_number_of_columns(table);
    nrows = db_get_num_rows(&cursor);
    if (parms.d && parms.format != JSON) {
        for (col = 0; col < ncols; col++) {
            column = db_get_table_column(table, col);
            print_column_definition(column);
        }
        return DB_OK;
    }
    if (parms.format == JSON) {
        /* info object */
        G_JSON_Value *info_value = G_json_value_init_object();
        if (info_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        info_object = G_json_value_get_object(info_value);

        if (parms.table)
            G_json_object_set_string(info_object, "table", parms.table);
        if (parms.d) {
            G_json_object_set_number(info_object, "ncols", ncols);
            G_json_object_set_number(info_object, "nrows", nrows);
        }
        G_JSON_Value *columns_value = G_json_value_init_array();
        if (columns_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        G_json_object_set_value(info_object, "columns", columns_value);
        columns_array = G_json_object_get_array(info_object, "columns");

        for (col = 0; col < ncols; col++) {
            column = db_get_table_column(table, col);

            G_JSON_Value *col_value = G_json_value_init_object();
            if (col_value == NULL) {
                G_fatal_error(
                    _("Failed to initialize JSON object. Out of memory?"));
            }
            G_JSON_Object *col_object = G_json_value_get_object(col_value);
            if (parms.d) {
                G_json_object_set_number(col_object, "length",
                                         db_get_column_length(column));
                G_json_object_set_number(col_object, "position", col + 1);
            }
            G_json_object_set_string(col_object, "name",
                                     db_get_column_name(column));

            int sql_type = db_get_column_sqltype(column);
            G_json_object_set_string(col_object, "sql_type",
                                     db_sqltype_name(sql_type));

            int c_type = db_sqltype_to_Ctype(sql_type);

            G_json_object_set_boolean(col_object, "is_number",
                                      c_type == DB_C_TYPE_INT ||
                                          c_type == DB_C_TYPE_DOUBLE);
            G_json_array_append_value(columns_array, col_value);
        }
        if (parms.d) {
            if (results_array) {
                G_json_array_append_value(results_array, info_value);
            }
            else {
                G_json_value_free(info_value);
            }
            return DB_OK;
        }
        root_value = G_json_value_init_object();
        if (root_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        root_object = G_json_value_get_object(root_value);
        G_json_object_set_value(root_object, "info", info_value);

        /* records array */
        G_JSON_Value *records_value = G_json_value_init_array();
        if (records_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        G_json_object_set_value(root_object, "records", records_value);
        records_array = G_json_object_get_array(root_object, "records");
    }

    db_init_string(&value_string);

    /* column names if horizontal output */
    if (parms.h && parms.c && parms.format != JSON) {
        for (col = 0; col < ncols; col++) {
            column = db_get_table_column(table, col);
            if (col)
                fprintf(stdout, "%s", parms.fs);
            fprintf(stdout, "%s", db_get_column_name(column));
        }
        fprintf(stdout, "\n");
    }

    /* fetch the data */
    while (TRUE) {
        if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
            return DB_FAILED;
        if (!more)
            break;

        if (parms.format == JSON) {
            G_JSON_Value *row_value = G_json_value_init_object();
            G_JSON_Object *row_object = G_json_value_get_object(row_value);

            for (col = 0; col < ncols; col++) {
                column = db_get_table_column(table, col);
                value = db_get_column_value(column);

                const char *col_name = db_get_column_name(column);

                if (db_test_value_isnull(value)) {
                    G_json_object_set_null(row_object, col_name);
                }
                else {
                    db_convert_column_value_to_string(column, &value_string);
                    int sql_type = db_get_column_sqltype(column);
                    int c_type = db_sqltype_to_Ctype(sql_type);

                    if (c_type == DB_C_TYPE_INT || c_type == DB_C_TYPE_DOUBLE) {
                        G_json_object_set_number(
                            row_object, col_name,
                            atof(db_get_string(&value_string)));
                    }
                    else {
                        G_json_object_set_string(row_object, col_name,
                                                 db_get_string(&value_string));
                    }
                }
            }

            G_json_array_append_value(records_array, row_value);
            continue;
        }

        for (col = 0; col < ncols; col++) {
            column = db_get_table_column(table, col);
            value = db_get_column_value(column);
            db_convert_column_value_to_string(column, &value_string);
            if (parms.c && !parms.h)
                fprintf(stdout, "%s%s", db_get_column_name(column), parms.fs);
            if (col && parms.h)
                fprintf(stdout, "%s", parms.fs);
            if (parms.nv && db_test_value_isnull(value))
                fprintf(stdout, "%s", parms.nv);
            else {
                char *str = db_get_string(&value_string);
                if (parms.format == CSV) {
                    if (strchr(str, '"'))
                        str = G_str_replace(str, "\"", "\"\"");
                    int type =
                        db_sqltype_to_Ctype(db_get_column_sqltype(column));

                    /* Don't quote numbers, quote text and datetime. */
                    if (type == DB_C_TYPE_INT || type == DB_C_TYPE_DOUBLE)
                        fprintf(stdout, "%s", str);
                    else
                        fprintf(stdout, "\"%s\"", str);
                }
                else
                    fprintf(stdout, "%s", str);
            }
            if (!parms.h)
                fprintf(stdout, "\n");
        }
        if (parms.h)
            fprintf(stdout, "\n");
        else if (parms.vs)
            fprintf(stdout, "%s\n", parms.vs);
    }

    if (parms.format == JSON && results_array) {
        G_json_array_append_value(results_array, root_value);
    }

    return DB_OK;
}

void parse_command_line(int argc, char **argv)
{
    struct Option *driver, *database, *table, *sql, *fs, *vs, *nv, *input,
        *output, *format;
    struct Flag *c, *d, *v, *flag_test;
    struct GModule *module;
    const char *drv, *db;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    sql = G_define_standard_option(G_OPT_DB_SQL);
    sql->guisection = _("Query");

    input = G_define_standard_option(G_OPT_F_INPUT);
    input->required = NO;
    input->label = _("Name of file containing SQL select statement(s)");
    input->description = _("'-' for standard input");
    input->guisection = _("Query");

    table = G_define_standard_option(G_OPT_DB_TABLE);
    table->description = _("Name of table to query");
    table->guisection = _("Query");

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    if ((drv = db_get_default_driver_name()))
        driver->answer = (char *)drv;
    driver->guisection = _("Connection");

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    if ((db = db_get_default_database_name()))
        database->answer = (char *)db;
    database->guisection = _("Connection");

    fs = G_define_standard_option(G_OPT_F_SEP);
    fs->answer = NULL;
    fs->guisection = _("Format");

    vs = G_define_standard_option(G_OPT_F_SEP);
    vs->key = "vertical_separator";
    vs->label = _("Vertical record separator (requires -v flag [deprecated] or "
                  "format=vertical)");
    vs->answer = NULL;
    vs->guisection = _("Format");

    nv = G_define_standard_option(G_OPT_M_NULL_VALUE);
    nv->guisection = _("Format");

    format = G_define_standard_option(G_OPT_F_FORMAT);
    format->key = "format";
    format->type = TYPE_STRING;
    format->required = NO;
    format->answer = NULL;
    format->options = "plain,csv,json,vertical";
    format->descriptions =
        "plain;Configurable plain text output;"
        "csv;CSV (Comma Separated Values);"
        "json;JSON (JavaScript Object Notation);"
        "vertical;Plain text vertical output (instead of horizontal)";
    format->guisection = _("Format");

    output = G_define_standard_option(G_OPT_F_OUTPUT);
    output->required = NO;
    output->description =
        _("Name for output file (if omitted or \"-\" output to stdout)");

    c = G_define_flag();
    c->key = 'c';
    c->description = _("Do not include column names in output");
    c->guisection = _("Format");

    d = G_define_flag();
    d->key = 'd';
    d->description = _("Describe query only (don't run it)");
    d->guisection = _("Query");

    v = G_define_flag();
    v->key = 'v';
    v->label = _("Vertical output instead of horizontal [deprecated]");
    v->description = _("Use format=vertical instead.");
    v->guisection = _("Format");

    flag_test = G_define_flag();
    flag_test->key = 't';
    flag_test->description = _("Only test query, do not execute");
    flag_test->guisection = _("Query");

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("SQL"));
    module->label = _("Selects data from attribute table.");
    module->description = _("Performs SQL query statement(s).");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    parms.driver = driver->answer;
    parms.database = database->answer;
    parms.table = table->answer;
    parms.sql = sql->answer;
    if (format->answer) {
        if (strcmp("json", format->answer) == 0) {
            parms.format = JSON;
        }
        else if (strcmp("csv", format->answer) == 0) {
            parms.format = CSV;
        }
        else if (strcmp("vertical", format->answer) == 0) {
            parms.format = VERTICAL;
        }
        else {
            parms.format = PLAIN;
        }
    }
    else {
        parms.format = PLAIN; // DEFAULT
    }
    if (fs->answer) {
        parms.fs = G_option_to_separator(fs);
    }
    else {
        if (parms.format == CSV) {
            parms.fs = G_store(",");
        }
        else if (parms.format == PLAIN || parms.format == VERTICAL) {
            parms.fs = G_store("|");
        }
        else { /* Something like a separator is part of the format. */
            parms.fs = NULL;
        }
    }
    parms.vs = NULL;
    if (vs->answer)
        parms.vs = G_option_to_separator(vs);
    parms.nv = nv->answer;
    parms.input = input->answer;
    parms.output = output->answer;

    if (!c->answer)
        parms.c = TRUE;
    else
        parms.c = FALSE;
    parms.d = d->answer;
    if (v->answer || parms.format == VERTICAL)
        parms.h = FALSE;
    else
        parms.h = TRUE;

    parms.test_only = flag_test->answer;

    if (parms.input && *parms.input == 0) {
        G_usage();
        exit(EXIT_FAILURE);
    }

    if (!parms.input && !parms.sql && !parms.table)
        G_fatal_error(
            _("You must provide one of these options: <%s>, <%s>, or <%s>"),
            sql->key, input->key, table->key);
    // Check for mutually exclusive options
    if (parms.format == JSON) {
        fatal_error_option_value_excludes_option(
            format, fs, _("Separator is part of the format"));
        fatal_error_option_value_excludes_option(
            format, vs, _("Vertical separator is part of the format"));
        fatal_error_option_value_excludes_option(
            format, nv, _("Null value is part of the format"));
        fatal_error_option_value_excludes_flag(
            format, c, _("Column names are always included"));
    }
    if (v->answer) {
        G_verbose_message(
            _("Flag 'v' is deprecated and will be removed in a future "
              "release. Please use format=vertical instead."));
        if (format->answer && parms.format != VERTICAL) {
            G_fatal_error(_("Flag 'v' is only allowed with format=vertical."));
        }
    }
}

int get_stmt(FILE *fd, dbString *stmt)
{
    char buf[DB_SQL_MAX], buf2[DB_SQL_MAX];
    size_t len;

    db_zero_string(stmt);

    if (G_getl2(buf, sizeof(buf), fd) == 0)
        return 0;

    strcpy(buf2, buf);
    G_chop(buf2);
    len = strlen(buf2);

    if (buf2[len - 1] == ';') { /* end of statement */
        buf2[len - 1] = 0;      /* truncate ';' */
    }

    db_set_string(stmt, buf);

    return 1;
}

int stmt_is_empty(dbString *stmt)
{
    char dummy[2];

    return (sscanf(db_get_string(stmt), "%1s", dummy) != 1);
}
