#include <stdio.h>
#include <string.h>
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
            json_object_set_string(unit_object, "unit", "cell counts");
            json_object_set_number(unit_object, "value", count_sum(&_ns, nl));
        }
        else if (unit[i].type == PERCENT_COVER) {
            json_object_set_string(unit_object, "unit", "% cover");
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
    char str[500];

    if (!is_fp[nl] || as_int)
        json_object_set_number(object, "description", cats[nl]);
    else {
        /* find or construct the label for floating point range to print */
        if (Rast_is_c_null_value(&cats[nl]))
            json_object_set_null(object, "description");
        else if (cat_ranges) {
            json_object_set_string(object, "description",
                                   Rast_get_ith_d_cat(&layers[nl].labels,
                                                      cats[nl], &dLow, &dHigh));
        }
        else {
            dLow = (DMAX[nl] - DMIN[nl]) / (double)nsteps *
                       (double)(cats[nl] - 1) +
                   DMIN[nl];
            dHigh = (DMAX[nl] - DMIN[nl]) / (double)nsteps * (double)cats[nl] +
                    DMIN[nl];
            char *from = Rast_get_d_cat(&dLow, &layers[nl].labels);
            char *to = Rast_get_d_cat(&dHigh, &layers[nl].labels);
            sprintf(str, "from %s to %s", from, to);
            json_object_set_string(object, "description", str);

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

void print_json()
{
    for (int i = 0; i < nunits; i++) {
        switch (unit[i].type) {
        case CELL_COUNTS:
            unit[i].label[0] = " cell";
            unit[i].label[1] = "count";
            break;

        case PERCENT_COVER:
            unit[i].label[0] = "  %  ";
            unit[i].label[1] = "cover";
            break;

        case SQ_METERS:
            unit[i].label[0] = "square";
            unit[i].label[1] = "meters";
            unit[i].factor = 1.0;
            break;

        case SQ_KILOMETERS:
            unit[i].label[0] = "  square  ";
            unit[i].label[1] = "kilometers";
            unit[i].factor = 1.0e-6;
            break;

        case ACRES:
            unit[i].label[0] = "";
            unit[i].label[1] = "acres";
            unit[i].factor = 2.47105381467165e-4; /* 640 acres in a sq mile */
            break;

        case HECTARES:
            unit[i].label[0] = "";
            unit[i].label[1] = "hectares";
            unit[i].factor = 1.0e-4;
            break;

        case SQ_MILES:
            unit[i].label[0] = "square";
            unit[i].label[1] = " miles";
            unit[i].factor = 3.86102158542446e-7; /* 1 / ( (0.0254m/in * 12in/ft
                                                   * 5280ft/mi)^2 ) */
            break;

        default:
            G_fatal_error("Unit %d not yet supported", unit[i].type);
        }
    }
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_object(root_value);

    json_object_set_string(root_object, "location", G_location());
    json_object_set_string(root_object, "created", G_date());

    JSON_Value *region_value = json_value_init_object();
    JSON_Object *region_object = json_object(region_value);
    json_object_set_number(region_object, "north", window.north);
    json_object_set_number(region_object, "south", window.south);
    json_object_set_number(region_object, "east", window.east);
    json_object_set_number(region_object, "west", window.west);
    json_object_set_number(region_object, "ew_res", window.ew_res);
    json_object_set_number(region_object, "ns_res", window.ns_res);
    json_object_set_value(root_object, "region", region_value);

    char *mask = maskinfo();
    if (strcmp(mask, "none") == 0) {
        json_object_set_null(root_object, "mask");
    }
    else {
        json_object_set_string(root_object, "mask", maskinfo());
    }

    JSON_Value *maps_value = json_value_init_array();
    JSON_Array *maps_array = json_array(maps_value);
    for (int i = 0; i < nlayers; i++) {
        JSON_Value *map_value = json_value_init_object();
        JSON_Object *map_object = json_object(map_value);
        json_object_set_string(map_object, "name", layers[i].name); // fixme
        json_object_set_string(map_object, "description", layers[i].mapset);
        json_object_set_string(map_object, "layer", layers[i].name);
        json_object_set_string(map_object, "type", "raster"); // fixme
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
