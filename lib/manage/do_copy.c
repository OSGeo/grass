#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include "list.h"

/*
 *  returns 0 - success
 *          1 - error
 */
int do_copy(int n, const char *old, const char *mapset, const char *new)
{
    int i, ret, len;
    char path[GPATH_MAX], path2[GPATH_MAX];
    int result = 0;

    G_debug(3, "Copy %s", list[n].alias);

    G_message(_("Copy %s <%s> to current mapset as <%s>"),
	      list[n].maindesc, G_fully_qualified_name(old, mapset), new);

    len = get_description_len(n);

    hold_signals(1);
    if (G_strcasecmp(list[n].alias, "vect") == 0) {
	ret = Vect_copy(old, mapset, new);
	if (ret == -1) {
	    G_warning("Cannot copy <%s> to current mapset as <%s>",
		      G_fully_qualified_name(old, mapset), new);
	    result = 1;
	}
    }
    else {
	for (i = 0; i < list[n].nelem; i++) {
	    G__make_mapset_element(list[n].element[i]);
	    G_file_name(path, list[n].element[i], old, mapset);
	    if (access(path, 0) != 0) {
		G_remove(list[n].element[i], new);
		if (G_verbose() == G_verbose_max())
		    G_message(_("%s: missing"), list[n].desc[i]);

		continue;
	    }
	    G_file_name(path2, list[n].element[i], new, G_mapset());
	    if (G_recursive_copy(path, path2) == 1) {
		result = 1;
	    }
	    else {
		if (G_verbose() == G_verbose_max())
		    G_message(_("%s: copied"), list[n].desc[i]);
	    }
	}
    }

    /* special case: remove (yes, remove) the secondary color table, if it exists */
    if (G_strcasecmp(list[n].element[0], "cell") == 0) {
	char colr2[GNAME_MAX];

	sprintf(colr2, "colr2/%s", G_mapset());
	G_remove(colr2, new);
    }
    hold_signals(0);

    return result;
}
