
/****************************************************************************
 *
 * MODULE:       r.random
 *
 * AUTHOR(S):    Dr. James Hinthorne - Central Wash. Uni. GIS Lab
 *               Modified for GRASS 5.x
 *                 Eric G. Miller
 *               Modified for GRASS 6.x and updates
 *                 Brad Douglas
 *
 * PURPOSE:      Generate a vector or raster map of random points
 *               selected from an input map
 *
 * COPYRIGHT:    (C) 2003-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

static int has_percent(char *);

int main(int argc, char *argv[])
{
    short percent;
    double percentage;
    long targets;
    long count;
    struct rr_state myState;

    struct GModule *module;
    struct
    {
	struct Option *input, *cover, *raster, *sites, *npoints;
    } parm;
    struct
    {
	struct Flag *zero, *info, *z_geometry;
    } flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Creates a raster map layer and vector point map "
	  "containing randomly located points.");

    parm.input = G_define_standard_option(G_OPT_R_INPUT);
    parm.input->description = _("Name of input raster map");

    parm.cover = G_define_standard_option(G_OPT_R_INPUT);
    parm.cover->key = "cover";
    parm.cover->required = NO;
    parm.cover->description = _("Name of cover raster map");

    parm.npoints = G_define_option();
    parm.npoints->key = "n";
    parm.npoints->key_desc = "number[%]";
    parm.npoints->type = TYPE_STRING;
    parm.npoints->required = YES;
    parm.npoints->description = _("The number of points to allocate");

    parm.raster = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.raster->required = NO;
    parm.raster->key = "raster_output";

    parm.sites = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.sites->required = NO;
    parm.sites->key = "vector_output";

    flag.zero = G_define_flag();
    flag.zero->key = 'z';
    flag.zero->description = _("Generate points also for NULL category");

    flag.info = G_define_flag();
    flag.info->key = 'i';
    flag.info->description =
	_("Report information about input raster and exit");

    flag.z_geometry = G_define_flag();
    flag.z_geometry->key = 'd';
    flag.z_geometry->description = _("Generate vector points as 3D points");

    if (G_parser(argc, argv) != 0)
	exit(EXIT_FAILURE);

    /* Set some state variables */
    myState.use_nulls = flag.zero->answer;
    myState.inraster = parm.input->answer;
    if (parm.cover->answer) {
	myState.docover = 1;
	myState.inrcover = parm.cover->answer;
    }
    else {
	myState.docover = 0;
	myState.cmapset = NULL;
	myState.inrcover = NULL;
    }
    myState.outraster = parm.raster->answer;
    myState.outvector = parm.sites->answer;
    myState.z_geometry = flag.z_geometry->answer;

    myState.mapset = G_find_cell(myState.inraster, "");
    if (myState.mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), myState.inraster);

    if (myState.docover == 1) {
	myState.cmapset = G_find_cell(myState.inrcover, "");
	if (myState.cmapset == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), myState.inrcover);
    }

    /* If they only want info we ignore the rest */
    get_stats(&myState);

    if (flag.info->answer) {
	G_message("Raster:      %s@%s\n"
		  "Cover:       %s@%s\n"
		  "Cell Count:  %d\n"
		  "Null Cells:  %d\n\n",
		  myState.inraster, myState.mapset, myState.inrcover,
		  myState.cmapset, (int)myState.nCells, (int)myState.nNulls);

	exit(EXIT_SUCCESS);
    }

    if (!(parm.raster->answer || parm.sites->answer))
	G_fatal_error(_("Note: one (or both) of %s and %s must be specified"),
		      parm.raster->key, parm.sites->key);

    if (myState.outraster)
	if (G_legal_filename(myState.outraster) < 0)
	    G_fatal_error(_("<%s> is an illegal file name"),
			  myState.outraster);

    if (myState.outvector)
	if (G_legal_filename(myState.outvector) < 0)
	    G_fatal_error(_("<%s> is an illegal file name"),
			  myState.outvector);

    /* look for n[%] */
    percent = has_percent(parm.npoints->answer);
    if (percent) {
	if (sscanf(parm.npoints->answer, "%lf", &percentage) != 1
	    || percentage <= 0.0 || percentage > 100.0) {
	    G_fatal_error(_("<%s=%s> invalid percentage"),
			  parm.npoints->key, parm.npoints->answer);
	}
    }
    else {
	if (sscanf(parm.npoints->answer, "%ld", &targets) != 1
	    || targets <= 0) {
	    G_fatal_error(_("<%s=%s> invalid number of points"),
			  parm.npoints->key, parm.npoints->answer);
	}
    }

    count = (myState.use_nulls) ? myState.nCells :
	myState.nCells - myState.nNulls;

    if (percent)
	myState.nRand = (int)(count * percentage / 100.0 + .5);
    else {
	if (targets > count) {
	    if (myState.use_nulls)
		G_fatal_error(_("There aren't [%ld] cells in the current region"),
			      targets);
	    else
		G_fatal_error(_("There aren't [%ld] non-NULL cells in the current region"),
			      targets);
	}

	if (targets <= 0)
	    G_fatal_error(_("There are no valid locations in the current region"));

	myState.nRand = targets;
    }

    execute_random(&myState);

    if (myState.outraster)
	make_support(&myState, percent, percentage);

    return EXIT_SUCCESS;
}


static int has_percent(char *s)
{
    while (*s)
	if (*s++ == '%')
	    return 1;

    return 0;
}
