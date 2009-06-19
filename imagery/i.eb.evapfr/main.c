
/****************************************************************************
 *
 * MODULE:       i.eb.evapfr
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates the evaporative fraction
 *               as seen in Bastiaanssen (1995) 
 *
 * COPYRIGHT:    (C) 2002-2006 by the GRASS Development Team
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
double evap_fr(double r_net, double g0, double h0);
double soilmoisture(double evapfr);

int main(int argc, char *argv[]) 
{
    int nrows, ncols;
    int row, col;
    int makin = 0;		/*Makin Flag for root zone soil moisture output */
    struct GModule *module;
    struct Option *input1, *input2, *input3, *output1, *output2;
    struct Flag *flag1, *flag2;
    struct History history;	/*metadata */
    char *result1, *result2;	/*output raster name */
    
    int infd_rnet, infd_g0, infd_h0;
    int outfd1, outfd2;
    char *rnet, *g0, *h0;
    char *evapfr, *theta;
    void *inrast_rnet, *inrast_g0, *inrast_h0;

    DCELL * outrast1, *outrast2;

    /************************************/ 
    G_gisinit(argv[0]);
    module = G_define_module();
    module->keywords =
	_("evaporative fraction, soil moisture, energy balance, SEBAL");
    module->description =
	_("evaporative fraction (Bastiaanssen, 1995) and root zone soil moisture (Makin, Molden and Bastiaanssen, 2001)");
    
    /* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);
    input1->key = _("rnet");
    input1->description = _("Name of the Net Radiation map [W/m2]");
    input1->answer = _("rnet");

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = _("g0");
    input2->description = _("Name of the soil heat flux map [W/m2]");
    input2->answer = _("g0");

    input3 = G_define_standard_option(G_OPT_R_INPUT);
    input3->key = _("h0");
    input3->description = _("Name of the sensible heat flux map [W/m2]");
    input3->answer = _("h0");

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);
    output1->key = _("evapfr");
    output1->description =
	_("Name of the output evaporative fraction layer");
    output1->answer = _("evapfr");

    output2 = G_define_standard_option(G_OPT_R_OUTPUT);
    output2->key = _("theta");
    output2->required = NO;
    output2->description =
	_("Name of the output root zone soil moisture layer");
    output2->answer = _("theta");

    flag1 = G_define_flag();
    flag1->key = 'm';
    flag1->description =
	_("root zone soil moisture output (Makin, Molden and Bastiaanssen, 2001)");

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
    if ((infd_rnet = G_open_cell_old(rnet, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), rnet);
    inrast_rnet = G_allocate_d_raster_buf();
    
    /***************************************************/ 
    if ((infd_g0 = G_open_cell_old(g0, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), g0);
    inrast_g0 = G_allocate_d_raster_buf();
    
    /***************************************************/ 
    if ((infd_h0 = G_open_cell_old(h0, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), h0);
    inrast_h0 = G_allocate_d_raster_buf();
    
    /***************************************************/ 
    nrows = G_window_rows();
    ncols = G_window_cols();
    outrast1 = G_allocate_d_raster_buf();
    if (makin) 
	outrast2 = G_allocate_d_raster_buf();
    
    /* Create New raster files */ 
    if ((outfd1 = G_open_raster_new(result1,DCELL_TYPE)) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), result1);
    if (makin) 
	if ((outfd2 = G_open_raster_new(result2,DCELL_TYPE)) < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"), result2);
        
    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	DCELL d;
	DCELL d_rnet;
	DCELL d_g0;
	DCELL d_h0;
	G_percent(row, nrows, 2);
	
        /* read input maps */ 
        if (G_get_d_raster_row(infd_rnet, inrast_rnet, row)<0)
	    G_fatal_error(_("Unable to read from <%s>"), rnet);
	if (G_get_d_raster_row(infd_g0, inrast_g0, row) < 0)
	    G_fatal_error(_("Unable to read from <%s>"), g0);
	if (G_get_d_raster_row(infd_h0, inrast_h0, row) < 0)
	    G_fatal_error(_("Unable to read from <%s>"), h0);
        /*process the data */ 
        for (col = 0; col < ncols; col++)
        {
            d_rnet = ((DCELL *) inrast_rnet)[col];
            d_g0 = ((DCELL *) inrast_g0)[col];
            d_h0 = ((DCELL *) inrast_h0)[col];
	    if (G_is_d_null_value(&d_rnet) || 
                G_is_d_null_value(&d_g0) ||
		G_is_d_null_value(&d_h0)) {
		G_set_d_null_value(&outrast1[col], 1);
		if (makin) 
		    G_set_d_null_value(&outrast2[col], 1);
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
	if (G_put_d_raster_row(outfd1, outrast1) < 0)
	    G_fatal_error(_("Unable to write to output raster map"));
	if (makin) 
        {
            if (G_put_d_raster_row(outfd2, outrast2) < 0)
                G_fatal_error(_("Unable to write to output raster map"));
        }
    }
    G_free(inrast_rnet);
    G_free(inrast_g0);
    G_free(inrast_h0);
    G_close_cell(infd_rnet);
    G_close_cell(infd_g0);
    G_close_cell(infd_h0);
    G_free(outrast1);
    G_free(outrast2);
    if (makin) {
	G_close_cell(outfd1);
	G_close_cell(outfd2);
    }
    G_short_history(result1, "raster", &history);
    G_command_history(&history);
    G_write_history(result1, &history);
    if (makin) {
	G_short_history(result2, "raster", &history);
	G_command_history(&history);
	G_write_history(result2, &history);
    }
    exit(EXIT_SUCCESS);
}


