
/****************************************************************************
 *
 * MODULE:       r.uslek
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Transforms percentage of texture (sand/clay/silt)
 *               into USDA 1951 (p209) soil texture classes and then
 *               into USLE soil erodibility factor (K) as an output
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

#define POLYGON_DIMENSION 20

double tex2usle_k(int texture, double om_in);
int prct2tex(double sand_input, double clay_input, double silt_input);
double point_in_triangle(double point_x, double point_y, double point_z,
			 double t1_x, double t1_y, double t1_z, double t2_x,
			 double t2_y, double t2_z, double t3_x, double t3_y,
			 double t3_z);
    
int main(int argc, char *argv[]) 
{
    int nrows, ncols;
    int row, col;
    struct GModule *module;
    struct Option *input1, *input2, *input3, *input4, *output1;
    struct History history;	/*metadata */

    char *result;		/*output raster name */
    int infd_psand, infd_psilt, infd_pclay, infd_pomat;
    int outfd;
    char *psand, *psilt, *pclay, *pomat;
    int i = 0;
    void *inrast_psand, *inrast_psilt, *inrast_pclay, *inrast_pomat;
    DCELL *outrast;
    
    /************************************/ 
    G_gisinit(argv[0]);
    module = G_define_module();
    module->keywords = _("raster, soil, erosion, USLE");
    module->description = _("USLE Soil Erodibility Factor (K)");
    
    /* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);
    input1->key = _("psand");
    input1->description = _("Name of the Soil sand fraction map [0.0-1.0]");

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = _("pclay");
    input2->description = _("Name of the Soil clay fraction map [0.0-1.0]");

    input3 = G_define_standard_option(G_OPT_R_INPUT);
    input3->key = _("psilt");
    input3->description = _("Name of the Soil silt fraction map [0.0-1.0]");

    input4 = G_define_standard_option(G_OPT_R_INPUT);
    input4->key = _("pomat");
    input4->description = _("Name of the Soil Organic Matter map [0.0-1.0]");

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);
    output1->key = _("usle_k");
    output1->description = _("Name of the output USLE K factor layer");

    /********************/ 
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    psand = input1->answer;
    pclay = input2->answer;
    psilt = input3->answer;
    pomat = input4->answer;
    result = output1->answer;
    
    /***************************************************/ 
    if ((infd_psand = G_open_cell_old(psand, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), psand);
    inrast_psand = G_allocate_d_raster_buf();
    
    if ((infd_psilt = G_open_cell_old(psilt, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), psilt);
    inrast_psilt = G_allocate_d_raster_buf();
    
    if ((infd_pclay = G_open_cell_old(pclay, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), pclay);
    inrast_pclay = G_allocate_d_raster_buf();
    
    if ((infd_pomat = G_open_cell_old(pomat, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), pomat);
    inrast_pomat = G_allocate_d_raster_buf();
    /***************************************************/ 
    nrows = G_window_rows();
    ncols = G_window_cols();
    outrast = G_allocate_d_raster_buf();
    
    /* Create New raster files */ 
    if ((outfd = G_open_raster_new(result, DCELL_TYPE)) < 0)
	G_fatal_error(_("Could not open <%s>"), result);
    
    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	DCELL d;
	DCELL d_sand;
	DCELL d_clay;
	DCELL d_silt;
	DCELL d_om;
	G_percent(row, nrows, 2);
	
	/* read soil input maps */ 
	if (G_get_d_raster_row(infd_psand, inrast_psand, row) < 0)
	    G_fatal_error(_("Could not read from <%s>"), psand);
	if (G_get_d_raster_row(infd_psilt, inrast_psilt, row) < 0)
	    G_fatal_error(_("Could not read from <%s>"), psilt);
	if (G_get_d_raster_row(infd_pclay, inrast_pclay, row) < 0)
	    G_fatal_error(_("Could not read from <%s>"), pclay);
	if (G_get_d_raster_row(infd_pomat, inrast_pomat, row) < 0)
	    G_fatal_error(_("Could not read from <%s>"), pomat);
	
        /*process the data */ 
	for (col = 0; col < ncols; col++)
	{
	    d_sand = ((DCELL *) inrast_psand)[col];
	    d_silt = ((DCELL *) inrast_psilt)[col];
	    d_clay = ((DCELL *) inrast_pclay)[col];
            d_om = ((DCELL *) inrast_pomat)[col];
	    if (G_is_d_null_value(&d_sand) || 
                G_is_d_null_value(&d_clay) ||
		G_is_d_null_value(&d_silt)) 
		    G_set_d_null_value(&outrast[col], 1);
	    else {
                /***************************************/ 
		/* In case some map input not standard */
		if ((d_sand + d_clay + d_silt) != 1.0) 
		    G_set_d_null_value(&outrast[col], 1);
		/* if OM==NULL then make it 0.0 */
		else if (G_is_d_null_value(&d_om))
		    d_om = 0.0;	
		else {
                    /************************************/ 
		    /* convert to usle_k                */ 
		    d = (double)prct2tex(d_sand, d_clay, d_silt);
		    d = tex2usle_k((int)d, d_om);
		    outrast[col] = d;
                }
	    }
	}
	if (G_put_d_raster_row(outfd, outrast) < 0)
	    G_fatal_error(_("Unable to write output raster file"));
    }
    G_free(inrast_psand);
    G_free(inrast_psilt);
    G_free(inrast_pclay);
    G_free(inrast_pomat);
    G_close_cell(infd_psand);
    G_close_cell(infd_psilt);
    G_close_cell(infd_pclay);
    G_close_cell(infd_pomat);
    G_free(outrast);
    G_close_cell(outfd);
    
    G_short_history(result, "raster", &history);
    G_command_history(&history);
    G_write_history(result, &history);
    
    exit(EXIT_SUCCESS);
}


