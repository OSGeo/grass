/***************************************************************
 *
 * MODULE:       v.build
 *
 * AUTHOR(S):    Radim Blazek
 *
 * PURPOSE:      Build topology
 *
 * COPYRIGHT:    (C) 2001, 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/gjson.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *map_opt, *opt, *err_opt;
    struct Flag *chk;
    struct Map_info Map;
    int i, build, dump, sdump, cdump, fdump;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char *opt_desc;
    struct Option *format_opt;

    G_gisinit(argv[0]);

    format_opt = G_define_option();
    format_opt->key = "format";
    format_opt->type = TYPE_STRING;
    format_opt->required = NO;
    format_opt->options = "plain,json";
    format_opt->answer = "plain";
    format_opt->description = _("Output format");

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("topology"));
    G_add_keyword(_("geometry"));
    module->label = _("Creates topology for vector map.");
    module->description = _("Optionally also checks for topological errors.");

    map_opt = G_define_standard_option(G_OPT_V_MAP);
    map_opt->label = NULL;
    map_opt->description = _("Name of vector map");

    err_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    err_opt->key = "error";
    err_opt->description = _("Name for output vector map where erroneous "
                             "vector features are written to");
    err_opt->required = NO;
    err_opt->guisection = _("Errors");

    opt = G_define_option();
    opt->key = "option";
    opt->type = TYPE_STRING;
    opt->options = "build,dump,sdump,cdump,fdump";
    opt->required = NO;
    opt->multiple = YES;
    opt->answer = "build";
    opt->description =
        _("Build topology or dump topology or indices to standard output");
    opt_desc = NULL;
    G_asprintf(&opt_desc, "build;%s;dump;%s;sdump;%s;cdump;%s;fdump;%s",
               _("build topology"), _("write topology to stdout"),
               _("write spatial index to stdout"),
               _("write category index to stdout"),
               _("write feature index to stdout (OGR simple-feature-based "
                 "formats only)"));
    opt->descriptions = opt_desc;
    opt->required = YES;

    chk = G_define_flag();
    chk->key = 'e';
    chk->label = _("Extensive checks for topological errors");
    chk->description = _("Perform in-depth checks for topological errors when "
                         "building topology");
    chk->guisection = _("Errors");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    build = dump = sdump = cdump = fdump = FALSE;
    i = 0;
    while (opt->answers[i]) {
        if (*opt->answers[i] == 'b')
            build = TRUE;
        else if (*opt->answers[i] == 'd')
            dump = TRUE;
        else if (*opt->answers[i] == 's')
            sdump = TRUE;
        else if (*opt->answers[i] == 'c')
            cdump = TRUE;
        else if (*opt->answers[i] == 'f')
            fdump = TRUE;
        i++;
    }
    if (err_opt->answer) {
        Vect_check_input_output_name(map_opt->answer, err_opt->answer,
                                     G_FATAL_EXIT);
    }

    /* build topology */
    if (build) {
        if (G_name_is_fully_qualified(map_opt->answer, xname, xmapset)) {
            if (0 == G_strcasecmp(xmapset, "OGR")) {
                G_fatal_error(_("Direct read access to OGR layers is not "
                                "supported by this module. "
                                "Run %s to create a link as GRASS vector map "
                                "in the current mapset."),
                              "'v.external'");
            }
            if (0 != strcmp(xmapset, G_mapset())) {
                G_fatal_error(_("Vector map <%s> is not in current mapset"),
                              map_opt->answer);
            }
        }
        Vect_set_open_level(1);

        if (Vect_open_old(&Map, map_opt->answer, G_mapset()) < 0)
            G_fatal_error(_("Unable to open vector map <%s>"), map_opt->answer);

        Vect_build(&Map);
    }
    /* dump topology */
    if (dump || sdump || cdump || fdump) {
        int is_json = (strcmp(format_opt->answer, "json") == 0);

        G_JSON_Value *root_value = NULL;
        G_JSON_Object *root_obj = NULL;

        if (!build) {
            Vect_set_open_level(2);

            if (Vect_open_old(&Map, map_opt->answer, "") < 0)
                G_fatal_error(_("Unable to open vector map <%s>"),
                              map_opt->answer);
        }
        if (is_json) {
            root_value = G_json_value_init_object();
            root_obj = G_json_object(root_value);
            G_json_object_set_string(root_obj, "module", "v.build");
            G_json_object_set_string(root_obj, "map", map_opt->answer);
        }
        if (dump) {
            if (is_json) {
                G_json_object_set_number(root_obj, "n_lines",
                                         Vect_get_num_lines(&Map));
            }
            else {
                Vect_topo_dump(&Map, stdout);
            }
        }
        if (sdump) {
            if (is_json) {
                G_JSON_Value *nodes_val;
                G_JSON_Array *nodes_array;
                int n_nodes, i;

                G_json_object_set_string(root_obj, "type",
                                         "spatial_index_dump");

                nodes_val = G_json_value_init_array();
                nodes_array = G_json_array(nodes_val);

                n_nodes = Vect_get_num_nodes(&Map);

                for (i = 1; i <= n_nodes; i++) {
                    G_JSON_Value *node_val = G_json_value_init_object();
                    G_JSON_Object *node_obj = G_json_object(node_val);

                    if (!node_val)
                        continue;

                    G_json_object_set_number(node_obj, "node_id", i);
                    G_json_object_set_number(node_obj, "n_lines",
                                             Vect_get_node_n_lines(&Map, i));

                    G_json_array_append_value(nodes_array, node_val);
                }
                G_json_object_set_value(root_obj, "nodes", nodes_val);
            }
            else {
                Vect_sidx_dump(&Map, stdout);
            }
        }
        if (cdump) {
            if (is_json) {
                G_json_object_set_string(root_obj, "action_c",
                                         "category_index_dump");
                G_JSON_Value *layers_val = G_json_value_init_array();
                G_JSON_Array *layers_array = G_json_array(layers_val);

                int n_layers = Vect_cidx_get_num_fields(&Map);
                for (int i = 0; i < n_layers; i++) {
                    G_JSON_Value *layer_val = G_json_value_init_object();
                    G_JSON_Object *layer_obj = G_json_object(layer_val);

                    int layer_num = Vect_cidx_get_field_number(&Map, i);
                    G_json_object_set_number(layer_obj, "layer", layer_num);
                    G_json_object_set_number(
                        layer_obj, "n_cats",
                        Vect_cidx_get_num_cats_by_index(&Map, i));

                    G_json_array_append_value(layers_array, layer_val);
                }
                G_json_object_set_value(root_obj, "categories", layers_val);
            }
            else {
                Vect_cidx_dump(&Map, stdout);
            }
        }

        if (fdump) {
            if (is_json) {
                G_json_object_set_string(root_obj, "action_f",
                                         "feature_index_dump");
                G_json_object_set_number(root_obj, "n_features",
                                         Vect_get_num_lines(&Map));
            }
            else {
                Vect_fidx_dump(&Map, stdout);
            }
        }
        if (root_value) {
            char *json_str = G_json_serialize_to_string_pretty(root_value);
            if (json_str) {
                fprintf(stdout, "%s\n", json_str);
                G_json_free_serialized_string(json_str);
            }
            G_json_value_free(root_value);
        }
    }

    if (err_opt->answer) {
        int nlines, line, type, area, left, right, err, narea;
        struct Map_info Err;
        struct line_pnts *Points;
        struct line_cats *Cats;

        Points = Vect_new_line_struct();
        Cats = Vect_new_cats_struct();

        if (Vect_open_new(&Err, err_opt->answer, Vect_is_3d(&Map)) < 0)
            G_fatal_error(_("Unable to create vector map <%s>"),
                          err_opt->answer);

        nlines = Vect_get_num_lines(&Map);

        for (line = 1; line <= nlines; line++) {
            err = 0;

            if (!Vect_line_alive(&Map, line))
                continue;

            type = Vect_read_line(&Map, Points, Cats, line);

            if (type == GV_BOUNDARY) {
                Vect_get_line_areas(&Map, line, &left, &right);
                if (left == 0 || right == 0)
                    err = 1;
            }
            else if (type == GV_CENTROID) {
                area = Vect_get_centroid_area(&Map, line);
                if (area <= 0)
                    err = 1;
            }

            if (err)
                Vect_write_line(&Err, type, Points, Cats);
        }

        narea = Vect_get_num_areas(&Map);

        for (area = 1; area <= narea; area++) {
            if (!Vect_area_alive(&Map, area))
                continue;

            if (Vect_get_area_centroid(&Map, area) == 0) {
                Vect_get_area_points(&Map, area, Points);
                Vect_reset_cats(Cats);
                Vect_write_line(&Err, GV_BOUNDARY, Points, Cats);
            }
        }

        if (chk->answer)
            Vect_topo_check(&Map, &Err);

        Vect_build(&Err);
        Vect_close(&Err);
    }
    else if (chk->answer)
        Vect_topo_check(&Map, NULL);

    if (build || dump || sdump || cdump) {
        Vect_close(&Map);
    }

    exit(EXIT_SUCCESS);
}
