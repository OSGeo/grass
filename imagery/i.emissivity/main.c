/****************************************************************************
 *
 * MODULE:       i.emissivity
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates the emissivity from NDVI (empirical)
 *                as seen in Caselles and Colles (1997). 
 *
 * COPYRIGHT:    (C) 2002-2008 by the GRASS Development Team
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

double emissivity_generic(double ndvi);

int main(int argc, char *argv[]) 
{
    int nrows, ncols;
    int row, col;
    struct GModule *module;
    struct Option *input, *output;
    struct History history;	/*metadata */
    
    /************************************/ 
    char *result1;    /*output raster name */
    int infd, outfd;    /*File Descriptors */ 
    char *ndvi;
    char *emissivity;
    void *inr;
    DCELL * outr;

    /************************************/ 
    G_gisinit(argv[0]);
    module = G_define_module();
    module->keywords = _("emissivity, land flux, energy balance");
    module->description =
	_("Emissivity from NDVI, generic method for spares land.");
    
    /* Define the different options */ 
    input = G_define_standard_option(G_OPT_R_INPUT);
    input->description = _("Name of the NDVI map [-]");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->description = _("Name of the output emissivity layer");
    
    /********************/ 
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    ndvi = input->answer;
    result1 = output->answer;
    
    /***************************************************/ 
    if ((infd = G_open_cell_old(ndvi, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), ndvi);
    inr = G_allocate_d_raster_buf();
    
    /***************************************************/ 
    nrows = G_window_rows();
    ncols = G_window_cols();
    outr = G_allocate_d_raster_buf();
    
    /* Create New raster files */ 
    if ((outfd = G_open_raster_new(result1, DCELL_TYPE)) < 0)
	G_fatal_error(_("Could not open <%s>"), result1);
    
    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	DCELL d;
	DCELL d_ndvi;
	G_percent(row, nrows, 2);
	
        /* read input maps */ 
        if (G_get_raster_row(infd,inr,row,DCELL_TYPE)< 0)
	    G_fatal_error(_("Could not read from <%s>"), ndvi);
	
        /*process the data */ 
        for (col = 0; col < ncols; col++)
        {
            d_ndvi = ((DCELL *) inr)[col];
	    if (G_is_d_null_value(&d_ndvi)) 
		G_set_d_null_value(&outr[col], 1);
	    else {
                /****************************/ 
                /* calculate emissivity     */ 
                d = emissivity_generic(d_ndvi);
		outr[col] = d;
	    }
        }
	if (G_put_raster_row(outfd, outr, DCELL_TYPE) < 0)
	    G_fatal_error(_("Cannot write to output raster file"));
    }
    G_free(inr);
    G_close_cell(infd);
    G_free(outr);
    G_close_cell(outfd);

    G_short_history(result1, "raster", &history);
    G_command_history(&history);
    G_write_history(result1, &history);

    exit(EXIT_SUCCESS);
}


