/****************************************************************************
 *
 * MODULE:       r.coin
 *
 * AUTHOR(S):    Sumit Chintanwar
 *
 * PURPOSE:      Json Output for r.coin
 *
 * COPYRIGHT:    (C) 2026 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include "coin.h"
#include <grass/gjson.h>
#include <grass/gis.h>
#include <time.h>

#define F_CTOK(C)    ((double)(C)) / 1000000.0
#define F_CTOM(C)    F_CTOK(C) * 0.386102158542446
#define F_CTOA(C)    F_CTOK(C) * 247.105381467165
#define F_CTOH(C)    F_CTOK(C) * 100.0000
#define F_CTOP(C, R) ((int)R) ? (double)C / (double)R * 100.0 : 0.0

static const char *get_unit_name(char unit)
{
    switch (unit) {
    case 'c':
        return "cells";
    case 'p':
        return "percent";
    case 'x':
        return "percent_of_column";
    case 'y':
        return "percent_of_row";
    case 'a':
        return "acres";
    case 'h':
        return "hectares";
    case 'k':
        return "square_kilometers";
    case 'm':
        return "square_miles";
    default:
        return "unknown";
    }
}

static double cell_value(int r, int c, char unit)
{
    int idx = r * ncat1 + c;
    long total_count;
    double total_area;

    switch (unit) {
    case 'c':
        return (double)table[idx].count;
    case 'p':
        return F_CTOP(table[idx].area, window_area);
    case 'x':
        col_total(c, 1, &total_count, &total_area);
        return F_CTOP(table[idx].area, total_area);
    case 'y':
        row_total(r, 1, &total_count, &total_area);
        return F_CTOP(table[idx].area, total_area);
    case 'a':
        return F_CTOA(table[idx].area);
    case 'h':
        return F_CTOH(table[idx].area);
    case 'k':
        return F_CTOK(table[idx].area);
    case 'm':
        return F_CTOM(table[idx].area);
    default:
        return (double)table[idx].count;
    }
}

void print_json(char unit)
{
    int r, c;
    char *json_string;
    char timestamp[64];
    time_t now;
    struct tm *tm_info;
    struct Cell_head window;

    time(&now);
    tm_info = gmtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_info);

    G_get_window(&window);

    G_JSON_Value *root_value = G_json_value_init_object();
    G_JSON_Object *root = G_json_object(root_value);

    G_json_object_set_string(root, "project", G_location());
    G_json_object_set_string(root, "created", timestamp);

    G_JSON_Value *region_value = G_json_value_init_object();
    G_JSON_Object *region = G_json_object(region_value);
    G_json_object_set_number(region, "north", window.north);
    G_json_object_set_number(region, "south", window.south);
    G_json_object_set_number(region, "east", window.east);
    G_json_object_set_number(region, "west", window.west);
    G_json_object_set_number(region, "ewres", window.ew_res);
    G_json_object_set_number(region, "nsres", window.ns_res);
    G_json_object_set_value(root, "region", region_value);

    if (G_find_raster("MASK", G_mapset()) != NULL)
        G_json_object_set_string(root, "mask", "MASK");
    else
        G_json_object_set_null(root, "mask");

    G_JSON_Value *maps_value = G_json_value_init_array();
    G_JSON_Array *maps_array = G_json_array(maps_value);

    {
        G_JSON_Value *m = G_json_value_init_object();
        G_JSON_Object *o = G_json_object(m);
        G_json_object_set_string(o, "name", map1name);
        G_json_object_set_string(o, "title", title1 ? title1 : "");
        G_json_object_set_string(o, "type", "raster");
        G_json_array_append_value(maps_array, m);
    }
    {
        G_JSON_Value *m = G_json_value_init_object();
        G_JSON_Object *o = G_json_object(m);
        G_json_object_set_string(o, "name", map2name);
        G_json_object_set_string(o, "title", title2 ? title2 : "");
        G_json_object_set_string(o, "type", "raster");
        G_json_array_append_value(maps_array, m);
    }
    G_json_object_set_value(root, "maps", maps_value);

    G_json_object_set_string(root, "unit", get_unit_name(unit));

    G_JSON_Value *row_cats_value = G_json_value_init_array();
    G_JSON_Array *row_cats = G_json_array(row_cats_value);
    for (r = 0; r < ncat2; r++)
        G_json_array_append_number(row_cats, (double)catlist2[r]);
    G_json_object_set_value(root, "row_cats", row_cats_value);

    G_JSON_Value *col_cats_value = G_json_value_init_array();
    G_JSON_Array *col_cats_arr = G_json_array(col_cats_value);
    for (c = 0; c < ncat1; c++)
        G_json_array_append_number(col_cats_arr, (double)catlist1[c]);
    G_json_object_set_value(root, "col_cats", col_cats_value);

    G_JSON_Value *matrix_value = G_json_value_init_array();
    G_JSON_Array *matrix = G_json_array(matrix_value);

    double *row_total_arr = G_calloc(ncat2, sizeof(double));
    double *row_total_nz_arr = G_calloc(ncat2, sizeof(double));
    double *col_total_arr = G_calloc(ncat1, sizeof(double));
    double *col_total_nz_arr = G_calloc(ncat1, sizeof(double));
    double grand_total = 0.0;
    double grand_total_nz = 0.0;

    for (r = 0; r < ncat2; r++) {
        G_JSON_Value *row_value = G_json_value_init_array();
        G_JSON_Array *row_arr = G_json_array(row_value);

        for (c = 0; c < ncat1; c++) {
            double val = cell_value(r, c, unit);

            G_json_array_append_number(row_arr, val);

            row_total_arr[r] += val;
            col_total_arr[c] += val;
            grand_total += val;

            if (catlist1[c] != 0 && catlist2[r] != 0) {
                row_total_nz_arr[r] += val;
                col_total_nz_arr[c] += val;
                grand_total_nz += val;
            }
        }
        G_json_array_append_value(matrix, row_value);
    }
    G_json_object_set_value(root, "matrix", matrix_value);

    G_JSON_Value *rt_value = G_json_value_init_array();
    G_JSON_Array *rt = G_json_array(rt_value);
    G_JSON_Value *rt_nz_value = G_json_value_init_array();
    G_JSON_Array *rt_nz = G_json_array(rt_nz_value);

    for (r = 0; r < ncat2; r++) {
        G_json_array_append_number(rt, row_total_arr[r]);
        G_json_array_append_number(rt_nz, row_total_nz_arr[r]);
    }
    G_json_object_set_value(root, "row_totals", rt_value);
    G_json_object_set_value(root, "row_totals_without_zero", rt_nz_value);

    G_JSON_Value *ct_value = G_json_value_init_array();
    G_JSON_Array *ct = G_json_array(ct_value);
    G_JSON_Value *ct_nz_value = G_json_value_init_array();
    G_JSON_Array *ct_nz = G_json_array(ct_nz_value);

    for (c = 0; c < ncat1; c++) {
        G_json_array_append_number(ct, col_total_arr[c]);
        G_json_array_append_number(ct_nz, col_total_nz_arr[c]);
    }
    G_json_object_set_value(root, "col_totals", ct_value);
    G_json_object_set_value(root, "col_totals_without_zero", ct_nz_value);

    G_JSON_Value *totals_value = G_json_value_init_object();
    G_JSON_Object *totals = G_json_object(totals_value);
    G_json_object_set_number(totals, "value", grand_total);
    G_json_object_set_number(totals, "value_without_zero", grand_total_nz);
    G_json_object_set_value(root, "totals", totals_value);

    json_string = G_json_serialize_to_string_pretty(root_value);
    fprintf(stdout, "%s\n", json_string);

    G_json_free_serialized_string(json_string);
    G_json_value_free(root_value);
    G_free(row_total_arr);
    G_free(row_total_nz_arr);
    G_free(col_total_arr);
    G_free(col_total_nz_arr);
}
