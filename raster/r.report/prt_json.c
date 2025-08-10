#include <stdio.h>
#include <string.h>
#include <time.h>
#include "global.h"
#include <grass/parson.h>
#include <grass/glocale.h>

JSON_Value *make_units(int ns, int nl)
{
    JSON_Value *units_value = json_value_init_array();
    JSON_Array *units_array = json_array(units_value);
    for (int i = 0; i < nunits; i++) {
        int _ns = ns;

        JSON_Value *unit_value = json_value_init_object();
        JSON_Object *unit_object = json_object(unit_value);

        if (unit[i].type == CELL_COUNTS) {
            json_object_set_string(unit_object, "unit", "cells");
            json_object_set_number(unit_object, "value", count_sum(&_ns, nl));
        }
        else if (unit[i].type == PERCENT_COVER) {
            json_object_set_string(unit_object, "unit", "percent");
            int k = ns - 1;
            while (k >= 0 && same_cats(k, ns, nl - 1))
                k--;
            k++;
            double area = area_sum(&k, nl - 1);
            area = 100.0 * area_sum(&_ns, nl) / area;
            json_object_set_number(unit_object, "value", area);
        }
        else {
            char *unit_name = NULL;
            if (unit[i].type == ACRES) {
                unit_name = "acres";
            }
            else if (unit[i].type == HECTARES) {
                unit_name = "hectares";
            }
            else if (unit[i].type == SQ_MILES) {
                unit_name = "square miles";
            }
            else if (unit[i].type == SQ_METERS) {
                unit_name = "square meters";
            }
            else if (unit[i].type == SQ_KILOMETERS) {
                unit_name = "square kilometers";
            }
            json_object_set_string(unit_object, "unit", unit_name);
            json_object_set_number(unit_object, "value",
                                   area_sum(&_ns, nl) * unit[i].factor);
        }
        json_array_append_value(units_array, unit_value);
    }
    return units_value;
}

JSON_Value *make_category(int ns, int nl, JSON_Value *sub_categories)
{
    JSON_Value *object_value = json_value_init_object();
    JSON_Object *object = json_object(object_value);

    CELL *cats = Gstats[ns].cats;
    json_object_set_number(object, "category", cats[nl]);

    DCELL dLow, dHigh;

    if (!is_fp[nl] || as_int)
        json_object_set_string(object, "label",
                               Rast_get_c_cat(&cats[nl], &layers[nl].labels));
    else {
        /* find or construct the label for floating point range to print */
        if (Rast_is_c_null_value(&cats[nl]))
            json_object_set_null(object, "label");
        else if (cat_ranges) {
            json_object_set_string(object, "label",
                                   Rast_get_ith_d_cat(&layers[nl].labels,
                                                      cats[nl], &dLow, &dHigh));
        }
        else {
            dLow = (DMAX[nl] - DMIN[nl]) / (double)nsteps *
                       (double)(cats[nl] - 1) +
                   DMIN[nl];
            dHigh = (DMAX[nl] - DMIN[nl]) / (double)nsteps * (double)cats[nl] +
                    DMIN[nl];

            json_object_set_string(object, "label", "from to");

            JSON_Value *range_value = json_value_init_object();
            JSON_Object *range_object = json_object(range_value);
            json_object_set_number(range_object, "from", dLow);
            json_object_set_number(range_object, "to", dHigh);
            json_object_set_value(object, "range", range_value);
        }
    }

    JSON_Value *units_value = make_units(ns, nl);
    json_object_set_value(object, "units", units_value);

    if (sub_categories != NULL) {
        json_object_set_value(object, "categories", sub_categories);
    }
    return object_value;
}

JSON_Value *make_categories(int start, int end, int level)
{
    JSON_Value *array_value = json_value_init_array();
    JSON_Array *array = json_array(array_value);
    if (level == nlayers - 1) {
        for (int i = start; i < end; i++) {
            JSON_Value *category = make_category(i, level, NULL);
            json_array_append_value(array, category);
        }
    }
    else {
        while (start < end) {
            int curr = start;
            while ((curr < end) && same_cats(start, curr, level)) {
                curr++;
            }
            JSON_Value *sub_categories =
                make_categories(start, curr, level + 1);
            JSON_Value *category = make_category(start, level, sub_categories);
            json_array_append_value(array, category);
            start = curr;
        }
    }
    return array_value;
}

void print_json(void)
{
    compute_unit_format(0, nunits - 1, JSON);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_object(root_value);

    json_object_set_string(root_object, "project", G_location());

    char date[64];
    time_t now;
    struct tm *tm_info;

    time(&now);
    tm_info = localtime(&now);
    strftime(date, 64, "%Y-%m-%dT%H:%M:%S%z", tm_info);
    json_object_set_string(root_object, "created", date);

    JSON_Value *region_value = json_value_init_object();
    JSON_Object *region_object = json_object(region_value);
    json_object_set_number(region_object, "north", window.north);
    json_object_set_number(region_object, "south", window.south);
    json_object_set_number(region_object, "east", window.east);
    json_object_set_number(region_object, "west", window.west);
    json_object_set_number(region_object, "ewres", window.ew_res);
    json_object_set_number(region_object, "nsres", window.ns_res);
    json_object_set_value(root_object, "region", region_value);

    char *mask = maskinfo();
    if (strcmp(mask, "none") == 0) {
        json_object_set_null(root_object, "mask");
    }
    else {
        json_object_set_string(root_object, "mask", mask);
    }

    JSON_Value *maps_value = json_value_init_array();
    JSON_Array *maps_array = json_array(maps_value);

    for (int i = 0; i < nlayers; i++) {
        JSON_Value *map_value = json_value_init_object();
        JSON_Object *map_object = json_object(map_value);
        json_object_set_string(map_object, "name", layers[i].name);

        char *label;
        label = Rast_get_cats_title(&(layers[i].labels));
        if (label == NULL || *label == 0) {
            json_object_set_null(map_object, "title");
        }
        else {
            G_strip(label);
            json_object_set_string(map_object, "title", label);
        }

        json_object_set_string(map_object, "type", "raster");
        json_array_append_value(maps_array, map_value);
    }
    json_object_set_value(root_object, "maps", maps_value);

    JSON_Value *root_categories_value = make_categories(0, nstats, 0);
    json_object_set_value(root_object, "categories", root_categories_value);

    JSON_Value *totals = make_units(0, -1);
    json_object_set_value(root_object, "totals", totals);

    char *serialized_string = NULL;
    serialized_string = json_serialize_to_string_pretty(root_value);
    if (serialized_string == NULL) {
        G_fatal_error(_("Failed to initialize pretty JSON string."));
    }
    puts(serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}
