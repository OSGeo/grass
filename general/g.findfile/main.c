/****************************************************************************
 *
 * MODULE:       g.findfile
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jan-Oliver Wagner <jan intevation.de>
 *               Martin landa <landa.martin gmail.com>
 * PURPOSE:      Searches for GRASS data base files
 * COPYRIGHT:    (C) 1999-2008, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/manage.h>
#include <grass/glocale.h>

#include "local_proto.h"

enum OutputFormat { SHELL, JSON };

void print_json(JSON_Value *root_value)
{
    char *json_string = G_json_serialize_to_string_pretty(root_value);
    if (!json_string) {
        G_json_value_free(root_value);
        G_fatal_error(_("Failed to serialize JSON to pretty format."));
    }

    puts(json_string);

    G_json_free_serialized_string(json_string);
    G_json_value_free(root_value);
}

int main(int argc, char *argv[])
{
    char file[GPATH_MAX], name[GNAME_MAX];
    const char *search_mapset, *mapset;
    struct GModule *module;
    struct Option *elem_opt;
    struct Option *mapset_opt;
    struct Option *file_opt;
    struct Option *frmt_opt;
    struct Flag *n_flag, *l_flag, *t_flag;
    size_t len;
    enum OutputFormat format;
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("scripts"));
    module->description = _("Searches for GRASS data base files "
                            "and sets variables for the shell.");

    G_gisinit(argv[0]);

    /* Define the different options */

    elem_opt = G_define_option();
    elem_opt->key = "element";
    elem_opt->type = TYPE_STRING;
    elem_opt->required = YES;
    elem_opt->description = _("Name of an element");

    file_opt = G_define_option();
    file_opt->key = "file";
    file_opt->type = TYPE_STRING;
    file_opt->required = YES;
    file_opt->description = _("Name of an existing map");

    mapset_opt = G_define_option();
    mapset_opt->key = "mapset";
    mapset_opt->type = TYPE_STRING;
    mapset_opt->required = NO;
    mapset_opt->label = _("Name of a mapset (default: search path)");
    mapset_opt->description = _("'.' for current mapset");

    frmt_opt = G_define_standard_option(G_OPT_F_FORMAT);
    frmt_opt->options = "shell,json";
    frmt_opt->answer = "shell";
    frmt_opt->descriptions = _("shell;shell script style output;"
                               "json;JSON (JavaScript Object Notation);");
    frmt_opt->guisection = _("Print");

    n_flag = G_define_flag();
    n_flag->key = 'n';
    n_flag->label = _("Do not add quotes");
    n_flag->description = _("Ignored when format is set to JSON");

    l_flag = G_define_flag();
    l_flag->key = 'l';
    l_flag->description = _("List available elements and exit");
    l_flag->suppress_required = YES;

    t_flag = G_define_flag();
    t_flag->key = 't';
    t_flag->label = _("Return code 0 when file found, 1 otherwise");
    t_flag->description =
        _("Behave like the test utility, 0 for true, 1 for false, no output");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (strcmp(frmt_opt->answer, "json") == 0) {
        format = JSON;
    }
    else {
        format = SHELL;
    }

    if (l_flag->answer) {
        if (format == JSON) {
            G_fatal_error(_("-%c flag cannot be used with format=json"),
                          l_flag->key);
        }
        list_elements();
        return EXIT_SUCCESS;
    }

    search_mapset = mapset_opt->answer;
    if (!search_mapset) {
        search_mapset = G_store("");
    }
    if (strcmp(".", search_mapset) == 0)
        search_mapset = G_mapset();

    if (mapset_opt->answer && strlen(mapset_opt->answer) > 0) {
        char **map_mapset = G_tokenize(file_opt->answer, "@");

        if (G_number_of_tokens(map_mapset) > 1) {
            if (strcmp(map_mapset[1], mapset_opt->answer))
                G_fatal_error(
                    _("Parameter 'file' contains reference to <%s> mapset, "
                      "but mapset parameter <%s> does not correspond"),
                    map_mapset[1], mapset_opt->answer);
            else
                strcpy(name, file_opt->answer);
        }
        if (G_number_of_tokens(map_mapset) == 1)
            strcpy(name, file_opt->answer);
        G_free_tokens(map_mapset);
    }
    else {
        len = G_strlcpy(name, file_opt->answer, sizeof(name));
        if (len >= sizeof(name)) {
            G_fatal_error(_("Name <%s> is too long"), file_opt->answer);
        }
    }

    const struct list *element;
    int n;
    M_read_list(FALSE, &n);
    n = M_get_element(elem_opt->answer);
    char *main_element;
    if (n >= 0) {
        element = M_get_list(n);
        main_element = G_store(element->mainelem);
    }
    else {
        main_element = G_store(elem_opt->answer);
    }
    mapset = G_find_file2(main_element, name, search_mapset);

    if (t_flag->answer) {
        if (mapset)
            return 0;
        else
            return 1;
    }

    if (format == JSON) {
        root_value = G_json_value_init_object();
        if (root_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        root_object = G_json_object(root_value);
    }

    if (mapset) {
        char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
        const char *qchar = n_flag->answer ? "" : "'";
        const char *qual = G_fully_qualified_name(name, mapset);

        G_unqualified_name(name, mapset, xname, xmapset);
        G_file_name(file, main_element, name, mapset);
        switch (format) {
        case SHELL:
            fprintf(stdout, "name=%s%s%s\n", qchar, xname, qchar);
            fprintf(stdout, "mapset=%s%s%s\n", qchar, xmapset, qchar);
            fprintf(stdout, "fullname=%s%s%s\n", qchar, qual, qchar);
            fprintf(stdout, "file=%s%s%s\n", qchar, file, qchar);
            break;

        case JSON:
            G_json_object_set_string(root_object, "name", xname);
            G_json_object_set_string(root_object, "mapset", xmapset);
            G_json_object_set_string(root_object, "fullname", qual);
            G_json_object_set_string(root_object, "file", file);

            print_json(root_value);
            break;
        }

        G_free(main_element);
        return EXIT_SUCCESS;
    }
    else {
        switch (format) {
        case SHELL:
            fprintf(stdout, "name=\n");
            fprintf(stdout, "mapset=\n");
            fprintf(stdout, "fullname=\n");
            fprintf(stdout, "file=\n");
            break;

        case JSON:
            G_json_object_set_null(root_object, "name");
            G_json_object_set_null(root_object, "mapset");
            G_json_object_set_null(root_object, "fullname");
            G_json_object_set_null(root_object, "file");

            print_json(root_value);

            G_free(main_element);
            return EXIT_SUCCESS;
        }
    }

    G_free(main_element);
    G_verbose_message(_("In the next major release, g.findfile will no longer "
                        "return exit code 1 when a file is not found. Please "
                        "use the -t flag instead to get the error code."));
    return EXIT_FAILURE;
}
