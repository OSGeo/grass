/****************************************************************************
 *
 * MODULE:       r.distance
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               Sort/reverse sort by distance by Huidae Cho
 *
 * PURPOSE:      Locates the closest points between objects in two
 *               raster maps.
 *
 * COPYRIGHT:    (C) 2003-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <grass/glocale.h>

#include "defs.h"

struct ReportLine {
    CELL cat1;
    CELL cat2;
    int isnull1;
    int isnull2;
    double east1;
    double north1;
    double east2;
    double north2;
    double distance;
};

static void print(struct ReportLine *, struct Parms *, G_JSON_Array *);
static int compare(const void *, const void *);
static int revcompare(const void *, const void *);

void report(struct Parms *parms)
{
    int i1, i2;
    struct Map *map1, *map2;
    double distance, north1, east1, north2, east2;
    struct Cell_head region;
    struct CatEdgeList *list1, *list2;
    struct ReportLine *lines;
    int nlines;
    G_JSON_Value *root_value = NULL;
    G_JSON_Array *root_array = NULL;

    G_get_set_window(&region);
    G_begin_distance_calculations();

    map1 = &parms->map1;
    map2 = &parms->map2;

    G_message(_("Processing..."));

    if (parms->format == JSON) {
        root_value = G_json_value_init_array();
        if (root_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        root_array = G_json_array(root_value);
    }
    else if (parms->format == CSV) {
        fprintf(stdout, "%s%s%s%s%s%s%s%s%s%s%s%s%s", "from_category",
                parms->fs, "to_category", parms->fs, "distance", parms->fs,
                "from_easting", parms->fs, "from_northing", parms->fs,
                "to_easting", parms->fs, "to_northing");

        if (parms->labels) {
            fprintf(stdout, "%s%s%s%s", parms->fs, "from_label", parms->fs,
                    "to_label");
        }

        fprintf(stdout, "\n");
    }

    if (parms->sort > 0)
        lines = (struct ReportLine *)G_malloc(
            map1->edges.ncats * map2->edges.ncats * sizeof(struct ReportLine));
    else
        lines = NULL;
    nlines = 0;

    for (i1 = 0; i1 < map1->edges.ncats; i1++) {
        int isnull1;

        list1 = &map1->edges.catlist[i1];
        isnull1 = parms->null ? Rast_is_c_null_value(&(list1->cat)) : 0;

        for (i2 = 0; i2 < map2->edges.ncats; i2++) {
            int isnull2;
            struct ReportLine line;

            list2 = &map2->edges.catlist[i2];
            isnull2 = parms->null ? Rast_is_c_null_value(&(list2->cat)) : 0;

            find_minimum_distance(list1, list2, &east1, &north1, &east2,
                                  &north2, &distance, &region, parms->overlap,
                                  map1->name, map2->name);

            line.cat1 = list1->cat;
            line.cat2 = list2->cat;
            line.isnull1 = isnull1;
            line.isnull2 = isnull2;
            line.east1 = east1;
            line.north1 = north1;
            line.east2 = east2;
            line.north2 = north2;
            line.distance = distance;

            if (parms->sort > 0)
                lines[nlines++] = line;
            else
                print(&line, parms, root_array);
        }
    }

    if (parms->sort > 0) {
        int i;

        if (parms->sort == 1)
            qsort(lines, nlines, sizeof(struct ReportLine), compare);
        else
            qsort(lines, nlines, sizeof(struct ReportLine), revcompare);

        for (i = 0; i < nlines; i++)
            print(&lines[i], parms, root_array);
    }

    if (parms->format == JSON) {
        char *json_string = G_json_serialize_to_string_pretty(root_value);
        if (!json_string) {
            G_json_value_free(root_value);
            G_fatal_error(_("Failed to serialize JSON to pretty format."));
        }

        puts(json_string);

        G_json_free_serialized_string(json_string);
        G_json_value_free(root_value);
    }
    G_free(lines);
}

static void print(struct ReportLine *line, struct Parms *parms,
                  G_JSON_Array *root_array)
{
    char *fs;
    char temp[100];
    G_JSON_Value *cell_value = NULL, *from_cell_value = NULL,
                 *to_cell_value = NULL;
    G_JSON_Object *cell_object = NULL, *from_cell_object = NULL,
                  *to_cell_object = NULL;

    fs = parms->fs;

    switch (parms->format) {
    case CSV:
    case PLAIN:
        /* print cat numbers */
        if (line->isnull1 && line->isnull2)
            fprintf(stdout, "*%s*", fs);
        else if (line->isnull1)
            fprintf(stdout, "*%s%ld", fs, (long)line->cat2);
        else if (line->isnull2)
            fprintf(stdout, "%ld%s*", (long)line->cat1, fs);
        else
            fprintf(stdout, "%ld%s%ld", (long)line->cat1, fs, (long)line->cat2);

        /* print distance */
        snprintf(temp, sizeof(temp), "%.10f", line->distance);
        G_trim_decimal(temp);
        fprintf(stdout, "%s%s", fs, temp);

        /* print coordinates of the closest pair */
        G_format_easting(line->east1, temp,
                         G_projection() == PROJECTION_LL ? -1 : 0);
        fprintf(stdout, "%s%s", fs, temp);
        G_format_northing(line->north1, temp,
                          G_projection() == PROJECTION_LL ? -1 : 0);
        fprintf(stdout, "%s%s", fs, temp);
        G_format_easting(line->east2, temp,
                         G_projection() == PROJECTION_LL ? -1 : 0);
        fprintf(stdout, "%s%s", fs, temp);
        G_format_northing(line->north2, temp,
                          G_projection() == PROJECTION_LL ? -1 : 0);
        fprintf(stdout, "%s%s", fs, temp);

        /* print category labels */
        if (parms->labels) {
            struct Map *map1, *map2;

            map1 = &parms->map1;
            map2 = &parms->map2;

            fprintf(stdout, "%s%s", fs, get_label(map1, line->cat1));
            fprintf(stdout, "%s%s", fs, get_label(map2, line->cat2));
        }
        fprintf(stdout, "\n");
        break;

    case JSON:
        /* initialize JSON objects */
        cell_value = G_json_value_init_object();
        if (cell_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        cell_object = G_json_object(cell_value);

        from_cell_value = G_json_value_init_object();
        if (from_cell_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        from_cell_object = G_json_object(from_cell_value);

        to_cell_value = G_json_value_init_object();
        if (to_cell_value == NULL) {
            G_fatal_error(
                _("Failed to initialize JSON object. Out of memory?"));
        }
        to_cell_object = G_json_object(to_cell_value);

        /* print cat numbers */
        if (line->isnull1)
            G_json_object_set_null(from_cell_object, "category");
        else
            G_json_object_set_number(from_cell_object, "category",
                                     (long)line->cat1);

        if (line->isnull2)
            G_json_object_set_null(to_cell_object, "category");
        else
            G_json_object_set_number(to_cell_object, "category",
                                     (long)line->cat2);

        /* print coordinates of the closest pair */
        G_json_object_set_number(from_cell_object, "easting", line->east1);
        G_json_object_set_number(from_cell_object, "northing", line->north1);
        G_json_object_set_number(to_cell_object, "easting", line->east2);
        G_json_object_set_number(to_cell_object, "northing", line->north2);

        /* print category labels */
        if (parms->labels) {
            struct Map *map1, *map2;

            map1 = &parms->map1;
            map2 = &parms->map2;

            G_json_object_set_string(from_cell_object, "label",
                                     get_label(map1, line->cat1));
            G_json_object_set_string(to_cell_object, "label",
                                     get_label(map2, line->cat2));
        }

        /* print distance */
        G_json_object_set_value(cell_object, "from_cell", from_cell_value);
        G_json_object_set_value(cell_object, "to_cell", to_cell_value);
        G_json_object_set_number(cell_object, "distance", line->distance);

        /* add the cell object to the root array */
        G_json_array_append_value(root_array, cell_value);
        break;
    }
}

static int compare(const void *p1, const void *p2)
{
    const struct ReportLine *line1, *line2;

    line1 = (const struct ReportLine *)p1;
    line2 = (const struct ReportLine *)p2;

    if (line1->distance < line2->distance)
        return -1; /* short distance first */
    if (line1->distance > line2->distance)
        return 1;

    if (!line1->isnull1 && line2->isnull1)
        return -1; /* non-null first */
    if (line1->isnull1 && !line2->isnull1)
        return 1;
    if (!line1->isnull1 && !line2->isnull1) {
        if (line1->cat1 < line2->cat1)
            return -1; /* small cat first */
        if (line1->cat1 > line2->cat1)
            return 1;
    }

    if (!line1->isnull2 && line2->isnull2)
        return -1;
    if (line1->isnull2 && !line2->isnull2)
        return 1;
    if (!line1->isnull2 && !line2->isnull2) {
        if (line1->cat2 < line2->cat2)
            return -1;
        if (line1->cat2 > line2->cat2)
            return 1;
    }

    return 0; /* same cat1, same cat2 */
}

static int revcompare(const void *p1, const void *p2)
{
    return -compare(p1, p2);
}
