
/****************************************************************************
 *
 * MODULE:       r3.timestamp
 * AUTHOR(S):    Michael Pelizzari <michael.pelizzari lmco.com> 
 *                     (original contributor)
 *               Glynn Clements <glynn gclements.plus.com> Markus Neteler <neteler itc.it>
 * PURPOSE:      Stamps grid3 files with date and time.  
 * COPYRIGHT:    (C) 2001-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* based on r.timestamp by Michael Shapiro and v.timestamp by Markus Neteler:
 * 
 * Stamps grid3 files with date and time.  This main.c is linked to functions 
 * currently residing in lib/gis/timestamp.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Option *map, *date;
    struct TimeStamp ts;
    char *name;
    char *mapset;
    int modify;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("voxel"));
    module->description =
	_("Print/add/remove a timestamp for a 3D raster map");

    map = G_define_option();
    map->key = "map";
    map->required = YES;
    map->type = TYPE_STRING;
    map->gisprompt = "old,grid3,3d raster";
    map->description = _("Input grid3 filename");

    date = G_define_option();
    date->key = "date";
    date->key_desc = "timestamp";
    date->required = NO;
    date->type = TYPE_STRING;
    date->description = _("Datetime, datetime1/datetime2, or none");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = map->answer;

    modify = date->answer != NULL;

    if (modify)
	mapset = G_find_grid3(name, G_mapset());
    else
	mapset = G_find_grid3(name, "");

    if (mapset == NULL) {
	G_fatal_error(_("Grid3 <%s> not found %s"), name,
		      modify ? "in current mapset" : "");
	exit(EXIT_FAILURE);
    }

    if (!modify) {
	if (G_read_grid3_timestamp(name, mapset, &ts) == 1) {
	    G__write_timestamp(stdout, &ts);
	    exit(EXIT_SUCCESS);
	}
	else
	    exit(EXIT_FAILURE);
    }
    if (strcmp(date->answer, "none") == 0) {
	G_remove_grid3_timestamp(name);
	exit(EXIT_SUCCESS);
    }

    G_scan_timestamp(&ts, date->answer);
    G_write_grid3_timestamp(name, &ts);
    exit(EXIT_SUCCESS);
}
