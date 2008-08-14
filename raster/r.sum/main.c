
/****************************************************************************
 *
 * MODULE:       r.sum
 * AUTHOR(S):    Bill Brown, UIUC GIS Laboratory (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jachym Cepicky <jachym les-ejk.cz>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      sums up raster cell values
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* this module should be removed before GRASS 7 released (functionality now
   duplicated in r.statistics) */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

typedef int FILEDESC;

int main(int argc, char *argv[])
{

    struct GModule *module;
    struct Option *rast;
    char *cellmap;
    FILEDESC cellfile = 0;
    FCELL *dbuf, *ibuf;
    int row, col, shh = 0;
    struct Cell_head w;


    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description = _("Sums up the raster cell values.");

    rast = G_define_option();
    rast->key = "rast";
    rast->type = TYPE_STRING;
    rast->required = YES;
    rast->gisprompt = "old,cell,raster";
    rast->description = _("Name of incidence or density file.");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    cellmap = G_find_file2("cell", rast->answer, "");
    if (!cellmap) {
	G_fatal_error(_("Raster map <%s> not found"), rast->answer);
    }
    if ((cellfile = G_open_cell_old(rast->answer, cellmap)) == -1) {
	G_fatal_error(_("Unable to open raster map <%s>"), rast->answer);
    }


    G_get_set_window(&w);
    ibuf = (FCELL *) G_malloc(w.cols * w.rows * sizeof(FCELL));
    dbuf = (FCELL *) G_malloc(w.cols * w.rows * sizeof(FCELL));

    if (!shh)
	G_message(_("Reading %s..."), rast->answer);
    {
	FCELL *tf;
	double dsum = 0.0;

	tf = ibuf;
	for (row = 0; row < w.rows; row++) {

	    if (!shh)
		G_percent(row, w.rows - 1, 10);

	    G_get_f_raster_row(cellfile, tf, row);

	    for (col = 0; col < w.cols; col++) {
		if (G_is_f_null_value(tf))
		    *tf = 0.0;
		dsum += *tf;
		tf++;
	    }

	}
	fprintf(stdout, "SUM = %f\n", dsum);
    }

    G_free(ibuf);
    G_free(dbuf);
    G_close_cell(cellfile);

    return (0);

}
