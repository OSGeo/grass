/*!
  \file lib/manage/do_list.c
  
  \brief Manage Library - List elements
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/gis.h>

#include "manage_local_proto.h"

/*!
  \brief List elements

  \param n element id
  \param mapset name of mapset
*/
void M_do_list(int n, const char *mapset)
{
    G_list_element(list[n].element[0], list[n].desc[0], mapset, (int (*)())0);
}
