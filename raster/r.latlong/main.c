/****************************************************************************
 *
 * MODULE:       r.latlong
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates the longitude of the pixels in the map. 
 *
 * COPYRIGHT: (C) 2002-2008, 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *   	    	 Public License (>=v2). Read the file COPYING that
 *   	    	 comes with GRASS for details.
 *
 *****************************************************************************/
    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

int main(int argc, char *argv[]) 
{
    struct Cell_head cellhd;	/*region+header info */
    int nrows, ncols;
    int row, col;
    int not_ll = 0;		/*if proj is not lat/long, it will be 1. */

    struct GModule *module;
    struct Option *input1, *output1;
    struct Flag *flag1;
    struct History history;	/*metadata */
    struct pj_info iproj;
    struct pj_info oproj;
    struct Key_Value *in_proj_info, *in_unit_info;

    /************************************/ 
    char *result1;		/*output raster name */
    
    int infd;
    int outfd1;
    char *in;
    double xmin, ymin;
    double xmax, ymax;
    double stepx, stepy;
    double latitude, longitude;
    void *inrast;
    DCELL * outrast1;
    DCELL d;

    /************************************/ 
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("latitude"));
    G_add_keyword(_("longitude"));
    G_add_keyword(_("projection"));
    module->description = _("Creates a latitude/longitude raster map.");
    
    /* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);
    output1->description = _("Name for output latitude or longitude raster map");
    
    flag1 = G_define_flag();
    flag1->key = 'l';
    flag1->description = _("Longitude output");

    /********************/ 
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    in = input1->answer;
    result1 = output1->answer;

    /***************************************************/ 
    infd = Rast_open_old(in, "");
    Rast_get_cellhd(in, "", &cellhd);
    inrast = Rast_allocate_d_buf();
    
    /***************************************************/ 
    xmin = cellhd.west;
    xmax = cellhd.east;
    ymin = cellhd.south;
    ymax = cellhd.north;
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    stepx = abs(xmax-xmin)/(double)ncols;
    stepy = abs(ymax-ymin)/(double)nrows;
    
    /*Stolen from r.sun */ 
    /* Set up parameters for projection to lat/long if necessary */ 
    if ((G_projection() != PROJECTION_LL)) 
    {
	not_ll = 1;

	if ((in_proj_info = G_get_projinfo()) == NULL)
	    G_fatal_error(_("Unable to get projection info of current location"));
	if ((in_unit_info = G_get_projunits()) == NULL)
	    G_fatal_error(_("Unable to get projection units of current location"));
	if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
	    G_fatal_error(_("Unable to get projection key values of current location"));
	G_free_key_value(in_proj_info);
	G_free_key_value(in_unit_info);
	
        /* Set output projection to latlong w/ same ellipsoid */ 
	oproj.zone = 0;
	oproj.meters = 1.;
	sprintf(oproj.proj, "ll");
	if ((oproj.pj = pj_latlong_from_proj(iproj.pj)) == NULL)
	    G_fatal_error(_("Unable to set up lat/long projection parameters"));
    }	/* End of stolen from r.sun */

    outrast1 = Rast_allocate_d_buf();

    outfd1 = Rast_open_new(result1, DCELL_TYPE);

    for (row = 0; row < nrows; row++)
    {
	G_percent(row, nrows, 2);

	Rast_get_d_row(infd, inrast, row);

	for (col = 0; col < ncols; col++)
        {
	    latitude = ymax - ((double)row * stepy);
	    longitude = xmin + ((double)col * stepx);
	    if (not_ll) 
		if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0) 
		    G_fatal_error(_("Error in pj_do_proj"));
            if(flag1->answer)
	        d = longitude;
            else
	        d = latitude;
	    outrast1[col] = d;
	}
	Rast_put_d_row(outfd1, outrast1);
    }
    G_free(inrast);
    Rast_close(infd);
    G_free(outrast1);
    Rast_close(outfd1);

    Rast_short_history(result1, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result1, &history);

    exit(EXIT_SUCCESS);
}


