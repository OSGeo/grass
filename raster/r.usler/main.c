/****************************************************************************
 *
 * MODULE:       r.usler
 * AUTHOR(S):    Natalia Medvedeva - natmead@gmail.com
 *		 Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates USLE R factor 
 * 		 Rainfall Erosion index according to four methods 
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

double elswaify_1985(double annaul_pmm);
double morgan_1974(double annual_pmm);
double foster_1981(double annual_pmm);
double roose_1975(double annual_pmm);

int main(int argc, char *argv[]) 
{
    int nrows, ncols;
    int row, col;
    char *nameflag;		/*Switch for particular method */
    struct GModule *module;
    struct Option *input1, *input2, *output;
    struct History history;	/*metadata */

    /************************************/ 
    char *result;		/*output raster name */
    int infd_annual_pmm;
    int outfd;
    char *annual_pmm;

    void *inrast_annual_pmm;
    DCELL * outrast;

    /************************************/ 
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster, rainfall, erosion, USLE");
    module->description = _("Computes USLE R factor, Rainfall erosivity index.");
    
    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->description = _("Name of the annual precipitation map");

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    /* Define the different options */ 
    input1 = G_define_option();
    input1->key = "method";
    input1->type = TYPE_STRING;
    input1->required = YES;
    input1->description = _("Name of USLE R equation");
    input1->options = "roose, morgan, foster, elswaify";
    input1->descriptions = _("roose;Roosle (1975);"
			     "morgan;Morgan (1974);"
			     "foster;Foster(1981);"
			     "elswaify;El-Swaify (1985)");
    input1->answer = "morgan";

    /********************/ 
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    nameflag = input1->answer;
    annual_pmm = input2->answer;
    result = output->answer;
    
    /***************************************************/ 
    if ((infd_annual_pmm = G_open_cell_old(annual_pmm, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), annual_pmm);
    inrast_annual_pmm = G_allocate_d_raster_buf();
    
    /***************************************************/ 
    nrows = G_window_rows();
    ncols = G_window_cols();
    outrast = G_allocate_d_raster_buf();
    
    /* Create New raster files */ 
    if ((outfd = G_open_raster_new(result, DCELL_TYPE)) < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), result);
    
    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
        DCELL d;
	DCELL d_annual_pmm;
	G_percent(row, nrows, 2);
	
	/* read input map */ 
	if (G_get_d_raster_row(infd_annual_pmm, inrast_annual_pmm, row) < 0)
	    G_fatal_error(_("Unable to read raster map <%s> row %d"),
			  annual_pmm, row);
	
	/*process the data */ 
	for (col = 0; col < ncols; col++)
	{
	    d_annual_pmm = ((DCELL *) inrast_annual_pmm)[col];
	    if (G_is_d_null_value(&d_annual_pmm)) 
		G_set_d_null_value(&outrast[col], 1);
	    else 
            {
                /*calculate morgan       */ 
                if (!strcmp(nameflag, "morgan"))
                    d = morgan_1974(d_annual_pmm);
		/*calculate roose        */ 
		if (!strcmp(nameflag, "roose")) 
		    d = roose_1975(d_annual_pmm);
		/*calculate foster       */ 
		if (!strcmp(nameflag, "foster"))
		    d = foster_1981(d_annual_pmm);
		/*calculate elswaify     */ 
		if (!strcmp(nameflag, "elswaify")) 
		    d = elswaify_1985(d_annual_pmm);
	    }
	if (G_put_d_raster_row(outfd, outrast) < 0)
	    G_fatal_error(_("Failed writing raster map <%s> row %d"),
			  result, row);
	}
    }
    G_free(inrast_annual_pmm);
    G_close_cell(infd_annual_pmm);
    G_free(outrast);
    G_close_cell(outfd);

    G_short_history(result, "raster", &history);
    G_command_history(&history);
    G_write_history(result, &history);

    exit(EXIT_SUCCESS);
}
