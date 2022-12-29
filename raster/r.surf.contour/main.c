
/****************************************************************************
 *
 * MODULE:       r.surf.contour
 * AUTHOR(S):    Chuck Ehlschlaeger (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>,
 *               Markus Metz
 * PURPOSE:      Interpolates a raster elevation map from a rasterized
 *               contour map
 * COPYRIGHT:    (C) 1999-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "contour.h"
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int nrows;
int ncols;
int minc;
int minr;
int maxc;
int maxr;
int array_size;
double i_val_l_f;
DCELL **con;
FLAG *seen, *mask;
NODE *zero;

int main(int argc, char *argv[])
{
    int r, c;
    DCELL con1, con2;
    double d1, d2;
    DCELL *alt_row;
    const char *con_name, *alt_name;
    int file_fd;
    DCELL value;
    struct History history;
    struct GModule *module;
    struct Option *opt1, *opt2;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("interpolation"));
    module->description =
	_("Generates surface raster map from rasterized contours.");

    opt1 = G_define_standard_option(G_OPT_R_INPUT);
    opt1->description = _("Name of input raster map containing contours");

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    con_name = opt1->answer;
    alt_name = opt2->answer;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    i_val_l_f = nrows + ncols;
    con = read_cell(con_name);
    alt_row = (DCELL *) G_malloc(ncols * sizeof(DCELL));
    seen = flag_create(nrows, ncols);
    mask = flag_create(nrows, ncols);
    if (NULL != G_find_file("cell", "MASK", G_mapset())) {
	file_fd = Rast_open_old("MASK", G_mapset());
	for (r = 0; r < nrows; r++) {
	    Rast_get_d_row_nomask(file_fd, alt_row, r);
	    for (c = 0; c < ncols; c++)
		if (Rast_is_d_null_value(&(alt_row[c])) || alt_row[c] == 0)
		    FLAG_SET(mask, r, c);
	}
	Rast_close(file_fd);
    }
    zero = (NODE *) G_malloc(INIT_AR * sizeof(NODE));
    minc = minr = 0;
    maxc = ncols - 1;
    maxr = nrows - 1;
    array_size = INIT_AR;
    file_fd = Rast_open_new(alt_name, DCELL_TYPE);
    for (r = 0; r < nrows; r++) {
	G_percent(r, nrows, 1);
	Rast_set_d_null_value(alt_row, ncols);
	for (c = 0; c < ncols; c++) {
	    if (FLAG_GET(mask, r, c))
		continue;
	    value = con[r][c];
	    if (!Rast_is_d_null_value(&value)) {
		alt_row[c] = value;
		continue;
	    }
	    find_con(r, c, &d1, &d2, &con1, &con2);
	    if (!Rast_is_d_null_value(&con2))
		alt_row[c] = d2 * con1 / (d1 + d2) + 
		             d1 * con2 / (d1 + d2);
	    else
		alt_row[c] = con1;
	}
	Rast_put_row(file_fd, alt_row, DCELL_TYPE);
    }
    G_percent(1, 1, 1);
    
    free_cell(con);
    flag_destroy(seen);
    flag_destroy(mask);
    Rast_close(file_fd);
    
    Rast_short_history(alt_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(alt_name, &history);

    exit(EXIT_SUCCESS);
}
