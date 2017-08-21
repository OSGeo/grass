
/****************************************************************************
 *
 * MODULE:       r.transect
 * AUTHOR(S):    Michael Shapiro (CERL) (original contributor),
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      This program outputs, in ASCII, the values in a raster map
 *               which lie along one or more user-defined transect lines.
 *               The transects are described by their starting coordinates,
 *               azimuth, and distance. 
 * COPYRIGHT:    (C) 1999-2006,2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include "local_proto.h"
#include <grass/glocale.h>
#include <grass/spawn.h>

static int profile(int coords, const char *map, const char *nulls, char **line)
{
    double e1, n1, e2, n2;
    char buf[1024], profile[1024];
    const char *argv[7];
    int argc = 0;
    int n;
    int projection;

    projection = G_projection();

    argv[argc++] = "r.profile";

    if (coords)
	argv[argc++] = "-g";

    sprintf(buf, "input=%s", map);
    argv[argc++] = G_store(buf);

    argv[argc++] = "output=-";

    sprintf(buf, "null_value=%s", nulls);
    argv[argc++] = G_store(buf);

    strcpy(profile, "coordinates=");
    for (n = 0; line[n]; n += 4) {
	int err = parse_line("line", &line[n], &e1, &n1, &e2, &n2, projection);

	if (err) {
	    G_usage();
	    exit(EXIT_FAILURE);
	}

	if (n > 0)
	    strcat(profile, ",");
	G_format_easting(e1, buf, projection);
	strcat(profile, buf);

	G_format_northing(n1, buf, projection);
	strcat(profile, ",");
	strcat(profile, buf);

	G_format_easting(e2, buf, projection);
	strcat(profile, ",");
	strcat(profile, buf);

	G_format_northing(n2, buf, projection);
	strcat(profile, ",");
	strcat(profile, buf);
    }

    argv[argc++] = profile;

    argv[argc++] = NULL;

    G_verbose_message(_("End coordinate: %.15g, %.15g"), e2, n2);

    return G_vspawn_ex(argv[0], argv);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *map;
	struct Option *line;
	struct Option *null_str;
    } parms;
    struct Flag *coord;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("profile"));
    G_add_keyword(_("transect"));
    module->description =
	_("Outputs raster map layer values lying along "
	  "user defined transect line(s).");

    parms.map = G_define_standard_option(G_OPT_R_MAP);
    parms.map->description = _("Raster map to be queried");

    parms.line = G_define_option();
    parms.line->key = "line";
    parms.line->key_desc = "east,north,azimuth,distance";
    parms.line->type = TYPE_STRING;
    parms.line->description = _("Transect definition");
    parms.line->required = YES;
    parms.line->multiple = YES;

    parms.null_str = G_define_standard_option(G_OPT_M_NULL_VALUE);
    parms.null_str->answer = "*";

    coord = G_define_flag();
    coord->key = 'g';
    coord->description =
	_("Output easting and northing in first two columns of four column output");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    return profile(coord->answer,
		   parms.map->answer,
		   parms.null_str->answer,
		   parms.line->answers) != 0;
}
