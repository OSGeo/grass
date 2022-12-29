
/****************************************************************************
 *
 * MODULE:       r.random.surface
 * AUTHOR(S):    Charles Ehlschlaeger, Michael Goodchild, and Chih-chang Lin; 
 *                      (National Center for Geographic Information and 
 *                      Analysis, University of California, Santa Barbara)
 *                      (original contributors)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jachym Cepicky <jachym les-ejk.cz>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      generates a spatially dependent random surface
 * COPYRIGHT:    (C) 2000-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* main.c */
#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "ransurf.h"
#include "local_proto.h"

BIGF BigF;
double **RSurface, NS, EW, FilterSD, AllMaxDist, *Norm;
int MapCount, FDM, Rs, Cs, Theory;
CELL *CellBuffer;
FILTER *AllFilters, Filter;
CATINFO CatInfo;
int *Seeds, Seed, NumSeeds, Low, High, NumMaps, NumFilters, OutFD;
char Buf[240], **OutNames, *TheoryName, *Mapset;

struct Flag *Uniform;
struct Option *Distance, *Exponent, *Weight;
struct Option *Output;
struct Option *range_high_stuff;
struct Option *SeedStuff;

int main(int argc, char **argv)
{
    struct GModule *module;

    int DoMap, DoFilter, MapSeed;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("random"));
    module->description =
	_("Generates random surface(s) with spatial dependence.");

    Output = G_define_option();
    Output->key = "output";
    Output->type = TYPE_STRING;
    Output->required = YES;
    Output->multiple = YES;
    Output->description = _("Name for output raster map(s)");
    Output->gisprompt = "new,cell,raster";

    Distance = G_define_option();
    Distance->key = "distance";
    Distance->type = TYPE_DOUBLE;
    Distance->required = NO;
    Distance->multiple = NO;
    Distance->description =
	_("Maximum distance of spatial correlation (value >= 0.0)");
    Distance->answer = "0.0";

    Exponent = G_define_option();
    Exponent->key = "exponent";
    Exponent->type = TYPE_DOUBLE;
    Exponent->multiple = NO;
    Exponent->required = NO;
    Exponent->description = _("Distance decay exponent (value > 0.0)");
    Exponent->answer = "1.0";

    Weight = G_define_option();
    Weight->key = "flat";
    Weight->type = TYPE_DOUBLE;
    Weight->multiple = NO;
    Weight->required = NO;
    Weight->description =
	_("Distance filter remains flat before beginning exponent");
    Weight->answer = "0.0";

    SeedStuff = G_define_option();
    SeedStuff->key = "seed";
    SeedStuff->type = TYPE_INTEGER;
    SeedStuff->required = NO;
    SeedStuff->description =
	_("Random seed, default [random]");

    range_high_stuff = G_define_option();
    range_high_stuff->key = "high";
    range_high_stuff->type = TYPE_INTEGER;
    range_high_stuff->required = NO;
    range_high_stuff->description = _("Maximum cell value of distribution");
    range_high_stuff->answer = "255";

    Uniform = G_define_flag();
    Uniform->key = 'u';
    Uniform->description = _("Uniformly distributed cell values");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Init();

    if (Uniform->answer)
	GenNorm();

    CalcSD();

    for (DoMap = 0; DoMap < NumMaps; DoMap++) {
	OutFD = Rast_open_c_new(OutNames[DoMap]);

	G_message(_("Generating raster map <%s>..."), OutNames[DoMap]);

	if (Seeds[DoMap] < 0)
	    G_srand48_auto();
	else
	    G_srand48(Seeds[DoMap]);

	MapSeed = Seed = Seeds[DoMap];
	ZeroMapCells();

	for (DoFilter = 0; DoFilter < NumFilters; DoFilter++) {
	    CopyFilter(&Filter, AllFilters[DoFilter]);
	    G_debug(1,
		    "Starting filter #%d, distance: %.*lf, exponent: %.*lf, flat: %.*lf",
		    DoFilter, Digits(2.0 * Filter.MaxDist, 6),
		    2.0 * Filter.MaxDist, Digits(1.0 / Filter.Exp, 6),
		    1.0 / Filter.Exp, Digits(Filter.Mult, 6), Filter.Mult);

	    MakeBigF();
	    CalcSurface();
	}

	SaveMap(DoMap, MapSeed);
    }

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
