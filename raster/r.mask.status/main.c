/****************************************************************************
 *
 * MODULE:       r.mask.status
 * AUTHORS:      Vaclav Petras
 * PURPOSE:      Report status of raster mask
 * COPYRIGHT:    (C) 2024 by Vaclav Petras and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/parson.h>
#include <grass/raster.h>
#include <grass/glocale.h>

struct Parameters {
    struct Option *format;
    struct Flag *like_test;
};

void parse_parameters(struct Parameters *params, int argc, char **argv)
{
    struct GModule *module;

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("mask"));
    G_add_keyword(_("reclassification"));
    module->label = _("Reports presence or absence of a raster mask");
    module->description =
        _("Provides information about the presence of a 2D raster mask"
          " as text output or return code");

    params->format = G_define_option();
    params->format->key = "format";
    params->format->type = TYPE_STRING;
    params->format->required = NO;
    params->format->answer = "plain";
    params->format->options = "plain,json,shell,yaml";
    params->format->descriptions =
        "plain;Plain text output;"
        "json;JSON (JavaScript Object Notation);"
        "shell;Shell script style output;"
        "yaml;YAML (human-friendly data serialization language)";
    params->format->description = _("Format for reporting");

    params->like_test = G_define_flag();
    params->like_test->key = 't';
    params->like_test->label =
        _("Return code 0 when mask present, 1 otherwise");
    params->like_test->description =
        _("Behave like the test utility, 0 for true, 1 for false, no output");
    // suppress_required is not required given the default value for format.
    // Both no parameters and only -t work as expected.

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);
}

int report_status(struct Parameters *params)
{

    char name[GNAME_MAX];
    char mapset[GMAPSET_MAX];
    char reclass_name[GNAME_MAX];
    char reclass_mapset[GMAPSET_MAX];

    bool is_mask_reclass;
    bool present = Rast_mask_status(name, mapset, &is_mask_reclass,
                                    reclass_name, reclass_mapset);

    // This does not have to be exclusive with the printing, but leaving this
    // to a different boolean flag which could do the return code and printing.
    // The current implementation really behaves like the test utility which
    // facilitates the primary usage of this which is prompt building
    // (and there any output would be noise).
    if (params->like_test->answer) {
        if (present)
            return 0;
        return 1;
    }

    // Mask raster
    char *full_mask = Rast_mask_name();
    // Underlying raster if applicable
    char *full_underlying = NULL;
    if (is_mask_reclass)
        full_underlying = G_fully_qualified_name(reclass_name, reclass_mapset);

    if (strcmp(params->format->answer, "json") == 0) {
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_object(root_value);
        json_object_set_boolean(root_object, "present", present);
        json_object_set_string(root_object, "name", full_mask);
        if (is_mask_reclass)
            json_object_set_string(root_object, "is_reclass_of",
                                   full_underlying);
        else
            json_object_set_null(root_object, "is_reclass_of");
        char *serialized_string = json_serialize_to_string_pretty(root_value);
        if (!serialized_string)
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        puts(serialized_string);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
    }
    else if (strcmp(params->format->answer, "shell") == 0) {
        printf("present=");
        if (present)
            printf("1");
        else
            printf("0");
        printf("\nname=%s", full_mask);
        printf("\nis_reclass_of=");
        if (is_mask_reclass)
            printf("%s", full_underlying);
        printf("\n");
    }
    else if (strcmp(params->format->answer, "yaml") == 0) {
        printf("present: ");
        if (present)
            printf("true");
        else
            printf("false");
        printf("\nname: ");
        printf("|-\n  %s", full_mask);
        printf("\nis_reclass_of: ");
        // Using block scalar with |- to avoid need for escaping.
        // Alternatively, we could check mapset naming limits against YAML
        // escaping needs for different types of strings and do the necessary
        // escaping here.
        // Null values in YAML can be an empty (no) value (rather than null),
        // so we could use that, but using the explicit null as a reasonable
        // starting point.
        if (is_mask_reclass)
            printf("|-\n  %s", full_underlying);
        else
            printf("null");
        printf("\n");
    }
    else {
        if (present) {
            printf(_("Mask is active"));
            printf(_("Mask name: %s"), full_mask);
        }
        else {
            printf(_("Mask is not present"));
            printf(_("If activated, mask name will be: %s"), full_mask);
        }
        if (is_mask_reclass) {
            printf("\n");
            printf(_("Mask is a raster reclassified from: %s"),
                   full_underlying);
        }
        printf("\n");
    }

    G_free(full_mask);
    G_free(full_underlying);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    struct Parameters params;

    G_gisinit(argv[0]);
    parse_parameters(&params, argc, argv);
    return report_status(&params);
}
