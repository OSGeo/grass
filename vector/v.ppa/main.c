/****************************************************************************
 *
 * MODULE:       v.ppa
 * AUTHOR(S):    Corey T. White <corey.white openplains com>
 * PURPOSE:      Point pattern analysis with the G, F, K, and L summary
 *               functions
 * COPYRIGHT:    (C) 2024-2026 by Corey T. White and the GRASS Development
 *               Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/glocale.h>
#include <grass/vector.h>

#include "local_proto.h"

enum output_format {
    FORMAT_PLAIN,
    FORMAT_CSV,
    FORMAT_JSON,
};

static void write_table(FILE *fp, const char *method,
                        const struct ppa_result *result, char separator);
static void write_json(FILE *fp, const char *method,
                       const struct ppa_result *result, int n, double intensity,
                       const struct ppa_window *window,
                       const char *correction_name);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input_opt, *output_opt, *method_opt, *format_opt,
        *num_distances_opt, *random_points_opt, *max_distance_opt,
        *correction_opt, *seed_opt, *nprocs_opt;
    struct Map_info map;
    struct line_pnts *line_points;
    struct line_cats *line_cats;
    struct Cell_head region;
    struct ppa_window window;
    struct ppa_point *points;
    struct ppa_result *result = NULL;
    struct kdtree *tree;
    enum output_format format;
    enum ppa_correction correction;
    const char *method, *correction_name = NULL;
    FILE *fp = stdout;
    double area, intensity, max_distance, shorter_side;
    long seed_value;
    int num_distances, num_random_points;
    int line_type, n, n_alloc, n_outside;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("point pattern analysis"));
    G_add_keyword(_("parallel"));
    module->description =
        _("Point pattern analysis using the G, F, K, and L summary "
          "functions.");

    input_opt = G_define_standard_option(G_OPT_V_INPUT);

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = YES;
    method_opt->options = "g,f,k,l";
    method_opt->label = _("Summary function to compute");
    method_opt->descriptions =
        _("g;Nearest neighbor distance distribution function;"
          "f;Empty space function;"
          "k;Ripley's K function;"
          "l;L function (variance stabilized K function)");

    output_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    output_opt->required = NO;
    output_opt->label = _("Name for output file");
    output_opt->description =
        _("If omitted or '-', the results are printed to standard output");

    format_opt = G_define_standard_option(G_OPT_F_FORMAT);
    format_opt->options = "plain,csv,json";
    format_opt->descriptions = _("plain;Human readable text output;"
                                 "csv;CSV (Comma Separated Values);"
                                 "json;JSON (JavaScript Object Notation);");

    num_distances_opt = G_define_option();
    num_distances_opt->key = "num_distances";
    num_distances_opt->type = TYPE_INTEGER;
    num_distances_opt->required = NO;
    num_distances_opt->answer = "100";
    num_distances_opt->label = _("Number of distances");
    num_distances_opt->description =
        _("Number of equally spaced distances at which the function is "
          "estimated");

    random_points_opt = G_define_option();
    random_points_opt->key = "random_points";
    random_points_opt->type = TYPE_INTEGER;
    random_points_opt->required = NO;
    random_points_opt->answer = "1000";
    random_points_opt->label = _("Number of random points for the F function");
    random_points_opt->description =
        _("Empty space distances are sampled at this many uniformly "
          "random locations");

    max_distance_opt = G_define_option();
    max_distance_opt->key = "max_distance";
    max_distance_opt->type = TYPE_DOUBLE;
    max_distance_opt->required = NO;
    max_distance_opt->label = _("Maximum distance for the K and L functions");
    max_distance_opt->description =
        _("Default is one quarter of the shorter side of the computational "
          "region");

    correction_opt = G_define_option();
    correction_opt->key = "correction";
    correction_opt->type = TYPE_STRING;
    correction_opt->required = NO;
    correction_opt->options = "isotropic,none";
    correction_opt->answer = "isotropic";
    correction_opt->label = _("Edge correction for the K and L functions");
    correction_opt->descriptions =
        _("isotropic;Ripley's isotropic edge correction;"
          "none;No edge correction");

    seed_opt = G_define_standard_option(G_OPT_M_SEED);
    seed_opt->description =
        _("Used for the random locations of the F function");

    nprocs_opt = G_define_standard_option(G_OPT_M_NPROCS);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    G_set_omp_num_threads(nprocs_opt);

    method = method_opt->answer;

    if (strcmp(format_opt->answer, "json") == 0)
        format = FORMAT_JSON;
    else if (strcmp(format_opt->answer, "csv") == 0)
        format = FORMAT_CSV;
    else
        format = FORMAT_PLAIN;

    correction = strcmp(correction_opt->answer, "isotropic") == 0
                     ? PPA_CORRECTION_ISOTROPIC
                     : PPA_CORRECTION_NONE;

    num_distances = atoi(num_distances_opt->answer);
    if (num_distances < 1)
        G_fatal_error(_("Option <%s> must be positive"),
                      num_distances_opt->key);
    num_random_points = atoi(random_points_opt->answer);
    if (num_random_points < 1)
        G_fatal_error(_("Option <%s> must be positive"),
                      random_points_opt->key);

    if (seed_opt->answer) {
        seed_value = atol(seed_opt->answer);
        G_srand48(seed_value);
        G_verbose_message(_("Read random seed from %s option: %ld"),
                          seed_opt->key, seed_value);
    }
    else {
        seed_value = G_srand48_auto();
        G_verbose_message(_("Autogenerated random seed set to: %ld"),
                          seed_value);
    }

    /* The computational region is the observation window: it defines
     * the area for the intensity estimate, the sampling window of the
     * F function, and the edge correction geometry. */
    G_get_window(&region);
    window.west = region.west;
    window.east = region.east;
    window.south = region.south;
    window.north = region.north;
    area = (window.east - window.west) * (window.north - window.south);
    shorter_side = fmin(window.east - window.west, window.north - window.south);

    max_distance = 0.25 * shorter_side;
    if (max_distance_opt->answer) {
        max_distance = atof(max_distance_opt->answer);
        if (max_distance <= 0)
            G_fatal_error(_("Option <%s> must be positive"),
                          max_distance_opt->key);
        if (max_distance > 0.5 * shorter_side)
            G_warning(_("Option <%s> is larger than half of the shorter "
                        "side of the computational region; the estimate "
                        "becomes unreliable at large distances"),
                      max_distance_opt->key);
    }

    if (Vect_open_old(&map, input_opt->answer, "") < 0)
        G_fatal_error(_("Unable to open vector map <%s>"), input_opt->answer);

    line_points = Vect_new_line_struct();
    line_cats = Vect_new_cats_struct();
    tree = kdtree_create(2, NULL);

    n = 0;
    n_outside = 0;
    n_alloc = 1024;
    points = G_malloc(n_alloc * sizeof(struct ppa_point));

    while ((line_type = Vect_read_next_line(&map, line_points, line_cats)) >
           0) {
        double x, y, coords[2];

        if (line_type != GV_POINT)
            continue;
        x = line_points->x[0];
        y = line_points->y[0];
        if (x < window.west || x > window.east || y < window.south ||
            y > window.north) {
            n_outside++;
            continue;
        }
        if (n == n_alloc) {
            n_alloc *= 2;
            points = G_realloc(points, n_alloc * sizeof(struct ppa_point));
        }
        points[n].x = x;
        points[n].y = y;
        coords[0] = x;
        coords[1] = y;
        kdtree_insert(tree, coords, n, 1);
        n++;
    }
    if (line_type == -1)
        G_fatal_error(_("Unable to read vector map <%s>"), input_opt->answer);
    Vect_destroy_line_struct(line_points);
    Vect_destroy_cats_struct(line_cats);
    Vect_close(&map);

    if (n_outside > 0)
        G_warning(n_("One point outside of the computational region was "
                     "ignored",
                     "%d points outside of the computational region were "
                     "ignored",
                     n_outside),
                  n_outside);
    if (n == 0)
        G_fatal_error(_("No points found in the computational region"));
    if (n < 2 && strcmp(method, "f") != 0)
        G_fatal_error(_("At least 2 points are required, %d found"), n);

    intensity = n / area;
    G_verbose_message(_("Number of points: %d"), n);
    G_verbose_message(_("Estimated intensity: %g"), intensity);

    if (strcmp(method, "g") == 0) {
        result = g_function(tree, points, n, intensity, num_distances);
    }
    else if (strcmp(method, "f") == 0) {
        result = f_function(tree, num_random_points, &window, intensity,
                            num_distances);
    }
    else if (strcmp(method, "k") == 0) {
        result = k_function(tree, points, n, &window, max_distance,
                            num_distances, correction);
        correction_name = correction_opt->answer;
    }
    else {
        result = l_function(tree, points, n, &window, max_distance,
                            num_distances, correction);
        correction_name = correction_opt->answer;
    }

    kdtree_destroy(tree);
    G_free(points);

    if (output_opt->answer && strcmp(output_opt->answer, "-") != 0) {
        fp = fopen(output_opt->answer, "w");
        if (fp == NULL)
            G_fatal_error(_("Unable to create output file <%s>"),
                          output_opt->answer);
    }

    switch (format) {
    case FORMAT_PLAIN:
        write_table(fp, method, result, ' ');
        break;
    case FORMAT_CSV:
        write_table(fp, method, result, ',');
        break;
    case FORMAT_JSON:
        write_json(fp, method, result, n, intensity, &window, correction_name);
        break;
    }

    if (fp != stdout)
        fclose(fp);
    ppa_result_free(result);

    exit(EXIT_SUCCESS);
}

static void write_table(FILE *fp, const char *method,
                        const struct ppa_result *result, char separator)
{
    fprintf(fp, "distance%c%s%c%s_csr\n", separator, method, separator, method);
    for (int i = 0; i < result->num_distances; i++)
        fprintf(fp, "%.10g%c%.10g%c%.10g\n", result->distance[i], separator,
                result->value[i], separator, result->csr[i]);
    fflush(fp);
}

static void write_json(FILE *fp, const char *method,
                       const struct ppa_result *result, int n, double intensity,
                       const struct ppa_window *window,
                       const char *correction_name)
{
    G_JSON_Value *root_value, *window_value, *results_value;
    G_JSON_Object *root_object, *window_object;
    G_JSON_Array *results_array;
    char csr_key[16];
    char *serialized;

    root_value = G_json_value_init_object();
    window_value = G_json_value_init_object();
    results_value = G_json_value_init_array();
    if (root_value == NULL || window_value == NULL || results_value == NULL)
        G_fatal_error(_("Unable to initialize JSON output"));
    root_object = G_json_object(root_value);
    window_object = G_json_object(window_value);
    results_array = G_json_array(results_value);

    G_json_object_set_string(root_object, "method", method);
    G_json_object_set_number(root_object, "points", n);
    G_json_object_set_number(root_object, "intensity", intensity);
    if (correction_name)
        G_json_object_set_string(root_object, "correction", correction_name);

    G_json_object_set_number(window_object, "west", window->west);
    G_json_object_set_number(window_object, "east", window->east);
    G_json_object_set_number(window_object, "south", window->south);
    G_json_object_set_number(window_object, "north", window->north);
    G_json_object_set_value(root_object, "window", window_value);

    snprintf(csr_key, sizeof(csr_key), "%s_csr", method);
    for (int i = 0; i < result->num_distances; i++) {
        G_JSON_Value *row_value = G_json_value_init_object();
        G_JSON_Object *row_object;

        if (row_value == NULL)
            G_fatal_error(_("Unable to initialize JSON output"));
        row_object = G_json_object(row_value);
        G_json_object_set_number(row_object, "distance", result->distance[i]);
        G_json_object_set_number(row_object, method, result->value[i]);
        G_json_object_set_number(row_object, csr_key, result->csr[i]);
        G_json_array_append_value(results_array, row_value);
    }
    G_json_object_set_value(root_object, "results", results_value);

    serialized = G_json_serialize_to_string_pretty(root_value);
    if (serialized == NULL)
        G_fatal_error(_("Unable to serialize JSON output"));
    fputs(serialized, fp);
    fputc('\n', fp);
    fflush(fp);

    G_json_free_serialized_string(serialized);
    G_json_value_free(root_value);
}
