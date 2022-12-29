/****************************************************************************
 *
 * MODULE:       i.eb.evapfr
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates the evaporative fraction
 *               as seen in Bastiaanssen (1995) 
 *
 * COPYRIGHT:    (C) 2002-2011 by the GRASS Development Team
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

double evap_fr(double r_net, double g0, double h0);
double soilmoisture(double evapfr);

int main(int argc, char *argv[]) 
{
    int nrows, ncols;
    int row, col;
    int makin = 0;		/*Makin Flag for root zone soil moisture output */
    struct GModule *module;
    struct Option *input1, *input2, *input3, *output1, *output2;
    struct Flag *flag1;
    struct History history;	/*metadata */
    char *result1, *result2;	/*output raster name */
    
    int infd_rnet, infd_g0, infd_h0;
    int outfd1, outfd2;
    char *rnet, *g0, *h0;
    void *inrast_rnet, *inrast_g0, *inrast_h0;

    DCELL * outrast1, *outrast2;

    /************************************/ 
    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("energy balance"));
    G_add_keyword(_("soil moisture"));
    G_add_keyword(_("evaporative fraction"));
    G_add_keyword(_("SEBAL"));
    module->description =
	_("Computes evaporative fraction and root zone soil moisture.");
    
    /* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);
    input1->key = "netradiation";
    input1->description = _("Name of Net Radiation raster map [W/m2]");

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = "soilheatflux";
    input2->description = _("Name of soil heat flux raster map [W/m2]");

    input3 = G_define_standard_option(G_OPT_R_INPUT);
    input3->key = "sensibleheatflux";
    input3->description = _("Name of sensible heat flux raster map [W/m2]");

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);
    output1->key = "evaporativefraction";
    output1->description =
	_("Name for output evaporative fraction raster map");

    output2 = G_define_standard_option(G_OPT_R_OUTPUT);
    output2->key = "soilmoisture";
    output2->required = NO;
    output2->description =
	_("Name for output root zone soil moisture raster map");

    flag1 = G_define_flag();
    flag1->key = 'm';
    flag1->description =
	_("Root zone soil moisture output (Makin, Molden and Bastiaanssen, 2001)");
    
    /********************/
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    rnet = input1->answer;
    g0 = input2->answer;
    h0 = input3->answer;
    result1 = output1->answer;
    result2 = output2->answer;
    makin = flag1->answer;
    
    /***************************************************/ 
    infd_rnet = Rast_open_old(rnet, "");
    inrast_rnet = Rast_allocate_d_buf();
    
    /***************************************************/ 
    infd_g0 = Rast_open_old(g0, "");
    inrast_g0 = Rast_allocate_d_buf();
    
    /***************************************************/ 
    infd_h0 = Rast_open_old(h0, "");
    inrast_h0 = Rast_allocate_d_buf();
    
    /***************************************************/ 
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast1 = Rast_allocate_d_buf();

    if (makin) 
	outrast2 = Rast_allocate_d_buf();
    
    /* Create New raster files */ 
    outfd1 = Rast_open_new(result1, DCELL_TYPE);
    if (makin) 
	outfd2 = Rast_open_new(result2, DCELL_TYPE);
        
    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	DCELL d;
	DCELL d_rnet;
	DCELL d_g0;
	DCELL d_h0;
	G_percent(row, nrows, 2);
	
        /* read input maps */ 
        Rast_get_d_row(infd_rnet, inrast_rnet, row);
	Rast_get_d_row(infd_g0, inrast_g0, row);
	Rast_get_d_row(infd_h0, inrast_h0, row);
	
        /*process the data */ 
        for (col = 0; col < ncols; col++)
        {
            d_rnet = ((DCELL *) inrast_rnet)[col];
            d_g0 = ((DCELL *) inrast_g0)[col];
            d_h0 = ((DCELL *) inrast_h0)[col];
	    if (Rast_is_d_null_value(&d_rnet) || 
                Rast_is_d_null_value(&d_g0) ||
		Rast_is_d_null_value(&d_h0)) {
		Rast_set_d_null_value(&outrast1[col], 1);
		if (makin) 
		    Rast_set_d_null_value(&outrast2[col], 1);
	    }
	    else {
                /* calculate evaporative fraction       */ 
                d = evap_fr(d_rnet, d_g0, d_h0);
		outrast1[col] = d;
                /* calculate soil moisture              */ 
                if (makin) 
                {
                    d = soilmoisture(d);
                    outrast2[col] = d;
		}
	    }
        }
	Rast_put_d_row(outfd1, outrast1);
	if (makin) 
            Rast_put_d_row(outfd2, outrast2);
    }
    G_free(inrast_rnet);
    G_free(inrast_g0);
    G_free(inrast_h0);
    Rast_close(infd_rnet);
    Rast_close(infd_g0);
    Rast_close(infd_h0);
    G_free(outrast1);
    Rast_close(outfd1);
    if (makin) {
        G_free(outrast2);
	Rast_close(outfd2);
    }
    Rast_short_history(result1, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result1, &history);
    if (makin) {
	Rast_short_history(result2, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(result2, &history);
    }

    exit(EXIT_SUCCESS);
}


