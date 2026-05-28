/****************************************************************************
 *
 * MODULE:       r.describe
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Prints terse list of category values found in a raster
 *               map layer.
 *
 * COPYRIGHT:    (C) 2006-2025 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

static void initialize_json_object(G_JSON_Value **, G_JSON_Object **);
static void initialize_json_array(G_JSON_Value **, G_JSON_Array **);
static void append_category_ranges(G_JSON_Array *range_array, long min,
                                   long max);
static void output_pretty_json(G_JSON_Value *);
static int show(CELL, CELL, int *, DCELL, DCELL, RASTER_MAP_TYPE, int,
                enum OutputFormat, G_JSON_Array *);

static void initialize_json_object(G_JSON_Value **root_value,
                                   G_JSON_Object **root_object)
{
    *root_value = G_json_value_init_object();
    if (*root_value == NULL) {
        G_fatal_error(_("Failed to initialize JSON object. Out of memory?"));
    }
    *root_object = G_json_object(*root_value);
}

static void initialize_json_array(G_JSON_Value **root_value,
                                  G_JSON_Array **root_array)
{
    *root_value = G_json_value_init_array();
    if (*root_value == NULL) {
        G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
    }
    *root_array = G_json_array(*root_value);
}

static void append_category_ranges(G_JSON_Array *range_array, long min,
                                   long max)
{
    G_JSON_Object *cat_object;
    G_JSON_Value *cat_value;
    initialize_json_object(&cat_value, &cat_object);

    G_json_object_set_number(cat_object, "min", min);
    G_json_object_set_number(cat_object, "max", max);

    G_json_array_append_value(range_array, cat_value);
}

static void output_pretty_json(G_JSON_Value *root_value)
{
    char *serialized_string = G_json_serialize_to_string_pretty(root_value);
    if (!serialized_string) {
        G_json_value_free(root_value);
        G_fatal_error(_("Failed to initialize pretty JSON string."));
    }
    puts(serialized_string);

    G_json_free_serialized_string(serialized_string);
    G_json_value_free(root_value);
}

int long_list(struct Cell_stats *statf, DCELL dmin, DCELL dmax,
              char *no_data_str, int skip_nulls, RASTER_MAP_TYPE map_type,
              int nsteps, enum OutputFormat format)
{
    CELL cat;
    long count; /* not used, but required by cell stats call */
    G_JSON_Value *root_value, *range_value;
    G_JSON_Object *root_object;
    G_JSON_Array *range_array;

    if (format == JSON) {
        initialize_json_object(&root_value, &root_object);
        initialize_json_array(&range_value, &range_array);
    }

    Rast_get_stats_for_null_value(&count, statf);
    if (!skip_nulls) {
        if (count != 0) {
            switch (format) {
            case PLAIN:
                fprintf(stdout, "%s\n", no_data_str);
                break;
            case JSON:
                G_json_object_set_boolean(root_object, "has_nulls", true);
                break;
            }
        }
        else {
            if (format == JSON) {
                G_json_object_set_boolean(root_object, "has_nulls", false);
            }
        }
    }

    while (Rast_next_cell_stat(&cat, &count, statf)) {
        switch (format) {
        case PLAIN:
            if (map_type != CELL_TYPE)
                fprintf(stdout, "%f-%f\n",
                        dmin + (double)(cat - 1) * (dmax - dmin) / nsteps,
                        dmin + (double)cat * (dmax - dmin) / nsteps);
            else
                fprintf(stdout, "%ld\n", (long)cat);
            break;
        case JSON:
            if (map_type != CELL_TYPE) {
                G_JSON_Object *cat_object;
                G_JSON_Value *cat_value;
                initialize_json_object(&cat_value, &cat_object);

                G_json_object_set_number(cat_object, "min",
                                         dmin + (double)(cat - 1) *
                                                    (dmax - dmin) / nsteps);
                G_json_object_set_number(cat_object, "max",
                                         dmin + (double)cat * (dmax - dmin) /
                                                    nsteps);

                G_json_array_append_value(range_array, cat_value);
            }
            else {
                G_json_array_append_number(range_array, (long)cat);
            }

            break;
        }
    }

    if (format == JSON) {
        if (map_type != CELL_TYPE) {
            G_json_object_set_value(root_object, "ranges", range_value);
        }
        else {
            G_json_object_set_value(root_object, "values", range_value);
        }

        output_pretty_json(root_value);
    }

    return (0);
}

int compact_list(struct Cell_stats *statf, DCELL dmin, DCELL dmax,
                 char *no_data_str, int skip_nulls, RASTER_MAP_TYPE map_type,
                 int nsteps, enum OutputFormat format)
{
    CELL cat1, cat2, temp;
    int len;
    long count; /* not used, but required by cell stats call */
    G_JSON_Value *root_value, *range_value;
    G_JSON_Object *root_object;
    G_JSON_Array *range_array = NULL;

    if (format == JSON) {
        initialize_json_object(&root_value, &root_object);
        initialize_json_array(&range_value, &range_array);
    }

    len = 0;
    Rast_get_stats_for_null_value(&count, statf);
    if (!skip_nulls) {
        if (count != 0) {
            switch (format) {
            case PLAIN:
                fprintf(stdout, "%s ", no_data_str);
                break;
            case JSON:
                G_json_object_set_boolean(root_object, "has_nulls", true);
                break;
            }
        }
        else {
            if (format == JSON) {
                G_json_object_set_boolean(root_object, "has_nulls", false);
            }
        }
    }

    if (!Rast_next_cell_stat(&cat1, &count, statf))
        /* map doesn't contain any non-null data */
        return 1;

    cat2 = cat1;
    while (Rast_next_cell_stat(&temp, &count, statf)) {
        if (temp != cat2 + (CELL)1) {
            show(cat1, cat2, &len, dmin, dmax, map_type, nsteps, format,
                 range_array);
            cat1 = temp;
        }
        cat2 = temp;
    }
    show(cat1, cat2, &len, dmin, dmax, map_type, nsteps, format, range_array);

    switch (format) {
    case PLAIN:
        fprintf(stdout, "\n");
        break;
    case JSON:
        G_json_object_set_value(root_object, "ranges", range_value);
        output_pretty_json(root_value);
        break;
    }

    return (0);
}

static int show(CELL low, CELL high, int *len, DCELL dmin, DCELL dmax,
                RASTER_MAP_TYPE map_type, int nsteps, enum OutputFormat format,
                G_JSON_Array *root_array)
{
    char text[100] = {0};
    char xlen;
    G_JSON_Object *cat_object;
    G_JSON_Value *cat_value;

    if (low + 1 == high) {
        show(low, low, len, dmin, dmax, map_type, nsteps, format, root_array);
        show(high, high, len, dmin, dmax, map_type, nsteps, format, root_array);
        return 0;
    }

    if (format == JSON)
        initialize_json_object(&cat_value, &cat_object);

    if (map_type != CELL_TYPE) {
        switch (format) {
        case PLAIN:
            snprintf(text, sizeof(text), "%f%s%f ",
                     dmin + (low - 1) * (dmax - dmin) / nsteps,
                     dmin < 0 ? " thru " : "-",
                     dmin + high * (dmax - dmin) / nsteps);
            break;
        case JSON:
            G_json_object_set_number(cat_object, "min",
                                     dmin + (low - 1) * (dmax - dmin) / nsteps);
            G_json_object_set_number(cat_object, "max",
                                     dmin + high * (dmax - dmin) / nsteps);
            break;
        }
    }
    else {
        switch (format) {
        case PLAIN:
            if (low == high)
                snprintf(text, sizeof(text), "%ld ", (long)low);
            else
                snprintf(text, sizeof(text), "%ld%s%ld ", (long)low,
                         low < 0 ? " thru " : "-", (long)high);
            break;
        case JSON:
            G_json_object_set_number(cat_object, "min", (long)low);
            G_json_object_set_number(cat_object, "max", (long)high);
            break;
        }
    }

    xlen = strlen(text);
    if (xlen + *len > 78) {
        if (format == PLAIN)
            fprintf(stdout, "\n");
        *len = 0;
    }

    switch (format) {
    case PLAIN:
        fprintf(stdout, "%s", text);
        break;
    case JSON:
        G_json_array_append_value(root_array, cat_value);
        break;
    }

    *len += xlen;
    return (0);
}

int compact_range_list(CELL negmin, CELL negmax, CELL zero, CELL posmin,
                       CELL posmax, CELL null, char *no_data_str,
                       int skip_nulls, enum OutputFormat format)
{
    G_JSON_Value *root_value, *range_value;
    G_JSON_Object *root_object;
    G_JSON_Array *range_array;

    if (format == JSON) {
        initialize_json_object(&root_value, &root_object);
        initialize_json_array(&range_value, &range_array);
    }

    if (negmin) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "%ld", (long)negmin);
            if (negmin != negmax)
                fprintf(stdout, " thru %ld", (long)negmax);
            fprintf(stdout, "\n");
            break;
        case JSON:
            append_category_ranges(range_array, (long)negmin, (long)negmax);
            break;
        }
    }
    if (zero) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "0\n");
            break;
        case JSON:
            append_category_ranges(range_array, 0, 0);
            break;
        }
    }
    if (posmin) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "%ld", (long)posmin);
            if (posmin != posmax)
                fprintf(stdout, " thru %ld", (long)posmax);
            fprintf(stdout, "\n");
            break;
        case JSON:
            append_category_ranges(range_array, (long)posmin, (long)posmax);
            break;
        }
    }

    if (!skip_nulls) {
        if (null) {
            switch (format) {
            case PLAIN:
                fprintf(stdout, "%s\n", no_data_str);
                break;
            case JSON:
                G_json_object_set_boolean(root_object, "has_nulls", true);
                break;
            }
        }
        else {
            if (format == JSON) {
                G_json_object_set_boolean(root_object, "has_nulls", false);
            }
        }
    }

    if (format == JSON) {
        G_json_object_set_value(root_object, "ranges", range_value);
        output_pretty_json(root_value);
    }

    return (0);
}

int range_list(CELL negmin, CELL negmax, CELL zero, CELL posmin, CELL posmax,
               CELL null, char *no_data_str, int skip_nulls,
               enum OutputFormat format)
{
    G_JSON_Value *root_value, *range_value;
    G_JSON_Object *root_object;
    G_JSON_Array *range_array;

    if (format == JSON) {
        initialize_json_object(&root_value, &root_object);
        initialize_json_array(&range_value, &range_array);
    }

    if (negmin) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "%ld\n", (long)negmin);
            if (negmin != negmax)
                fprintf(stdout, "%ld\n", (long)negmax);
            break;
        case JSON:
            append_category_ranges(range_array, (long)negmin, (long)negmax);
            break;
        }
    }

    if (zero) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "0\n");
            break;
        case JSON:
            append_category_ranges(range_array, 0, 0);
            break;
        }
    }

    if (posmin) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "%ld\n", (long)posmin);
            if (posmin != posmax)
                fprintf(stdout, "%ld\n", (long)posmax);
            break;
        case JSON:
            append_category_ranges(range_array, (long)posmin, (long)posmax);
            break;
        }
    }

    if (!skip_nulls) {
        if (null) {
            switch (format) {
            case PLAIN:
                fprintf(stdout, "%s\n", no_data_str);
                break;
            case JSON:
                G_json_object_set_boolean(root_object, "has_nulls", true);
                break;
            }
        }
        else {
            if (format == JSON) {
                G_json_object_set_boolean(root_object, "has_nulls", false);
            }
        }
    }

    if (format == JSON) {
        G_json_object_set_value(root_object, "ranges", range_value);
        output_pretty_json(root_value);
    }

    return (0);
}
