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
#include <grass/raster.h>
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
    G_add_keyword(_("imagery"));
    G_add_keyword(_("emissivity"));
    G_add_keyword(_("land flux"));
    G_add_keyword(_("energy balance"));
    module->description =
	_("Computes emissivity from NDVI, generic method for sparse land.");
    
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
    infd = Rast_open_old(ndvi, "");
    inr = Rast_allocate_d_buf();
    
    /***************************************************/ 
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outr = Rast_allocate_d_buf();
    
    /* Create New raster files */ 
    outfd = Rast_open_new(result1, DCELL_TYPE);
    
    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	DCELL d;
	DCELL d_ndvi;
	G_percent(row, nrows, 2);
	
        /* read input maps */ 
        Rast_get_row(infd,inr,row,DCELL_TYPE);
	
        /*process the data */ 
        for (col = 0; col < ncols; col++)
        {
            d_ndvi = ((DCELL *) inr)[col];
	    if (Rast_is_d_null_value(&d_ndvi)) 
		Rast_set_d_null_value(&outr[col], 1);
	    else {
                /****************************/ 
                /* calculate emissivity     */ 
                d = emissivity_generic(d_ndvi);
		outr[col] = d;
	    }
        }
	Rast_put_row(outfd, outr, DCELL_TYPE);
    }
    G_free(inr);
    Rast_close(infd);
    G_free(outr);
    Rast_close(outfd);

    Rast_short_history(result1, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result1, &history);

    exit(EXIT_SUCCESS);
}


