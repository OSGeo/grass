/*
 * Functionality to handle list of strings
 *
 * Authors:
 *  Vaclav Petras
 *
 * SPDX-FileCopyrightText: 2015 Vaclav Petras
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef __STRING_LIST_H__
#define __STRING_LIST_H__

/* multiple files */

struct StringList {
    int num_items;
    int max_items;
    char **items;
};

void string_list_from_file(struct StringList *string_list, char *filename);
void string_list_from_one_item(struct StringList *string_list, char *item);
void string_list_free(struct StringList *string_list);

#endif /* __STRING_LIST_H__ */
