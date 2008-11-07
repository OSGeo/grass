#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include <grass/G3d.h>
#include "list.h"

/* 
 *  returns 0 - success
 *          1 - error
 */
int do_remove(int n, char *old)
{
    int i, ret;

    /* int len; */
    char *mapset;
    int result = 0;
    int removed = 0;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    G_message(_("Removing %s <%s>"), list[n].maindesc, old);

    /* len = get_description_len(n); */

    hold_signals(1);

    if (G__name_is_fully_qualified(old, xname, xmapset)) {
	if (strcmp(xmapset, G_mapset()) != 0)
	    G_fatal_error("%s is not in the current mapset (%s)", old,
			  G_mapset());
	old = xname;
    }

    if (G_strcasecmp(list[n].alias, "vect") == 0) {
	if ((mapset = G_find_vector2(old, "")) == NULL) {
	    G_warning(_("Vector map <%s> not found"), old);
	}
	else {
	    ret = Vect_delete(old);
	    if (ret != -1) {
		removed = 1;
	    }
	    else {
		G_warning(_("couldn't be removed"));
		result = 1;
	    }
	}
    }
    else {
	if (G_strcasecmp(list[n].alias, "rast") == 0) {
	    if ((mapset = G_find_cell2(old, "")) == NULL)
		G_warning(_("Raster map <%s> not found"), old);
	}

	if (G_strcasecmp(list[n].alias, "rast3d") == 0) {
	    if ((mapset = G_find_grid3(old, "")) == NULL)
		G_warning(_("3D raster map <%s> not found"), old);
	}

	for (i = 0; i < list[n].nelem; i++) {

	    switch (G_remove(list[n].element[i], old)) {
	    case -1:
		G_warning(_("%s: couldn't be removed"), list[n].desc[i]);
		result = 1;
		break;
	    case 0:
		if (G_verbose() == G_verbose_max())
		    G_message(_("%s: missing"), list[n].desc[i]);
		break;
	    case 1:
		if (G_verbose() == G_verbose_max())
		    G_message(_("%s: removed"), list[n].desc[i]);
		removed = 1;
		break;
	    }
	}
    }

    if (G_strcasecmp(list[n].element[0], "cell") == 0) {
	char colr2[50];

	sprintf(colr2, "colr2/%s", G_mapset());
	switch (G_remove(colr2, old)) {
	case -1:
	    G_warning("%s: %s", colr2, _("couldn't be removed"));
	    result = 1;
	    break;
	case 0:
	    if (G_verbose() == G_verbose_max())
		G_message(_("%s: missing"), colr2);
	    break;
	case 1:
	    if (G_verbose() == G_verbose_max())
		G_message(_("%s: removed"), colr2);
	    removed = 1;
	    break;
	}
    }

    hold_signals(0);

    if (!removed)
	G_warning(_("<%s> nothing removed"), old);

    return result;
}
