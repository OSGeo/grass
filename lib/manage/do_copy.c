/*!
  \file lib/manage/do_copy.c
  
  \brief Manage Library - Copy element
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>

#include "manage_local_proto.h"

/*!
  \brief Copy element

  \param n   element id
  \param old source name
  \param mapset name of source mapset
  \param new destination name

  \return 0 on success
  \return 1 on error
*/
int M_do_copy(int n, const char *old, const char *mapset, const char *new)
{
    int i, ret;
    char path[GPATH_MAX], path2[GPATH_MAX];
    int result = 0;

    G_debug(3, "Copy %s", list[n].alias);

    G_message(_("Copy %s <%s> to current mapset as <%s>"),
	      list[n].maindesc, G_fully_qualified_name(old, mapset), new);

    M__hold_signals(1);
    if (G_strcasecmp(list[n].alias, "vector") == 0) {
	ret = Vect_copy(old, mapset, new);
	if (ret == -1) {
	    G_warning(_("Unable to copy <%s> to current mapset as <%s>"),
		      G_fully_qualified_name(old, mapset), new);
	    result = 1;
	}
    }
    else {
	for (i = 0; i < list[n].nelem; i++) {
	    G_make_mapset_element(list[n].element[i]);
	    G_file_name(path, list[n].element[i], old, mapset);
	    if (access(path, 0) != 0) {
		G_remove(list[n].element[i], new);
		G_verbose_message(_("%s is missing"), list[n].desc[i]);

		continue;
	    }
	    G_file_name(path2, list[n].element[i], new, G_mapset());
	    if (G_recursive_copy(path, path2) == 1) {
		G_warning(_("Unable to copy <%s> to current mapset as <%s>"),
			  G_fully_qualified_name(old, mapset), new);
		result = 1;
	    }
	    else {
		G_verbose_message(_("%s copied"), list[n].desc[i]);
	    }
	}
    }

    /* special case: remove (yes, remove) the secondary color table, if it exists */
    if (G_strcasecmp(list[n].element[0], "cell") == 0) {
	char colr2[GNAME_MAX];

	sprintf(colr2, "colr2/%s", G_mapset());
	G_remove(colr2, new);
    }
    M__hold_signals(0);

    return result;
}
