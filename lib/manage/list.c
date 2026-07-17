/*!
   \file lib/manage/list.c

   \brief Manage Library - Element info

   SPDX-FileCopyrightText: 2001-2011 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Original author CERL
 */

#include <grass/gis.h>

#include "manage_local_proto.h"

/*!
   \brief Get list structure

   \param n element id

   \return pointer to list structure
   \return NULL on error
 */
const struct list *M_get_list(int n)
{
    if (n >= nlist)
        return NULL;

    return &(list[n]);
}

/*!
   \brief Find element type by name

   \param data_type element type

   \return element id
   \return -1 not found
 */
int M_get_element(const char *data_type)
{
    int n;

    for (n = 0; n < nlist; n++) {
        if (G_strcasecmp(list[n].alias, data_type) == 0)
            return n;
    }

    return -1;
}
