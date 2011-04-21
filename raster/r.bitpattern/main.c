
/****************************************************************************
 *
 * MODULE:      r.bitpattern
 * AUTHOR(S):   Radim Blazek
 * PURPOSE:     bit pattern comparison
 *		Functionality:
 *		1. define position: set bit(s) to 1 you want to match
 *		   then convert this position pattern to integer, set pattern=
 *		   parameter with that integer value
 *		2. define pattern *value* which should be in that position:
 *		   first bit pattern of value, convert to integer, set
 *		   patval= parameter
 *
 *		128 64 32 16 8 4 2 1
 *		Example:
 *		1. define position 
 *			xx xx 1x xx
 *			binary: 1000 -> integer: 8 -> pattern=8
 *		2. define value 
 *                      Ex.: we want to check for 0 in that position
 *			xx xx 0x xx
 *			binary: 0000 -> integer: 0 -> patval=0
 *                 if value can be arbitray (0/1), then assume 0 value
 *
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

extern CELL f_c(CELL);

int main(int argc, char *argv[])
{
    struct Cell_head cellhd;
    char *name, *result;
    void *inrast;
    unsigned char *outrast;
    int nrows, ncols;
    int row, col;
    int infd, outfd;
    int verbose;
    RASTER_MAP_TYPE data_type;
    int pat, patv;
    struct GModule *module;
    struct Option *input, *output, *pattern, *patval;
    struct Flag *flag1;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("algebra"));
    module->description = _("Compares bit patterns with a raster map.");

    /* Define the different options */

    input = G_define_standard_option(G_OPT_R_INPUT);

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    pattern = G_define_option();
    pattern->key = "pattern";
    pattern->type = TYPE_INTEGER;
    pattern->required = YES;
    pattern->description = _("Bit pattern position(s)");

    patval = G_define_option();
    patval->key = "patval";
    patval->type = TYPE_INTEGER;
    patval->required = YES;
    patval->description = _("Bit pattern value");

    /* Define the different flags */

    flag1 = G_define_flag();
    flag1->key = 'q';
    flag1->description = _("Quiet");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = input->answer;
    result = output->answer;
    verbose = (!flag1->answer);
    pat = atoi(pattern->answer);
    patv = atoi(patval->answer);

    infd = Rast_open_old(name, "");

    /* determine the inputmap type (CELL/FCELL/DCELL) */
    data_type = Rast_get_map_type(infd);

    Rast_get_cellhd(name, "", &cellhd);

    /* Allocate input buffer */
    inrast = Rast_allocate_buf(data_type);

    /* Allocate output buffer, use input map data_type */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast = Rast_allocate_buf(data_type);

    outfd = Rast_open_new(result, data_type);

    for (row = 0; row < nrows; row++) {
	CELL c;

	if (verbose)
	    G_percent(row, nrows, 2);

	/* read input map */
	Rast_get_row(infd, inrast, row, data_type);

	/*process the data */
	for (col = 0; col < ncols; col++) {

	    c = ((CELL *) inrast)[col];
	    /*((CELL *) outrast)[col] = c; */
	    if ((c & pat) == patv)
		((CELL *) outrast)[col] = 1;
	    else
		((CELL *) outrast)[col] = 0;

	}

	Rast_put_row(outfd, outrast, data_type);
    }

    G_free(inrast);
    G_free(outrast);
    Rast_close(infd);
    Rast_close(outfd);

    return (EXIT_SUCCESS);
}
