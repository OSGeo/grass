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
#include <grass/glocale.h>
    
#define PI 3.1415927

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
    module->keywords = _("sunshine, hours, daytime");
    module->description = _("creates a sunshine hours map");
    
    /* Define the different options */ 
    input1 = G_define_standard_option(G_OPT_R_INPUT);
    input1->key = _("doy");
    input1->description = _("Name of the doy input map");

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = _("lat");
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
    if ((infd_doy = G_open_cell_old(doy, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), doy);
    inrast_doy = G_allocate_d_raster_buf();

    /***************************************************/ 
    if ((infd_lat = G_open_cell_old(lat, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), lat);
    inrast_lat = G_allocate_d_raster_buf();

    /***************************************************/ 
    nrows = G_window_rows();
    ncols = G_window_cols();

    outrast1 = G_allocate_d_raster_buf();
    if ((outfd1 = G_open_raster_new(result1, DCELL_TYPE)) < 0)
	G_fatal_error(_("Could not open <%s>"), result1);

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

	if (G_get_raster_row(infd_doy, inrast_doy, row, DCELL_TYPE) < 0)
	    G_fatal_error(_("Could not read from <%s>"), doy);
	if (G_get_raster_row(infd_lat, inrast_lat, row, DCELL_TYPE) < 0)
	    G_fatal_error(_("Could not read from <%s>"), lat);

	for (col = 0; col < ncols; col++)
        {
            d_doy = ((DCELL *) inrast_doy)[col];
            d_lat = ((DCELL *) inrast_lat)[col];

	    d_da = 2 * PI * (d_doy - 1) / 365.0;
	    d_delta = 
		0.006918 - 0.399912 * cos(d_da) + 0.070257 * sin(d_da) -
		0.006758 * cos(2 * d_da) + 0.000907 * sin(2 * d_da) -
		0.002697 * cos(3 * d_da) + 0.00148 * sin(3 * d_da);
	    d_Ws = acos(-tan(d_lat * PI / 180) * tan(d_delta));
	    d_N = (360.0 / (15.0 * PI)) * d_Ws;
	    ((DCELL *) outrast1)[col] = d_N;
        }
	if (G_put_raster_row(outfd1, outrast1, DCELL_TYPE) < 0)
	    G_fatal_error(_("Cannot write to output raster file"));
    }
    G_free(inrast_lat);
    G_free(inrast_doy);
    G_close_cell(infd_lat);
    G_close_cell(infd_doy);
    G_free(outrast1);
    G_close_cell(outfd1);

    G_short_history(result1, "raster", &history);
    G_command_history(&history);
    G_write_history(result1, &history);

    exit(EXIT_SUCCESS);
}


