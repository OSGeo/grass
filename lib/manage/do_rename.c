#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/G3d.h>
#include "list.h"

/*
 *  returns 0 - success
 *          1 - error
 */
int do_rename(int n, char *old, char *new)
{
    int i, ret;
    int len;
    char *mapset;
    int result = 0;
    int renamed = 0;

    /* verbosity: --quiet is completely quiet.
       GRASS_VERBOSE=1 shows only map names.
       normal and --verbose show map names and elements. */
    /* there should be used G_message or other derived fn */
    if (G_verbose() > G_verbose_min())
	fprintf(stderr, _("Rename %s <%s> to <%s>\n"),
		list[n].maindesc, old, new);

    if (G_strcasecmp(old, new) == 0)
	return 1;

    len = get_description_len(n);

    hold_signals(1);

    if (G_strcasecmp(list[n].alias, "vect") == 0) {
	if ((mapset = G_find_vector2(old, "")) == NULL) {
	    G_warning(_("Vector map <%s> not found"), old);
	}
	else {
	    ret = Vect_rename(old, new);
	    if (ret != -1) {
		renamed = 1;
	    }
	    else {
		G_warning(_("Cannot rename <%s> to <%s>"), old, new);
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
	    G_remove(list[n].element[i], new);
	    switch (G_rename(list[n].element[i], old, new)) {
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
		    G_message(_("%s: renamed"), list[n].desc[i]);
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
		G_warning(_("%s: couldn't be renamed"), colr2);
		result = 1;
		break;
	    case 0:
		if (G_verbose() == G_verbose_max())
		    G_message(_("%s: missing"), colr2);
		break;
	    case 1:
		if (G_verbose() == G_verbose_max())
		    G_message(_("%s: renamed"), colr2);
		renamed = 1;
		break;
	    }
	}
    }
    hold_signals(0);

    if (!renamed)
	G_warning(_("<%s> nothing renamed"), old);

    return result;
}
