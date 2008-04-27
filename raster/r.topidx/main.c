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
 * COPYRIGHT:    (C) 2000-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define	MAIN
#include <grass/glocale.h>
#include "global.h"

int
main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct	Option	*input;
	struct	Option	*output;
    } params;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Creates topographic index [ln(a/tan(beta))] map from elevation map.");

    params.input		= G_define_standard_option(G_OPT_R_INPUT);
    params.input->description	= _("Input elevation map");

    params.output		= G_define_standard_option(G_OPT_R_OUTPUT);
    params.output->key		= "output";
    params.output->description	= _("Output topographic index map");

    if(G_parser(argc, argv)) {
	exit(EXIT_FAILURE);
    }

    /* Make sure that the current projection is not lat/long */
    if ((G_projection() == PROJECTION_LL))
	G_fatal_error (_("Lat/Long location is not supported by %s. Please reproject map first."),
		       G_program_name());
    
    iname   = params.input->answer;
    mapset  = G_find_cell2 (iname, "");
    oname   = params.output->answer;

    if(check_ready())
	exit(EXIT_FAILURE);

    G_get_window(&window);

    getcells();
    initialize();
    atanb();
    putcells();

    exit(EXIT_SUCCESS);
}
