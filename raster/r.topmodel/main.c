/*
 * r.topmodel: simulates TOPMODEL based on TMOD9502.FOR.
 *
 * TMOD9502.FOR Author: Keith Beven <k.beven@lancaster.ac.uk>
 *                      http://www.es.lancs.ac.uk/hfdg/topmodel.html
 *
 *      Copyright (C) 2000, 2010, 2013 by the GRASS Development Team
 *      Author: Huidae Cho <grass4u@gmail.com>
 *              Hydro Laboratory, Kyungpook National University
 *              South Korea
 *
 *      This program is free software under the GPL (>=v2)
 *      Read the file COPYING coming with GRASS for details.
 *
 */

#define _MAIN_C_
#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "global.h"

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *params;
	struct Option *topidxstats;
	struct Option *input;
	struct Option *output;
	struct Option *qobs;
	struct Option *timestep;
	struct Option *topidxclass;
	struct Option *topidx;
	struct Option *ntopidxclasses;
	struct Option *outtopidxstats;
    } params;
    struct
    {
	struct Flag *preprocess;
    } flags;

    /* Initialize GRASS and parse command line */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    module->description =
	_("Simulates TOPMODEL which is a physically based hydrologic model.");

    /* Parameter definitions */
    params.params = G_define_standard_option(G_OPT_F_INPUT);
    params.params->key = "parameters";
    params.params->description = _("Name of TOPMODEL parameters file");

    params.topidxstats = G_define_standard_option(G_OPT_F_INPUT);
    params.topidxstats->key = "topidxstats";
    params.topidxstats->description =
	_("Name of topographic index statistics file");

    params.input = G_define_standard_option(G_OPT_F_INPUT);
    params.input->description =
	_("Name of rainfall and potential evapotranspiration data file");

    params.output = G_define_standard_option(G_OPT_F_OUTPUT);
    params.output->description = _("Name for output file");

    params.qobs = G_define_standard_option(G_OPT_F_INPUT);
    params.qobs->key = "qobs";
    params.qobs->description = _("Name of observed flow file");
    params.qobs->required = NO;

    params.timestep = G_define_option();
    params.timestep->key = "timestep";
    params.timestep->label = _("Time step");
    params.timestep->description = _("Generate output for this time step.");
    params.timestep->type = TYPE_INTEGER;
    params.timestep->required = NO;

    params.topidxclass = G_define_option();
    params.topidxclass->key = "topidxclass";
    params.topidxclass->label = _("Topographic index class");
    params.topidxclass->description =
	_("Generate output for this topographic index class.");
    params.topidxclass->type = TYPE_INTEGER;
    params.topidxclass->required = NO;

    params.topidx = G_define_standard_option(G_OPT_R_INPUT);
    params.topidx->key = "topidx";
    params.topidx->label =
	_("Name of input topographic index ln(a/tanB) raster map");
    params.topidx->description =
	_("Must be clipped to the catchment boundary. Used for generating outtopidxstats.");
    params.topidx->required = NO;
    params.topidx->guisection = _("Preprocess");

    params.ntopidxclasses = G_define_option();
    params.ntopidxclasses->key = "ntopidxclasses";
    params.ntopidxclasses->label = _("Number of topographic index classes");
    params.ntopidxclasses->description =
	_("Used for generating outtopidxstats.");
    params.ntopidxclasses->type = TYPE_INTEGER;
    params.ntopidxclasses->required = NO;
    params.ntopidxclasses->answer = "30";
    params.ntopidxclasses->guisection = _("Preprocess");

    params.outtopidxstats = G_define_standard_option(G_OPT_F_OUTPUT);
    params.outtopidxstats->key = "outtopidxstats";
    params.outtopidxstats->label =
	_("Name for output topographic index statistics file");
    params.outtopidxstats->description =
	_("Requires topidx and ntopidxclasses.");
    params.outtopidxstats->required = NO;
    params.outtopidxstats->guisection = _("Preprocess");

    flags.preprocess = G_define_flag();
    flags.preprocess->key = 'p';
    flags.preprocess->description =
	_("Preprocess only and stop after generating outtopidxstats");
    flags.preprocess->suppress_required = YES;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Store given parameters */
    file.params = params.params->answer;
    file.topidxstats = params.topidxstats->answer;
    file.input = params.input->answer;
    file.output = params.output->answer;
    file.qobs = params.qobs->answer;

    if (!params.timestep->answer)
	params.timestep->answer = "0";
    misc.timestep = atoi(params.timestep->answer);

    if (!params.topidxclass->answer)
	params.topidxclass->answer = "0";
    misc.topidxclass = atoi(params.topidxclass->answer);

    if (params.topidx->answer && params.outtopidxstats->answer) {
	char *topidx;
	int ntopidxclasses;
	char *outtopidxstats;
	
	topidx = params.topidx->answer;
	ntopidxclasses = atoi(params.ntopidxclasses->answer);
	outtopidxstats = params.outtopidxstats->answer;

	if (ntopidxclasses <= 0)
	    G_fatal_error(_("ntopidxclasses must be positive."));

	create_topidxstats(topidx, ntopidxclasses, outtopidxstats);
    } else if (params.topidx->answer) {
	G_warning(_("Ignoring topidx because outtopidxstats is not specified."));
    } else if (params.outtopidxstats->answer) {
	G_warning(_("Ignoring outtopidxstats because topidx is not specified."));
    }

    if (flags.preprocess->answer)
        exit(EXIT_SUCCESS);

    /* Read input */
    read_input();

    /* Run TOPMODEL */
    run_topmodel();

    /* Write output */
    write_output();

    exit(EXIT_SUCCESS);
}
