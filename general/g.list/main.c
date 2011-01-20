
/****************************************************************************
 *
 * MODULE:       g.list
 *               
 * AUTHOR(S):    Michael Shapiro,
 *               U.S.Army Construction Engineering Research Laboratory
 *               Some updates by various authors from GRASS Development Team
 *               
 * PURPOSE:      Lists available GRASS data base elements of the user-specified data type to
 *               standard output
 *
 * COPYRIGHT:    (C) 1999-2009, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include <grass/glocale.h>
#include <grass/manage.h>

struct Option *element;

int main(int argc, char *argv[])
{
    int i, n, nlist;
    struct GModule *module;
    struct Option *mapset_opt;
    struct Flag *full;
    const char *mapset;

    G_gisinit(argv[0]);

    M_read_list(0, &nlist);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("list"));
    module->description =
	_("Lists available GIS elements "
	  "of the user-specified data type.");

    element = G_define_standard_option(G_OPT_M_DATATYPE);
    element->options = M_get_options(TRUE);
    element->descriptions = M_get_option_desc(TRUE);
    
    mapset_opt = G_define_standard_option(G_OPT_M_MAPSET);
    
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
	n = M_get_element(element->answers[i]);

	if (full->answer) {
	    char lister[GPATH_MAX];

	    sprintf(lister, "%s/etc/lister/%s", G_gisbase(),
		    M_get_list(n)->element[0]);
	    G_debug(3, "lister CMD: %s", lister);
	    if (access(lister, 1) == 0)	/* execute permission? */
		G_spawn(lister, lister, mapset, NULL);
	    else
		M_do_list(n, mapset);
	}
	else {
	    M_do_list(n, mapset);
	}

	i++;
    }

    exit(EXIT_SUCCESS);
}
