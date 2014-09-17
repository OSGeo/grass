/****************************************************************************
 *
 * MODULE:       i.eb.soilheatflux
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates an approximation of soil heat flux
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

double g_0(double bbalb, double ndvi, double tempk, double rnet,
	     double time, int roerink);

int main(int argc, char *argv[]) 
{
    int nrows, ncols;
    int row, col;
    int roerink = 0;		/*Roerink Flag for HAPEX-Sahel conditions */
    struct GModule *module;
    struct Option *input1, *input2, *input3, *input4, *input5, *output1;
    struct Flag *flag1;
    struct History history;	/*metadata */
    struct Colors colors;	/*metadata */
    char *result;		/*output raster name */
    int infd_albedo, infd_ndvi, infd_tempk, infd_rnet, infd_time;
    int outfd;
    char *albedo, *ndvi, *tempk, *rnet, *time;
    void *inrast_albedo, *inrast_ndvi, *inrast_tempk, *inrast_rnet,
	*inrast_time;
    DCELL * outrast;
    CELL val1, val2;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("energy balance"));
    G_add_keyword(_("soil heat flux"));
    G_add_keyword(_("SEBAL"));
    module->description = _("Soil heat flux approximation (Bastiaanssen, 1995).");
    
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
	_("Name of Surface temperature raster map [K]");

    input4 = G_define_standard_option(G_OPT_R_INPUT);
    input4->key = "netradiation";
    input4->description = _("Name of Net Radiation raster map [W/m2]");

    input5 = G_define_standard_option(G_OPT_R_INPUT);
    input5->key = "localutctime";
    input5->description =
	_("Name of time of satellite overpass raster map [local time in UTC]"); 

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);

    flag1 = G_define_flag();
    flag1->key = 'r';
    flag1->description =
	_("HAPEX-Sahel empirical correction (Roerink, 1995)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    albedo = input1->answer;
    ndvi = input2->answer;
    tempk = input3->answer;
    rnet = input4->answer;
    time = input5->answer;
    result = output1->answer;
    roerink = flag1->answer;

    infd_albedo = Rast_open_old(albedo, "");
    inrast_albedo = Rast_allocate_d_buf();

    infd_ndvi = Rast_open_old(ndvi, "");
    inrast_ndvi = Rast_allocate_d_buf();

    infd_tempk = Rast_open_old(tempk, "");
    inrast_tempk = Rast_allocate_d_buf();

    infd_rnet = Rast_open_old(rnet, "");
    inrast_rnet = Rast_allocate_d_buf();

    infd_time = Rast_open_old(time, "");
    inrast_time = Rast_allocate_d_buf();

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast = Rast_allocate_d_buf();
    
    /* Create New raster files */ 
    outfd = Rast_open_new(result,DCELL_TYPE);

    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	DCELL d;
	DCELL d_albedo;
	DCELL d_ndvi;
	DCELL d_tempk;
	DCELL d_rnet;
	DCELL d_time;
	G_percent(row, nrows, 2);	
        /* read input maps */ 
        Rast_get_d_row(infd_albedo, inrast_albedo, row);
	Rast_get_d_row(infd_ndvi, inrast_ndvi, row);
	Rast_get_d_row(infd_tempk, inrast_tempk, row);
	Rast_get_d_row(infd_rnet, inrast_rnet, row);
	Rast_get_d_row(infd_time, inrast_time, row);
        /*process the data */ 
        for (col = 0; col < ncols; col++)
        {
            d_albedo = ((DCELL *) inrast_albedo)[col];
            d_ndvi   = ((DCELL *) inrast_ndvi)[col];
            d_tempk  = ((DCELL *) inrast_tempk)[col];
            d_rnet   = ((DCELL *) inrast_rnet)[col];
            d_time   = ((DCELL *) inrast_time)[col];
	    if (Rast_is_d_null_value(&d_albedo) || 
                 Rast_is_d_null_value(&d_ndvi) ||
                 Rast_is_d_null_value(&d_tempk) ||
		 Rast_is_d_null_value(&d_rnet) || 
                 Rast_is_d_null_value(&d_time)) {
		Rast_set_d_null_value(&outrast[col], 1);
	    }
	    else {
                /* calculate soil heat flux         */ 
                d=g_0(d_albedo,d_ndvi,d_tempk,d_rnet,d_time,roerink);
		outrast[col] = d;
	    }
	}
	Rast_put_d_row(outfd, outrast);
    }
    G_free(inrast_albedo);
    G_free(inrast_ndvi);
    G_free(inrast_tempk);
    G_free(inrast_rnet);
    G_free(inrast_time);
    Rast_close(infd_albedo);
    Rast_close(infd_ndvi);
    Rast_close(infd_tempk);
    Rast_close(infd_rnet);
    Rast_close(infd_time);
    G_free(outrast);
    Rast_close(outfd);
    
    /* Colors in grey shade */ 
    Rast_init_colors(&colors);
    val1 = 0;
    val2 = 200;
    Rast_add_c_color_rule(&val1, 0, 0, 0, &val2, 255, 255, 255, &colors);
    Rast_short_history(result, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result, &history);

    exit(EXIT_SUCCESS);
}
