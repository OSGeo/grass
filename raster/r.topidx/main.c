
/****************************************************************************
 *
 * MODULE:       r.topidx
 *
 * AUTHOR(S):    Keith Beven <k.beven lancaster.ac.uk>,
 *               Huidae Cho <grass4u gmail.com>, Hydro Laboratory,
 *               Kyungpook National University
 *
 * PURPOSE:      Creates topographic index map from elevation map.
 *               Based on GRIDATB.FOR.
 *
 * COPYRIGHT:    (C) 2000-2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define _MAIN_C_
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "global.h"

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *input;
	struct Option *output;
    } params;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    module->description =
	_("Creates topographic index map from elevation raster map.");

    params.input = G_define_standard_option(G_OPT_R_ELEV);
    params.input->key = "input";

    params.output = G_define_standard_option(G_OPT_R_OUTPUT);
    params.output->description = _("Name for output topographic index map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Make sure that the current projection is not lat/long */
    if ((G_projection() == PROJECTION_LL))
	G_fatal_error(_("Lat/Long location is not supported by %s. Please reproject map first."),
		      G_program_name());

    input = params.input->answer;
    output = params.output->answer;

    G_get_window(&window);

    read_cells();
    initialize();
    calculate_atanb();
    write_cells();

    exit(EXIT_SUCCESS);
}
