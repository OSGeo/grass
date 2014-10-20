#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

char *construct_pattern(char **names)
{
    char *pattern, *p;
    int i, len, found_illegal_names;
    const char *mapset;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    len = 0;
    for (i = 0; names[i]; i++) {
	if ((p = strchr(names[i], '@')))
	    len += p - names[i];
	else
	    len += strlen(names[i]);
    }
    len += i; /* # names - 1 commas + \0 */

    pattern = p = (char *)G_malloc(len);

    mapset = G_mapset();
    found_illegal_names = 0;

    for (i = 0; names[i]; i++) {
	char *name;

	name = names[i];
	if (G_name_is_fully_qualified(name, xname, xmapset)) {
	    if (strcmp(xmapset, mapset) != 0)
		G_fatal_error(_("%s: Cannot remove or exclude files not in "
				"the current mapset."), name);
	    name = xname;
	}

	if (G_legal_filename(name) == -1)
	    found_illegal_names = 1;

	if (i)
	    *p++ = ',';
	strcpy(p, name);
	p += strlen(name);
    }

    if (found_illegal_names)
	G_fatal_error(_("Illegal filenames not allowed in the names or ignore "
			"option."));

    return pattern;
}
