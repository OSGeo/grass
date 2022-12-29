/****************************************************************************
 *
 * MODULE:       r.usler
 * AUTHOR(S):    Natalia Medvedeva - natmead@gmail.com
 *		 Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates USLE R factor 
 * 		 Rainfall Erosion index according to four methods 
 *
 * COPYRIGHT:    (C) 2002-2008, 2010 by the GRASS Development Team
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

double elswaify_1985(double annual_pmm);
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
    char *desc;

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
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("rainfall"));
    G_add_keyword(_("soil"));
    G_add_keyword(_("erosion"));
    module->description = _("Computes USLE R factor, Rainfall erosivity index.");
    
    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->description = _("Name of annual precipitation raster map [mm/year]");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->description = _("Name for output USLE R raster map [MJ.mm/ha.hr.year]");

    /* Define the different options */ 
    input1 = G_define_option();
    input1->key = "method";
    input1->type = TYPE_STRING;
    input1->required = YES;
    input1->description = _("Name of USLE R equation");
    input1->options = "roose, morgan, foster, elswaify";
    desc = NULL;
    G_asprintf(&desc,
	       "roose;%s;morgan;%s;foster;%s;elswaify;%s",
	       _("Roosle (1975)"),
	       _("Morgan (1974)"),
	       _("Foster (1981)"),
	       _("El-Swaify (1985)"));
    input1->descriptions = desc;
    input1->answer = "morgan";

    /********************/ 
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    nameflag = input1->answer;
    annual_pmm = input2->answer;
    result = output->answer;
    
    /***************************************************/ 
    infd_annual_pmm = Rast_open_old(annual_pmm, "");
    inrast_annual_pmm = Rast_allocate_d_buf();
    
    /***************************************************/ 
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast = Rast_allocate_d_buf();
    
    /* Create New raster files */ 
    outfd = Rast_open_new(result, DCELL_TYPE);
    
    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
        DCELL d;
	DCELL d_annual_pmm;
	G_percent(row, nrows, 2);
	
	/* read input map */ 
	Rast_get_d_row(infd_annual_pmm, inrast_annual_pmm, row);
	
	/*process the data */ 
	for (col = 0; col < ncols; col++)
	{
	    d_annual_pmm = ((DCELL *) inrast_annual_pmm)[col];
	    if (Rast_is_d_null_value(&d_annual_pmm)) 
		Rast_set_d_null_value(&outrast[col], 1);
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
		outrast[col] = d ;
	    }
	}
	Rast_put_d_row(outfd, outrast);
    }
    G_free(inrast_annual_pmm);
    Rast_close(infd_annual_pmm);
    G_free(outrast);
    Rast_close(outfd);

    Rast_short_history(result, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result, &history);

    exit(EXIT_SUCCESS);
}
