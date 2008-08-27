
/****************************************************************************
 *
 * MODULE:       r.median
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Roberto Flor <flor itc.it>, Markus Neteler <neteler itc.it>
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
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
#include <grass/glocale.h>
#include "stats.h"
#include "local_proto.h"


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
    char command[1024];
    struct Categories cover_cats;
    FILE *stats_fd, *reclass_fd;
    int first;
    long basecat, covercat, catb, catc;
    double area;
    struct stats stats;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Finds the median of values in a cover map within "
	  "areas assigned the same category value in a "
	  "user-specified base map.");

    parm.base = G_define_standard_option(G_OPT_R_INPUT);
    parm.base->key = "base";
    parm.base->description = _("Name of base raster map");

    parm.cover = G_define_standard_option(G_OPT_R_INPUT);
    parm.cover->key = "cover";
    parm.cover->description = _("Name of cover raster map");

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    basemap = parm.base->answer;
    covermap = parm.cover->answer;
    outmap = parm.output->answer;

    if (G_read_cats(covermap, "", &cover_cats) < 0)
	G_fatal_error(_("Unable to read category labels of raster map <%s>"),
		      covermap);

    sprintf(command, "r.stats -an \"%s,%s\"", basemap, covermap);

    /* strcpy (command,"cat /tmp/t"); */
    G_debug(3, "command: %s", command);
    stats_fd = popen(command, "r");

    G_debug(3, "r.reclass i=\"%s\" o=\"%s\"", basemap, outmap);

    sprintf(command, "r.reclass i=\"%s\" o=\"%s\"", basemap, outmap);

    reclass_fd = popen(command, "w");

    first = 1;
    while (read_stats(stats_fd, &basecat, &covercat, &area)) {
	if (first) {
	    stats.n = 0;
	    stats.nalloc = 16;
	    stats.cat = (long *)
		G_calloc(stats.nalloc, sizeof(long));
	    stats.area = (double *)
		G_calloc(stats.nalloc, sizeof(double));
	    first = 0;
	    catb = basecat;
	}
	if (basecat != catb) {
	    catc = median(&stats);
	    write_reclass(reclass_fd, catb, catc,
			  G_get_cat(catc, &cover_cats));
	    catb = basecat;
	    stats.n = 0;
	}
	stats.n++;
	if (stats.n > stats.nalloc) {
	    stats.nalloc *= 2;
	    stats.cat = (long *)
		G_realloc(stats.cat, stats.nalloc * sizeof(long));
	    stats.area = (double *)
		G_realloc(stats.area, stats.nalloc * sizeof(double));
	}
	stats.cat[stats.n - 1] = covercat;
	stats.area[stats.n - 1] = area;
    }
    if (!first) {
	catc = median(&stats);
	write_reclass(reclass_fd, catb, catc, G_get_cat(catc, &cover_cats));
    }

    pclose(stats_fd);
    pclose(reclass_fd);

    exit(EXIT_SUCCESS);
}
