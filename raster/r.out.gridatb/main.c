/*
 * r.out.gridatb: Exports GRASS raster map to GRIDATB.FOR map file (TOPMODEL)
 *
 * GRIDATB.FOR Author: Keith Beven <k.beven@lancaster.ac.uk>
 *
 *      Copyright (C) 2000 by the GRASS Development Team
 *      Author: Huidae Cho <grass4u@gmail.com>
 *              Hydro Laboratory, Kyungpook National University
 *              South Korea
 *
 *      This program is free software under the GPL (>=v2)
 *      Read the file COPYING coming with GRASS for details.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static void rdwr_gridatb(const char *iname, const char *file)
{
    int fd = Rast_open_old(iname, "");
    FILE *fp = fopen(file, "w");
    DCELL *dcell = Rast_allocate_d_buf();
    struct Cell_head cellhd;
    int row, col;

    Rast_get_window(&cellhd);

    fprintf(fp, "%s\n", Rast_get_cell_title(iname, ""));
    fprintf(fp, "%d %d %lf\n", cellhd.cols, cellhd.rows, cellhd.ns_res);

    for (row = 0; row < cellhd.rows; row++) {
	G_percent(row, cellhd.rows, 2);

	Rast_get_d_row(fd, dcell, row);

	for (col = 0; col < cellhd.cols; col++) {
	    if (Rast_is_d_null_value(&dcell[col]))
		fprintf(fp, "  9999.00 ");
	    else
		fprintf(fp, "%9.2f", (double) dcell[col]);
	    if (!((col + 1) % 8) || col == cellhd.cols - 1)
		fprintf(fp, "\n");
	}
    }

    Rast_close(fd);
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *input;
	struct Option *output;
    } params;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    module->description =
	_("Exports GRASS raster map to GRIDATB.FOR map file (TOPMODEL).");

    params.input = G_define_standard_option(G_OPT_R_INPUT);

    params.output = G_define_standard_option(G_OPT_F_OUTPUT);

    if (G_parser(argc, argv))
	exit(1);

    rdwr_gridatb(params.input->answer, params.output->answer);

    return EXIT_SUCCESS;
}

