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
    //flags.like_test->guisection = _("");
    // suppress_required

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);
}

char *min_json_escape(const char *str)
{
    char *tmp1 = G_str_replace(str, "\\", "\\\\");
    char *tmp2 = G_str_replace(tmp1, "\"", "\\\"");

    G_free(tmp1);
    return tmp2;
}

void json_print_name_mapset(const char *name, const char *mapset)
{
    // Being paranoid about what is in the name.
    char *escaped_name = min_json_escape(name);
    char *escaped_mapset = min_json_escape(mapset);

    printf("\"%s@%s\"", name, mapset);
    G_free(escaped_name);
    G_free(escaped_mapset);
}

int report_status(struct Parameters *params)
{

    char name[GNAME_MAX];
    char mapset[GMAPSET_MAX];
    char reclass_name[GNAME_MAX];
    char reclass_mapset[GMAPSET_MAX];

    bool is_mask_reclass;
    bool present =
        Rast_mask_status(name, mapset, &is_mask_reclass, reclass_name, reclass_mapset);
    // bool present = Rast_mask_present(name, mapset);  // This would check the map presence rather than the automasking state in the library.

    // printf("%s", Rast_mask_info());

    if (params->like_test->answer) {
        if (present)
            return 0;
        else
            return 1;
    }
    else if (strcmp(params->format->answer, "json") == 0) {
        printf("{\"present\":");
        if (present)
            printf("true");
        else
            printf("false");
        printf(",\n\"full_name\":");
        if (present)
            json_print_name_mapset("MASK", G_mapset()); // Too much mask details here, move this to the library.
        else
            printf("null");
        printf(",\n\"is_reclass_of\": ");
        if (is_mask_reclass)
            json_print_name_mapset(name, mapset);
        else
            printf("null");
        printf("}\n");
    }
    else if (strcmp(params->format->answer, "bash") == 0) {
        printf("present=");
        if (present)
            printf("1"); // Good choice here or not?
        else
            printf("0");
        printf("\nfull_name=");
        if (present) {
            json_print_name_mapset(name, mapset);
        }
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
            printf("|-\n  MASK@%s", G_mapset()); // MASK or MASK@current_mapset
        else
            printf("null");
        printf("\nis_reclass_of: ");
        // Using block scalar with |- to avoid need for escaping.
        if (is_mask_reclass)
            printf("|-\n  %s@%s", name, mapset);
        else
            printf("null");
        // true if MASK in current mapset and is reclass, false otherwise,
        // then also outputting mask cats is needed to inform user about the portion of the map
        // printf("\nmask_reclass: ");
        // if (is_mask_reclass)
        //     printf("true");
        // else
        //     printf("false");
        printf("\n");
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    struct Parameters params;

    G_gisinit(argv[0]);
    parse_parameters(&params, argc, argv);
    return report_status(&params);
}
