#include <grass/gis.h>
#include <grass/glocale.h>
#include "proto.h"

/* select monitor */
int select_mon(const char *name)
{
    const char *curr_mon;
    char **list;
    int   i, n, found;

    curr_mon = G_getenv_nofatal("MONITOR");
    if (G_strcasecmp(name, curr_mon) == 0) {
	G_warning(_("Monitor <%s> is already selected"), name);
	return 0;
    }

    list_mon(&list, &n);
    found = FALSE;
    for (i = 0; i < n; i++) {
	if (G_strcasecmp(list[i], name) == 0) {
	    found = TRUE;
	    break;
	}
    }
    
    if (found)
	G_setenv("MONITOR", name);
    else
	G_fatal_error(_("Monitor <%s> is not running"), name);
    
    return 0;
}
