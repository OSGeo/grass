/*!
  \file lib/manage/add_elem.c
  
  \brief Manage Library - Add element to the list
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
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
    list[n].element = G_realloc(list[n].element, (nelem + 1) * sizeof(const char *));
    list[n].element[nelem] = G_store(elem);
    list[n].desc = G_realloc(list[n].desc, (nelem + 1) * sizeof(const char *));
    list[n].desc[nelem] = G_store(desc);
}
