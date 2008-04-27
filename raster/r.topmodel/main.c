/*
 * r.topmodel: simulates TOPMODEL based on TMOD9502.FOR.
 *
 * TMOD9502.FOR Author: Keith Beven <k.beven@lancaster.ac.uk>
 *			http://www.es.lancs.ac.uk/hfdg/topmodel.html
 *
 *	Copyright (C) 2000 by the GRASS Development Team
 *	Author: Huidae Cho <grass4u@gmail.com>
 *		Hydro Laboratory, Kyungpook National University
 *		South Korea
 *
 *	This program is free software under the GPL (>=v2)
 *	Read the file COPYING coming with GRASS for details.
 *
 */

#define	MAIN
#include <stdio.h>
#include <stdlib.h>
#include <grass/glocale.h>
#include "global.h"


int
main (int argc, char **argv)
{

	struct GModule *module;
	struct
	{
		struct	Option	*basin;
		struct	Option	*elev;
		struct	Option	*fill;
		struct	Option	*dir;
		struct	Option	*belev;
		struct	Option	*topidx;
		struct	Option	*nidxclass;
		struct	Option	*idxstats;
		struct	Option	*params;
		struct	Option	*input;
		struct	Option	*output;
		struct	Option	*Qobs;
		struct	Option	*timestep;
		struct	Option	*idxclass;
	} param;

	struct
	{
		struct	Flag	*input;
	} flag;

	/* Initialize GRASS and parse command line */
	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("raster");
    module->description =
	_("Simulates TOPMODEL which is a physically based hydrologic model.");

	/* Parameter definitions */
	param.basin			= G_define_option();
	param.basin->key		= "basin";
	param.basin->description	=
		_("(i)   Basin map created by r.water.outlet (MASK)");
	param.basin->type		= TYPE_STRING;
	param.basin->required		= NO;
	param.basin->gisprompt		= "old,cell,raster";

	param.elev			= G_define_option();
	param.elev->key			= "elevation";
	param.elev->description		=
		_("(i)   Elevation map");
	param.elev->type		= TYPE_STRING;
	param.elev->required		= NO;
	param.elev->gisprompt		= "old,cell,raster";

	param.fill			= G_define_option();
	param.fill->key			= "depressionless";
	param.fill->description		=
		_("(o)   Depressionless elevation map");
	param.fill->type		= TYPE_STRING;
	param.fill->required		= NO;
	param.fill->gisprompt		= "new,cell,raster";

	param.dir			= G_define_option();
	param.dir->key			= "direction";
	param.dir->description		=
		_("(o)   Direction map for depressionless elevation map");
	param.dir->type			= TYPE_STRING;
	param.dir->required		= NO;
	param.dir->gisprompt		= "new,cell,raster";

	param.belev			= G_define_option();
	param.belev->key		= "belevation";
	param.belev->description	=
		_("(o/i) Basin elevation map (MASK applied)");
	param.belev->type		= TYPE_STRING;
	param.belev->required		= NO;
	param.belev->gisprompt		= "new,cell,raster";

	param.topidx			= G_define_option();
	param.topidx->key		= "topidx";
	param.topidx->description	=
		_("(o)   Topographic index ln(a/tanB) map (MASK applied)");
	param.topidx->type		= TYPE_STRING;
	param.topidx->required		= NO;
	param.topidx->gisprompt		= "new,cell,raster";

	param.nidxclass			= G_define_option();
	param.nidxclass->key		= "nidxclass";
	param.nidxclass->description	=
		_("(i)   Number of topographic index classes");
	param.nidxclass->type		= TYPE_INTEGER;
	param.nidxclass->required	= NO;
	param.nidxclass->answer		= "30";

	param.idxstats			= G_define_option();
	param.idxstats->key		= "idxstats";
	param.idxstats->description	=
		_("(o/i) Topographic index statistics file");
	param.idxstats->type		= TYPE_STRING;
	param.idxstats->required	= YES;

	param.params			= G_define_option();
	param.params->key		= "parameters";
	param.params->description	=
		_("(i)   TOPMODEL Parameters file");
	param.params->type		= TYPE_STRING;
	param.params->required		= YES;

	param.input			= G_define_option();
	param.input->key		= "input";
	param.input->description	=
		_("(i)   Rainfall and potential evapotranspiration data file");
	param.input->type		= TYPE_STRING;
	param.input->required		= YES;

	param.output			= G_define_option();
	param.output->key		= "output";
	param.output->description	=
		_("(o)   Output file");
	param.output->type		= TYPE_STRING;
	param.output->required		= YES;

	param.Qobs			= G_define_option();
	param.Qobs->key			= "Qobs";
	param.Qobs->description		=
		_("(i)   OPTIONAL Observed flow file");
	param.Qobs->type		= TYPE_STRING;
	param.Qobs->required		= NO;

	param.timestep			= G_define_option();
	param.timestep->key		= "timestep";
	param.timestep->description	=
		_("(i)   OPTIONAL Output for given time step");
	param.timestep->type		= TYPE_INTEGER;
	param.timestep->required	= NO;

	param.idxclass			= G_define_option();
	param.idxclass->key		= "idxclass";
	param.idxclass->description	=
		_("(i)   OPTIONAL Output for given topographic index class");
	param.idxclass->type		= TYPE_INTEGER;
	param.idxclass->required	= NO;


	/* Flag definitions */
	flag.input			= G_define_flag();
	flag.input->key			= 'i';
	flag.input->description		=
		_("Input data given for (o/i)");

	if(G_parser(argc, argv)){
	        exit(-1);
	}

	/* Store given parameters and flags */
	map.basin	= param.basin->answer;
	map.elev	= param.elev->answer;
	map.belev	= param.belev->answer;
	map.fill	= param.fill->answer;
	map.dir		= param.dir->answer;
	map.topidx	= param.topidx->answer;
	file.idxstats	= param.idxstats->answer;
	file.params	= param.params->answer;
	file.input	= param.input->answer;
	file.output	= param.output->answer;
	file.Qobs	= param.Qobs->answer;

	misc.nidxclass	= atoi(param.nidxclass->answer);

	if(!param.timestep->answer)
		param.timestep->answer = "0";
	if(!param.idxclass->answer)
		param.idxclass->answer = "0";

	misc.timestep	= atoi(param.timestep->answer);
	misc.idxclass	= atoi(param.idxclass->answer);

	flg.input	= flag.input->answer;
	flg.overwr	= module->overwrite;


	gisbase = G_gisbase();
	mapset  = G_mapset();


	/* Check run conditions */
	if(check_ready())
		exit(1);

	/* Adjust cell header */
	gregion();

	/* Create required maps */
	if(!flg.input){
		if(map.fill)
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

