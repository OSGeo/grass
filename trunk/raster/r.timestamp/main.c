
/****************************************************************************
 *
 * MODULE:       r.timestamp
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, Roberto Flor <flor itc.it>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>, Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006, 2010 by the GRASS Development Team
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
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *map, *date;
    struct TimeStamp ts;
    char *name;
    const char *mapset;
    int modify;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("metadata"));
    G_add_keyword(_("timestamp"));
    G_add_keyword(_("time"));
    module->label = _("Modifies a timestamp for a raster map.");
    module->description = _("Print/add/remove a timestamp for a raster map.");

    map = G_define_standard_option(G_OPT_R_MAP);

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

    if (modify)
	mapset = G_find_raster(name, G_mapset());
    else
	mapset = G_find_raster(name, "");

    if (mapset == NULL) {
	G_fatal_error(_("Raster map <%s> not found %s"), name,
		      modify ? "in current mapset" : "");
	exit(EXIT_FAILURE);
    }

    if (!modify) {
	if (G_read_raster_timestamp(name, "", &ts) == 1) {
	    G_write_timestamp(stdout, &ts);
	    exit(EXIT_SUCCESS);
	}
	else
	    exit(EXIT_FAILURE);
    }
    if (strcmp(date->answer, "none") == 0) {
	G_remove_raster_timestamp(name);
	exit(EXIT_SUCCESS);
    }

    if(G_scan_timestamp(&ts, date->answer) != 1)
        G_fatal_error("Timestamp format is invalid");

    G_write_raster_timestamp(name, &ts);

    exit(EXIT_SUCCESS);
}
