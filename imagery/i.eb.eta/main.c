
/****************************************************************************
 *
 * MODULE:       i.eb.eta
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates the actual evapotranspiration for diurnal period
 *               as seen in Bastiaanssen (1995) 
 *
 * COPYRIGHT:    (C) 2002-2009 by the GRASS Development Team
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
double et_a(double r_net_day, double evap_fr, double tempk);

int main(int argc, char *argv[]) 
{
    int nrows, ncols;
    int row, col;
    struct GModule *module;
    struct Option *input1, *input2, *input3, *output1;
    struct History history;	/*metadata */
    char *result1;		/*output raster name */
    int infd_rnetday, infd_evapfr, infd_tempk;
    int outfd1;
    char *rnetday, *evapfr, *tempk;
    void *inrast_rnetday, *inrast_evapfr, *inrast_tempk;

    DCELL * outrast1;
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("imagery, actual evapotranspiration, energy balance, SEBAL");
    module->description =
	_("actual evapotranspiration for diurnal period (Bastiaanssen, 1995)");
    
    /* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);
    input1->key = "rnetday";
    input1->description = _("Name of the diurnal Net Radiation map [W/m2]");
    input1->answer = "rnetday";

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = "evapfr";
    input2->description = _("Name of the evaporative fraction map [-]");
    input2->answer = "evapfr";

    input3 = G_define_standard_option(G_OPT_R_INPUT);
    input3->key = "tempk";
    input3->description = _("Name of the surface skin temperature [K]");
    input3->answer = "tempk";

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);
    output1->description =
	_("Name of the output actual evapotranspiration layer [mm/d]");
    output1->answer = "eta";

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    rnetday = input1->answer;
    evapfr = input2->answer;
    tempk = input3->answer;
    result1 = output1->answer;
    
    if ((infd_rnetday = Rast_open_cell_old(rnetday, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), rnetday);
    inrast_rnetday = Rast_allocate_d_raster_buf();
    
    if ((infd_evapfr = Rast_open_cell_old(evapfr, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), evapfr);
    inrast_evapfr = Rast_allocate_d_raster_buf();
    
    if ((infd_tempk = Rast_open_cell_old(tempk, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), tempk);
    inrast_tempk = Rast_allocate_d_raster_buf();
    
    nrows = G_window_rows();
    ncols = G_window_cols();
    outrast1 = Rast_allocate_d_raster_buf();
    
    if ((outfd1 = Rast_open_raster_new(result1, DCELL_TYPE)) < 0)
        G_fatal_error(_("Unable to create raster map <%s>"), result1);
    
    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
        DCELL d;
	DCELL d_rnetday;
	DCELL d_evapfr;
	DCELL d_tempk;
	G_percent(row, nrows, 2);
	
	/* read input maps */ 
	if (Rast_get_d_raster_row(infd_rnetday,inrast_rnetday,row)<0)
	    G_fatal_error(_("Unable to read from <%s> row %d"), rnetday, row);
	if (Rast_get_d_raster_row(infd_evapfr,inrast_evapfr,row)<0)
	    G_fatal_error(_("Unable to read from <%s> row %d"), evapfr, row);
	if (Rast_get_d_raster_row(infd_tempk,inrast_tempk,row)<0)
	    G_fatal_error(_("Unable to read from <%s> row %d"), tempk, row);
	
    /*process the data */ 
    for (col = 0; col < ncols; col++)
    {
            d_rnetday = ((DCELL *) inrast_rnetday)[col];
            d_evapfr = ((DCELL *) inrast_evapfr)[col];
            d_tempk = ((DCELL *) inrast_tempk)[col];
	    if (Rast_is_d_null_value(&d_rnetday) ||
		 Rast_is_d_null_value(&d_evapfr) ||
		 Rast_is_d_null_value(&d_tempk)) 
		Rast_set_d_null_value(&outrast1[col], 1);
	    else {
		d = et_a(d_rnetday, d_evapfr, d_tempk);
		outrast1[col] = d;
	    }
	}
	if (Rast_put_d_raster_row(outfd1,outrast1) < 0)
	    G_fatal_error(_("Failed writing raster map <%s>"), result1);
    }
    G_free(inrast_rnetday);
    G_free(inrast_evapfr);
    G_free(inrast_tempk);
    Rast_close_cell(infd_rnetday);
    Rast_close_cell(infd_evapfr);
    Rast_close_cell(infd_tempk);
    G_free(outrast1);
    Rast_close_cell(outfd1);
    Rast_short_history(result1, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result1, &history);
    exit(EXIT_SUCCESS);
}


