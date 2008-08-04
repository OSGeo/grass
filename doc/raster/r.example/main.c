
/****************************************************************************
 *
 * MODULE:       r.example
 * AUTHOR(S):    Markus Neteler - neteler itc.it
 *               with hints from: Glynn Clements - glynn gclements.plus.com
 * PURPOSE:      Just copies a raster map, preserving the raster map type
 *               Intended to explain GRASS raster programming
 *
 * COPYRIGHT:    (C) 2002,2005 by the GRASS Development Team
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
#include <grass/glocale.h>

/* 
 * global function declaration 
 */
extern CELL f_c(CELL);
extern FCELL f_f(FCELL);
extern DCELL f_d(DCELL);

/*
 * function definitions 
 */

CELL c_calc(CELL x)
{
    /* we do nothing exciting here */
    return x;
}

FCELL f_calc(FCELL x)
{
    /* we do nothing exciting here */
    return x;
}

DCELL d_calc(DCELL x)
{
    /* we do nothing exciting here */
    return x;
}

/*
 * main function
 * it copies raster input raster map, calling the appropriate function for each
 * data type (CELL, DCELL, FCELL)
 */
int main(int argc, char *argv[])
{
    struct Cell_head cellhd;	/* it stores region information,
				   and header information of rasters */
    char *name;			/* input raster name */
    char *result;		/* output raster name */
    char *mapset;		/* mapset name */
    void *inrast;		/* input buffer */
    unsigned char *outrast;	/* output buffer */
    int nrows, ncols;
    int row, col;
    int infd, outfd;		/* file descriptor */
    int verbose;
    RASTER_MAP_TYPE data_type;	/* type of the map (CELL/DCELL/...) */
    struct History history;	/* holds meta-data (title, comments,..) */

    struct GModule *module;	/* GRASS module for parsing arguments */

    struct Option *input, *output;	/* options */
    struct Flag *flag1;		/* flags */

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    module->keywords = _("raster, keyword2, keyword3");
    module->description = _("My first raster module");

    /* Define the different options as defined in gis.h */
    input = G_define_standard_option(G_OPT_R_INPUT);

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    /* Define the different flags */
    flag1 = G_define_flag();
    flag1->key = 'q';
    flag1->description = _("Quiet");

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* stores options and flags to variables */
    name = input->answer;
    result = output->answer;
    verbose = (!flag1->answer);

    /* returns NULL if the map was not found in any mapset, 
     * mapset name otherwise */
    mapset = G_find_cell2(name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), name);

    if (G_legal_filename(result) < 0)
	G_fatal_error(_("<%s> is an illegal file name"), result);


    /* determine the inputmap type (CELL/FCELL/DCELL) */
    data_type = G_raster_map_type(name, mapset);

    /* G_open_cell_old - returns file destriptor (>0) */
    if ((infd = G_open_cell_old(name, mapset)) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), name);


    /* controlling, if we can open input raster */
    if (G_get_cellhd(name, mapset, &cellhd) < 0)
	G_fatal_error(_("Unable to read file header of <%s>"), name);

    G_debug(3, "number of rows %d", cellhd.rows);

    /* Allocate input buffer */
    inrast = G_allocate_raster_buf(data_type);

    /* Allocate output buffer, use input map data_type */
    nrows = G_window_rows();
    ncols = G_window_cols();
    outrast = G_allocate_raster_buf(data_type);

    /* controlling, if we can write the raster */
    if ((outfd = G_open_raster_new(result, data_type)) < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), result);

    /* for each row */
    for (row = 0; row < nrows; row++) {
	CELL c;
	FCELL f;
	DCELL d;

	if (verbose)
	    G_percent(row, nrows, 2);

	/* read input map */
	if (G_get_raster_row(infd, inrast, row, data_type) < 0)
	    G_fatal_error(_("Unable to read raster map <%s> row %d"), name,
			  row);

	/* process the data */
	for (col = 0; col < ncols; col++) {
	    /* use different function for each data type */
	    switch (data_type) {
	    case CELL_TYPE:
		c = ((CELL *) inrast)[col];
		c = c_calc(c);	/* calculate */
		((CELL *) outrast)[col] = c;
		break;
	    case FCELL_TYPE:
		f = ((FCELL *) inrast)[col];
		f = f_calc(f);	/* calculate */
		((FCELL *) outrast)[col] = f;
		break;
	    case DCELL_TYPE:
		d = ((DCELL *) inrast)[col];
		d = d_calc(d);	/* calculate */
		((DCELL *) outrast)[col] = d;
		break;
	    }
	}

	/* write raster row to output raster map */
	if (G_put_raster_row(outfd, outrast, data_type) < 0)
	    G_fatal_error(_("Failed writing raster map <%s>"), result);
    }

    /* memory cleanup */
    G_free(inrast);
    G_free(outrast);

    /* closing raster maps */
    G_close_cell(infd);
    G_close_cell(outfd);

    /* add command line incantation to history file */
    G_short_history(result, "raster", &history);
    G_command_history(&history);
    G_write_history(result, &history);


    exit(EXIT_SUCCESS);
}
