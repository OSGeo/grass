/*!
  \file lib/manage/find.c
  
  \brief Manage Library - Find element in data base
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <string.h>
#include <grass/gis.h>

#include "manage_local_proto.h"

/*!
  \brief Find element in data base

  \param n element id
  \param name element name
  \param subprojects name of subprojects

  \return subproject if found
  \return if not found
*/
const char *M_find(int n, char *name, const char *subprojects)
{
    const char *subproject;

    subproject = G_find_file2(list[n].element[0], name, subprojects);
    if (subproject) {
	char temp[GNAME_MAX];

	sscanf(name, "%s", temp);
	strcpy(name, temp);
    }
    return subproject;
}
