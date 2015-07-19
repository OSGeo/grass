/*!
  \file lib/manage/do_remove.c
  
  \brief Manage Library - Remove elements
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/raster3d.h>

#include "manage_local_proto.h"

/*!
  \brief Remove elements from data base

  \param n element id
  \param old name of element to be removed

  \return 0 on success
  \return 1 on error
*/
int M_do_remove(int n, const char *old)
{
    int i, ret;

    /* int len; */
    const char *mapset;
    int result = 0;
    int removed = 0;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    G_message(_("Removing %s <%s>"), list[n].maindesc, old);

    /* len = get_description_len(n); */

    M__hold_signals(1);

    if (G_name_is_fully_qualified(old, xname, xmapset)) {
	if (strcmp(xmapset, G_mapset()) != 0)
	    G_fatal_error("%s is not in the current mapset (%s)", old,
			  G_mapset());
	old = xname;
    }

    if (G_strcasecmp(list[n].alias, "vector") == 0) {
	if ((mapset = G_find_vector2(old, "")) == NULL) {
	    G_warning(_("Vector map <%s> not found"), old);
	}
	else {
	    ret = Vect_delete(old);
	    if (ret != -1) {
		removed = 1;
	    }
	    else {
		G_warning(_("Unable to delete vector map"));
		result = 1;
	    }
	}
    }
    else {
	if (G_strcasecmp(list[n].alias, "raster") == 0) {
	    if ((mapset = G_find_raster2(old, "")) == NULL)
		G_warning(_("Raster map <%s> not found"), old);
	}

	if (G_strcasecmp(list[n].alias, "raster_3d") == 0) {
	    if ((mapset = G_find_raster3d(old, "")) == NULL)
		G_warning(_("3D raster map <%s> not found"), old);
	}

	for (i = 0; i < list[n].nelem; i++) {

	    switch (G_remove(list[n].element[i], old)) {
	    case -1:
		G_warning(_("Unable to remove %s element"), list[n].desc[i]);
		result = 1;
		break;
	    case 0:
		G_verbose_message(_("%s is missing"), list[n].desc[i]);
		break;
	    case 1:
		G_verbose_message(_("%s removed"), list[n].desc[i]);
		removed = 1;
		break;
	    }
	}
    }

    if (G_strcasecmp(list[n].element[0], "cell") == 0) {
	char colr2[GPATH_MAX];

	G_snprintf(colr2, GPATH_MAX, "colr2/%s", G_mapset());
	switch (G_remove(colr2, old)) {
	case -1:
	    G_warning(_("Unable to remove %s"), colr2);
	    result = 1;
	    break;
	case 0:
	    G_verbose_message(_("%s is missing"), colr2);
	    break;
	case 1:
	    G_verbose_message(_("%s removed"), colr2);
	    removed = 1;
	    break;
	}
    }

    M__hold_signals(0);

    if (!removed)
	G_warning(_("<%s> nothing removed"), old);

    return result;
}
