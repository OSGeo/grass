
/****************************************************************************
 *
 * MODULE:       r.basins.fill
 *
 * AUTHOR(S):    Dale White - Dept. of Geography, Pennsylvania State U.
 *               Larry Band - Dept. of Geography, University of Toronto
 *
 * PURPOSE:      Generates a raster map layer showing watershed subbasins.
 *
 * COPYRIGHT:    (C) 2005, 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

/*====================================================================*/
/* program to propogate the link label into the hillslope areas;      */
/* processes CELL files only and works on window derived from link    */
/* label map                                                          */

/*====================================================================*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

#define NOMASK 1


int main(int argc, char *argv[])
{
    int partfd;
    int nrows, ncols;
    const char *drain_name;
    const char *ridge_name;
    const char *part_name;
    CELL *drain, *ridge;
    struct Cell_head window;
    int row, col, npass, tpass;
    struct GModule *module;
    struct Option *num_opt, *drain_opt, *ridge_opt, *part_opt;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("watershed"));
    module->description =
	_("Generates watershed subbasins raster map.");

    drain_opt = G_define_standard_option(G_OPT_R_INPUT);
    drain_opt->key = "cnetwork";
    drain_opt->description = _("Name of input coded stream network raster map");

    ridge_opt = G_define_standard_option(G_OPT_R_INPUT);
    ridge_opt->key = "tnetwork";
    ridge_opt->description = _("Name of input thinned ridge network raster map");

    part_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    
    num_opt = G_define_option();
    num_opt->key = "number";
    num_opt->type = TYPE_INTEGER;
    num_opt->required = YES;
    num_opt->description = _("Number of passes through the dataset");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    sscanf(num_opt->answer, "%d", &tpass);

    drain_name = drain_opt->answer;

    /* this isn't a nice thing to do. Rast_align_window() should be used first */
    Rast_get_cellhd(drain_name, "", &window);
    Rast_set_window(&window);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    ridge_name = ridge_opt->answer;

    part_name = part_opt->answer;

    drain = read_map(drain_name, NOMASK, nrows, ncols);
    ridge = read_map(ridge_name, NOMASK, nrows, ncols);

    partfd = Rast_open_c_new(part_name);

    /* run through file and set streams to zero at locations where ridges exist */
    for (row = 0; row < nrows; row++) {
	for (col = 0; col < ncols; col++)
	    if (ridge[row * ncols + col] != 0)
		drain[row * ncols + col] = 0;
    }

    for (npass = 1; npass <= tpass; npass++) {
	for (row = 1; row < nrows - 1; row++) {
	    for (col = 1; col < ncols - 1; col++) {
		if (drain[row * ncols + col] == 0 &&
		    ridge[row * ncols + col] == 0) {
		    if (drain[(row - 1) * ncols + col] != 0 &&
			ridge[(row - 1) * ncols + col] == 0)
			drain[row * ncols + col] =
			    drain[(row - 1) * ncols + col];
		    if (drain[row * ncols + (col - 1)] != 0 &&
			ridge[row * ncols + (col - 1)] == 0)
			drain[row * ncols + col] =
			    drain[row * ncols + (col - 1)];
		}
	    }
	}
	G_message(_("Forward sweep complete"));

	for (row = nrows - 3; row > 1; --row) {
	    for (col = ncols - 3; col > 1; --col) {
		if (drain[row * ncols + col] == 0 &&
		    ridge[row * ncols + col] == 0) {
		    if (drain[(row + 1) * ncols + col] != 0 &&
			ridge[(row + 1) * ncols + col] == 0)
			drain[row * ncols + col] =
			    drain[(row + 1) * ncols + col];
		    if (drain[row * ncols + (col + 1)] != 0 &&
			ridge[row * ncols + (col + 1)] == 0)
			drain[row * ncols + col] =
			    drain[row * ncols + (col + 1)];
		}
	    }
	}
	G_message(_("Reverse sweep complete"));
    }

    /* write out partitioned watershed map */
    for (row = 0; row < nrows; row++)
	Rast_put_row(partfd, drain + (row * ncols), CELL_TYPE);

    G_message(_("Creating support files for <%s>..."), part_name);
    Rast_close(partfd);

    exit(EXIT_SUCCESS);
}
