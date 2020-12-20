#include <ctype.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

char *construct_pattern(char **names)
{
    char *pattern, *p;
    int i, j, len, found_illegal_names;
    const char *subproject;
    char xname[GNAME_MAX], xsubproject[GMAPSET_MAX];

    len = 0;
    for (i = 0; names[i]; i++) {
	if ((p = strchr(names[i], '@')))
	    len += p - names[i];
	else
	    len += strlen(names[i]);

	/* make room for escaping special characters */
	for (j = 0; names[i][j]; j++)
	    len += !isalnum(names[i][j]);
    }
    len += i; /* # names - 1 commas + \0 */

    pattern = p = (char *)G_malloc(len);

    subproject = G_subproject();
    found_illegal_names = 0;

    for (i = 0; names[i]; i++) {
	char *name;

	name = names[i];
	if (G_name_is_fully_qualified(name, xname, xsubproject)) {
	    if (strcmp(xsubproject, subproject) != 0)
		G_fatal_error(_("%s: Cannot remove or exclude files not in "
				"the current subproject."), name);
	    name = xname;
	}

	if (G_legal_filename(name) == -1)
	    found_illegal_names = 1;

	if (i)
	    *p++ = ',';

	for (j = 0; name[j]; j++) {
	    if (!isalnum(name[j]))
		*p++ = '\\';
	    *p++ = name[j];
	}
    }
    *p = '\0';

    if (found_illegal_names)
	G_fatal_error(_("Illegal filenames not allowed in the name or ignore "
			"option."));

    return pattern;
}
