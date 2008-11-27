
/****************************************************************************
 *
 * MODULE:       g.list
 *               
 * AUTHOR(S):    Michael Shapiro,
 *               U.S.Army Construction Engineering Research Laboratory
 *               
 * PURPOSE:      Lists available GRASS data base files of the
 *               user-specified data type to standard output
 *
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/spawn.h>
#include <grass/list.h>

struct Option *element;

int parse(const char *data_type);

int main(int argc, char *argv[])
{
    int i, n, len;
    struct GModule *module;
    struct Option *mapset_opt;
    struct Flag *full;
    const char *mapset;
    char *str;

    G_gisinit(argv[0]);

    read_list(0);

    module = G_define_module();
    module->keywords = _("general, map management");
    module->description =
	_("Lists available GRASS data base files "
	  "of the user-specified data type to standard output.");

    element = G_define_option();
    element->key = "type";
    element->key_desc = "datatype";
    element->type = TYPE_STRING;
    element->required = YES;
    element->multiple = YES;
    element->description = "Data type";

    for (len = 0, n = 0; n < nlist; n++)
	len += strlen(list[n].alias) + 1;
    str = G_malloc(len);

    for (n = 0; n < nlist; n++) {
	if (n) {
	    strcat(str, ",");
	    strcat(str, list[n].alias);
	}
	else
	    strcpy(str, list[n].alias);
    }
    element->options = str;

    mapset_opt = G_define_option();
    mapset_opt->key = "mapset";
    mapset_opt->type = TYPE_STRING;
    mapset_opt->required = NO;
    mapset_opt->multiple = NO;
    mapset_opt->description = _("Mapset to list (default: current search path)");

    full = G_define_flag();
    full->key = 'f';
    full->description = _("Verbose listing (also list map titles)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    mapset = mapset_opt->answer;
    if (!mapset)
	mapset = "";

    if (G_strcasecmp(mapset, ".") == 0)
	mapset = G_mapset();

    i = 0;
    while (element->answers[i]) {
	n = parse(element->answers[i]);

	if (full->answer) {
	    char lister[GPATH_MAX];

	    sprintf(lister, "%s/etc/lister/%s", G_gisbase(),
		    list[n].element[0]);
	    G_debug(3, "lister CMD: %s", lister);
	    if (access(lister, 1) == 0)	/* execute permission? */
		G_spawn(lister, lister, mapset, NULL);
	    else
		do_list(n, mapset);
	}
	else {
	    do_list(n, mapset);
	}

	i++;
    }

    exit(EXIT_SUCCESS);
}

int parse(const char *data_type)
{
    int n;

    for (n = 0; n < nlist; n++) {
	if (G_strcasecmp(list[n].alias, data_type) == 0)
	    break;
    }

    return n;
}
