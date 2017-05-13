
/****************************************************************************
 *
 * MODULE:       r.mode
 * AUTHOR(S):    Michael Shapiro (CERL)  (original contributor),
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jachym Cepicky <jachym les-ejk.cz>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      calculates the most frequently occurring value (i. e., mode)
 *               of data contained in a cover raster map layer for areas 
 *               assigned the same category value in the user-specified 
 *               base raster map
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "local_proto.h"
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *base, *cover, *output;
    } parm;
    char *basemap;
    char *covermap;
    char *outmap;
    char input[GNAME_MAX*2+8];
    char output[GNAME_MAX+8];
    const char *args[5];
    struct Popen stats_child, reclass_child;
    struct Categories cover_cats;
    struct Colors colors;
    FILE *stats, *reclass;
    int first;
    long basecat, covercat, catb, catc;
    double value, max;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("algebra"));
    module->description =
	_("Finds the mode of values in a cover map within "
	  "areas assigned the same category value in a "
	  "user-specified base map.");

    parm.base = G_define_option();
    parm.base->key = "base";
    parm.base->description = _("Base map to be reclassified");
    parm.base->required = YES;
    parm.base->type = TYPE_STRING;
    parm.base->gisprompt = "old,cell,raster";

    parm.cover = G_define_option();
    parm.cover->key = "cover";
    parm.cover->description = _("Coverage map");
    parm.cover->required = YES;
    parm.cover->type = TYPE_STRING;
    parm.cover->gisprompt = "old,cell,raster";

    parm.output = G_define_option();
    parm.output->key = "output";
    parm.output->description = _("Output map");
    parm.output->required = YES;
    parm.output->type = TYPE_STRING;
    parm.output->gisprompt = "new,cell,raster";

    if (G_parser(argc, argv))
	exit(1);

    basemap = parm.base->answer;
    covermap = parm.cover->answer;
    outmap = parm.output->answer;

    if (Rast_read_cats(covermap, "", &cover_cats) < 0) {
	G_fatal_error(_("%s: Unable to read category labels"), covermap);
    }

    sprintf(input, "input=%s,%s", basemap, covermap);

    args[0] = "r.stats";
    args[1] = "-an";
    args[2] = input;
    args[3] = NULL;

    stats = G_popen_read(&stats_child, "r.stats", args);

    sprintf(input, "input=%s", basemap);
    sprintf(output, "output=%s", outmap);

    args[0] = "r.reclass";
    args[1] = input;
    args[2] = output;
    args[3] = "rules=-";
    args[4] = NULL;

    reclass = G_popen_write(&reclass_child, "r.reclass", args);

    first = 1;
    while (read_stats(stats, &basecat, &covercat, &value)) {
	if (first) {
	    first = 0;
	    catb = basecat;
	    catc = covercat;
	    max = value;
	}
	if (basecat != catb) {
	  write_reclass(reclass, catb, catc, Rast_get_c_cat((CELL *) &catc, &cover_cats));
	    catb = basecat;
	    catc = covercat;
	    max = value;
	}
	if (value > max) {
	    catc = covercat;
	    max = value;
	}
    }
    if (first) {
	catb = catc = 0;
    }
    write_reclass(reclass, catb, catc, Rast_get_c_cat((CELL *) &catc, &cover_cats));

    G_popen_close(&reclass_child);
    G_popen_close(&stats_child);

    if (Rast_read_colors(parm.cover->answer, "", &colors) < 0)
	G_fatal_error(_("Unable to read color table for %s"),
			parm.cover->answer);
    Rast_write_colors(parm.output->answer, G_mapset(), &colors);

    return 0;
}
