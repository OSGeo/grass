/*
 * Copyright (C) 2000 by the GRASS Development Team
 * Author: Bob Covill <bcovill@tekmap.ns.ca>
 *
 * This Program is free software under the GPL (>=v2)
 * Read the file COPYING coming with GRASS for details
 *
 */

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/gjson.h>
#include "local_proto.h"

int read_rast(double east, double north, double dist, int fd, int coords,
              RASTER_MAP_TYPE data_type, FILE *fp, char *null_string,
              enum OutputFormat format, G_JSON_Array *array,
              ColorFormat clr_frmt)
{
    static DCELL *dcell;
    static int cur_row = -1;
    static CELL nullcell;
    static int nrows, ncols;
    static struct Cell_head window;
    int row, col;
    int outofbounds = FALSE;
    G_JSON_Object *object;
    G_JSON_Value *value;

    if (format == JSON) {
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
            Rast_get_d_row(fd, dcell, row);
        cur_row = row;
    }

    switch (format) {
    case JSON:
        if (coords) {
            G_json_object_set_number(object, "easting", east);
            G_json_object_set_number(object, "northing", north);
        }
        G_json_object_set_number(object, "distance", dist);
        break;
    case PLAIN:
        if (coords)
            fprintf(fp, "%f %f", east, north);

        fprintf(fp, " %f", dist);
        break;
    case CSV:
        if (coords)
            fprintf(fp, "%f%s%f%s", east, fs, north, fs);

        fprintf(fp, "%f", dist);
        break;
    }

    if (outofbounds || Rast_is_d_null_value(&dcell[col])) {
        switch (format) {
        case JSON:
            G_json_object_set_null(object, "value");
            break;
        case PLAIN:
            fprintf(fp, " %s", null_string);
            break;
        case CSV:
            fprintf(fp, "%s%s", fs, null_string);
            break;
        }
    }
    else {
        if (data_type == CELL_TYPE) {
            int dvalue = (int)dcell[col];
            switch (format) {
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
            switch (format) {
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

        switch (format) {
        case JSON:
            G_color_to_str(red, green, blue, clr_frmt, color_str);
            G_json_object_set_string(object, "color", color_str);
            break;
        case PLAIN:
            if (clr_frmt != TRIPLET) {
                G_color_to_str(red, green, blue, clr_frmt, color_str);
                fprintf(fp, " %s", color_str);
            }
            else {
                /* For backward compatibility */
                fprintf(fp, " %03d:%03d:%03d", red, green, blue);
            }
            break;
        case CSV:
            G_color_to_str(red, green, blue, clr_frmt, color_str);
            if (clr_frmt != HEX)
                fprintf(fp, "%s\"%s\"", fs, color_str);
            else
                fprintf(fp, "%s%s", fs, color_str);
            break;
        }
    }

    switch (format) {
    case JSON:
        G_json_array_append_value(array, value);
        break;
    case PLAIN:
    case CSV:
        fprintf(fp, "\n");
        break;
    }

    return 0;
}
