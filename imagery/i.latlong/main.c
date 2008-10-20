/****************************************************************************
 *
 * MODULE:       i.latlong
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates the longitude of the pixels in the map. 
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
    module->keywords = _("imagery, latitude, longitude, projection");
    module->description = _("creates a latitude/longitude map");
    
    /* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);
    input1->description = _("Name of the input map");

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);
    output1->description = _("Name of the output latitude or longitude layer");
    
    flag1 = G_define_flag();
    flag1->key = 'l';
    flag1->description = _("Longitude output");

    /********************/ 
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    in = input1->answer;
    result1 = output1->answer;

    /***************************************************/ 
    if ((infd = G_open_cell_old(in, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), in);
    if (G_get_cellhd(in, "", &cellhd) < 0)
	G_fatal_error(_("Cannot read file header of [%s])"), in);
    inrast = G_allocate_d_raster_buf();
    
    /***************************************************/ 
    stepx = cellhd.ew_res;
    stepy = cellhd.ns_res;
    xmin = cellhd.west;
    xmax = cellhd.east;
    ymin = cellhd.south;
    ymax = cellhd.north;
    nrows = G_window_rows();
    ncols = G_window_cols();
    
    /*Stolen from r.sun */ 
    /* Set up parameters for projection to lat/long if necessary */ 
    if ((G_projection() != PROJECTION_LL)) 
    {
	not_ll = 1;

	if ((in_proj_info = G_get_projinfo()) == NULL)
	    G_fatal_error(_("Can't get projection info of current location"));
	if ((in_unit_info = G_get_projunits()) == NULL)
	    G_fatal_error(_("Can't get projection units of current location"));
	if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
	    G_fatal_error(_("Can't get projection key values of current location"));
	G_free_key_value(in_proj_info);
	G_free_key_value(in_unit_info);
	
        /* Set output projection to latlong w/ same ellipsoid */ 
	oproj.zone = 0;
	oproj.meters = 1.;
	sprintf(oproj.proj, "ll");
	if ((oproj.pj = pj_latlong_from_proj(iproj.pj)) == NULL)
	    G_fatal_error(_("Unable to set up lat/long projection parameters"));
    }	/* End of stolen from r.sun */

    outrast1 = G_allocate_d_raster_buf();

    if ((outfd1 = G_open_raster_new(result1,DCELL_TYPE)) < 0)
	G_fatal_error(_("Could not open <%s>"), result1);

    for (row = 0; row < nrows; row++)
    {
	G_percent(row, nrows, 2);

	if (G_get_d_raster_row(infd, inrast, row) < 0)
	    G_fatal_error(_("Could not read from <%s>"), in);

	for (col = 0; col < ncols; col++)
        {
	    latitude = ymax - (row * stepy);
	    longitude = xmin + (col * stepx);
	    if (not_ll) 
		if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0) 
		    G_fatal_error(_("Error in pj_do_proj"));
            if(flag1->answer)
	        d = longitude;
            else
	        d = latitude;
	    outrast1[col] = d;
	}
	if (G_put_d_raster_row(outfd1, outrast1) < 0)
	    G_fatal_error(_("Cannot write to output raster file"));
    }
    G_free(inrast);
    G_close_cell(infd);
    G_free(outrast1);
    G_close_cell(outfd1);

    G_short_history(result1, "raster", &history);
    G_command_history(&history);
    G_write_history(result1, &history);

    exit(EXIT_SUCCESS);
}


