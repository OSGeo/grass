/*****************************************************************************
 *
 * MODULE:       v.what
 *
 * AUTHOR(S):    Trevor Wiens - derived from d.what.vect - 15 Jan 2006
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               Multiple features by Huidae Cho <grass4u gmail.com>
 *
 * PURPOSE:      To select and report attribute information for objects at a
 *               user specified location. This replaces d.what.vect by removing
 *               the interactive component to enable its use with the new
 *               gis.m and future GUI.
 *
 * COPYRIGHT:    (C) 2006-2010, 2011, 2017 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "what.h"

int main(int argc, char **argv)
{
    struct {
        struct Flag *print, *topo, *shell, *json, *multiple, *connection;
    } flag;
    struct {
        struct Option *map, *field, *coords, *maxdist, *type, *cols, *format;
    } opt;
    struct Cell_head window;
    struct GModule *module;

    char **vect;
    int nvects;
    struct Map_info *Map;

    char buf[2000];
    int i, level, ret, type;
    int *field;
    char *columns;
    double xval, yval, xres, yres, maxd, x;
    double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;
    char nsres[30], ewres[30];
    char ch;
    enum OutputFormat format;
    G_JSON_Value *root_value = NULL;
    G_JSON_Array *root_array = NULL;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("querying"));
    G_add_keyword(_("position"));
    module->description = _("Queries a vector map at given locations.");

    opt.map = G_define_standard_option(G_OPT_V_MAPS);

    opt.field = G_define_standard_option(G_OPT_V_FIELD_ALL);
    opt.field->multiple = YES;

    opt.type = G_define_standard_option(G_OPT_V3_TYPE);
    opt.type->answer = "point,line,area,face";

    opt.coords = G_define_standard_option(G_OPT_M_COORDS);
    opt.coords->required = YES;
    opt.coords->label = _("Coordinates for query");
    opt.coords->description = _("'-' for standard input");

    opt.maxdist = G_define_option();
    opt.maxdist->type = TYPE_DOUBLE;
    opt.maxdist->key = "distance";
    opt.maxdist->answer = "0";
    opt.maxdist->multiple = NO;
    opt.maxdist->description = _("Query threshold distance");
    opt.maxdist->guisection = _("Threshold");

    opt.cols = G_define_standard_option(G_OPT_DB_COLUMNS);
    opt.cols->label = _("Name of attribute column(s)");
    opt.cols->description = _("Default: all columns");

    opt.format = G_define_standard_option(G_OPT_F_FORMAT);
    opt.format->options = "plain,shell,json";
    opt.format->required = NO;
    opt.format->answer = NULL;
    opt.format->descriptions = _("plain;Plain text output;"
                                 "shell;shell script style output;"
                                 "json;JSON (JavaScript Object Notation);");
    opt.format->guisection = _("Print");

    flag.print = G_define_flag();
    flag.print->key = 'a';
    flag.print->description = _("Print attribute information");
    flag.print->guisection = _("Print");

    flag.connection = G_define_flag();
    flag.connection->key = 'i';
    flag.connection->description =
        _("Print attribute database connection information");
    flag.connection->guisection = _("Print");

    flag.topo = G_define_flag();
    flag.topo->key = 'd';
    flag.topo->label = _("Print topological information (debugging)");
    flag.topo->description =
        _("Prints internal information for topology debugging");
    flag.topo->guisection = _("Print");

    flag.shell = G_define_flag();
    flag.shell->key = 'g';
    flag.shell->label = _("Print the stats in shell script style [deprecated]");
    flag.shell->description = _(
        "This flag is deprecated and will be removed in a future release. Use "
        "format=shell instead.");
    flag.shell->guisection = _("Print");

    flag.json = G_define_flag();
    flag.json->key = 'j';
    flag.json->label = _("Print the stats in JSON [deprecated]");
    flag.json->description = _(
        "This flag is deprecated and will be removed in a future release. Use "
        "format=json instead.");
    flag.json->guisection = _("Print");

    flag.multiple = G_define_flag();
    flag.multiple->key = 'm';
    flag.multiple->label =
        _("Print multiple features for each map if they meet the criteria");
    flag.multiple->description =
        _("For JSON, this places features under a \"features\" key");
    flag.multiple->guisection = _("Print");

    G_option_exclusive(flag.shell, flag.json, opt.format, NULL);
    G_option_requires(flag.connection, flag.print, NULL);
    G_option_requires(opt.cols, flag.print, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* initialize variables */
    vect = NULL;
    Map = NULL;
    nvects = 0;
    field = NULL;

    if (opt.map->answers && opt.map->answers[0])
        vect = opt.map->answers;
    else
        G_fatal_error(_("No input vector maps!"));

    /* get specified column names */
    if (opt.cols->answers && opt.cols->answers[0]) {
        int col_alloc;
        int new_len;

        col_alloc = 1024;
        columns = (char *)G_calloc(col_alloc, sizeof(char));
        for (i = 0; opt.cols->answers[i]; i++) {
            new_len = strlen(columns) + strlen(opt.cols->answers[i]) + 1;
            if (new_len >= col_alloc) {
                col_alloc = new_len + 1024;
                columns = G_realloc(columns, col_alloc);
            }
            if (i > 0)
                strcat(columns, ",");
            strcat(columns, opt.cols->answers[i]);
        }
    }
    else
        columns = "*";

    maxd = atof(opt.maxdist->answer);
    type = Vect_option_to_types(opt.type);
    if (opt.format->answer == NULL || opt.format->answer[0] == '\0') {
        format = flag.shell->answer ? SHELL
                                    : (flag.json->answer ? LEGACY_JSON : PLAIN);
        if (format == LEGACY_JSON) {
            G_verbose_message(
                _("Flag 'j' is deprecated and will be removed in a future "
                  "release. Please use format=json instead."));
        }
        else if (format == SHELL) {
            G_verbose_message(
                _("Flag 'g' is deprecated and will be removed in a future "
                  "release. Please use format=shell instead."));
        }
    }
    else {
        if (strcmp(opt.format->answer, "json") == 0) {
            format = JSON;
        }
        else if (strcmp(opt.format->answer, "shell") == 0) {
            format = SHELL;
        }
        else {
            format = PLAIN;
        }
    }

    /* For backward compatibility */
    if (format != JSON) {
        if (!flag.connection->answer) {
            G_verbose_message(_(
                "Flag 'i' prints attribute database connection information. "
                "It is currently always enabled for backward compatibility, "
                "but this behavior will be removed in a future release. "
                "Please use the 'i' flag together with the 'a' flag instead."));
        }
        flag.connection->answer = 1;
    }

    if (format == JSON) {
        root_value = G_json_value_init_array();
        if (root_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        root_array = G_json_array(root_value);
    }

    if (maxd == 0.0) {
        /* this code is a translation from d.what.vect which uses display
         * resolution to figure out a querying distance
         * display resolution is not available here
         * using raster resolution instead to determine vector querying
         * distance does not really make sense
         * maxd = 0 can make sense */
        G_get_window(&window);
        x = window.proj;
        G_format_resolution(window.ew_res, ewres, x);
        G_format_resolution(window.ns_res, nsres, x);
        G_begin_distance_calculations();
        EW_DIST1 =
            G_distance(window.east, window.north, window.west, window.north);
        /* EW Dist at South Edge */
        EW_DIST2 =
            G_distance(window.east, window.south, window.west, window.south);
        /* NS Dist at East edge */
        NS_DIST1 =
            G_distance(window.east, window.north, window.east, window.south);
        /* NS Dist at West edge */
        NS_DIST2 =
            G_distance(window.west, window.north, window.west, window.south);
        xres = ((EW_DIST1 + EW_DIST2) / 2) / window.cols;
        yres = ((NS_DIST1 + NS_DIST2) / 2) / window.rows;
        if (xres > yres)
            maxd = xres;
        else
            maxd = yres;
    }

    /* Look at maps given on command line */
    if (vect) {
        for (i = 0; vect[i]; i++)
            ;
        nvects = i;

        for (i = 0; opt.field->answers[i]; i++)
            ;

        if (nvects != i)
            G_fatal_error(_("Number of given vector maps (%d) differs from "
                            "number of layers (%d)"),
                          nvects, i);

        Map = (struct Map_info *)G_malloc(nvects * sizeof(struct Map_info));
        field = (int *)G_malloc(nvects * sizeof(int));

        for (i = 0; i < nvects; i++) {
            level = Vect_open_old2(&Map[i], vect[i], "", opt.field->answers[i]);
            if (level < 2)
                G_fatal_error(_("You must build topology on vector map <%s>"),
                              vect[i]);
            field[i] = Vect_get_field_number(&Map[i], opt.field->answers[i]);
        }
    }

    if (strcmp(opt.coords->answer, "-") == 0) {
        /* read them from stdin */
        setvbuf(stdin, NULL, _IOLBF, 0);
        setvbuf(stdout, NULL, _IOLBF, 0);
        while (fgets(buf, sizeof(buf), stdin) != NULL) {
            ret = sscanf(buf, "%lf%c%lf", &xval, &ch, &yval);
            if (ret == 3 && (ch == ',' || ch == ' ' || ch == '\t')) {
                what(Map, nvects, vect, xval, yval, maxd, type,
                     flag.topo->answer, flag.print->answer, format,
                     flag.multiple->answer, field, columns, root_array,
                     flag.connection->answer);
            }
            else {
                G_warning(_("Unknown input format, skipping: '%s'"), buf);
                continue;
            }
        }
    }
    else {
        /* use coords given on command line */
        for (i = 0; opt.coords->answers[i] != NULL; i += 2) {
            xval = atof(opt.coords->answers[i]);
            yval = atof(opt.coords->answers[i + 1]);
            what(Map, nvects, vect, xval, yval, maxd, type, flag.topo->answer,
                 flag.print->answer, format, flag.multiple->answer, field,
                 columns, root_array, flag.connection->answer);
        }
    }

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

    for (i = 0; i < nvects; i++)
        Vect_close(&Map[i]);

    exit(EXIT_SUCCESS);
}
