/****************************************************************************
 *
 * MODULE:       r.what
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Brad Douglas <rez touchofmadness.com>,
 *               Huidae Cho <grass4u gmail.com>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Soeren Gebbert <soeren.gebbert gmx.de>
 * PURPOSE:
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/colors.h>
#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/raster.h>
#include <grass/glocale.h>

enum OutputFormat { PLAIN, JSON };

static const char *fmt;

static int do_value(const char *buf, RASTER_MAP_TYPE type,
                    struct Colors *colors, enum OutputFormat outputFormat,
                    ColorFormat colorFormat, G_JSON_Array *root_array,
                    G_JSON_Value *root_value)
{
    CELL ival;
    DCELL fval;
    int red, grn, blu;
    char color_str[COLOR_STRING_LENGTH];

    G_JSON_Object *color_object = NULL;
    G_JSON_Value *color_value = NULL;

    if (outputFormat == JSON) {
        color_value = G_json_value_init_object();
        if (color_value == NULL) {
            G_json_value_free(root_value);
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        color_object = G_json_object(color_value);
    }

    switch (type) {
    case CELL_TYPE:
        if (sscanf(buf, "%d", &ival) != 1) {
            switch (outputFormat) {
            case PLAIN:
                fprintf(stdout, "*: *\n");
                break;

            case JSON:
                G_json_object_set_null(color_object, "value");
                G_json_object_set_null(color_object, "color");
                G_json_array_append_value(root_array, color_value);
                break;
            }
            return 0;
        }
        if (!Rast_get_c_color(&ival, &red, &grn, &blu, colors)) {
            switch (outputFormat) {
            case PLAIN:
                fprintf(stdout, "%d: *\n", ival);
                break;

            case JSON:
                G_json_object_set_number(color_object, "value", ival);
                G_json_object_set_null(color_object, "color");
                G_json_array_append_value(root_array, color_value);
                break;
            }
            return 0;
        }
        switch (outputFormat) {
        case PLAIN:
            fprintf(stdout, "%d: ", ival);
            if (strcmp(fmt, "plain") != 0) {
                fprintf(stdout, fmt, red, grn, blu);
            }
            else {
                G_color_to_str(red, grn, blu, colorFormat, color_str);
                fprintf(stdout, "%s", color_str);
            }
            fprintf(stdout, "\n");
            break;

        case JSON:
            G_json_object_set_number(color_object, "value", ival);
            G_color_to_str(red, grn, blu, colorFormat, color_str);
            G_json_object_set_string(color_object, "color", color_str);
            G_json_array_append_value(root_array, color_value);
            break;
        }
        return 1;

    case FCELL_TYPE:
    case DCELL_TYPE:
        if (sscanf(buf, "%lf", &fval) != 1) {
            switch (outputFormat) {
            case PLAIN:
                fprintf(stdout, "*: *\n");
                break;

            case JSON:
                G_json_object_set_null(color_object, "value");
                G_json_object_set_null(color_object, "color");
                G_json_array_append_value(root_array, color_value);
                break;
            }
            return 0;
        }
        if (!Rast_get_d_color(&fval, &red, &grn, &blu, colors)) {
            switch (outputFormat) {
            case PLAIN:
                fprintf(stdout, "%.15g: *\n", fval);
                break;

            case JSON:
                G_json_object_set_number(color_object, "value", fval);
                G_json_object_set_null(color_object, "color");
                G_json_array_append_value(root_array, color_value);
                break;
            }
            return 0;
        }
        switch (outputFormat) {
        case PLAIN:
            fprintf(stdout, "%.15g: ", fval);
            if (strcmp(fmt, "plain") != 0) {
                fprintf(stdout, fmt, red, grn, blu);
            }
            else {
                G_color_to_str(red, grn, blu, colorFormat, color_str);
                fprintf(stdout, "%s", color_str);
            }
            fprintf(stdout, "\n");
            break;

        case JSON:
            G_json_object_set_number(color_object, "value", fval);
            G_color_to_str(red, grn, blu, colorFormat, color_str);
            G_json_object_set_string(color_object, "color", color_str);
            G_json_array_append_value(root_array, color_value);
            break;
        }
        return 1;
    default:
        if (outputFormat == JSON)
            G_json_value_free(root_value);
        G_fatal_error("Invalid map type %d", type);
        return 0;
    }
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct {
        struct Option *input, *value, *format, *color_format;
    } opt;
    struct {
        struct Flag *i;
    } flag;
    const char *name;
    struct Colors colors;
    RASTER_MAP_TYPE type;

    enum OutputFormat outputFormat;
    ColorFormat colorFormat;

    G_JSON_Array *root_array = NULL;
    G_JSON_Value *root_value = NULL;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("querying"));
    G_add_keyword(_("color table"));
    module->description = _("Queries colors for a raster map layer.");

    opt.input = G_define_standard_option(G_OPT_R_INPUT);

    opt.value = G_define_option();
    opt.value->key = "value";
    opt.value->type = TYPE_DOUBLE;
    opt.value->required = NO;
    opt.value->multiple = YES;
    opt.value->description = _("Values to query colors for");

    opt.format = G_define_option();
    opt.format->key = "format";
    opt.format->type = TYPE_STRING;
    opt.format->required = NO;
    opt.format->answer = "%d:%d:%d";
    opt.format->label =
        _("Output format ('plain', 'json', or printf-style string)");
    opt.format->description = _("Output format printf-style is deprecated, use "
                                "'color_format' option instead.");

    opt.color_format = G_define_standard_option(G_OPT_C_FORMAT);
    opt.color_format->description =
        _("Color format for output values. Applies only when format is set to "
          "'plain' or 'json'.");
    opt.color_format->guisection = _("Color");

    flag.i = G_define_flag();
    flag.i->key = 'i';
    flag.i->description = _("Read values from stdin");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (!opt.value->answer && !flag.i->answer)
        G_fatal_error(_("Either \"-i\" or \"value=\" must be given"));

    name = opt.input->answer;

    type = Rast_map_type(name, "");
    if (type < 0)
        G_fatal_error("Unable to determine type of input map %s", name);

    if (Rast_read_colors(name, "", &colors) < 0)
        G_fatal_error("Unable to read colors for input map %s", name);

    fmt = opt.format->answer;

    if (strcmp(fmt, "json") == 0) {
        outputFormat = JSON;

        root_value = G_json_value_init_array();
        if (root_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        root_array = G_json_array(root_value);
    }
    else {
        outputFormat = PLAIN;

        if (strcmp(fmt, "plain") != 0) {
            G_verbose_message(
                _("The printf-style output format is deprecated and will "
                  "be removed in a future release. Please use the "
                  "'color_format' option instead, along with 'format=plain'."));
        }
    }

    if (strcmp(fmt, "plain") == 0 || strcmp(fmt, "json") == 0) {
        colorFormat = G_option_to_color_format(opt.color_format);
    }

    if (flag.i->answer) {
        for (;;) {
            char buf[64];

            if (!fgets(buf, sizeof(buf), stdin))
                break;

            do_value(buf, type, &colors, outputFormat, colorFormat, root_array,
                     root_value);
        }
    }
    else if (opt.value->answer) {
        const char *ans;
        int i;

        for (i = 0; ans = opt.value->answers[i], ans; i++)
            do_value(ans, type, &colors, outputFormat, colorFormat, root_array,
                     root_value);
    }

    if (outputFormat == JSON) {
        char *serialized_string = NULL;
        serialized_string = G_json_serialize_to_string_pretty(root_value);
        if (serialized_string == NULL) {
            G_json_value_free(root_value);
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        G_json_free_serialized_string(serialized_string);
        G_json_value_free(root_value);
    }

    return EXIT_SUCCESS;
}
