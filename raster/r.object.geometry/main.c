/****************************************************************************
 *
 * MODULE:       r.object.geometry
 *
 * AUTHOR(S):    Moritz Lennert
 *               Markus Metz
 *
 * PURPOSE:      Fetch object geometry parameters.
 *
 * COPYRIGHT:    (C) 2016-2021 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/gjson.h>

enum OutputFormat { PLAIN, JSON };

/* compare two cell values
 * return 0 if equal, 1 if different */
static int cmp_cells(CELL a, CELL b, int a_null, int b_null)
{
    return (a_null + b_null == 1 || (a_null + b_null == 0 && a != b));
}

int main(int argc, char *argv[])
{
    int row, col, nrows, ncols;

    struct Range range;
    CELL min, max;
    int in_fd;
    int i;
    struct GModule *module;
    struct Option *opt_in;
    struct Option *opt_out;
    struct Option *opt_sep;
    struct Option *fmt_opt;
    struct Flag *flag_m;
    char *sep;
    FILE *out_fp;
    CELL *prev_in, *cur_in, *temp_in;
    CELL cur, top, left;
    int cur_null, top_null, left_null;
    int len;
    struct obj_geo {
        double area, perimeter, x, y;
        int min_row, max_row, min_col, max_col; /* bounding box */
        int num;
    } *obj_geos;
    double unit_area;
    int n_objects;
    int planimetric = 0, compute_areas = 0;
    struct Cell_head cellhd;

    enum OutputFormat format;
    G_JSON_Array *root_array;
    G_JSON_Object *object;
    G_JSON_Value *root_value, *object_value;

    G_gisinit(argv[0]);

    /* Define the different options */

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("reclass"));
    G_add_keyword(_("clumps"));
    module->description =
        _("Calculates geometry parameters for raster objects.");

    opt_in = G_define_standard_option(G_OPT_R_INPUT);

    opt_out = G_define_standard_option(G_OPT_F_OUTPUT);
    opt_out->required = NO;

    opt_sep = G_define_standard_option(G_OPT_F_SEP);

    flag_m = G_define_flag();
    flag_m->key = 'm';
    flag_m->label = _("Use meters as units instead of cells");

    fmt_opt = G_define_standard_option(G_OPT_F_FORMAT);
    fmt_opt->guisection = _("Print");

    /* parse options */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (strcmp(fmt_opt->answer, "json") == 0) {
        format = JSON;
        root_value = G_json_value_init_array();
        root_array = G_json_array(root_value);
    }
    else {
        format = PLAIN;
    }

    sep = G_option_to_separator(opt_sep);
    in_fd = Rast_open_old(opt_in->answer, "");

    if (Rast_get_map_type(in_fd) != CELL_TYPE)
        G_fatal_error(_("Input raster must be of type CELL"));

    if (opt_out->answer != NULL && strcmp(opt_out->answer, "-") != 0) {
        if (!(out_fp = fopen(opt_out->answer, "w")))
            G_fatal_error(_("Unable to open file <%s> for writing"),
                          opt_out->answer);
    }
    else {
        out_fp = stdout;
    }

    Rast_get_window(&cellhd);
    nrows = cellhd.rows;
    ncols = cellhd.cols;

    /* allocate CELL buffers two columns larger than current window */
    len = (ncols + 2) * sizeof(CELL);
    prev_in = (CELL *)G_malloc(len);
    cur_in = (CELL *)G_malloc(len);

    /* fake a previous row which is all NULL */
    Rast_set_c_null_value(prev_in, ncols + 2);

    /* set left and right edge to NULL */
    Rast_set_c_null_value(&cur_in[0], 1);
    Rast_set_c_null_value(&cur_in[ncols + 1], 1);

    Rast_read_range(opt_in->answer, "", &range);
    Rast_get_range_min_max(&range, &min, &max);
    if (Rast_is_c_null_value(&min) || Rast_is_c_null_value(&max))
        G_fatal_error(_("Empty input map <%s>"), opt_in->answer);

    /* REMARK: The following is only true if object ids are continuously
     * numbered */
    n_objects = max - min + 1;
    obj_geos = G_malloc(n_objects * sizeof(struct obj_geo));

    for (i = 0; i < n_objects; i++) {
        obj_geos[i].area = 0;
        obj_geos[i].perimeter = 0;
        obj_geos[i].min_row = nrows;
        obj_geos[i].max_row = 0;
        obj_geos[i].min_col = ncols;
        obj_geos[i].max_col = 0;
        obj_geos[i].x = 0;
        obj_geos[i].y = 0;
        obj_geos[i].num = 0;
    }

    unit_area = 0.0;
    if (flag_m->answer) {
        switch (G_begin_cell_area_calculations()) {
        case 0: /* areas don't make sense, but ignore this for now */
        case 1:
            planimetric = 1;
            unit_area = G_area_of_cell_at_row(0);
            break;
        default:
            planimetric = 0;
            break;
        }
    }
    compute_areas = flag_m->answer && !planimetric;
    G_begin_distance_calculations();

    G_message(_("Calculating statistics"));
    for (row = 0; row < nrows; row++) {
        G_percent(row, nrows + 1, 2);

        Rast_get_c_row(in_fd, cur_in + 1, row);
        cur = cur_in[0];
        cur_null = 1;
        if (compute_areas)
            unit_area = G_area_of_cell_at_row(row);
        for (col = 1; col <= ncols; col++) {
            left = cur;
            cur = cur_in[col];
            top = prev_in[col];

            left_null = cur_null;
            cur_null = Rast_is_c_null_value(&cur);
            top_null = Rast_is_c_null_value(&top);

            if (!cur_null) {
                if (flag_m->answer) {
                    obj_geos[cur - min].area += unit_area;
                    obj_geos[cur - min].num += 1;
                }
                else {
                    obj_geos[cur - min].area += 1;
                }
                obj_geos[cur - min].x +=
                    Rast_col_to_easting(col - 0.5, &cellhd);
                obj_geos[cur - min].y +=
                    Rast_row_to_northing(row + 0.5, &cellhd);
                if (obj_geos[cur - min].min_row > row)
                    obj_geos[cur - min].min_row = row;
                if (obj_geos[cur - min].max_row < row + 1)
                    obj_geos[cur - min].max_row = row + 1;
                if (obj_geos[cur - min].min_col > col)
                    obj_geos[cur - min].min_col = col;
                if (obj_geos[cur - min].max_col < col + 1)
                    obj_geos[cur - min].max_col = col + 1;
            }

            if (cmp_cells(cur, top, cur_null, top_null)) {
                if (flag_m->answer) {
                    double perimeter;

                    /* could be optimized */
                    perimeter =
                        G_distance(cellhd.west + col * cellhd.ew_res,
                                   Rast_row_to_northing(row, &cellhd),
                                   cellhd.west + (col + 1) * cellhd.ew_res,
                                   Rast_row_to_northing(row, &cellhd));

                    if (!cur_null)
                        obj_geos[cur - min].perimeter += perimeter;
                    if (!top_null)
                        obj_geos[top - min].perimeter += perimeter;
                }
                else {
                    if (!cur_null)
                        obj_geos[cur - min].perimeter += 1;
                    if (!top_null)
                        obj_geos[top - min].perimeter += 1;
                }
            }
            if (cmp_cells(cur, left, cur_null, left_null)) {
                if (flag_m->answer) {
                    double perimeter;

                    /* could be optimized */
                    perimeter =
                        G_distance(cellhd.west + col * cellhd.ew_res,
                                   Rast_row_to_northing(row, &cellhd),
                                   cellhd.west + (col)*cellhd.ew_res,
                                   Rast_row_to_northing(row + 1, &cellhd));

                    if (!cur_null)
                        obj_geos[cur - min].perimeter += perimeter;
                    if (!left_null)
                        obj_geos[left - min].perimeter += perimeter;
                }
                else {
                    if (!cur_null)
                        obj_geos[cur - min].perimeter += 1;
                    if (!left_null)
                        obj_geos[left - min].perimeter += 1;
                }
            }
        }

        /* last col, right borders */
        if (flag_m->answer) {
            double perimeter;

            perimeter =
                G_distance(cellhd.east, Rast_row_to_northing(row, &cellhd),
                           cellhd.east, Rast_row_to_northing(row + 1, &cellhd));
            if (!cur_null)
                obj_geos[cur - min].perimeter += perimeter;
        }
        else {
            if (!cur_null)
                obj_geos[cur - min].perimeter += 1;
        }

        /* switch the buffers so that the current buffer becomes the previous */
        temp_in = cur_in;
        cur_in = prev_in;
        prev_in = temp_in;
    }

    /* last row, bottom borders */
    G_percent(row, nrows + 1, 2);
    for (col = 1; col <= ncols; col++) {
        top = prev_in[col];
        top_null = Rast_is_c_null_value(&top);

        if (flag_m->answer) {
            double perimeter;

            /* could be optimized */
            perimeter = G_distance(cellhd.west + col * cellhd.ew_res,
                                   Rast_row_to_northing(row, &cellhd),
                                   cellhd.west + (col + 1) * cellhd.ew_res,
                                   Rast_row_to_northing(row, &cellhd));

            if (!top_null)
                obj_geos[top - min].perimeter += perimeter;
        }
        else {
            if (!top_null)
                obj_geos[top - min].perimeter += 1;
        }
    }
    G_percent(1, 1, 1);

    Rast_close(in_fd);
    G_free(cur_in);
    G_free(prev_in);

    G_message(_("Writing output"));
    if (format == PLAIN) {
        /* print table */
        fprintf(out_fp, "cat%s", sep);
        fprintf(out_fp, "area%s", sep);
        fprintf(out_fp, "perimeter%s", sep);
        fprintf(out_fp, "compact_square%s", sep);
        fprintf(out_fp, "compact_circle%s", sep);
        fprintf(out_fp, "fd%s", sep);
        fprintf(out_fp, "mean_x%s", sep);
        fprintf(out_fp, "mean_y");
        fprintf(out_fp, "\n");
    }

    /* print table body */
    for (i = 0; i < n_objects; i++) {
        G_percent(i, n_objects - 1, 1);
        /* skip empty objects */
        if (obj_geos[i].area == 0)
            continue;

        double compact_square =
            4 * sqrt(obj_geos[i].area) / obj_geos[i].perimeter;
        double compact_circle =
            obj_geos[i].perimeter / (2 * sqrt(M_PI * obj_geos[i].area));
        /* log 1 = 0, so avoid that by always adding 0.001 to the area: */
        double fd =
            2 * log(obj_geos[i].perimeter) / log(obj_geos[i].area + 0.001);
        if (!flag_m->answer) {
            obj_geos[i].num = obj_geos[i].area;
        }
        double mean_x = obj_geos[i].x / obj_geos[i].num;
        double mean_y = obj_geos[i].y / obj_geos[i].num;
        switch (format) {
        case PLAIN:
            fprintf(out_fp, "%d%s", min + i, sep);
            fprintf(out_fp, "%f%s", obj_geos[i].area, sep);
            fprintf(out_fp, "%f%s", obj_geos[i].perimeter, sep);
            fprintf(out_fp, "%f%s", compact_square, sep);
            fprintf(out_fp, "%f%s", compact_circle, sep);
            fprintf(out_fp, "%f%s", fd, sep);
            fprintf(out_fp, "%f%s", mean_x, sep);
            fprintf(out_fp, "%f", mean_y);
            break;
        case JSON:
            object_value = G_json_value_init_object();
            object = G_json_object(object_value);
            G_json_object_set_number(object, "category", min + i);
            G_json_object_set_number(object, "area", obj_geos[i].area);
            G_json_object_set_number(object, "perimeter",
                                     obj_geos[i].perimeter);
            G_json_object_set_number(object, "compact_square", compact_square);
            G_json_object_set_number(object, "compact_circle", compact_circle);
            G_json_object_set_number(object, "fd", fd);
            G_json_object_set_number(object, "mean_x", mean_x);
            G_json_object_set_number(object, "mean_y", mean_y);
            break;
        }
        /* object id: i + min */

        /* TODO */
        /* smoothness */
        /* perimeter of bounding box / perimeter -> smoother objects have a
         * higher smoothness value smoothness is in the range 0 < smoothness <=
         * 1 */

        /* bounding box perimeter */

        /* bounding box size */

        /* variance of X and Y to approximate bounding ellipsoid */

        switch (format) {
        case PLAIN:
            fprintf(out_fp, "\n");
            break;
        case JSON:
            G_json_array_append_value(root_array, object_value);
            break;
        }
    }

    if (format == JSON) {
        char *serialized_string = G_json_serialize_to_string_pretty(root_value);
        if (serialized_string == NULL) {
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        G_json_free_serialized_string(serialized_string);
        G_json_value_free(root_value);
    }

    if (out_fp != stdout)
        fclose(out_fp);

    exit(EXIT_SUCCESS);
}
