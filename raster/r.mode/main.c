
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
#include "local_proto.h"
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *base, *cover, *output;
    } parm;
    char *basemap, *base_mapset;
    char *covermap, *cover_mapset;
    char *outmap;
    char command[1024];
    struct Categories cover_cats;
    FILE *stats, *reclass;
    int first;
    long basecat, covercat, catb, catc;
    double value, max;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
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

    base_mapset = G_find_cell2(basemap, "");
    if (base_mapset == NULL) {
	G_fatal_error(_("%s: base raster map not found"), basemap);
    }

    cover_mapset = G_find_cell2(covermap, "");
    if (cover_mapset == NULL) {
	G_fatal_error(_("%s: cover raster map not found"), covermap);
    }
    if (G_legal_filename(outmap) < 0) {
	G_fatal_error(_("<%s> is an illegal file name"), outmap);
    }
    if (strcmp(G_mapset(), base_mapset) == 0 && strcmp(basemap, outmap) == 0) {
	G_fatal_error(_("%s: base map and output map must be different"),
		      outmap);
    }
    if (G_read_cats(covermap, cover_mapset, &cover_cats) < 0) {
	G_fatal_error(_("%s: Unable to read category labels"), covermap);
    }

    strcpy(command, "r.stats -an \"");
    strcat(command, G_fully_qualified_name(basemap, base_mapset));
    strcat(command, ",");
    strcat(command, G_fully_qualified_name(covermap, cover_mapset));
    strcat(command, "\"");

    /* printf(command); */
    stats = popen(command, "r");

    sprintf(command, "r.reclass i=\"%s\" o=\"%s\"",
	    G_fully_qualified_name(basemap, base_mapset), outmap);

    /* printf(command); */
    reclass = popen(command, "w");

    first = 1;
    while (read_stats(stats, &basecat, &covercat, &value)) {
	if (first) {
	    first = 0;
	    catb = basecat;
	    catc = covercat;
	    max = value;
	}
	if (basecat != catb) {
	    write_reclass(reclass, catb, catc, G_get_cat(catc, &cover_cats));
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
    write_reclass(reclass, catb, catc, G_get_cat(catc, &cover_cats));

    pclose(stats);
    pclose(reclass);

    exit(0);
}
