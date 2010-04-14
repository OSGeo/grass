/*
 * r.topmodel: simulates TOPMODEL based on TMOD9502.FOR.
 *
 * TMOD9502.FOR Author: Keith Beven <k.beven@lancaster.ac.uk>
 *                      http://www.es.lancs.ac.uk/hfdg/topmodel.html
 *
 *      Copyright (C) 2000, 2010 by the GRASS Development Team
 *      Author: Huidae Cho <grass4u@gmail.com>
 *              Hydro Laboratory, Kyungpook National University
 *              South Korea
 *
 *      This program is free software under the GPL (>=v2)
 *      Read the file COPYING coming with GRASS for details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/glocale.h>
#include "global.h"

struct idxstats idxstats;
struct params params;
struct input input;
struct map map;
struct file file;
struct misc misc;
struct flg flg;
const char *mapset;

int main(int argc, char **argv)
{

    struct GModule *module;
    struct
    {
	struct Option *basin;
	struct Option *elev;
	struct Option *fill;
	struct Option *dir;
	struct Option *belev;
	struct Option *topidx;
	struct Option *nidxclass;
	struct Option *idxstats;
	struct Option *params;
	struct Option *input;
	struct Option *output;
	struct Option *Qobs;
	struct Option *timestep;
	struct Option *idxclass;
    } param;

    struct
    {
	struct Flag *input;
    } flag;

    /* Initialize GRASS and parse command line */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    module->description =
	_("Simulates TOPMODEL which is a physically based hydrologic model.");

    /* Parameter definitions */
    param.elev = G_define_standard_option(G_OPT_R_ELEV);
    param.elev->required = NO;
    param.elev->guisection = _("Input");

    param.basin = G_define_standard_option(G_OPT_R_INPUT);
    param.basin->key = "basin";
    param.basin->label =
	_("Name of input basin raster map");
    param.basin->description = _("Created by r.water.outlet (MASK)");
    param.basin->required = NO;
    param.basin->guisection = _("Input");

    param.fill = G_define_standard_option(G_OPT_R_OUTPUT);
    param.fill->key = "depressionless";
    param.fill->description = _("Name for output depressionless elevation raster map");
    param.fill->required = NO;
    param.fill->guisection = _("Output");

    param.dir = G_define_standard_option(G_OPT_R_OUTPUT);
    param.dir->key = "direction";
    param.dir->description =
	_("Name for output flow direction map for depressionless elevation raster map");
    param.dir->required = NO;
    param.dir->guisection = _("Output");

    param.belev = G_define_standard_option(G_OPT_R_OUTPUT);
    param.belev->key = "basin_elevation";
    param.belev->label = _("Name for output basin elevation raster map (o/i)");
    param.belev->description = _("MASK applied");
    param.belev->required = NO;
    param.belev->guisection = _("Output");

    param.topidx = G_define_standard_option(G_OPT_R_OUTPUT);
    param.topidx->key = "topidx";
    param.topidx->label =
	_("Name for output opographic index ln(a/tanB) raster map");
    param.topidx->description = _("MASK applied");
    param.topidx->required = NO;
    param.topidx->guisection = _("Output");

    param.nidxclass = G_define_option();
    param.nidxclass->key = "nidxclass";
    param.nidxclass->description =
	_("Number of topographic index classes");
    param.nidxclass->type = TYPE_INTEGER;
    param.nidxclass->required = NO;
    param.nidxclass->answer = "30";
    param.nidxclass->guisection = _("Parameters");
    
    param.idxstats = G_define_standard_option(G_OPT_F_INPUT);
    param.idxstats->key = "idxstats";
    param.idxstats->description =
	_("Name of topographic index statistics file (o/i)");
    
    param.params = G_define_standard_option(G_OPT_F_INPUT);
    param.params->key = "parameters";
    param.params->description = _("Name of TOPMODEL parameters file");
    
    param.input = G_define_standard_option(G_OPT_F_INPUT);
    param.input->description =
	_("Name of rainfall and potential evapotranspiration data file");

    param.output = G_define_standard_option(G_OPT_F_OUTPUT);
    param.output->description = _("Name for output file");

    param.Qobs = G_define_standard_option(G_OPT_F_OUTPUT);
    param.Qobs->key = "qobs";
    param.Qobs->description = _("Name for observed flow file");
    param.Qobs->required = NO;
    param.Qobs->guisection = _("Output");
    
    param.timestep = G_define_option();
    param.timestep->key = "timestep";
    param.timestep->description =
	_("Time step");
    param.timestep->type = TYPE_INTEGER;
    param.timestep->required = NO;
    param.timestep->guisection = _("Parameters");
    
    param.idxclass = G_define_option();
    param.idxclass->key = "idxclass";
    param.idxclass->description =
	_("Topographic index class");
    param.idxclass->type = TYPE_INTEGER;
    param.idxclass->required = NO;
    param.idxclass->guisection = _("Parameters");

    /* Flag definitions */
    flag.input = G_define_flag();
    flag.input->key = 'i';
    flag.input->description = _("Input data given for (o/i)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Store given parameters and flags */
    map.basin = param.basin->answer;
    map.elev = param.elev->answer;
    map.belev = param.belev->answer;
    map.fill = param.fill->answer;
    map.dir = param.dir->answer;
    map.topidx = param.topidx->answer;
    file.idxstats = param.idxstats->answer;
    file.params = param.params->answer;
    file.input = param.input->answer;
    file.output = param.output->answer;
    file.Qobs = param.Qobs->answer;

    misc.nidxclass = atoi(param.nidxclass->answer);

    if (!param.timestep->answer)
	param.timestep->answer = "0";
    if (!param.idxclass->answer)
	param.idxclass->answer = "0";

    misc.timestep = atoi(param.timestep->answer);
    misc.idxclass = atoi(param.idxclass->answer);

    flg.input = flag.input->answer;
    flg.overwr = module->overwrite;

    mapset = G_mapset();

    /* Check run conditions */
    if (check_ready())
	exit(1);

    /* Adjust cell header */
    gregion();

    /* Create required maps */
    if (!flg.input) {
	if (map.fill)
	    depressionless();
	basin_elevation();
    }

    top_index();

    /* Read required files */
    read_inputs();

    /* Implement TOPMODEL */
    topmodel();

    /* Write outputs */
    write_outputs();


    exit(0);
}
