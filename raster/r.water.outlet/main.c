
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
 * COPYRIGHT:    (C) 1999-2006, 2010 by the GRASS Development Team
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

SHORT drain[3][3]	= {{ 7,6,5 },{ 8,-17,4 },{ 1,2,3 }};
SHORT updrain[3][3]	= {{ 3,2,1 },{ 4,-17,8 },{ 5,6,7 }};
char dr_mod[9]	= {0,1,1,1,0,-1,-1,-1,0};
char dc_mod[9]	= {0,1,0,-1,-1,-1,0,1,1};
char basin_name[GNAME_MAX], swale_name[GNAME_MAX],
  half_name[GNAME_MAX], elev_name[GNAME_MAX], armsed_name[GNAME_MAX];
int nrows, ncols, done, total;
int array_size, high_index, do_index;
char *drain_ptrs, ha_f, el_f, ar_f;
RAMSEG ba_seg, pt_seg, sl_seg;
int ncols_less_one, nrows_less_one;
NODE *to_do;
FILE *arm_fd, *fp;
FLAG *doner, *swale, *left;
CELL *bas;
double half_res, diag, max_length, dep_slope;
struct Cell_head window;

int main(int argc, char *argv[])
{
    double N, E;
    int row, col, basin_fd, drain_fd;
    CELL *cell_buf;
    char drain_name[GNAME_MAX], E_f, dr_f, ba_f, N_f, errr;
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3, *opt4;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->description = _("Creates watershed basins.");
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("watershed"));
	
    opt1 = G_define_standard_option(G_OPT_R_INPUT);

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);
    
    opt3 = G_define_option();
    opt3->key = "easting";
    opt3->type = TYPE_STRING;
    opt3->key_desc = "x";
    opt3->multiple = NO;
    opt3->required = YES;
    opt3->description = _("Easting grid coordinates");

    opt4 = G_define_option();
    opt4->key = "northing";
    opt4->type = TYPE_STRING;
    opt4->key_desc = "y";
    opt4->multiple = NO;
    opt4->required = YES;
    opt4->description = _("Northing grid coordinates");

    /*   Parse command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_window(&window);

    strcpy(drain_name, opt1->answer);
    strcpy(basin_name, opt2->answer);
    if (!G_scan_easting(*opt3->answers, &E, G_projection())) {
	G_warning(_("Illegal east coordinate <%s>\n"), opt3->answer);
	G_usage();
	exit(EXIT_FAILURE);
    }
    if (!G_scan_northing(*opt4->answers, &N, G_projection())) {
	G_warning(_("Illegal north coordinate <%s>\n"), opt4->answer);
	G_usage();
	exit(EXIT_FAILURE);
    }

    if (E < window.west || E > window.east || N < window.south ||
	N > window.north) {
	G_warning(_("Warning, ignoring point outside window: \n    %.4f,%.4f\n"),
		  E, N);
    }

    G_get_set_window(&window);
    dr_f = ba_f = N_f = E_f = errr = 0;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    total = nrows * ncols;
    nrows_less_one = nrows - 1;
    ncols_less_one = ncols - 1;
    drain_fd = Rast_open_old(drain_name, "");

    drain_ptrs =
	(char *)G_malloc(sizeof(char) * size_array(&pt_seg, nrows, ncols));
    bas = (CELL *) G_calloc(size_array(&ba_seg, nrows, ncols), sizeof(CELL));
    cell_buf = Rast_allocate_c_buf();

    for (row = 0; row < nrows; row++) {
	Rast_get_c_row(drain_fd, cell_buf, row);
	for (col = 0; col < ncols; col++) {
	    if (cell_buf[col] == 0) 
		total--;
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
	for (col = 0; col < ncols; col++) {
	    cell_buf[col] = bas[SEG_INDEX(ba_seg, row, col)];
	    if (cell_buf[col] == 0)
		Rast_set_null_value(&cell_buf[col], 1, CELL_TYPE);
	}
	Rast_put_row(basin_fd, cell_buf, CELL_TYPE);
    }
    G_free(bas);
    G_free(cell_buf);
    Rast_close(basin_fd);

    return 0;
}
