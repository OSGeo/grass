<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
/****************************************************************************
 *
 * MODULE:       r.in.pdal
 *
 * AUTHOR(S):    Vaclav Petras
 *
 * PURPOSE:      Imports LAS LiDAR point clouds to a raster map using
 *               aggregate statistics.
 *
 * COPYRIGHT:    (C) 2019 Vaclav Petras and the The GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include "string_list.h"

#include <grass/gis.h>
#include <grass/glocale.h>

#include <stdio.h>
#include <string.h>

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
#define SIZE_INCREMENT 10

static int string_list_add_item(struct StringList *string_list, char *item)
{
    int n = string_list->num_items++;

    if (string_list->num_items >= string_list->max_items) {
        string_list->max_items += SIZE_INCREMENT;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        string_list->items =
            G_realloc(string_list->items,
                      (size_t)string_list->max_items * sizeof(char *));
=======
        string_list->items = G_realloc(string_list->items,
                                       (size_t)string_list->max_items *
                                       sizeof(char *));
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
        string_list->items =
            G_realloc(string_list->items,
                      (size_t)string_list->max_items * sizeof(char *));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        string_list->items =
            G_realloc(string_list->items,
                      (size_t)string_list->max_items * sizeof(char *));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    }
    /* n contains the index */
    string_list->items[n] = item;
    return n;
}

void string_list_from_file(struct StringList *string_list, char *filename)
{
    string_list->num_items = 0;
    string_list->max_items = 0;
    string_list->items = NULL;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    FILE *file = fopen(filename, "r"); /* should check the result */
=======
    FILE *file = fopen(filename, "r");  /* should check the result */
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    FILE *file = fopen(filename, "r"); /* should check the result */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    FILE *file = fopen(filename, "r"); /* should check the result */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    if (!file)
        G_fatal_error(_("Cannot open file %s for reading"), filename);
    char *line = G_malloc(GPATH_MAX * sizeof(char));

    while (G_getl2(line, GPATH_MAX * sizeof(char), file)) {
        G_debug(5, "line content from file %s: %s\n", filename, line);
        string_list_add_item(string_list, line);
        line = G_malloc(GPATH_MAX);
    }
    /* last allocation was not necessary */
    G_free(line);
    fclose(file);
}

void string_list_from_one_item(struct StringList *string_list, char *item)
{
    string_list->num_items = 0;
    string_list->max_items = 0;
    string_list->items = NULL;
    string_list_add_item(string_list, strdup(item));
}

void string_list_free(struct StringList *string_list)
{
    int i;

    for (i = 0; i < string_list->num_items; i++)
        G_free(string_list->items[i]);
    G_free(string_list->items);
    string_list->num_items = 0;
    string_list->max_items = 0;
    string_list->items = NULL;
}
