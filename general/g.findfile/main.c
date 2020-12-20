
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
    const char *search_subproject, *subproject;
    struct GModule *module;
    struct Option *elem_opt;
    struct Option *subproject_opt;
    struct Option *file_opt;
    struct Flag *n_flag, *l_flag;

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("scripts"));
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

    subproject_opt = G_define_option();
    subproject_opt->key = "subproject";
    subproject_opt->type = TYPE_STRING;
    subproject_opt->required = NO;
    subproject_opt->label = _("Name of a subproject (default: search path)");
    subproject_opt->description = _("'.' for current subproject");

    n_flag = G_define_flag();
    n_flag->key = 'n';
    n_flag->description = _("Do not add quotes");

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

    search_subproject = subproject_opt->answer;
    if (!search_subproject) {
	search_subproject = G_store("");
    }
    if (strcmp(".", search_subproject) == 0)
	search_subproject = G_subproject();
    
    if (subproject_opt->answer && strlen(subproject_opt->answer) > 0) {
	char **map_subproject = G_tokenize(file_opt->answer, "@");

	if (G_number_of_tokens(map_subproject) > 1) {
	    if (strcmp(map_subproject[1], subproject_opt->answer))
		G_fatal_error(_("Parameter 'file' contains reference to <%s> subproject, "
				"but subproject parameter <%s> does not correspond"),
			      map_subproject[1], subproject_opt->answer);
	    else
		strcpy(name, file_opt->answer);
	}
	if (G_number_of_tokens(map_subproject) == 1)
	    strcpy(name, file_opt->answer);
	G_free_tokens(map_subproject);
    }
    else
	strcpy(name, file_opt->answer);

    subproject = G_find_file2(elem_opt->answer, name, search_subproject);
    if (subproject) {
	char xname[GNAME_MAX], xsubproject[GMAPSET_MAX];
	const char *qchar = n_flag->answer ? "" : "'";
	const char *qual = G_fully_qualified_name(name, subproject);
	G_unqualified_name(name, subproject, xname, xsubproject);
	G_file_name(file, elem_opt->answer, name, subproject);
	fprintf(stdout, "name=%s%s%s\n", qchar, xname, qchar);
	fprintf(stdout, "subproject=%s%s%s\n", qchar, xsubproject, qchar);
	fprintf(stdout, "fullname=%s%s%s\n", qchar, qual, qchar);
	fprintf(stdout, "file=%s%s%s\n", qchar, file, qchar);

	return EXIT_SUCCESS;
    }
    else {
	fprintf(stdout, "name=\n");
	fprintf(stdout, "subproject=\n");
	fprintf(stdout, "fullname=\n");
	fprintf(stdout, "file=\n");
    }
    
    return EXIT_FAILURE;
}
