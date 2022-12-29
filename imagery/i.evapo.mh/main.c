/*****************************************************************************
*
* MODULE:	i.evapo.mh
* AUTHOR:	Yann Chemin yann.chemin@gmail.com 
*
* PURPOSE:	To estimate the reference evapotranspiration by means
*		of Modified Hargreaves method (2001).
*		Also has a switch for original Hargreaves (1985),
*		and for Hargreaves-Samani (1985).
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

double mh_original(double ra, double tavg, double tmax, double tmin,
		   double p);
double mh_eto(double ra, double tavg, double tmax, double tmin, double p);
double mh_samani(double ra, double tavg, double tmax, double tmin);

int main(int argc, char *argv[])
{
    /* buffer for input-output rasters */
    void *inrast_TEMPKAVG, *inrast_TEMPKMIN, *inrast_TEMPKMAX, *inrast_RNET,
	*inrast_P;
    DCELL *outrast;

    /* pointers to input-output raster files */
    int infd_TEMPKAVG, infd_TEMPKMIN, infd_TEMPKMAX, infd_RNET, infd_P;
    int outfd;

    /* names of input-output raster files */
    char *RNET, *TEMPKAVG, *TEMPKMIN, *TEMPKMAX, *P;
    char *ETa;

    /* input-output cell values */
    DCELL d_tempkavg, d_tempkmin, d_tempkmax, d_rnet, d_p;
    DCELL d_daily_et;

    /* region information and handler */
    struct Cell_head cellhd;
    int nrows, ncols;
    int row, col;

    /* parser stuctures definition */
    struct GModule *module;
    struct Option *input_RNET, *input_TEMPKAVG, *input_TEMPKMIN;
    struct Option *input_TEMPKMAX, *input_P;
    struct Option *output;
    struct Flag *zero, *original, *samani;
    struct Colors color;
    struct History history;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("evapotranspiration"));
    module->description =
	_("Computes evapotranspiration calculation "
	  "modified or original Hargreaves formulation, 2001.");

    /* Define different options */
    input_RNET = G_define_standard_option(G_OPT_R_INPUT);
    input_RNET->key = "netradiation_diurnal";
    input_RNET->description = _("Name of input diurnal net radiation raster map [W/m2/d]");

    input_TEMPKAVG = G_define_standard_option(G_OPT_R_INPUT);
    input_TEMPKAVG->key = "average_temperature";
    input_TEMPKAVG->description = _("Name of input average air temperature raster map [C]");

    input_TEMPKMIN = G_define_standard_option(G_OPT_R_INPUT);
    input_TEMPKMIN->key = "minimum_temperature";
    input_TEMPKMIN->description = _("Name of input minimum air temperature raster map [C]");

    input_TEMPKMAX = G_define_standard_option(G_OPT_R_INPUT);
    input_TEMPKMAX->key = "maximum_temperature";
    input_TEMPKMAX->description = _("Name of input maximum air temperature raster map [C]");

    input_P = G_define_standard_option(G_OPT_R_INPUT);
    input_P->required = NO;
    input_P->key = "precipitation";
    input_P->label =
	_("Name of precipitation raster map [mm/month]");
    input_P->description = _("Disabled for original Hargreaves (1985)");
    
    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->description = _("Name for output raster map [mm/d]");

    /* Define the different flags */
    zero = G_define_flag();
    zero->key = 'z';
    zero->description = _("Set negative ETa to zero");

    original = G_define_flag();
    original->key = 'h';
    original->description = _("Use original Hargreaves (1985)");

    samani = G_define_flag();
    samani->key = 's';
    samani->description = _("Use Hargreaves-Samani (1985)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* get entered parameters */
    RNET = input_RNET->answer;
    TEMPKAVG = input_TEMPKAVG->answer;
    TEMPKMIN = input_TEMPKMIN->answer;
    TEMPKMAX = input_TEMPKMAX->answer;
    P = input_P->answer;

    ETa = output->answer;

    /* open pointers to input raster files */
    infd_RNET = Rast_open_old(RNET, "");
    infd_TEMPKAVG = Rast_open_old(TEMPKAVG, "");
    infd_TEMPKMIN = Rast_open_old(TEMPKMIN, "");
    infd_TEMPKMAX = Rast_open_old(TEMPKMAX, "");
    if (!original->answer) {
        infd_P = Rast_open_old(P, "");
    }
    /* read headers of raster files */
    Rast_get_cellhd(RNET, "", &cellhd);
    Rast_get_cellhd(TEMPKAVG, "", &cellhd);
    Rast_get_cellhd(TEMPKMIN, "", &cellhd);
    Rast_get_cellhd(TEMPKMAX, "", &cellhd);
    if (!original->answer) {
	Rast_get_cellhd(P, "", &cellhd);
    }
    /* Allocate input buffer */
    inrast_RNET = Rast_allocate_d_buf();
    inrast_TEMPKAVG = Rast_allocate_d_buf();
    inrast_TEMPKMIN = Rast_allocate_d_buf();
    inrast_TEMPKMAX = Rast_allocate_d_buf();
    if (!original->answer) {
	inrast_P = Rast_allocate_d_buf();
    }
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
	Rast_get_d_row(infd_TEMPKAVG, inrast_TEMPKAVG, row);
	Rast_get_d_row(infd_TEMPKMIN, inrast_TEMPKMIN, row);
	Rast_get_d_row(infd_TEMPKMAX, inrast_TEMPKMAX, row);
	if (!original->answer) {
	    Rast_get_d_row(infd_P, inrast_P, row);
	}
	for (col = 0; col < ncols; col++) {
	    /* read current cell from line buffer */
            d_rnet = ((DCELL *) inrast_RNET)[col];
            d_tempkavg = ((DCELL *) inrast_TEMPKAVG)[col];
            d_tempkmin = ((DCELL *) inrast_TEMPKMIN)[col];
            d_tempkmax = ((DCELL *) inrast_TEMPKMAX)[col];
	    if (!original->answer) {
		    d_p = ((DCELL *) inrast_P)[col];
	    }
	    if (Rast_is_d_null_value(&d_rnet) ||
		Rast_is_d_null_value(&d_tempkavg) ||
		Rast_is_d_null_value(&d_tempkmin) ||
		Rast_is_d_null_value(&d_tempkmax) || Rast_is_d_null_value(&d_p)) {
		Rast_set_d_null_value(&outrast[col], 1);
	    }
	    else {
		if (original->answer) {
		    d_daily_et =
			mh_original(d_rnet, d_tempkavg, d_tempkmax,
				    d_tempkmin, d_p);
		}
		else if (samani->answer) {
		    d_daily_et =
			mh_samani(d_rnet, d_tempkavg, d_tempkmax, d_tempkmin);
		}
		else {
		    d_daily_et =
			mh_eto(d_rnet, d_tempkavg, d_tempkmax, d_tempkmin,
			       d_p);
		}
		if (zero->answer && d_daily_et < 0)
		    d_daily_et = 0.0;
		/* write calculated ETP to output line buffer */
		outrast[col] = d_daily_et;
	    }
	}
	/* write output line buffer to output raster file */
	Rast_put_d_row(outfd, outrast);
    }
    /* free buffers and close input maps */

    G_free(inrast_RNET);
    G_free(inrast_TEMPKAVG);
    G_free(inrast_TEMPKMIN);
    G_free(inrast_TEMPKMAX);
    if (!original->answer) {
	G_free(inrast_P);
    }
    Rast_close(infd_RNET);
    Rast_close(infd_TEMPKAVG);
    Rast_close(infd_TEMPKMIN);
    Rast_close(infd_TEMPKMAX);
    if (!original->answer) {
	Rast_close(infd_P);
    }
    /* generate color table between -20 and 20 */
    Rast_make_rainbow_colors(&color, -20, 20);
    Rast_write_colors(ETa, G_mapset(), &color);

    Rast_short_history(ETa, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(ETa, &history);

    /* free buffers and close output map */
    G_free(outrast);
    Rast_close(outfd);

    return(EXIT_SUCCESS);
}
