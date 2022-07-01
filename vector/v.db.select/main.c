
/*******************************************************************************
 *
 * MODULE:       v.db.select
 *
 * AUTHOR(S):    Radim Blazek
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               -e, -j, and -f flags by Huidae Cho <grass4u gmail.com>
 *               group option by Luca Delucchi <lucadeluge gmail.com>
 *               CSV and format option by Vaclav Petras <wenzeslaus gmail com>
 *
 * PURPOSE:      Print vector attributes
 *
 * COPYRIGHT:    (C) 2005-2021 by the GRASS Development Team
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

enum OutputFormat {
    PLAIN,
    JSON,
    CSV,
    VERTICAL
};

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
    int ncols, col, more;
    bool first_rec;
    struct Map_info Map;
    char query[DB_SQL_MAX];
    struct ilist *list_lines;
    char *fsep, *vsep;
    struct bound_box *min_box, *line_box;
    int i, line, area, cat, field_number;
    bool init_box;
    enum OutputFormat format;
    bool vsep_needs_newline;

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
        "plain;Configurable plain text output;"
        "csv;CSV (Comma Separated Values);"
        "json;JSON (JavaScript Object Notation);"
        "vertical;Plain text vertical output (instead of horizontal)";
    options.format->answer = "plain";
    options.format->guisection = _("Format");

    options.fsep = G_define_standard_option(G_OPT_F_SEP);
    options.fsep->answer = NULL;
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

    if (strcmp(options.format->answer, "csv") == 0)
        format = CSV;
    else if (strcmp(options.format->answer, "json") == 0)
        format = JSON;
    else if (strcmp(options.format->answer, "vertical") == 0)
        format = VERTICAL;
    else
        format = PLAIN;
    if (format == JSON) {
        fatal_error_option_value_excludes_flag(options.format, flags.escape,
                                               _("Escaping is based on the format"));
        fatal_error_option_value_excludes_flag(options.format, flags.colnames,
                                               _("Column names are always included"));
        fatal_error_option_value_excludes_option(options.format, options.fsep,
                                                 _("Separator is part of the format"));
        fatal_error_option_value_excludes_option(options.format, options.nullval,
                                                 _("Null value is part of the format"));
    }
    if (format != VERTICAL) {
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
    if (options.fsep->answer) {
        fsep = G_option_to_separator(options.fsep);
    }
    else {
        /* A different separator is needed to for each format and output. */
        if (format == CSV) {
            fsep = G_store(",");
        }
        else if (format == PLAIN || format == VERTICAL) {
            if (flags.region->answer)
               fsep = G_store("=");
            else
               fsep = G_store("|");
        }
        else
            fsep = NULL;  /* Something like a separator is part of the format. */
    }
    if (options.vsep->answer)
        vsep = G_option_to_separator(options.vsep);
    else
        vsep = NULL;
    vsep_needs_newline = true;
    if (vsep && !strcmp(vsep, "\n"))
        vsep_needs_newline = false;

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
        format != JSON && format != VERTICAL) {
        for (col = 0; col < ncols; col++) {
            column = db_get_table_column(table, col);
            if (col)
                fprintf(stdout, "%s", fsep);
            fprintf(stdout, "%s", db_get_column_name(column));
        }
        fprintf(stdout, "\n");
    }

    init_box = true;
    first_rec = true;

    if (format == JSON) {
        if (flags.region->answer)
            fprintf(stdout, "{\"extent\":\n");
        else
            fprintf(stdout, "{\"records\":[\n");
    }

    /* fetch the data */
    while (1) {
        if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
            G_fatal_error(_("Unable to fetch data from table <%s>"),
                          Fi->table);

        if (!more)
            break;

        if (first_rec)
            first_rec = false;
        else if (!flags.region->answer && format == JSON)
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

            if (!flags.colnames->answer && format == VERTICAL)
                fprintf(stdout, "%s%s", db_get_column_name(column), fsep);

            if (col && format != JSON && format != VERTICAL)
                fprintf(stdout, "%s", fsep);

            if (format == JSON) {
                if (!col)
                    fprintf(stdout, "{");
                fprintf(stdout, "\"%s\":", db_get_column_name(column));
            }

            if (db_test_value_isnull(value)) {
                if (format == JSON)
                    fprintf(stdout, "null");
                else if (options.nullval->answer)
                    fprintf(stdout, "%s", options.nullval->answer);
            }
            else {
                char *str = db_get_string(&value_string);

                /* Escaped charcters in different formats
                 * JSON (mandatory): \" \\ \r \n \t \f \b
                 * CSV (usually none, here optional): \\ \r \n \t \f \b
                 * Plain, vertical (optional): v7: \\ \r \n, v8 also: \t \f \b
                 */
                if (flags.escape->answer || format == JSON) {
                    if (strchr(str, '\\'))
                        str = G_str_replace(str, "\\", "\\\\");
                    if (strchr(str, '\r'))
                        str = G_str_replace(str, "\r", "\\r");
                    if (strchr(str, '\n'))
                        str = G_str_replace(str, "\n", "\\n");
                    if (strchr(str, '\t'))
                        str = G_str_replace(str, "\t", "\\t");
                    if (format == JSON && strchr(str, '"'))
                        str = G_str_replace(str, "\"", "\\\"");
                    if (strchr(str, '\f'))  /* form feed, somewhat unlikely */
                        str = G_str_replace(str, "\f", "\\f");
                    if (strchr(str, '\b'))  /* backspace, quite unlikely */
                        str = G_str_replace(str, "\b", "\\b");
                }
                /* Common CSV does not escape, but doubles quotes (and we quote all
                 * text fields which takes care of a separator character in text). */
                if (format == CSV && strchr(str, '"')) {
                    str = G_str_replace(str, "\"", "\"\"");
                }

                if (format == JSON || format == CSV) {
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

            if (format == VERTICAL)
                fprintf(stdout, "\n");
            else if (format == JSON) {
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
                    init_box = false;
                }
                else
                    Vect_box_extend(min_box, line_box);
            }
        }
        else {
            /* End of record in attribute printing */
            if (format != JSON && format != VERTICAL)
                fprintf(stdout, "\n");
            else if (vsep) {
                if (vsep_needs_newline)
                    fprintf(stdout, "%s\n", vsep);
                else
                    fprintf(stdout, "%s", vsep);
            }
        }
    }

    if (!flags.region->answer && format == JSON)
        fprintf(stdout, "\n]}\n");

    if (flags.region->answer) {
        if (format == CSV) {
            fprintf(stdout, "n%ss%sw%se", fsep, fsep, fsep);
            if (Vect_is_3d(&Map)) {
                fprintf(stdout, "%st%sb", fsep, fsep);
            }
            fprintf(stdout, "\n");
            fprintf(stdout, "%f%s%f%s%f%s%f", min_box->N, fsep, min_box->S,
                    fsep, min_box->W, fsep, min_box->E);
            if (Vect_is_3d(&Map)) {
                fprintf(stdout, "%s%f%s%f", fsep, min_box->T, fsep, min_box->B);
            }
            fprintf(stdout, "\n");
        }
        else if (format == JSON) {
            fprintf(stdout, "{");
            fprintf(stdout, "\"n\":%f,", min_box->N);
            fprintf(stdout, "\"s\":%f,", min_box->S);
            fprintf(stdout, "\"w\":%f,", min_box->W);
            fprintf(stdout, "\"e\":%f", min_box->E);
            if (Vect_is_3d(&Map)) {
                fprintf(stdout, ",\"t\":%f,", min_box->T);
                fprintf(stdout, "\"b\":%f", min_box->B);
            }
            fprintf(stdout, "\n}}\n");
        }
        else {
            fprintf(stdout, "n%s%f\n", fsep, min_box->N);
            fprintf(stdout, "s%s%f\n", fsep, min_box->S);
            fprintf(stdout, "w%s%f\n", fsep, min_box->W);
            fprintf(stdout, "e%s%f\n", fsep, min_box->E);
            if (Vect_is_3d(&Map)) {
                fprintf(stdout, "t%s%f\n", fsep, min_box->T);
                fprintf(stdout, "b%s%f\n", fsep, min_box->B);
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
