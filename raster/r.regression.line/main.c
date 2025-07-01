/****************************************************************************
 *
 * MODULE:       r.regression.line
 *
 * AUTHOR(S):    Dr. Agustin Lobo
 *               Markus Metz (conversion to C for speed)
 *
 * PURPOSE:      Calculates linear regression from two raster maps:
 *               y = a + b*x
 *
 * COPYRIGHT:    (C) 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/glocale.h>
#include <grass/raster.h>

enum OutputFormat { PLAIN, SHELL, JSON };

int main(int argc, char *argv[])
{
    unsigned int r, c, rows, cols; /*  totals  */
    int map1_fd, map2_fd;
    double sumX, sumY, sumsqX, sumsqY, sumXY;
    double meanX, meanY, varX, varY, sdX, sdY;
    double A, B, R, F;
    long count = 0;
    DCELL *map1_buf, *map2_buf, map1_val, map2_val;
    char *name;
    struct Option *input_map1, *input_map2, *output_opt, *format_opt;
    struct Flag *shell_style;
    struct Cell_head region;
    struct GModule *module;
    enum OutputFormat format;
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("regression"));
    module->description =
        _("Calculates linear regression from two raster maps: y = a + b*x.");

    /* Define the different options */
    input_map1 = G_define_standard_option(G_OPT_R_MAP);
    input_map1->key = "mapx";
    input_map1->description = (_("Map for x coefficient"));

    input_map2 = G_define_standard_option(G_OPT_R_MAP);
    input_map2->key = "mapy";
    input_map2->description = (_("Map for y coefficient"));

    output_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    output_opt->key = "output";
    output_opt->required = NO;
    output_opt->description =
        (_("ASCII file for storing regression coefficients (output to screen "
           "if file not specified)."));

    format_opt = G_define_standard_option(G_OPT_F_FORMAT);
    format_opt->options = "plain,shell,json";
    format_opt->descriptions = ("plain;Human readable text output;"
                                "shell;shell script style text output;"
                                "json;JSON (JavaScript Object Notation);");

    shell_style = G_define_flag();
    shell_style->key = 'g';
    shell_style->label = _("Print in shell script style [deprecated]");
    shell_style->description = _(
        "This flag is deprecated and will be removed in a future release. Use "
        "format=shell instead.");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    name = output_opt->answer;
    if (name != NULL && strcmp(name, "-") != 0) {
        if (NULL == freopen(name, "w", stdout)) {
            G_fatal_error(_("Unable to open file <%s> for writing"), name);
        }
    }

    if (strcmp(format_opt->answer, "json") == 0) {
        format = JSON;
        root_value = G_json_value_init_object();
        if (root_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        root_object = G_json_object(root_value);
    }
    else if (strcmp(format_opt->answer, "shell") == 0) {
        format = SHELL;
    }
    else {
        format = PLAIN;
    }

    if (shell_style->answer) {
        G_verbose_message(
            _("Flag 'g' is deprecated and will be removed in a future "
              "release. Please use format=shell instead."));
        if (format == JSON) {
            G_fatal_error(_("The -g flag cannot be used with format=json. "
                            "Please select only one output format."));
        }
        format = SHELL;
    }

    G_get_window(&region);
    rows = region.rows;
    cols = region.cols;

    /* open maps */
    map1_fd = Rast_open_old(input_map1->answer, "");
    map2_fd = Rast_open_old(input_map2->answer, "");

    map1_buf = Rast_allocate_d_buf();
    map2_buf = Rast_allocate_d_buf();

    sumX = sumY = sumsqX = sumsqY = sumXY = 0.0;
    meanX = meanY = varX = varY = sdX = sdY = 0.0;
    for (r = 0; r < rows; r++) {
        G_percent(r, rows, 2);
        Rast_get_d_row(map1_fd, map1_buf, r);
        Rast_get_d_row(map2_fd, map2_buf, r);
        for (c = 0; c < cols; c++) {
            map1_val = map1_buf[c];
            map2_val = map2_buf[c];
            if (Rast_is_d_null_value(&map1_val) ||
                Rast_is_d_null_value(&map2_val))
                continue;

            sumX += map1_val;
            sumY += map2_val;
            sumsqX += map1_val * map1_val;
            sumsqY += map2_val * map2_val;
            sumXY += map1_val * map2_val;
            count++;
        }
    }
    Rast_close(map1_fd);
    Rast_close(map2_fd);
    G_free(map1_buf);
    G_free(map2_buf);

    B = (sumXY - sumX * sumY / count) / (sumsqX - sumX * sumX / count);
    R = (sumXY - sumX * sumY / count) /
        sqrt((sumsqX - sumX * sumX / count) * (sumsqY - sumY * sumY / count));

    meanX = sumX / count;
    sumsqX = sumsqX / count;
    varX = sumsqX - (meanX * meanX);
    sdX = sqrt(varX);

    meanY = sumY / count;
    sumsqY = sumsqY / count;
    varY = sumsqY - (meanY * meanY);
    sdY = sqrt(varY);

    A = meanY - B * meanX;
    F = R * R / ((1 - R * R) / (count - 2));

    switch (format) {
    case SHELL:
        fprintf(stdout, "a=%f\n", A);
        fprintf(stdout, "b=%f\n", B);
        fprintf(stdout, "R=%f\n", R);
        fprintf(stdout, "N=%ld\n", count);
        fprintf(stdout, "F=%f\n", F);
        fprintf(stdout, "meanX=%f\n", meanX);
        fprintf(stdout, "sdX=%f\n", sdX);
        fprintf(stdout, "meanY=%f\n", meanY);
        fprintf(stdout, "sdY=%f\n", sdY);
        break;

    case PLAIN:
        fprintf(stdout, "y = a + b*x\n");
        fprintf(stdout, "   a (Offset): %f\n", A);
        fprintf(stdout, "   b (Gain): %f\n", B);
        fprintf(stdout, "   R (sumXY - sumX*sumY/N): %f\n", R);
        fprintf(stdout, "   N (Number of elements): %ld\n", count);
        fprintf(stdout, "   F (F-test significance): %f\n", F);
        fprintf(stdout, "   meanX (Mean of map1): %f\n", meanX);
        fprintf(stdout, "   sdX (Standard deviation of map1): %f\n", sdX);
        fprintf(stdout, "   meanY (Mean of map2): %f\n", meanY);
        fprintf(stdout, "   sdY (Standard deviation of map2): %f\n", sdY);
        break;

    case JSON:
        if (isfinite(A)) {
            G_json_object_set_number(root_object, "a", A);
        }
        else {
            G_json_object_set_null(root_object, "a");
        }

        if (isfinite(B)) {
            G_json_object_set_number(root_object, "b", B);
        }
        else {
            G_json_object_set_null(root_object, "b");
        }

        if (isfinite(R)) {
            G_json_object_set_number(root_object, "R", R);
        }
        else {
            G_json_object_set_null(root_object, "R");
        }

        G_json_object_set_number(root_object, "N", count);

        if (isfinite(F)) {
            G_json_object_set_number(root_object, "F", F);
        }
        else {
            G_json_object_set_null(root_object, "F");
        }

        if (isfinite(meanX)) {
            G_json_object_set_number(root_object, "x_mean", meanX);
        }
        else {
            G_json_object_set_null(root_object, "x_mean");
        }

        if (isfinite(sdX)) {
            G_json_object_set_number(root_object, "x_stddev", sdX);
        }
        else {
            G_json_object_set_null(root_object, "x_stddev");
        }

        if (isfinite(meanY)) {
            G_json_object_set_number(root_object, "y_mean", meanY);
        }
        else {
            G_json_object_set_null(root_object, "y_mean");
        }

        if (isfinite(sdY)) {
            G_json_object_set_number(root_object, "y_stddev", sdY);
        }
        else {
            G_json_object_set_null(root_object, "y_stddev");
        }

        break;
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

    exit(EXIT_SUCCESS);
}
