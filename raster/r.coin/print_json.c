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

void print_json(char unit)
{
    int r, c;
    long cat1, cat2;
    int idx;
    char *json_string;
    long total_count;
    double total_area;
    double val;
    char unit_str[2];

    G_JSON_Value *root_value = G_json_value_init_object();
    G_JSON_Object *root = G_json_object(root_value);

    G_json_object_set_string(root, "module", "r.coin");
    G_json_object_set_string(root, "map1", map1name);
    G_json_object_set_string(root, "map2", map2name);

    G_JSON_Value *unit_info_value = G_json_value_init_object();
    G_JSON_Object *unit_info = G_json_object(unit_info_value);

    unit_str[0] = unit;
    unit_str[1] = '\0';
    G_json_object_set_string(unit_info, "code", unit_str);
    G_json_object_set_string(unit_info, "name", get_unit_name(unit));
    G_json_object_set_value(root, "unit", unit_info_value);

    G_JSON_Value *coincidence_array_value = G_json_value_init_array();
    G_JSON_Array *coincidence_array = G_json_array(coincidence_array_value);

    for (r = 0; r < ncat2; r++) {
        for (c = 0; c < ncat1; c++) {
            idx = r * ncat1 + c;

            if (table[idx].count > 0) {
                cat1 = catlist1[c];
                cat2 = catlist2[r];
                Cndex = c;
                Rndex = r;

                switch (unit) {
                case 'c':
                    val = (double)table[idx].count;
                    break;
                case 'p':
                    val = F_CTOP(table[idx].area, window_area);
                    break;
                case 'x':
                    col_total(c, 1, &total_count, &total_area);
                    val = F_CTOP(table[idx].area, total_area);
                    break;
                case 'y':
                    row_total(r, 1, &total_count, &total_area);
                    val = F_CTOP(table[idx].area, total_area);
                    break;
                case 'a':
                    val = F_CTOA(table[idx].area);
                    break;
                case 'h':
                    val = F_CTOH(table[idx].area);
                    break;
                case 'k':
                    val = F_CTOK(table[idx].area);
                    break;
                case 'm':
                    val = F_CTOM(table[idx].area);
                    break;
                default:
                    val = (double)table[idx].count;
                    break;
                }

                G_JSON_Value *entry_value = G_json_value_init_object();
                G_JSON_Object *entry = G_json_object(entry_value);

                G_json_object_set_number(entry, "cat1", (double)cat1);
                G_json_object_set_number(entry, "cat2", (double)cat2);
                G_json_object_set_number(entry, "count",
                                         (double)table[idx].count);
                G_json_object_set_number(entry, "area", table[idx].area);
                G_json_object_set_number(entry, "value", val);

                G_json_array_append_value(coincidence_array, entry_value);
            }
        }
    }

    G_json_object_set_value(root, "coincidence", coincidence_array_value);

    json_string = G_json_serialize_to_string_pretty(root_value);
    fprintf(stdout, "%s\n", json_string);

    G_json_free_serialized_string(json_string);
    G_json_value_free(root_value);
}
