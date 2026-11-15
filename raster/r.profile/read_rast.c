/*
 * SPDX-FileCopyrightText: 2000 GRASS Development Team
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Author: Bob Covill <bcovill@tekmap.ns.ca>
 *
 */

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/gjson.h>
#include "local_proto.h"

int read_rast(double east, double north, double dist,
              const struct profile_output *out)
{
    static DCELL *dcell;
    static int cur_row = -1;
    static CELL nullcell;
    static int nrows, ncols;
    static struct Cell_head window;
    int row, col;
    int outofbounds = FALSE;
    FILE *fp = out->fp;
    G_JSON_Object *object;
    G_JSON_Value *value;

    if (out->format == JSON) {
        value = G_json_value_init_object();
        object = G_json_object(value);
    }

    if (!dcell) {
        Rast_set_c_null_value(&nullcell, 1);
        dcell = Rast_allocate_d_buf();
        G_get_window(&window);
        nrows = window.rows;
        ncols = window.cols;
    }

    row = (window.north - north) / window.ns_res;
    col = (east - window.west) / window.ew_res;
    G_debug(4, "row=%d:%d  col=%d:%d", row, nrows, col, ncols);

    if ((row < 0) || (row >= nrows) || (col < 0) || (col >= ncols))
        outofbounds = TRUE;

    if (!outofbounds) {
        if (row != cur_row)
            Rast_get_d_row(out->fd, dcell, row);
        cur_row = row;
    }

    if (out->with_cat) {
        switch (out->format) {
        case JSON:
            if (out->cat < 0)
                G_json_object_set_null(object, "cat");
            else
                G_json_object_set_number(object, "cat", out->cat);
            break;
        case PLAIN:
            if (out->cat < 0)
                fprintf(fp, "%s", out->null_string);
            else
                fprintf(fp, "%d", out->cat);
            break;
        case CSV:
            if (out->cat < 0)
                fprintf(fp, "%s%s", out->null_string, fs);
            else
                fprintf(fp, "%d%s", out->cat, fs);
            break;
        }
    }

    switch (out->format) {
    case JSON:
        if (out->coords) {
            G_json_object_set_number(object, "easting", east);
            G_json_object_set_number(object, "northing", north);
        }
        G_json_object_set_number(object, "distance", dist);
        break;
    case PLAIN:
        /* Rows without category start with easting or with a space */
        if (out->coords)
            fprintf(fp, "%s%f %f", out->with_cat ? " " : "", east, north);

        fprintf(fp, " %f", dist);
        break;
    case CSV:
        if (out->coords)
            fprintf(fp, "%f%s%f%s", east, fs, north, fs);

        fprintf(fp, "%f", dist);
        break;
    }

    if (outofbounds || Rast_is_d_null_value(&dcell[col])) {
        switch (out->format) {
        case JSON:
            G_json_object_set_null(object, "value");
            break;
        case PLAIN:
            fprintf(fp, " %s", out->null_string);
            break;
        case CSV:
            fprintf(fp, "%s%s", fs, out->null_string);
            break;
        }
    }
    else {
        if (out->data_type == CELL_TYPE) {
            int dvalue = (int)dcell[col];
            switch (out->format) {
            case JSON:
                G_json_object_set_number(object, "value", dvalue);
                break;
            case PLAIN:
                fprintf(fp, " %d", dvalue);
                break;
            case CSV:
                fprintf(fp, "%s%d", fs, dvalue);
                break;
            }
        }
        else {
            switch (out->format) {
            case JSON:
                G_json_object_set_number(object, "value", dcell[col]);
                break;
            case PLAIN:
                fprintf(fp, " %f", dcell[col]);
                break;
            case CSV:
                fprintf(fp, "%s%f", fs, dcell[col]);
                break;
            }
        }
    }

    if (clr) {
        int red, green, blue;

        if (outofbounds)
            Rast_get_c_color(&nullcell, &red, &green, &blue, &colors);
        else
            Rast_get_d_color(&dcell[col], &red, &green, &blue, &colors);

        char color_str[COLOR_STRING_LENGTH];

        switch (out->format) {
        case JSON:
            G_color_to_str(red, green, blue, out->clr_frmt, color_str);
            G_json_object_set_string(object, "color", color_str);
            break;
        case PLAIN:
            if (out->clr_frmt != TRIPLET) {
                G_color_to_str(red, green, blue, out->clr_frmt, color_str);
                fprintf(fp, " %s", color_str);
            }
            else {
                /* For backward compatibility */
                fprintf(fp, " %03d:%03d:%03d", red, green, blue);
            }
            break;
        case CSV:
            G_color_to_str(red, green, blue, out->clr_frmt, color_str);
            if (out->clr_frmt != HEX)
                fprintf(fp, "%s\"%s\"", fs, color_str);
            else
                fprintf(fp, "%s%s", fs, color_str);
            break;
        }
    }

    switch (out->format) {
    case JSON:
        G_json_array_append_value(out->array, value);
        break;
    case PLAIN:
    case CSV:
        fprintf(fp, "\n");
        break;
    }

    return 0;
}
