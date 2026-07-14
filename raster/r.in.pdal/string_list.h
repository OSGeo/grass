/****************************************************************************
 *
 * MODULE:       r.in.pdal
 *
 * AUTHOR(S):    Vaclav Petras
 *
 * PURPOSE:      Imports LAS LiDAR point clouds to a raster map using
 *               aggregate statistics.
 *
 * SPDX-FileCopyrightText: 2019 Vaclav Petras and the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 *****************************************************************************/

#ifndef __STRING_LIST_H__
#define __STRING_LIST_H__

/* A list which keeps multiple strings
 *
 * Intended for list of file names read from a file.
 */

struct StringList {
    int num_items;
    int max_items;
    char **items;
};

void string_list_from_file(struct StringList *string_list, char *filename);
void string_list_from_one_item(struct StringList *string_list, char *item);
void string_list_free(struct StringList *string_list);

#endif /* __STRING_LIST_H__ */
