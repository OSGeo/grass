
/****************************************************************************
 *
 * MODULE:       i.eb.g0
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates an approximation of soil heat flux
 *               as seen in Bastiaanssen (1995) using time of
 *               satellite overpass.
 *
 * COPYRIGHT:    (C) 2006-2007 by the GRASS Development Team
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

    /************************************/ 
    G_gisinit(argv[0]);
    module = G_define_module();
    module->keywords = _("soil heat flux, energy balance, SEBAL");
    module->description =
	_("soil heat flux approximation (Bastiaanssen, 1995)");
    
    /* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);
    input1->key = "albedo";
    input1->description = _("Name of the Albedo map [0.0;1.0]");
    input1->answer = "albedo";

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = "ndvi";
    input2->description = _("Name of the ndvi map [-1.0;+1.0]");
    input2->answer = "ndvi";

    input3 = G_define_standard_option(G_OPT_R_INPUT);
    input3->key = "tempk";
    input3->description =
	_("Name of the Surface temperature map [degree Kelvin]");
    input3->answer = "tempk";

    input4 = G_define_standard_option(G_OPT_R_INPUT);
    input4->key = "rnet";
    input4->description = _("Name of the Net Radiation map [W/m2]");
    input4->answer = "rnet";

    input5 = G_define_standard_option(G_OPT_R_INPUT);
    input5->key = "time";
    input5->description =
	_("Name of the time of satellite overpass map [local UTC]");
    input5->answer = "time";

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);
    output1->key = "g0";
    output1->description = _("Name of the output g0 layer");
    output1->answer = "g0";

    flag1 = G_define_flag();
    flag1->key = 'r';
    flag1->description =
	_("HAPEX-Sahel empirical correction (Roerink, 1995)");

    /********************/ 
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    albedo = input1->answer;
    ndvi = input2->answer;
    tempk = input3->answer;
    rnet = input4->answer;
    time = input5->answer;
    result = output1->answer;
    roerink = flag1->answer;

    if ((infd_albedo = G_open_cell_old(albedo, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), albedo);
    inrast_albedo = G_allocate_d_raster_buf();

    if ((infd_ndvi = G_open_cell_old(ndvi, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), ndvi);
    inrast_ndvi = G_allocate_d_raster_buf();

    if ((infd_tempk = G_open_cell_old(tempk, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), tempk);
    inrast_tempk = G_allocate_d_raster_buf();

    if ((infd_rnet = G_open_cell_old(rnet, "")) < 0)
	G_fatal_error(_("Unable to open raster map [%s]"), rnet);
    inrast_rnet = G_allocate_d_raster_buf();

    if ((infd_time = G_open_cell_old(time, "")) < 0)
	G_fatal_error(_("Unable to open raster map [%s]"), time);
    inrast_time = G_allocate_d_raster_buf();

    nrows = G_window_rows();
    ncols = G_window_cols();
    outrast = G_allocate_d_raster_buf();
    
    /* Create New raster files */ 
    if ((outfd = G_open_raster_new(result,DCELL_TYPE)) < 0)
	G_fatal_error(_("Unable to open raster map<%s>"), result);
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
        if (G_get_d_raster_row(infd_albedo, inrast_albedo, row) < 0)
	    G_fatal_error(_("Unable to read from <%s>"), albedo);
	if (G_get_d_raster_row(infd_ndvi, inrast_ndvi, row)<0)
	    G_fatal_error(_("Unable to read from <%s>"), ndvi);
	if (G_get_d_raster_row(infd_tempk, inrast_tempk, row)< 0)
	    G_fatal_error(_("Unable to read from <%s>"), tempk);
	if (G_get_d_raster_row(infd_rnet, inrast_rnet, row)<0)
	    G_fatal_error(_("Unable to read from <%s>"), rnet);
	if (G_get_d_raster_row(infd_time, inrast_time, row)<0)
	    G_fatal_error(_("Unable to read from <%s>"), time);
        /*process the data */ 
        for (col = 0; col < ncols; col++)
        {
            d_albedo = ((DCELL *) inrast_albedo)[col];
            d_ndvi   = ((DCELL *) inrast_ndvi)[col];
            d_tempk  = ((DCELL *) inrast_tempk)[col];
            d_rnet   = ((DCELL *) inrast_rnet)[col];
            d_time   = ((DCELL *) inrast_time)[col];
	    if (G_is_d_null_value(&d_albedo) || 
                 G_is_d_null_value(&d_ndvi) ||
                 G_is_d_null_value(&d_tempk) ||
		 G_is_d_null_value(&d_rnet) || 
                 G_is_d_null_value(&d_time)) {
		G_set_d_null_value(&outrast[col], 1);
	    }
	    else {
                /* calculate soil heat flux         */ 
                d=g_0(d_albedo,d_ndvi,d_tempk,d_rnet,d_time,roerink);
		outrast[col] = d;
	    }
	}
	if (G_put_d_raster_row(outfd, outrast) < 0)
	    G_fatal_error(_("Unable to write to output raster map"));
    }
    G_free(inrast_albedo);
    G_free(inrast_ndvi);
    G_free(inrast_tempk);
    G_free(inrast_rnet);
    G_free(inrast_time);
    G_close_cell(infd_albedo);
    G_close_cell(infd_ndvi);
    G_close_cell(infd_tempk);
    G_close_cell(infd_rnet);
    G_close_cell(infd_time);
    G_free(outrast);
    G_close_cell(outfd);
    
    /* Colors in grey shade */ 
    G_init_colors(&colors);
    G_add_color_rule(0.0, 0, 0, 0, 200.0, 255, 255, 255, &colors);
    G_short_history(result, "raster", &history);
    G_command_history(&history);
    G_write_history(result, &history);
    exit(EXIT_SUCCESS);
}

