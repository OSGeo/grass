/*!
  \file lib/manage/do_rename.c
  
  \brief Manage Library - Rename elements
  
  (C) 2001-2011 by the GRASS Development Team
 
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

#include "manage_local_proto.h"

/*!
  \brief Rename element

  \param n element id
  \param old source name
  \param new destination name

  \return 0 on success
  \return 1 on error
*/
int M_do_rename(int n, const char *old, const char *new)
{
    int i, ret;
    int len;
    const char *mapset;
    int result = 0;
    int renamed = 0;

    G_message(_("Rename %s <%s> to <%s>"),
	      list[n].maindesc, old, new);
    
    if (G_strcasecmp(old, new) == 0)
	return 1;

    len = M__get_description_len(n);

    M__hold_signals(1);

    if (G_strcasecmp(list[n].alias, "vector") == 0) {
	if ((mapset = G_find_vector2(old, "")) == NULL) {
	    G_warning(_("Vector map <%s> not found"), old);
	}
	else {
	    ret = Vect_rename(old, new);
	    if (ret != -1) {
		renamed = 1;
	    }
	    else {
		G_warning(_("Unable to rename vector map <%s> to <%s>"),
			  old, new);
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
	    G_remove(list[n].element[i], new);
	    switch (G_rename(list[n].element[i], old, new)) {
	    case -1:
		G_warning(_("Unable to rename %s"), list[n].desc[i]);
		result = 1;
		break;
	    case 0:
		G_verbose_message(_("%s is missing"), list[n].desc[i]);
		break;
	    case 1:
		G_verbose_message(_("%s renamed"), list[n].desc[i]);
		renamed = 1;
		break;
	    }
	}

	if (G_strcasecmp(list[n].element[0], "cell") == 0) {
	    char colr2[50];

	    sprintf(colr2, "colr2/%s", G_mapset());
	    G_remove(colr2, new);
	    switch (G_rename(colr2, old, new)) {
	    case -1:
		G_warning(_("Unable to rename %s"), colr2);
		result = 1;
		break;
	    case 0:
		G_verbose_message(_("%s is missing"), colr2);
		break;
	    case 1:
		G_verbose_message(_("%s renamed"), colr2);
		renamed = 1;
		break;
	    }
	}
    }
    M__hold_signals(0);

    if (!renamed)
	G_warning(_("<%s> nothing renamed"), old);

    return result;
}
