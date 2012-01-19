
/****************************************************************************
 *
 * MODULE:       v.timestamp
 *  
 * AUTHOR(S):    Soeren Gebbert <soerengebbert googlemail.com>
 *               based on r.timestamp from Michael Shapiro, CERL (original contributor)
 *
 * PURPOSE:      Print/add/remove a timestamp for a vector map
 * COPYRIGHT:    (C) 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *map, *date;
    struct TimeStamp ts;
    char *name;
    int modify;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("metadata"));
    G_add_keyword(_("timestamp"));
    module->label = _("Modifies a timestamp for a vector map.");
    module->description = _("Print/add/remove a timestamp for a vector map.");

    map = G_define_standard_option(G_OPT_V_MAP);

    date = G_define_option();
    date->key = "date";
    date->key_desc = "timestamp";
    date->required = NO;
    date->type = TYPE_STRING;
    date->label = _("Datetime, datetime1/datetime2, or 'none' to remove");
    date->description = _("Format: '15 jan 1994' (absolute) or '2 years' (relative)");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = map->answer;

    modify = date->answer != NULL;

    if (!modify) {
	if (G_read_vector_timestamp(name, "", &ts) == 1) {
	    G__write_timestamp(stdout, &ts);
	    exit(EXIT_SUCCESS);
	}
	else
	    exit(EXIT_FAILURE);
    }
    if (strcmp(date->answer, "none") == 0) {
	G_remove_vector_timestamp(name);
	exit(EXIT_SUCCESS);
    }

    if (1 == G_scan_timestamp(&ts, date->answer)) {
	G_write_vector_timestamp(name, &ts);
	exit(EXIT_SUCCESS);
    }
    else
	G_fatal_error(_("Invalid timestamp"));

    exit(EXIT_SUCCESS);
}
