
/****************************************************************************
 *
 * MODULE:       r.surf.contour
 * AUTHOR(S):    Chuck Ehlschlaeger (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      interpolates a raster elevation map from a rasterized
 *               contour map
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
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
#include <grass/glocale.h>

int nrows;
int ncols;
int minc;
int minr;
int maxc;
int maxr;
int array_size;
double i_val_l_f;
CELL **con;
FLAG *seen, *mask;
NODE *zero;
CELL on, off;

int main(int argc, char *argv[])
{
    int r, c;
    CELL con1, con2;
    double d1, d2;
    CELL *alt_row;
    const char *con_name, *alt_name, *con_mapset;
    int file_fd;
    CELL value;
    struct History history;
    struct GModule *module;
    struct Flag *flag1;
    struct Option *opt1, *opt2;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Surface generation program from rasterized contours.");

    opt1 = G_define_option();
    opt1->key = "input";
    opt1->type = TYPE_STRING;
    opt1->required = YES;
    opt1->gisprompt = "old,cell,raster";
    opt1->description = _("Name of existing raster map containing contours");

    opt2 = G_define_option();
    opt2->key = "output";
    opt2->type = TYPE_STRING;
    opt2->required = YES;
    opt2->gisprompt = "new,cell,raster";
    opt2->description = _("Output elevation raster map");

    flag1 = G_define_flag();
    flag1->key = 'f';
    flag1->description = _("Unused; retained for compatibility purposes, "
			   "will be removed in future");

    on = 1;
    off = 0;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    con_name = opt1->answer;
    alt_name = opt2->answer;

    con_mapset = G_find_cell2(con_name, "");
    if (!con_mapset)
	G_fatal_error("Contour raster map [%s] not found", con_name);

    nrows = G_window_rows();
    ncols = G_window_cols();
    i_val_l_f = nrows + ncols;
    con = read_cell(con_name, con_mapset);
    alt_row = (CELL *) G_malloc(ncols * sizeof(CELL));
    seen = flag_create(nrows, ncols);
    mask = flag_create(nrows, ncols);
    if (NULL != G_find_file("cell", "MASK", G_mapset())) {
	if ((file_fd = G_open_cell_old("MASK", G_mapset())) < 0)
	    G_fatal_error("Unable to open MASK");
	for (r = 0; r < nrows; r++) {
	    G_get_map_row_nomask(file_fd, alt_row, r);
	    for (c = 0; c < ncols; c++)
		if (!alt_row[c])
		    FLAG_SET(mask, r, c);
	}
	G_close_cell(file_fd);
    }
    zero = (NODE *) G_malloc(INIT_AR * sizeof(NODE));
    minc = minr = 0;
    maxc = ncols - 1;
    maxr = nrows - 1;
    array_size = INIT_AR;
    file_fd = G_open_cell_new(alt_name);
    if (!file_fd)
	G_fatal_error("Unable to open output map");
    for (r = 0; r < nrows; r++) {
	G_percent(r, nrows, 1);
	for (c = 0; c < ncols; c++) {
	    if (FLAG_GET(mask, r, c))
		continue;
	    value = con[r][c];
	    if (value != 0) {
		alt_row[c] = value;
		continue;
	    }
	    find_con(r, c, &d1, &d2, &con1, &con2);
	    if (con2 > 0)
		alt_row[c] = (CELL) (d2 * con1 / (d1 + d2) +
				     d1 * con2 / (d1 + d2) + 0.5);
	    else
		alt_row[c] = con1;
	}
	G_put_raster_row(file_fd, alt_row, CELL_TYPE);
    }
    G_percent(r, nrows, 1);
    free_cell(con);
    flag_destroy(seen);
    flag_destroy(mask);
    G_close_cell(file_fd);

    G_short_history(alt_name, "raster", &history);
    G_command_history(&history);
    G_write_history(alt_name, &history);

    exit(EXIT_SUCCESS);
}
