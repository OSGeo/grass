#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

int display_mapset_path(const char *fs)
{
    int n;
    int map;			/* pointer into list of available mapsets */
    int offset = 6;		/* accounts for " <x>, " */
    const char *name;
    int len;
    int nleft;

    if (!fs) {
	/* account for largest mapset number in offset value */
	for (n = nmapsets; n /= 10; offset++) ;

	fprintf(stdout, _("Your mapset search list:\n"));
    }

    nleft = 78;
    for (n = 0; (name = G__mapset_name(n)); n++) {
	/* match each mapset to its numeric equivalent */
	if (!fs) {
	    for (map = 0; map < nmapsets && strcmp(mapset_name[map], name);
		 map++) ;
	    if (map == nmapsets)
		G_fatal_error(_("<%s> not found in mapset list"),
			      name);
	}

	len = strlen(name);
	if (len > nleft) {
	    fprintf(stdout, "\n");
	    nleft = 78;
	}

	if (!fs) {
	    if (n)
		fprintf(stdout, ", ");
	    fprintf(stdout, "%s <%d>", name, map + 1);
	    nleft -= (len + offset);
	}
	else {
	    fprintf(stdout, "%s", name);
	    if (G__mapset_name(n+1))
		fprintf(stdout, "%s", fs);
	    nleft -= (len + 1);
	}
    }
    fprintf(stdout, "\n");
    if (!fs)
	fprintf(stdout, "\n");

    return 0;
}
