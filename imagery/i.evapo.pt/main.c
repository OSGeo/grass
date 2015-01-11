/*****************************************************************************
*
* MODULE:	i.evapo.pt
* AUTHOR:	Yann Chemin yann.chemin@gmail.com 
*
* PURPOSE:	To estimate the daily evapotranspiration by means
*		of Prestley and Taylor method (1972).
*
* COPYRIGHT:	(C) 2007-2011 by the GRASS Development Team
*
*		This program is free software under the GNU General Public
*		Licence (>=2). Read the file COPYING that comes with GRASS
*		for details.
*
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

double pt_delta(double tempka);
double pt_ghamma(double tempka, double p_atm);
double pt_daily_et(double alpha_pt, double delta_pt, double ghamma_pt,
		   double rnet, double g0, double tempka);

int main(int argc, char *argv[])
{
    /* buffer for input-output rasters */
    void *inrast_TEMPKA, *inrast_PATM, *inrast_RNET, *inrast_G0;
    DCELL *outrast;

    /* pointers to input-output raster files */
    int infd_TEMPKA, infd_PATM, infd_RNET, infd_G0;
    int outfd;

    /* names of input-output raster files */
    char *RNET, *TEMPKA, *PATM, *G0;
    char *ETa;

    /* input-output cell values */
    DCELL d_tempka, d_pt_patm, d_rnet, d_g0;
    DCELL d_pt_alpha, d_pt_delta, d_pt_ghamma, d_daily_et;

    /* region information and handler */
    struct Cell_head cellhd;
    int nrows, ncols;
    int row, col;

    /* parser stuctures definition */
    struct GModule *module;
    struct Option *input_RNET, *input_TEMPKA, *input_PATM, *input_G0,
	*input_PT;
    struct Option *output;
    struct Flag *zero;
    struct Colors color;
    struct History history;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("evapotranspiration"));
    module->description =
	_("Computes evapotranspiration calculation "
	  "Priestley and Taylor formulation, 1972.");
    
    /* Define different options */
    input_RNET = G_define_standard_option(G_OPT_R_INPUT);
    input_RNET->key = "net_radiation";
    input_RNET->description = _("Name of input net radiation raster map [W/m2]");

    input_G0 = G_define_standard_option(G_OPT_R_INPUT);
    input_G0->key = "soil_heatflux";
    input_G0->description = _("Name of input soil heat flux raster map [W/m2]");

    input_TEMPKA = G_define_standard_option(G_OPT_R_INPUT);
    input_TEMPKA->key = "air_temperature";
    input_TEMPKA->description = _("Name of input air temperature raster map [K]");

    input_PATM = G_define_standard_option(G_OPT_R_INPUT);
    input_PATM->key = "atmospheric_pressure";
    input_PATM->description = _("Name of input atmospheric pressure raster map [millibars]");

    input_PT = G_define_option();
    input_PT->key = "priestley_taylor_coeff";
    input_PT->type = TYPE_DOUBLE;
    input_PT->required = YES;
    input_PT->description = _("Priestley-Taylor coefficient");
    input_PT->answer = "1.26";

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->description = _("Name of output evapotranspiration raster map [mm/d]");

    /* Define the different flags */
    zero = G_define_flag();
    zero->key = 'z';
    zero->description = _("Set negative ETa to zero");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* get entered parameters */
    RNET = input_RNET->answer;
    TEMPKA = input_TEMPKA->answer;
    PATM = input_PATM->answer;
    G0 = input_G0->answer;
    d_pt_alpha = atof(input_PT->answer);

    ETa = output->answer;

    /* open pointers to input raster files */
    infd_RNET = Rast_open_old(RNET, "");
    infd_TEMPKA = Rast_open_old(TEMPKA, "");
    infd_PATM = Rast_open_old(PATM, "");
    infd_G0 = Rast_open_old(G0, "");

    /* read headers of raster files */
    Rast_get_cellhd(RNET, "", &cellhd);
    Rast_get_cellhd(TEMPKA, "", &cellhd);
    Rast_get_cellhd(PATM, "", &cellhd);
    Rast_get_cellhd(G0, "", &cellhd);

    /* Allocate input buffer */
    inrast_RNET = Rast_allocate_d_buf();
    inrast_TEMPKA = Rast_allocate_d_buf();
    inrast_PATM = Rast_allocate_d_buf();
    inrast_G0 = Rast_allocate_d_buf();

    /* get rows and columns number of the current region */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* allocate output buffer */
    outrast = Rast_allocate_d_buf();

    /* open pointers to output raster files */
    outfd = Rast_open_new(ETa, DCELL_TYPE);

    /* start the loop through cells */
    for (row = 0; row < nrows; row++) {

	G_percent(row, nrows, 2);
	/* read input raster row into line buffer */
	Rast_get_d_row(infd_RNET, inrast_RNET, row);
	Rast_get_d_row(infd_TEMPKA, inrast_TEMPKA, row);
	Rast_get_d_row(infd_PATM, inrast_PATM, row);
	Rast_get_d_row(infd_G0, inrast_G0, row);

	for (col = 0; col < ncols; col++) {
	    /* read current cell from line buffer */
            d_rnet = ((DCELL *) inrast_RNET)[col];
            d_tempka = ((DCELL *) inrast_TEMPKA)[col];
            d_pt_patm = ((DCELL *) inrast_PATM)[col];
            d_g0 = ((DCELL *) inrast_G0)[col];

	    /*Delta_pt and Ghamma_pt */
	    d_pt_delta = pt_delta(d_tempka);
	    d_pt_ghamma = pt_ghamma(d_tempka, d_pt_patm);

	    /*Calculate ET */
	    d_daily_et =
		pt_daily_et(d_pt_alpha, d_pt_delta, d_pt_ghamma, d_rnet, d_g0,
			    d_tempka);
	    if (zero->answer && d_daily_et < 0)
		d_daily_et = 0.0;

	    /* write calculated ETP to output line buffer */
	    outrast[col] = d_daily_et;
	}

	/* write output line buffer to output raster file */
	Rast_put_d_row(outfd, outrast);
    }
    /* free buffers and close input maps */

    G_free(inrast_RNET);
    G_free(inrast_TEMPKA);
    G_free(inrast_PATM);
    G_free(inrast_G0);
    Rast_close(infd_RNET);
    Rast_close(infd_TEMPKA);
    Rast_close(infd_PATM);
    Rast_close(infd_G0);

    /* generate color table between -20 and 20 */
    Rast_make_rainbow_colors(&color, -20, 20);
    Rast_write_colors(ETa, G_mapset(), &color);

    Rast_short_history(ETa, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(ETa, &history);

    /* free buffers and close output map */
    G_free(outrast);
    Rast_close(outfd);

    return (EXIT_SUCCESS);
}
