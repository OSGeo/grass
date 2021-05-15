
/*******************************************************************************
 *
 * MODULE:       v.db.select
 *
 * AUTHOR(S):    Radim Blazek
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               -e, -j, and -f flags by Huidae Cho <grass4u gmail.com>
 *               group option by Luca Delucchi <lucadeluge gmail.com>
 *
 * PURPOSE:      Print vector attributes
 *
 * COPYRIGHT:    (C) 2005-2020 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

void fatal_error_option_value_excludes_flag(struct Option *option,
                                            struct Flag *excluded,
                                            const char *because)
{
    if (!excluded->answer)
        return;
    G_fatal_error(_("The flag -%c is not allowed with %s=%s. %s"),
                  excluded->key, option->key, option->answer, because);
}

void fatal_error_option_value_excludes_option(struct Option *option,
                                              struct Option *excluded,
                                              const char *because)
{
    if (!excluded->answer)
        return;
    G_fatal_error(_("The option %s is not allowed with %s=%s. %s"),
                  excluded->key, option->key, option->answer, because);
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
        struct Option *map;
        struct Option *field;
        struct Option *format;
        struct Option *fsep;
        struct Option *vsep;
        struct Option *nullval;
        struct Option *cols;
        struct Option *where;
        struct Option *file;
        struct Option *group;
    } options;
    struct
    {
        struct Flag *region;
        struct Flag *colnames;
        struct Flag *vertical;
        struct Flag *escape;
        struct Flag *features;
    } flags;
    dbDriver *driver;
    dbString sql, value_string;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    struct field_info *Fi;
    int ncols, col, more, first_rec;
    struct Map_info Map;
    char query[DB_SQL_MAX];
    struct ilist *list_lines;
    char *fsep, *vsep;
    struct bound_box *min_box, *line_box;
    int i, line, area, init_box, cat, field_number;
    int json, csv, vertical;

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("database"));
    G_add_keyword(_("SQL"));
    G_add_keyword(_("export"));
    module->description = _("Prints vector map attributes.");

    options.map = G_define_standard_option(G_OPT_V_MAP);
    options.map->guisection = _("Main");

    options.field = G_define_standard_option(G_OPT_V_FIELD);
    options.field->guisection = _("Selection");

    options.cols = G_define_standard_option(G_OPT_DB_COLUMNS);
    options.cols->guisection = _("Selection");

    options.where = G_define_standard_option(G_OPT_DB_WHERE);
    options.where->guisection = _("Selection");

    options.group = G_define_option();
    options.group->key = "group";
    options.group->required = NO;
    options.group->description =
        _("GROUP BY conditions of SQL statement without 'group by' keyword");
    options.group->guisection = _("Selection");

    options.format = G_define_option();
    options.format->key = "format";
    options.format->type = TYPE_STRING;
    options.format->required = YES;
    options.format->label = _("Output format");
    options.format->options = "plain,csv,json,vertical";
    options.format->descriptions =
        "plain;Configurable plain text output;csv;CSV (Comma Separated Values);json;JSON (JavaScript Object Notation);vertical;Plain text vertical output (instead of horizontal)";
    options.format->answer = "plain";
    options.format->guisection = _("Format");

    /* TODO: value 'auto' is needed to set default differently for each format */
    options.fsep = G_define_standard_option(G_OPT_F_SEP);
    options.fsep->guisection = _("Format");

    options.vsep = G_define_standard_option(G_OPT_F_SEP);
    options.vsep->key = "vertical_separator";
    options.vsep->label = _("Output vertical record separator");
    options.vsep->answer = NULL;
    options.vsep->guisection = _("Format");

    options.nullval = G_define_standard_option(G_OPT_M_NULL_VALUE);
    options.nullval->guisection = _("Format");

    options.file = G_define_standard_option(G_OPT_F_OUTPUT);
    options.file->key = "file";
    options.file->required = NO;
    options.file->guisection = _("Main");
    options.file->description =
        _("Name for output file (if omitted or \"-\" output to stdout)");

    flags.region = G_define_flag();
    flags.region->key = 'r';
    flags.region->description =
        _("Print minimal region extent of selected vector features instead of attributes");
    flags.region->guisection = _("Region");

    flags.colnames = G_define_flag();
    flags.colnames->key = 'c';
    flags.colnames->description = _("Do not include column names in output");
    flags.colnames->guisection = _("Format");

    flags.escape = G_define_flag();
    flags.escape->key = 'e';
    flags.escape->description = _("Escape newline and backslash characters");
    flags.escape->guisection = _("Format");

    flags.features = G_define_flag();
    flags.features->key = 'f';
    flags.features->description =
        _("Exclude attributes not linked to features");
    flags.features->guisection = _("Selection");

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* set input vector map name and mapset */
    if (options.file->answer && strcmp(options.file->answer, "-") != 0) {
        if (NULL == freopen(options.file->answer, "w", stdout))
            G_fatal_error(_("Unable to open file <%s> for writing"),
                          options.file->answer);
    }

    csv = FALSE;
    json = FALSE;
    vertical = FALSE;
    if (strcmp(options.format->answer, "csv") == 0)
        csv = TRUE;
    else if (strcmp(options.format->answer, "json") == 0)
        json = TRUE;
    else if (strcmp(options.format->answer, "vertical") == 0)
        vertical = TRUE;
    if (csv || json) {
        fatal_error_option_value_excludes_flag(options.format, flags.escape,
                                               _("Escaping is based on the format"));
    }
    if (json) {
        fatal_error_option_value_excludes_flag(options.format, flags.colnames,
                                               _("Column names are always included"));
    }
    if (!vertical) {
        fatal_error_option_value_excludes_option(options.format, options.vsep,
                                                 _("Only vertical output can use vertical separator"));
    }

    min_box = line_box = NULL;
    list_lines = NULL;

    if (flags.region->answer) {
        min_box = (struct bound_box *)G_malloc(sizeof(struct bound_box));
        G_zero((void *)min_box, sizeof(struct bound_box));

        line_box = (struct bound_box *)G_malloc(sizeof(struct bound_box));
    }

    if (flags.region->answer || flags.features->answer)
        list_lines = Vect_new_list();

    /* the field separator */
    fsep = G_option_to_separator(options.fsep);
    if (options.vsep->answer)
        vsep = G_option_to_separator(options.vsep);
    else
        vsep = NULL;

    db_init_string(&sql);
    db_init_string(&value_string);

    /* open input vector */
    if (flags.region->answer || flags.features->answer) {
        if (2 > Vect_open_old2(&Map, options.map->answer, "",
                               options.field->answer)) {
            Vect_close(&Map);
            G_fatal_error(_("Unable to open vector map <%s> at topology level. "
                           "Flag '%c' requires topology level."),
                          options.map->answer, flags.region->key);
        }
        field_number = Vect_get_field_number(&Map, options.field->answer);
    }
    else {
        if (Vect_open_old_head2(&Map, options.map->answer, "",
                                options.field->answer) < 0)
            G_fatal_error(_("Unable to open vector map <%s>"),
                          options.map->answer);
        /* field_number won't be used, but is initialized to suppress compiler
         * warnings. */
        field_number = -1;
    }

    if ((Fi = Vect_get_field2(&Map, options.field->answer)) == NULL)
        G_fatal_error(_("Database connection not defined for layer <%s>"),
                      options.field->answer);

    driver = db_start_driver_open_database(Fi->driver, Fi->database);

    if (!driver)
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                      Fi->database, Fi->driver);
    db_set_error_handler_driver(driver);

    if (options.cols->answer)
        sprintf(query, "SELECT %s FROM ", options.cols->answer);
    else
        sprintf(query, "SELECT * FROM ");

    db_set_string(&sql, query);
    db_append_string(&sql, Fi->table);

    if (options.where->answer) {
        char *buf = NULL;

        buf = G_malloc((strlen(options.where->answer) + 8));
        sprintf(buf, " WHERE %s", options.where->answer);
        db_append_string(&sql, buf);
        G_free(buf);
    }

    if (options.group->answer) {
        char *buf = NULL;

        buf = G_malloc((strlen(options.group->answer) + 8));
        sprintf(buf, " GROUP BY %s", options.group->answer);
        db_append_string(&sql, buf);
        G_free(buf);
    }

    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK)
        G_fatal_error(_("Unable to open select cursor"));

    table = db_get_cursor_table(&cursor);
    ncols = db_get_table_number_of_columns(table);

    /* column names if horizontal output (ignore for -r, -c, JSON, vertical) */
    if (!flags.region->answer && !flags.colnames->answer &&
        !vertical && !json) {
        for (col = 0; col < ncols; col++) {
            column = db_get_table_column(table, col);
            if (col)
                fprintf(stdout, "%s", fsep);
            fprintf(stdout, "%s", db_get_column_name(column));
        }
        fprintf(stdout, "\n");
    }

    init_box = TRUE;
    first_rec = TRUE;

    if (!flags.region->answer && json)
        fprintf(stdout, "[");

    /* fetch the data */
    while (1) {
        if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
            G_fatal_error(_("Unable to fetch data from table <%s>"),
                          Fi->table);

        if (!more)
            break;

        if (first_rec)
            first_rec = FALSE;
        else if (!flags.region->answer && json)
            fprintf(stdout, ",\n");

        cat = -1;
        for (col = 0; col < ncols; col++) {
            column = db_get_table_column(table, col);
            value = db_get_column_value(column);

            if (cat < 0 && strcmp(Fi->key, db_get_column_name(column)) == 0) {
                cat = db_get_value_int(value);
                if (flags.region->answer)
                    break;
            }

            if (flags.region->answer)
                continue;

            if (flags.features->answer) {
                Vect_cidx_find_all(&Map, field_number, ~GV_AREA, cat,
                                   list_lines);
                /* if no features are found for this category, don't print
                 * anything. */
                if (list_lines->n_values == 0)
                    break;
            }

            db_convert_column_value_to_string(column, &value_string);

            if (!flags.colnames->answer && vertical)
                fprintf(stdout, "%s%s", db_get_column_name(column), fsep);

            if (col && !vertical && !json)
                fprintf(stdout, "%s", fsep);

            if (json) {
                if (!col)
                    fprintf(stdout, "{");
                fprintf(stdout, "\"%s\":", db_get_column_name(column));
            }

            if (db_test_value_isnull(value)) {
                if (json)
                    fprintf(stdout, "null");
                else if (options.nullval->answer)
                    fprintf(stdout, "%s", options.nullval->answer);
            }
            else {
                char *str = db_get_string(&value_string);

                if (flags.escape->answer || json) {
                    if (strchr(str, '\\'))
                        str = G_str_replace(str, "\\", "\\\\");
                    if (strchr(str, '\r'))
                        str = G_str_replace(str, "\r", "\\r");
                    if (strchr(str, '\n'))
                        str = G_str_replace(str, "\n", "\\n");
                    if (strchr(str, '"') && json) {
                        str = G_str_replace(str, "\"", "\\\"");
                    }
                }
                /* CSV escaping is different from escape and JSON */
                if (csv && strchr(str, '"')) {
                    str = G_str_replace(str, "\"", "\"\"");
                }

                if (json || csv) {
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

            if (vertical)
                fprintf(stdout, "\n");
            else if (json) {
                if (col < ncols - 1)
                    fprintf(stdout, ",");
                else
                    fprintf(stdout, "}");
            }
        }

        if (flags.features->answer && col < ncols)
            continue;

        if (flags.region->answer) {
            /* get minimal region extent */
            Vect_cidx_find_all(&Map, field_number, ~GV_AREA, cat, list_lines);
            for (i = 0; i < list_lines->n_values; i++) {
                line = list_lines->value[i];
                if (Vect_get_line_type(&Map, line) == GV_CENTROID) {
                    area = Vect_get_centroid_area(&Map, line);
                    if (area > 0 && !Vect_get_area_box(&Map, area, line_box))
                        G_fatal_error(_("Unable to get bounding box of area %d"),
                                      area);
                }
                else if (!Vect_get_line_box(&Map, line, line_box))
                    G_fatal_error(_("Unable to get bounding box of line %d"),
                                  line);
                if (init_box) {
                    Vect_box_copy(min_box, line_box);
                    init_box = FALSE;
                }
                else
                    Vect_box_extend(min_box, line_box);
            }
        }
        else {
            /* End of record in attribute printing */
            if (!(vertical || json))
                fprintf(stdout, "\n");
            else if (vsep)
                fprintf(stdout, "%s\n", vsep);
        }
    }

    if (!flags.region->answer && json)
        fprintf(stdout, "]\n");

    if (flags.region->answer) {
        if (csv) {
            fprintf(stdout, "n%ss%sw%se", fsep, fsep, fsep);
            if (Vect_is_3d(&Map)) {
                fprintf(stdout, "t%sb", fsep);
            }
            fprintf(stdout, "\n");
            fprintf(stdout, "%f%s%f%s%f%s%f", min_box->N, fsep, min_box->S,
                    fsep, min_box->W, fsep, min_box->E);
            if (Vect_is_3d(&Map)) {
                fprintf(stdout, "%f%s%f", min_box->T, fsep, min_box->B);
            }
            fprintf(stdout, "\n");
        }
        else if (json) {
            fprintf(stdout, "{");
            fprintf(stdout, "\"n\":%f,", min_box->N);
            fprintf(stdout, "\"s\":%f,", min_box->S);
            fprintf(stdout, "\"w\":%f,", min_box->W);
            fprintf(stdout, "\"e\":%f", min_box->E);
            if (Vect_is_3d(&Map)) {
                fprintf(stdout, ",\"t\":%f,\n", min_box->T);
                fprintf(stdout, "\"b\":%f\n", min_box->B);
            }
            fprintf(stdout, "}\n");
        }
        else {
            /* TODO: separator in theory here too */
            fprintf(stdout, "n=%f\n", min_box->N);
            fprintf(stdout, "s=%f\n", min_box->S);
            fprintf(stdout, "w=%f\n", min_box->W);
            fprintf(stdout, "e=%f\n", min_box->E);
            if (Vect_is_3d(&Map)) {
                fprintf(stdout, "t=%f\n", min_box->T);
                fprintf(stdout, "b=%f\n", min_box->B);
            }
        }
        fflush(stdout);

        G_free((void *)min_box);
        G_free((void *)line_box);

        Vect_destroy_list(list_lines);
    }

    db_close_cursor(&cursor);
    db_close_database_shutdown_driver(driver);
    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
