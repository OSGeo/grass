
/****************************************************************************
 *
 * MODULE:       r.clump
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Recategorizes data in a raster map layer by grouping cells
 *		 that form physically discrete areas into unique categories.
 *
 * COPYRIGHT:    (C) 2006-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct Colors colr;
    struct Range range;
    CELL min, max;
    int in_fd, out_fd;
    char title[512];
    char name[GNAME_MAX];
    char *OUTPUT;
    char *INPUT;
    struct GModule *module;
    struct Option *opt_in;
    struct Option *opt_out;
    struct Option *opt_title;

    G_gisinit(argv[0]);

    /* Define the different options */

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("reclass"));
    module->description =
	_("Recategorizes data in a raster map by grouping cells "
	  "that form physically discrete areas into unique categories.");

    opt_in = G_define_standard_option(G_OPT_R_INPUT);

    opt_out = G_define_standard_option(G_OPT_R_OUTPUT);

    opt_title = G_define_option();
    opt_title->key = "title";
    opt_title->type = TYPE_STRING;
    opt_title->required = NO;
    opt_title->description = _("Title for output raster map");

    /* parse options */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    INPUT = opt_in->answer;
    OUTPUT = opt_out->answer;

    strcpy(name, INPUT);

    in_fd = Rast_open_old(name, "");

    out_fd = Rast_open_c_new(OUTPUT);

    clump(in_fd, out_fd);

    G_debug(1, "Creating support files...");

    Rast_close(in_fd);
    Rast_close(out_fd);


    /* build title */
    if (opt_title->answer != NULL)
	strcpy(title, opt_title->answer);
    else
	sprintf(title, "clump of <%s@%s>", name, G_mapset());
    
    Rast_put_cell_title(OUTPUT, title);
    Rast_read_range(OUTPUT, G_mapset(), &range);
    Rast_get_range_min_max(&range, &min, &max);
    Rast_make_random_colors(&colr, min, max);
    Rast_write_colors(OUTPUT, G_mapset(), &colr);

    G_done_msg(_("%d clumps."), range.max);

    exit(EXIT_SUCCESS);
}
