/****************************************************************************
 *
 * MODULE:       m.measure
 * AUTHOR(S):    Created from d.measure by Glynn Clements, 2010
 *               James Westervelt and Michael Shapiro
 *                (CERL - original contributors)
 *               Markus Neteler <neteler itc.it>,
 *               Reinhard Brunzema <r.brunzema@web.de>,
 *               Bernhard Reiter <bernhard intevation.de>,
 *               Huidae Cho <grass4u gmail.com>,
 *               Eric G. Miller <egm2 jps.net>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Jan-Oliver Wagner <jan intevation.de>
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:      Distance and area measurement
 * COPYRIGHT:    (C) 1999-2006, 2010 by the GRASS Development Team
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
#include <grass/glocale.h>

enum OutputFormat { PLAIN, SHELL, JSON };

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *coords, *units, *frmt;
    struct Flag *shell;

    double *x, *y;
    double length, area, f, sq_f;
    int i, npoints;
    const char *units_name, *sq_units_name;
    enum OutputFormat format;
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->description = _("Measures the lengths and areas of features.");
    G_add_keyword(_("miscellaneous"));
    G_add_keyword(_("measurement"));
    G_add_keyword(_("distance"));
    G_add_keyword(_("area"));

    coords = G_define_standard_option(G_OPT_M_COORDS);
    coords->required = YES;
    coords->multiple = YES;

    units = G_define_standard_option(G_OPT_M_UNITS);
    units->label = _("Units");
    units->description = _("Default: project map units");

    frmt = G_define_standard_option(G_OPT_F_FORMAT);
    frmt->options = "plain,shell,json";
    frmt->descriptions = _("plain;Plain text output;"
                           "shell;shell script style output;"
                           "json;JSON (JavaScript Object Notation);");
    frmt->guisection = _("Print");

    shell = G_define_flag();
    shell->key = 'g';
    shell->label = _("Shell script style [deprecated]");
    shell->description = _(
        "This flag is deprecated and will be removed in a future release. Use "
        "format=shell instead.");

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
    }

    npoints = 0;
    for (i = 0; coords->answers[i]; i += 2)
        npoints++;
    x = G_malloc(npoints * sizeof(double));
    y = G_malloc(npoints * sizeof(double));

    for (i = 0; i < npoints; i++) {
        x[i] = atof(coords->answers[2 * i + 0]);
        y[i] = atof(coords->answers[2 * i + 1]);
    }

    /* determine units */
    if (G_projection() == PROJECTION_LL && !units->answer) {
        units_name = G_get_units_name(U_METERS, 1, 0);
        sq_units_name = G_get_units_name(U_METERS, 1, 1);
    }
    else {
        units_name = G_get_units_name(G_units(units->answer), 1, 0);
        sq_units_name = G_get_units_name(G_units(units->answer), 1, 1);
    }
    f = G_meters_to_units_factor(G_units(units->answer));
    sq_f = G_meters_to_units_factor_sq(G_units(units->answer));

    G_debug(1, "using '%s (%f) %s (%f)'", units_name, f, sq_units_name, sq_f);

    G_begin_distance_calculations();
    length = 0;
    for (i = 1; i < npoints; i++)
        length += G_distance(x[i - 1], y[i - 1], x[i], y[i]);

    switch (format) {
    case SHELL:
        printf("units=%s,%s\n", units_name, sq_units_name);
        /* length */
        printf("length=%.6f\n", f * length);
        break;

    case PLAIN:
        printf("%-8s %10.6f %s\n", _("Length:"), f * length, units_name);
        break;

    case JSON:
        G_json_object_dotset_string(root_object, "units.length", units_name);
        G_json_object_dotset_string(root_object, "units.area", sq_units_name);
        G_json_object_set_number(root_object, "length", f * length);
        break;
    }

    if (npoints > 3) {
        G_begin_polygon_area_calculations();
        area = G_area_of_polygon(x, y, npoints);
        switch (format) {
        case SHELL:
            printf("area=%.6f\n", sq_f * area);
            break;

        case PLAIN:
            printf("%-8s %10.6f %s\n", _("Area:"), sq_f * area, sq_units_name);
            break;

        case JSON:
            G_json_object_set_number(root_object, "area", sq_f * area);
            break;
        }
    }

    if (format == JSON) {
        if (npoints <= 3)
            G_json_object_set_number(root_object, "area", 0);

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
