/*!
   \file lib/manage/add_elem.c

   \brief Manage Library - Add element to the list

   SPDX-FileCopyrightText: 2001-2011 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Original author CERL
 */

#include <grass/gis.h>

#include "manage_local_proto.h"

/*!
   \brief Add element to the list

   \param elem element name
   \param desc description of the element
 */
void M__add_element(const char *elem, const char *desc)
{
    int n;
    int nelem;

    if (*desc == 0)
        desc = elem;

    n = nlist - 1;
    nelem = list[n].nelem++;
    list[n].element =
        G_realloc(list[n].element, (nelem + 1) * sizeof(const char *));
    list[n].element[nelem] = G_store(elem);
    list[n].desc = G_realloc(list[n].desc, (nelem + 1) * sizeof(const char *));
    list[n].desc[nelem] = G_store(desc);
}
