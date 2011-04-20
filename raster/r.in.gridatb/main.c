/*
 * r.in.gridatb: Imports GRIDATB.FOR map file (TOPMODEL) into GRASS raster map
 *
 * GRIDATB.FOR Author: Keith Beven <k.beven@lancaster.ac.uk>
 *
 *      Copyright (C) 2000 by the GRASS Development Team
 *      Author: Huidae Cho <grass4u@gmail.com>
 *              Hydro Laboratory, Kyungpook National University
 *              South Korea
 *
 *      This program is free software under the GPL (>=v2)
 *      Read the file COPYING coming with GRASS for details.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "local_proto.h"

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

struct Cell_head cellhd;
FCELL *cell;
const char *file;
const char *mapset, *oname;

int main(int argc, char **argv)
{
    struct
    {
	struct Option *input;
	struct Option *output;
    } params;

    struct GModule *module;


    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    module->description =
	_("Imports GRIDATB.FOR map file (TOPMODEL) into GRASS raster map");

    params.input = G_define_option();
    params.input->key = "input";
    params.input->description = _("GRIDATB i/o map file");
    params.input->type = TYPE_STRING;
    params.input->required = YES;

    params.output = G_define_option();
    params.output->key = "output";
    params.output->description = _("Name for output raster map");
    params.output->type = TYPE_STRING;
    params.output->required = YES;
    params.output->gisprompt = "new,cell,raster";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    file = params.input->answer;
    oname = params.output->answer;

    mapset = G_mapset();

    if (check_ready())
	G_fatal_error(_("File not found: %s"), file);


    rdwr_gridatb();

    exit(EXIT_SUCCESS);
}
