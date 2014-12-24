/****************************************************************************
 *
 * MODULE:       i.eb.netrad
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates the instantaneous net radiation at 
 *               as seen in Bastiaanssen (1995) using time of
 *               satellite overpass.
 *
 * COPYRIGHT:    (C) 2006-2011 by the GRASS Development Team
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

double r_net(double bbalb, double ndvi, double tempk, double dtair,
	      double e0, double tsw, double doy, double utc,
	      double sunzangle);
int main(int argc, char *argv[]) 
{
    struct Cell_head cellhd;	/*region+header info */
    char *mapset;		/*mapset name */
    int nrows, ncols;
    int row, col;
    struct GModule *module;
    struct Option *input1, *input2, *input3, *input4, *input5;
    struct Option *input6, *input7, *input8, *input9, *output1;
    struct Flag *flag1;
    struct History history;	/*metadata */
    struct Colors colors;	/*Color rules */
    char *name;			/*input raster name */
    char *result;		/*output raster name */
    int infd_albedo, infd_ndvi, infd_tempk, infd_time, infd_dtair;
    int infd_emissivity, infd_tsw, infd_doy, infd_sunzangle;
    int outfd;
    char *albedo, *ndvi, *tempk, *time, *dtair, *emissivity;
    char *tsw, *doy, *sunzangle;
    int i = 0, j = 0;
    void *inrast_albedo, *inrast_ndvi, *inrast_tempk, *inrast_rnet;
    void *inrast_time, *inrast_dtair, *inrast_emissivity, *inrast_tsw;
    void *inrast_doy, *inrast_sunzangle;
    DCELL * outrast;
    CELL val1,val2; /*For color range*/

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("energy balance"));
    G_add_keyword(_("net radiation"));
    G_add_keyword(_("SEBAL"));
    module->description =
	_("Net radiation approximation (Bastiaanssen, 1995).");
    
	/* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);
    input1->key = "albedo";
    input1->description = _("Name of albedo raster map [0.0;1.0]");

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = "ndvi";
    input2->description = _("Name of NDVI raster map [-1.0;+1.0]");

    input3 = G_define_standard_option(G_OPT_R_INPUT);
    input3->key = "temperature";
    input3->description =
	_("Name of surface temperature raster map [K]");

    input4 = G_define_standard_option(G_OPT_R_INPUT);
    input4->key = "localutctime";
    input4->description =
	_("Name of time of satellite overpass raster map [local time in UTC]"); 

    input5 = G_define_standard_option(G_OPT_R_INPUT);
    input5->key = "temperaturedifference2m";
    input5->description =
	_("Name of the difference map of temperature from surface skin to about 2 m height [K]");

    input6 = G_define_standard_option(G_OPT_R_INPUT);
    input6->key = "emissivity";
    input6->description = _("Name of the emissivity map [-]");

    input7 = G_define_standard_option(G_OPT_R_INPUT);
    input7->key = "transmissivity_singleway";
    input7->description =
	_("Name of the single-way atmospheric transmissivitymap [-]");

    input8 = G_define_standard_option(G_OPT_R_INPUT);
    input8->key = "dayofyear";
    input8->description = _("Name of the Day Of Year (DOY) map [-]");

    input9 = G_define_standard_option(G_OPT_R_INPUT);
    input9->key = "sunzenithangle";
    input9->description = _("Name of the sun zenith angle map [degrees]");

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);
    output1->description = _("Name of the output net radiation layer");
    
    /********************/ 
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    albedo = input1->answer;
    ndvi = input2->answer;
    tempk = input3->answer;
    time = input4->answer;
    dtair = input5->answer;
    emissivity = input6->answer;
    tsw = input7->answer;
    doy = input8->answer;
    sunzangle = input9->answer;
    result = output1->answer;
    
    /* Open access to image files and allocate row access memory */
    infd_albedo = Rast_open_old(albedo, "");
    inrast_albedo = Rast_allocate_d_buf();
    
    infd_ndvi = Rast_open_old(ndvi, "");
    inrast_ndvi = Rast_allocate_d_buf();

    infd_tempk = Rast_open_old(tempk, "");
    inrast_tempk = Rast_allocate_d_buf();

    infd_dtair = Rast_open_old(dtair, "");
    inrast_dtair = Rast_allocate_d_buf();

    infd_time = Rast_open_old(time, "");
    inrast_time = Rast_allocate_d_buf();

    infd_emissivity = Rast_open_old(emissivity, "");
    inrast_emissivity = Rast_allocate_d_buf();
    
    infd_tsw = Rast_open_old(tsw, "");
    inrast_tsw = Rast_allocate_d_buf();
    
    infd_doy = Rast_open_old(doy, "");
    inrast_doy = Rast_allocate_d_buf();
    
    infd_sunzangle = Rast_open_old(sunzangle, "");
    inrast_sunzangle = Rast_allocate_d_buf();
    
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    outfd = Rast_open_new(result, DCELL_TYPE);
    outrast = Rast_allocate_d_buf();
    
    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	DCELL d;
	DCELL d_albedo;
	DCELL d_ndvi;
	DCELL d_tempk;
	DCELL d_dtair;
	DCELL d_time;
	DCELL d_emissivity;
	DCELL d_tsw;
	DCELL d_doy;
	DCELL d_sunzangle;

	/* Display row process percentage */
	G_percent(row, nrows, 2);

	/* Load rows for each input image  */
	Rast_get_d_row(infd_albedo, inrast_albedo, row);
	Rast_get_d_row(infd_ndvi, inrast_ndvi, row);
	Rast_get_d_row(infd_tempk, inrast_tempk, row);
	Rast_get_d_row(infd_dtair, inrast_dtair, row);
	Rast_get_d_row(infd_time, inrast_time, row);
	Rast_get_d_row(infd_emissivity, inrast_emissivity, row);
	Rast_get_d_row(infd_tsw, inrast_tsw, row);
	Rast_get_d_row(infd_doy, inrast_doy, row);
	Rast_get_d_row(infd_sunzangle, inrast_sunzangle, row);
	
        /*process the data */ 
        for (col = 0; col < ncols; col++)
        {
            d_albedo = (double)((DCELL *) inrast_albedo)[col];
            d_ndvi = (double)((DCELL *) inrast_ndvi)[col];
            d_tempk = (double)((DCELL *) inrast_tempk)[col];
            d_dtair = (double)((DCELL *) inrast_dtair)[col];
            d_time = (double)((DCELL *) inrast_time)[col];
            d_emissivity = (double)((DCELL *) inrast_emissivity)[col];
            d_tsw = (double)((DCELL *) inrast_tsw)[col];
            d_doy = (double)((DCELL *) inrast_doy)[col];
            d_sunzangle = (double)((DCELL *) inrast_sunzangle)[col];
            /* process NULL Values */
	    if (Rast_is_d_null_value(&d_albedo) ||
	         Rast_is_d_null_value(&d_ndvi) ||
		 Rast_is_d_null_value(&d_tempk) ||
		 Rast_is_d_null_value(&d_dtair) || 
		 Rast_is_d_null_value(&d_time) ||
		 Rast_is_d_null_value(&d_emissivity) ||
		 Rast_is_d_null_value(&d_tsw) || 
		 Rast_is_d_null_value(&d_doy) ||
		 Rast_is_d_null_value(&d_sunzangle)) {
		Rast_set_d_null_value(&outrast[col], 1);
	    }
	    else {
                 /************************************/ 
		 /* calculate the net radiation      */ 
		 d = r_net(d_albedo, d_ndvi, d_tempk, d_dtair, d_emissivity, d_tsw, d_doy, d_time, d_sunzangle);
		 outrast[col] = d;
	    }
	}
	Rast_put_d_row(outfd, outrast);
    }
    G_free(inrast_albedo);
    G_free(inrast_ndvi);
    G_free(inrast_tempk);
    G_free(inrast_dtair);
    G_free(inrast_time);
    G_free(inrast_emissivity);
    G_free(inrast_tsw);
    G_free(inrast_doy);
    G_free(inrast_sunzangle);
    Rast_close(infd_albedo);
    Rast_close(infd_ndvi);
    Rast_close(infd_tempk);
    Rast_close(infd_dtair);
    Rast_close(infd_time);
    Rast_close(infd_emissivity);
    Rast_close(infd_tsw);
    Rast_close(infd_doy);
    Rast_close(infd_sunzangle);
    G_free(outrast);
    Rast_close(outfd);
    
    /* Colors in grey shade */ 
    Rast_init_colors(&colors);
    val1=0;
    val2=900;
    Rast_add_c_color_rule(&val1, 0, 0, 0, &val2, 255, 255, 255, &colors);
    
    /* Metadata */ 
    Rast_short_history(result, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result, &history);
    exit(EXIT_SUCCESS);
}


