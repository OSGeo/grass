/*
**********************************************************************
*
* MODULE:        r.support.stats
*
* AUTHOR(S):     Brad Douglas <rez touchofmadness com>
*
* PURPOSE:       Update raster statistics
*
* COPYRIGHT:     (C) 2006 by the GRASS Development Team
*
*                This program is free software under the GNU General
*                Purpose License (>=v2). Read the file COPYING that
*                comes with GRASS for details.
*
***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"


int main (int argc, char *argv[])
{
    char *mapset;
    struct GModule *module;
    struct {
        struct Option *raster;
    } parm;

    /* Initialize GIS engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->description = _("Update raster map statistics");

    parm.raster = G_define_standard_option(G_OPT_R_MAP);

    /* parse command-line options */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    mapset = G_find_cell2(parm.raster->answer, G_mapset());
    if (mapset == NULL)
        G_fatal_error(_("Raster map <%s> not found"), parm.raster->answer);

    check_stats(parm.raster->answer, mapset);

    G_message(_("Statistics for <%s> updated"), parm.raster->answer);

    return EXIT_SUCCESS;
}
