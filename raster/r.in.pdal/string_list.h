<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
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

#ifndef __STRING_LIST_H__
#define __STRING_LIST_H__

<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
/* A list which keeps multiple strings
 *
 * Intended for list of file names read from a file.
 */

<<<<<<< HEAD
<<<<<<< HEAD
struct StringList {
=======
struct StringList
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
struct StringList {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
    int num_items;
    int max_items;
    char **items;
};

void string_list_from_file(struct StringList *string_list, char *filename);
void string_list_from_one_item(struct StringList *string_list, char *item);
void string_list_free(struct StringList *string_list);

#endif /* __STRING_LIST_H__ */
