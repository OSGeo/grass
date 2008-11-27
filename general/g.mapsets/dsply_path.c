#include <string.h>
#include "externs.h"
#include <grass/gis.h>

int display_mapset_path(int verbose)
{
    int n;
    int map;			/* pointer into list of available mapsets */
    int offset = 6;		/* accounts for " <x>, " */
    const char *name;
    int len;
    int nleft;

    if (verbose) {
	/* account for largest mapset number in offset value */
	for (n = nmapsets; n /= 10; offset++) ;

	fprintf(stdout, "Your mapset search list:\n");
	ncurr_mapsets = 0;
    }

    nleft = 78;
    for (n = 0; name = G__mapset_name(n); n++) {
	/* match each mapset to its numeric equivalent */
	if (verbose) {
	    for (map = 0; map < nmapsets && strcmp(mapset_name[map], name);
		 map++) ;
	    if (map == nmapsets)
		G_fatal_error("%s not found in mapset list:  call greg",
			      name);
	}

	len = strlen(name);
	if (len > nleft) {
	    fprintf(stdout, "\n");
	    nleft = 78;
	}

	if (verbose) {
	    if (n)
		fprintf(stdout, ", ");
	    fprintf(stdout, "%s <%d>", name, map + 1);
	    nleft -= (len + offset);
	    curr_mapset[n] = map;
	    ++ncurr_mapsets;
	}
	else {
	    fprintf(stdout, "%s ", name);
	    nleft -= (len + 1);
	}
    }
    fprintf(stdout, "\n");
    if (verbose)
	fprintf(stdout, "\n");

    return 0;
}
