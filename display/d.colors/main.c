
/****************************************************************************
 *
 * MODULE:       d.colors
 * AUTHOR(S):    Jim Westervelt (CERL) (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>
 * PURPOSE:      Enables the user to change color table of raster map layers
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/display.h>
#include "colors.h"

int main(int argc, char **argv)
{
    char name[128] = "";
    struct Option *map;
    struct GModule *module;
    char *mapset;
    char buff[500];

    /* must run in a term window */
    G_putenv("GRASS_UI_TERM", "1");

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("raster"));
    module->description =
	"Allows the user to interactively change the color table "
	"of a raster map layer displayed on the graphics monitor.";

    map = G_define_option();
    map->key = "map";
    map->type = TYPE_STRING;
    if (*name)
	map->answer = name;
    if (*name)
	map->required = NO;
    else
	map->required = YES;
    map->gisprompt = "old,cell,raster";
    map->description = "Name of raster map";

    if (G_parser(argc, argv))
	exit(1);

    /* Make sure map is available */
    if (map->answer == NULL)
	exit(0);
    mapset = G_find_raster2(map->answer, "");
    if (mapset == NULL) {
	char msg[256];

	sprintf(msg, "Raster file [%s] not available", map->answer);
	G_fatal_error(msg);
    }

    if (Rast_map_is_fp(map->answer, mapset)) {
	sprintf(buff,
		"Raster file [%s] is floating point! \nd.colors only works with integer maps",
		map->answer);
	G_fatal_error(buff);
    }

    /* connect to the driver */
    if (R_open_driver() != 0)
	G_fatal_error("No graphics device selected");

    /* Read in the map region associated with graphics window */
    D_setup(0);

    get_map_info(map->answer, mapset);

    R_close_driver();
    exit(0);
}
