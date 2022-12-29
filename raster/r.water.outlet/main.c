
/****************************************************************************
 *
 * MODULE:       r.water.outlet
 * AUTHOR(S):    Charles Ehlschlaeger, USACERL (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>, 
 *               Soeren Gebbert <soeren.gebbert gmx.de>
 * PURPOSE:      this program makes a watershed basin raster map using the 
 *               drainage pointer map, from an outlet point defined by an 
 *               easting and a northing.
 * COPYRIGHT:    (C) 1999-2006, 2010, 2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "basin.h"
#include "outletP.h"

SHORT drain[3][3] = {{ 7,6,5 },{ 8,-17,4 },{ 1,2,3 }};
int nrows, ncols;
char *drain_ptrs;
RAMSEG ba_seg, pt_seg;
CELL *bas;

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct {
      struct Option *input, *output, *coords;
    } opt;
    double N, E;
    int row, col, basin_fd, drain_fd;
    CELL *cell_buf;
    char drain_name[GNAME_MAX], basin_name[GNAME_MAX];
    struct Cell_head window;
    struct History hist;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->description = _("Creates watershed basins from a drainage direction map.");
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("watershed"));
	
    opt.input = G_define_standard_option(G_OPT_R_INPUT);
    opt.input->description = _("Name of input drainage direction map");

    opt.output = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.output->description = _("Name for output watershed basin map");
    
    opt.coords = G_define_standard_option(G_OPT_M_COORDS);
    opt.coords->description = _("Coordinates of outlet point");
    opt.coords->required = YES;

    /*   Parse command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_window(&window);

    strcpy(drain_name, opt.input->answer);
    strcpy(basin_name, opt.output->answer);

    if (!G_scan_easting(opt.coords->answers[0], &E, G_projection()))
        G_fatal_error(_("Illegal east coordinate '%s'"), opt.coords->answers[0]);
    if (!G_scan_northing(opt.coords->answers[1], &N, G_projection()))
        G_fatal_error(_("Illegal north coordinate '%s'"), opt.coords->answers[1]);

    G_debug(1, "easting = %.4f northing = %.4f", E, N);
    if (E < window.west || E > window.east || N < window.south ||
	N > window.north) {
	G_warning(_("Ignoring point outside computation region: %.4f,%.4f"),
		  E, N);
    }

    G_get_set_window(&window);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    drain_fd = Rast_open_old(drain_name, "");

    drain_ptrs =
	(char *)G_malloc(sizeof(char) * size_array(&pt_seg, nrows, ncols));
    bas = (CELL *) G_calloc(size_array(&ba_seg, nrows, ncols), sizeof(CELL));
    cell_buf = Rast_allocate_c_buf();

    for (row = 0; row < nrows; row++) {
	Rast_get_c_row(drain_fd, cell_buf, row);
	for (col = 0; col < ncols; col++) {
	    drain_ptrs[SEG_INDEX(pt_seg, row, col)] = cell_buf[col];
	}
    }
    G_free(cell_buf);
    row = (window.north - N) / window.ns_res;
    col = (E - window.west) / window.ew_res;
    if (row >= 0 && col >= 0 && row < nrows && col < ncols)
	overland_cells(row, col);
    G_free(drain_ptrs);
    cell_buf = Rast_allocate_c_buf();
    basin_fd = Rast_open_c_new(basin_name);

    for (row = 0; row < nrows; row++) {
        G_percent(row, nrows, 5);
	for (col = 0; col < ncols; col++) {
	    cell_buf[col] = bas[SEG_INDEX(ba_seg, row, col)];
	    if (cell_buf[col] == 0)
		Rast_set_null_value(&cell_buf[col], 1, CELL_TYPE);
	}
	Rast_put_row(basin_fd, cell_buf, CELL_TYPE);
    }
    G_percent(1, 1, 1);

    G_free(bas);
    G_free(cell_buf);
    Rast_close(basin_fd);

    Rast_put_cell_title(basin_name, "Watershed basin");
    Rast_short_history(basin_name, "raster", &hist);
    Rast_command_history(&hist);
    Rast_write_history(basin_name, &hist);

    exit(EXIT_SUCCESS);
}
