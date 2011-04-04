/****************************************************************************
 *
 * MODULE:       i.sunhours
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates the sunshine hours (also called daytime period)
 * 		 under a perfect clear sky condition.
 * 		 Called generally "N" in meteorology. 
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
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>


int main(int argc, char *argv[]) 
{
    int nrows, ncols;
    int row, col;
    struct GModule *module;
    struct Option *input1, *input2, *output1;
    struct History history;	/*metadata */

    /************************************/ 
    char *name;			/*input raster name */
    char *result1;		/*output raster name */
    int infd_lat, infd_doy;    /*File Descriptors */ 
    int outfd1;
    char *lat, *doy;
    void *inrast_lat, *inrast_doy;
    DCELL * outrast1;

    /************************************/ 
    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("sunshine"));
    G_add_keyword(_("hours"));
    G_add_keyword(_("daytime"));
    module->description = _("Creates a sunshine hours map.");
    
    /* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);
    input1->key = _("dayofyear");
    input1->description = _("Name of the day of year input map");

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = _("latitude");
    input2->description = _("Name of the latitude input map");

    output1 = G_define_standard_option(G_OPT_R_OUTPUT);
    output1->description = _("Name of the output sunshine hours map");

    /********************/ 
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    doy = input1->answer;
    lat = input2->answer;
    result1 = output1->answer;

    /***************************************************/ 
    infd_doy = Rast_open_old(doy, "");
    inrast_doy = Rast_allocate_d_buf();

    /***************************************************/ 
    infd_lat = Rast_open_old(lat, "");
    inrast_lat = Rast_allocate_d_buf();

    /***************************************************/ 
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    outrast1 = Rast_allocate_d_buf();
    outfd1 = Rast_open_new(result1, DCELL_TYPE);

    for (row = 0; row < nrows; row++)
    {
	DCELL d;
	DCELL d_da;
	DCELL d_delta;
	DCELL d_Ws;
	DCELL d_N;
	DCELL d_lat;
	DCELL d_doy;
	G_percent(row, nrows, 2);

	Rast_get_row(infd_doy, inrast_doy, row, DCELL_TYPE);
	Rast_get_row(infd_lat, inrast_lat, row, DCELL_TYPE);

	for (col = 0; col < ncols; col++)
        {
            d_doy = ((DCELL *) inrast_doy)[col];
            d_lat = ((DCELL *) inrast_lat)[col];

	    d_da = 2 * M_PI * (d_doy - 1) / 365.0;
	    d_delta = 
		0.006918 - 0.399912 * cos(d_da) + 0.070257 * sin(d_da) -
		0.006758 * cos(2 * d_da) + 0.000907 * sin(2 * d_da) -
		0.002697 * cos(3 * d_da) + 0.00148 * sin(3 * d_da);
	    d_Ws = acos(-tan(d_lat * M_PI / 180) * tan(d_delta));
	    d_N = (360.0 / (15.0 * M_PI)) * d_Ws;
	    ((DCELL *) outrast1)[col] = d_N;
        }
	Rast_put_row(outfd1, outrast1, DCELL_TYPE);
    }
    G_free(inrast_lat);
    G_free(inrast_doy);
    Rast_close(infd_lat);
    Rast_close(infd_doy);
    G_free(outrast1);
    Rast_close(outfd1);

    Rast_short_history(result1, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result1, &history);

    exit(EXIT_SUCCESS);
}
