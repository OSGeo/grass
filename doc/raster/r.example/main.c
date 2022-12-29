
/****************************************************************************
 *
 * MODULE:       r.example
 * AUTHOR(S):    Markus Neteler - neteler itc.it
 *               with hints from: Glynn Clements - glynn gclements.plus.com
 * PURPOSE:      Just copies a raster map, preserving the raster map type
 *               Intended to explain GRASS raster programming
 *
 * COPYRIGHT:    (C) 2002, 2005-2009 by the GRASS Development Team
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
    RASTER_MAP_TYPE data_type;	/* type of the map (CELL/DCELL/...) */
    struct History history;	/* holds meta-data (title, comments,..) */

    struct GModule *module;	/* GRASS module for parsing arguments */

    struct Option *input, *output;	/* options */

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("keyword2"));
    G_add_keyword(_("keyword3"));
    module->description = _("My first raster module");

    /* Define the different options as defined in gis.h */
    input = G_define_standard_option(G_OPT_R_INPUT);

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* stores options and flags to variables */
    name = input->answer;
    result = output->answer;

    /* returns NULL if the map was not found in any mapset, 
     * mapset name otherwise */
    mapset = (char *) G_find_raster2(name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), name);

    /* determine the inputmap type (CELL/FCELL/DCELL) */
    data_type = Rast_map_type(name, mapset);

    /* Rast_open_old - returns file destriptor (>0) */
    infd = Rast_open_old(name, mapset);

    /* controlling, if we can open input raster */
    Rast_get_cellhd(name, mapset, &cellhd);

    G_debug(3, "number of rows %d", cellhd.rows);

    /* Allocate input buffer */
    inrast = Rast_allocate_buf(data_type);

    /* Allocate output buffer, use input map data_type */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast = Rast_allocate_buf(data_type);

    /* controlling, if we can write the raster */
    outfd = Rast_open_new(result, data_type);

    /* for each row */
    for (row = 0; row < nrows; row++) {
	CELL c;
	FCELL f;
	DCELL d;

	G_percent(row, nrows, 2);

	/* read input map */
	Rast_get_row(infd, inrast, row, data_type);

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
	Rast_put_row(outfd, outrast, data_type);
    }

    /* memory cleanup */
    G_free(inrast);
    G_free(outrast);

    /* closing raster maps */
    Rast_close(infd);
    Rast_close(outfd);

    /* add command line incantation to history file */
    Rast_short_history(result, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result, &history);


    exit(EXIT_SUCCESS);
}
