/*!
  \file lib/gis/parser_script.c
  
  \brief GIS Library - Argument parsing functions (script)
  
  (C) 2001-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
  \author Soeren Gebbert added Dec. 2009 WPS process_description document
*/

#include <stdio.h>

#include <grass/gis.h>

#include "parser_local_proto.h"

/*!
  \brief Generate Python script-like output
*/
void G__script(void)
{
    FILE *fp = stdout;
    char *type;

    fprintf(fp,
	    "#!/usr/bin/env python\n");
    fprintf(fp,
	    "############################################################################\n");
    fprintf(fp, "#\n");
    fprintf(fp, "# MODULE:       %s_wrapper\n", G_program_name());
    fprintf(fp, "# AUTHOR(S):    %s\n", G_whoami());
    fprintf(fp, "# PURPOSE:      Wrapper for %s\n", G_program_name());
    fprintf(fp, "# COPYRIGHT:    (C) %s by %s, and the GRASS Development Team\n",
	    GRASS_VERSION_DATE, G_whoami());
    fprintf(fp, "#\n");
    fprintf(fp,
	    "#  This program is free software; you can redistribute it and/or modify\n");
    fprintf(fp,
	    "#  it under the terms of the GNU General Public License as published by\n");
    fprintf(fp,
	    "#  the Free Software Foundation; either version 2 of the License, or\n");
    fprintf(fp, "#  (at your option) any later version.\n");
    fprintf(fp, "#\n");
    fprintf(fp,
	    "#  This program is distributed in the hope that it will be useful,\n");
    fprintf(fp,
	    "#  but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    fprintf(fp,
	    "#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    fprintf(fp, "#  GNU General Public License for more details.\n");
    fprintf(fp, "#\n");
    fprintf(fp,
	    "############################################################################\n\n");

    fprintf(fp, "#%%module\n");
    if (st->module_info.label)
	fprintf(fp, "#%% label: %s\n", st->module_info.label);
    if (st->module_info.description)
	fprintf(fp, "#%% description: %s\n", st->module_info.description);
    if (st->module_info.keywords) {
	fprintf(fp, "#%% keywords: ");
	G__print_keywords(fp, NULL);
	fprintf(fp, "\n");
    }
    fprintf(fp, "#%%end\n");

    if (st->n_flags) {
	struct Flag *flag;

	for (flag = &st->first_flag; flag; flag = flag->next_flag) {
	    fprintf(fp, "#%%flag\n");
	    fprintf(fp, "#%% key: %c\n", flag->key);
	    if (flag->suppress_required)
		fprintf(fp, "#%% suppress_required: yes\n");
	    if (flag->label)
		fprintf(fp, "#%% label: %s\n", flag->label);
	    if (flag->description)
		fprintf(fp, "#%% description: %s\n", flag->description);
	    if (flag->guisection)
		fprintf(fp, "#%% guisection: %s\n", flag->guisection);
	    fprintf(fp, "#%%end\n");
	}
    }

    if (st->n_opts) {
	struct Option *opt;

	for (opt = &st->first_option; opt; opt = opt->next_opt) {
	    switch (opt->type) {
	    case TYPE_INTEGER:
		type = "integer";
		break;
	    case TYPE_DOUBLE:
		type = "double";
		break;
	    case TYPE_STRING:
		type = "string";
		break;
	    default:
		type = "string";
		break;
	    }

	    fprintf(fp, "#%%option\n");
	    fprintf(fp, "#%% key: %s\n", opt->key);
	    fprintf(fp, "#%% type: %s\n", type);
	    fprintf(fp, "#%% required: %s\n", opt->required ? "yes" : "no");
	    fprintf(fp, "#%% multiple: %s\n", opt->multiple ? "yes" : "no");
	    if (opt->options)
		fprintf(fp, "#%% options: %s\n", opt->options);
	    if (opt->key_desc)
		fprintf(fp, "#%% key_desc: %s\n", opt->key_desc);
	    if (opt->label)
		fprintf(fp, "#%% label: %s\n", opt->label);
	    if (opt->description)
		fprintf(fp, "#%% description: %s\n", opt->description);
	    if (opt->descriptions)
		fprintf(fp, "#%% descriptions: %s\n", opt->descriptions);
	    if (opt->answer)
		fprintf(fp, "#%% answer: %s\n", opt->answer);
	    if (opt->gisprompt)
		fprintf(fp, "#%% gisprompt: %s\n", opt->gisprompt);
	    if (opt->guisection)
		fprintf(fp, "#%% guisection: %s\n", opt->guisection);
	    if (opt->guidependency)
		fprintf(fp, "#%% guidependency: %s\n", opt->guidependency);
	    fprintf(fp, "#%%end\n");
	}
    }

    fprintf(fp, "\nimport sys\n");
    fprintf(fp, "\nimport grass.script as grass\n");
    fprintf(fp, "\ndef main():");
    fprintf(fp, "\n    # put code here\n");
    fprintf(fp, "\n    return 0\n");
    fprintf(fp, "\nif __name__ == \"__main__\":");
    fprintf(fp, "\n    options, flags = grass.parser()");
    fprintf(fp, "\n    sys.exit(main())\n");
}
