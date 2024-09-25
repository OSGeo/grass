/****************************************************************************
 *
 * MODULE:       r.mask.status
 * AUTHORS:      Vaclav Petras
 * PURPOSE:      Report status of raster mask
 * COPYRIGHT:    (C) 2022 by Vaclav Petras and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

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
    G_add_keyword(_("reclassification"));
    module->label = _("Reclassify raster map based on category values.");
    module->description =
        _("Creates a new raster map whose category values are based "
          "upon a reclassification of the categories in an existing "
          "raster map.");

    params->format = G_define_option();
    params->format->key = "format";
    params->format->type = TYPE_STRING;
    params->format->required = NO;
    params->format->answer = "yaml";
    params->format->options = "yaml,json,bash";
    params->format->description = _("Format for reporting");

    params->like_test = G_define_flag();
    params->like_test->key = 't';
    params->like_test->label =
        _("Return code 0 when mask present, 1 otherwise");
    params->like_test->description =
        _("Behave like the test utility, 0 for true, 1 for false, no output");
    // flags.like_test->guisection = _("");
    // suppress_required

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
    // TODO: Review need the new function. Can it be made more universal now?
    // (to avoid need to repeat MASK here anyway)
    bool present = Rast_mask_status(name, mapset, &is_mask_reclass,
                                    reclass_name, reclass_mapset);
    // bool present = Rast_mask_present(name, mapset);  // This would check the
    // map presence rather than the automasking state in the library.

    // printf("%s", Rast_mask_info());

    // This does not have to be exclusive with the printing, but perhaps there
    // is a different boolean flag which does the return code and printing and
    // this really behaves like the test utility facilitate the primary usage of
    // this which is prompt building (there any output would be noise).
    if (params->like_test->answer) {
        if (present)
            return 0;
        return 1;
    }

    // Mask raster
    // TODO: Too much mask details here, refactor this to the library.
    // Specifics about mask name and mapset should be in the library,
    // but that's likely better done in #2392 (mask from env variable).
    char *full_mask = G_fully_qualified_name("MASK", G_mapset());
    // Underlying raster if applicable
    char *full_underlying = NULL;
    if (is_mask_reclass)
        full_underlying = G_fully_qualified_name(reclass_name, reclass_mapset);

    if (strcmp(params->format->answer, "json") == 0) {
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_object(root_value);
        json_object_set_boolean(root_object, "present", present);
        if (present)
            json_object_set_string(root_object, "full_name", full_mask);
        else
            json_object_set_null(root_object, "full_name");
        if (is_mask_reclass)
            json_object_set_string(root_object, "is_reclass_of",
                                   full_underlying);
        else
            json_object_set_null(root_object, "is_reclass_of");
        char *serialized_string = json_serialize_to_string_pretty(root_value);
        puts(serialized_string);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
    }
    else if (strcmp(params->format->answer, "bash") == 0) {
        printf("present=");
        if (present)
            printf("1"); // Good choice here or not?
        else
            printf("0");
        printf("\nfull_name=");
        if (present)
            printf("%s", full_mask);
        printf("\nis_reclass_of=");
        if (is_mask_reclass)
            printf("%s", full_underlying);
        printf("\n");
    }
    else {
        printf("present: ");
        if (present)
            printf("true");
        else
            printf("false");
        printf("\nfull_name: ");
        if (present)
            printf("|-\n  %s", full_mask);
        else
            printf("null");
        printf("\nis_reclass_of: ");
        // Using block scalar with |- to avoid need for escaping.
        if (is_mask_reclass)
            printf("|-\n  %s", full_underlying);
        else
            printf("null");
        // We could also produce true if is reclass, false otherwise.
        // printf("\nmask_reclass: ");
        // if (is_mask_reclass)
        //     printf("true");
        // else
        //     printf("false");
        // We could also outputting mask cats to inform user about the
        // relevant portion of the map, but that should be done by accessing
        // the actual mask anyway.
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
