/****************************************************************************
 *
 * MODULE:       r.describe
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Prints terse list of category values found in a raster
 *               map layer.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
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

static int show(CELL, CELL, int *, DCELL, DCELL, RASTER_MAP_TYPE, int,
                enum OutputFormat, JSON_Array *);
static void initialize_json_array(JSON_Value **, JSON_Array **);
static void json_append_category_value(JSON_Array *, const char *);
static void output_pretty_json(JSON_Value *);

void initialize_json_array(JSON_Value **root_value, JSON_Array **root_array)
{
    *root_value = json_value_init_array();
    if (*root_value == NULL) {
        G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
    }
    *root_array = json_array(*root_value);
}

void json_append_category_value(JSON_Array *root_array, const char *cat_str)
{
    JSON_Value *category_value;
    JSON_Object *category;

    category_value = json_value_init_object();
    if (category_value == NULL) {
        G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
    }
    category = json_object(category_value);

    json_object_set_string(category, "value", cat_str);
    json_array_append_value(root_array, category_value);
}

void output_pretty_json(JSON_Value *root_value)
{
    char *serialized_string = json_serialize_to_string_pretty(root_value);
    if (!serialized_string) {
        json_value_free(root_value);
        G_fatal_error(_("Failed to initialize pretty JSON string."));
    }
    puts(serialized_string);

    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

int long_list(struct Cell_stats *statf, DCELL dmin, DCELL dmax,
              char *no_data_str, int skip_nulls, RASTER_MAP_TYPE map_type,
              int nsteps, enum OutputFormat format)
{
    CELL cat;
    long count; /* not used, but required by cell stats call */
    char cat_str[MAX_STR_LEN];
    JSON_Value *root_value;
    JSON_Array *root_array;

    if (format == JSON) {
        initialize_json_array(&root_value, &root_array);
    }

    Rast_get_stats_for_null_value(&count, statf);
    if (count != 0 && !skip_nulls) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "%s\n", no_data_str);
            break;
        case JSON:
            json_append_category_value(root_array, no_data_str);
            break;
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
                snprintf(cat_str, sizeof(cat_str), "%f-%f",
                         dmin + (double)(cat - 1) * (dmax - dmin) / nsteps,
                         dmin + (double)cat * (dmax - dmin) / nsteps);
            }
            else {
                snprintf(cat_str, sizeof(cat_str), "%ld", (long)cat);
            }
            json_append_category_value(root_array, cat_str);
            break;
        }
    }

    if (format == JSON) {
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
    JSON_Value *root_value;
    JSON_Array *root_array;

    if (format == JSON) {
        initialize_json_array(&root_value, &root_array);
    }

    len = 0;
    Rast_get_stats_for_null_value(&count, statf);
    if (count != 0 && !skip_nulls) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "%s ", no_data_str);
            break;
        case JSON:
            json_append_category_value(root_array, no_data_str);
            break;
        }
    }

    if (!Rast_next_cell_stat(&cat1, &count, statf))
        /* map doesn't contain any non-null data */
        return 1;

    cat2 = cat1;
    while (Rast_next_cell_stat(&temp, &count, statf)) {
        if (temp != cat2 + (CELL)1) {
            show(cat1, cat2, &len, dmin, dmax, map_type, nsteps, format,
                 root_array);
            cat1 = temp;
        }
        cat2 = temp;
    }
    show(cat1, cat2, &len, dmin, dmax, map_type, nsteps, format, root_array);

    switch (format) {
    case PLAIN:
        fprintf(stdout, "\n");
        break;
    case JSON:
        output_pretty_json(root_value);
        break;
    }

    return (0);
}

static int show(CELL low, CELL high, int *len, DCELL dmin, DCELL dmax,
                RASTER_MAP_TYPE map_type, int nsteps, enum OutputFormat format,
                JSON_Array *root_array)
{
    char text[MAX_STR_LEN];
    char xlen;

    if (low + 1 == high) {
        show(low, low, len, dmin, dmax, map_type, nsteps, format, root_array);
        show(high, high, len, dmin, dmax, map_type, nsteps, format, root_array);
        return 0;
    }

    if (map_type != CELL_TYPE) {
        switch (format) {
        case PLAIN:
            sprintf(text, "%f%s%f ", dmin + (low - 1) * (dmax - dmin) / nsteps,
                    dmin < 0 ? " thru " : "-",
                    dmin + high * (dmax - dmin) / nsteps);
            break;
        case JSON:
            sprintf(text, "%f%s%f", dmin + (low - 1) * (dmax - dmin) / nsteps,
                    dmin < 0 ? " thru " : "-",
                    dmin + high * (dmax - dmin) / nsteps);
            break;
        }
    }
    else {
        switch (format) {
        case PLAIN:
            if (low == high)
                sprintf(text, "%ld ", (long)low);
            else
                sprintf(text, "%ld%s%ld ", (long)low, low < 0 ? " thru " : "-",
                        (long)high);
            break;
        case JSON:
            if (low == high)
                sprintf(text, "%ld", (long)low);
            else
                sprintf(text, "%ld%s%ld", (long)low, low < 0 ? " thru " : "-",
                        (long)high);
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
        json_append_category_value(root_array, text);
        break;
    }

    *len += xlen;
    return (0);
}

int compact_range_list(CELL negmin, CELL negmax, CELL zero, CELL posmin,
                       CELL posmax, CELL null, char *no_data_str,
                       int skip_nulls, enum OutputFormat format)
{
    char cat_str[MAX_STR_LEN];
    JSON_Value *root_value;
    JSON_Array *root_array;

    if (format == JSON) {
        initialize_json_array(&root_value, &root_array);
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
            snprintf(cat_str, sizeof(cat_str), "%ld", (long)negmin);
            if (negmin != negmax)
                snprintf(cat_str, sizeof(cat_str), "%ld thru %ld", (long)negmin,
                         (long)negmax);

            json_append_category_value(root_array, cat_str);
            break;
        }
    }
    if (zero) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "0\n");
            break;
        case JSON:
            json_append_category_value(root_array, "0");
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
            snprintf(cat_str, sizeof(cat_str), "%ld", (long)posmin);
            if (posmin != posmax)
                snprintf(cat_str, sizeof(cat_str), "%ld thru %ld", (long)posmin,
                         (long)posmax);

            json_append_category_value(root_array, cat_str);
            break;
        }
    }

    if (null && !skip_nulls) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "%s\n", no_data_str);
            break;
        case JSON:
            json_append_category_value(root_array, no_data_str);
            break;
        }
    }

    if (format == JSON) {
        output_pretty_json(root_value);
    }

    return (0);
}

int range_list(CELL negmin, CELL negmax, CELL zero, CELL posmin, CELL posmax,
               CELL null, char *no_data_str, int skip_nulls,
               enum OutputFormat format)
{
    char cat_str[MAX_STR_LEN];
    JSON_Value *root_value;
    JSON_Array *root_array;

    if (format == JSON) {
        initialize_json_array(&root_value, &root_array);
    }

    if (negmin) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "%ld\n", (long)negmin);
            if (negmin != negmax)
                fprintf(stdout, "%ld\n", (long)negmax);
            break;
        case JSON:
            snprintf(cat_str, sizeof(cat_str), "%ld", (long)negmin);
            json_append_category_value(root_array, cat_str);

            if (negmin != negmax) {
                snprintf(cat_str, sizeof(cat_str), "%ld", (long)negmax);
                json_append_category_value(root_array, cat_str);
            }
            break;
        }
    }

    if (zero) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "0\n");
            break;
        case JSON:
            json_append_category_value(root_array, "0");
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
            snprintf(cat_str, sizeof(cat_str), "%ld", (long)posmin);
            json_append_category_value(root_array, cat_str);

            if (posmin != posmax) {
                snprintf(cat_str, sizeof(cat_str), "%ld", (long)posmax);
                json_append_category_value(root_array, cat_str);
            }
            break;
        }
    }

    if (null && !skip_nulls) {
        switch (format) {
        case PLAIN:
            fprintf(stdout, "%s\n", no_data_str);
            break;
        case JSON:
            json_append_category_value(root_array, no_data_str);
            break;
        }
    }

    if (format == JSON) {
        output_pretty_json(root_value);
    }

    return (0);
}
