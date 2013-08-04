
/****************************************************************************
 *
 * MODULE:       g.findfile
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jan-Oliver Wagner <jan intevation.de>
 *               Martin landa <landa.martin gmail.com>
 * PURPOSE:      Searches for GRASS data base files
 * COPYRIGHT:    (C) 1999-2008, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    char file[GPATH_MAX], name[GNAME_MAX];
    const char *search_mapset, *mapset;
    struct GModule *module;
    struct Option *elem_opt;
    struct Option *mapset_opt;
    struct Option *file_opt;
    struct Flag *n_flag, *l_flag;

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    module->description =
	_("Searches for GRASS data base files "
	  "and sets variables for the shell.");

    G_gisinit(argv[0]);

    /* Define the different options */

    elem_opt = G_define_option();
    elem_opt->key = "element";
    elem_opt->type = TYPE_STRING;
    elem_opt->required = YES;
    elem_opt->description = _("Name of an element");

    file_opt = G_define_option();
    file_opt->key = "file";
    file_opt->type = TYPE_STRING;
    file_opt->required = YES;
    file_opt->description = _("Name of an existing map");

    mapset_opt = G_define_option();
    mapset_opt->key = "mapset";
    mapset_opt->type = TYPE_STRING;
    mapset_opt->required = NO;
    mapset_opt->label = _("Name of a mapset (default: search path)");
    mapset_opt->description = _("'.' for current mapset");

    n_flag = G_define_flag();
    n_flag->key = 'n';
    n_flag->description = _("Don't add quotes");

    l_flag = G_define_flag();
    l_flag->key = 'l';
    l_flag->description = _("List available elements and exit");
    l_flag->suppress_required = YES;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (l_flag->answer) {
	list_elements();
	return EXIT_SUCCESS;
    }

    search_mapset = mapset_opt->answer;
    if (!search_mapset) {
	search_mapset = G_store("");
    }
    if (strcmp(".", search_mapset) == 0)
	search_mapset = G_mapset();
    
    if (mapset_opt->answer && strlen(mapset_opt->answer) > 0) {
	char **map_mapset = G_tokenize(file_opt->answer, "@");

	if (G_number_of_tokens(map_mapset) > 1) {
	    if (strcmp(map_mapset[1], mapset_opt->answer))
		G_fatal_error(_("Parameter 'file' contains reference to <%s> mapset, "
				"but mapset parameter <%s> does not correspond"),
			      map_mapset[1], mapset_opt->answer);
	    else
		strcpy(name, file_opt->answer);
	}
	if (G_number_of_tokens(map_mapset) == 1)
	    strcpy(name, file_opt->answer);
	G_free_tokens(map_mapset);
    }
    else
	strcpy(name, file_opt->answer);

    mapset = G_find_file2(elem_opt->answer, name, search_mapset);
    if (mapset) {
	char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
	const char *qchar = n_flag->answer ? "" : "'";
	const char *qual = G_fully_qualified_name(name, mapset);
	G_unqualified_name(name, mapset, xname, xmapset);
	G_file_name(file, elem_opt->answer, name, mapset);
	fprintf(stdout, "name=%s%s%s\n", qchar, xname, qchar);
	fprintf(stdout, "mapset=%s%s%s\n", qchar, xmapset, qchar);
	fprintf(stdout, "fullname=%s%s%s\n", qchar, qual, qchar);
	fprintf(stdout, "file=%s%s%s\n", qchar, file, qchar);

	return EXIT_SUCCESS;
    }
    else {
	fprintf(stdout, "name=\n");
	fprintf(stdout, "mapset=\n");
	fprintf(stdout, "fullname=\n");
	fprintf(stdout, "file=\n");
    }
    
    return EXIT_FAILURE;
}
