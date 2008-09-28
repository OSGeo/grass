
/****************************************************************************
 *
 * MODULE:       i.vi
 * AUTHOR(S):    Baburao Kamble baburaokamble@gmail.com
 *		 Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculates 14 vegetation indices 
 * 		 based on biophysical parameters. 
 *
 * COPYRIGHT:    (C) 2002-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 * 
 * Remark:              
 *		 These are generic indices that use red and nir for most of them. 
 *               Those can be any use by standard satellite having V and IR.
 *		 However arvi uses red, nir and blue; 
 *		 GVI uses B,G,R,NIR, chan5 and chan 7 of landsat;
 *		 and GARI uses B,G,R and NIR.   
 *
 * Changelog:	 Added EVI on 20080718 (Yann)
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

double s_r(double redchan, double nirchan);
double nd_vi(double redchan, double nirchan);
double ip_vi(double redchan, double nirchan);
double d_vi(double redchan, double nirchan);
double e_vi(double bluechan, double redchan, double nirchan);
double p_vi(double redchan, double nirchan);
double wd_vi(double redchan, double nirchan);
double sa_vi(double redchan, double nirchan);
double msa_vi(double redchan, double nirchan);
double msa_vi2(double redchan, double nirchan);
double ge_mi(double redchan, double nirchan);
double ar_vi(double redchan, double nirchan, double bluechan);
double g_vi(double bluechan, double greenchan, double redchan,
	     double nirchan, double chan5chan, double chan7chan);
double ga_ri(double redchan, double nirchan, double bluechan,
	      double greenchan);

int main(int argc, char *argv[]) 
{
    int nrows, ncols;
    int row, col;
    char *viflag;		/*Switch for particular index */
    struct GModule *module;
    struct Option *input1, *input2, *input3, *input4, *input5, *input6,
	*input7, *output;
    struct History history;	/*metadata */
    struct Colors colors;	/*Color rules */

    /* FMEO Declarations */ 
    char *result;		/*output raster name */
    /* File Descriptors */ 
    int infd_redchan, infd_nirchan, infd_greenchan;
    int infd_bluechan, infd_chan5chan, infd_chan7chan;
    int outfd;
    char *bluechan, *greenchan, *redchan, *nirchan, *chan5chan, *chan7chan;
    DCELL *inrast_redchan, *inrast_nirchan, *inrast_greenchan;
    DCELL *inrast_bluechan, *inrast_chan5chan, *inrast_chan7chan;
    DCELL *outrast;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vegetation index, biophysical parameters");
    module->description =
	_("14 types of vegetation indices from red and nir, and only some requiring additional bands");

    /* Define the different options */ 
    input1 = G_define_option();
    input1->key = _("viname");
    input1->type = TYPE_STRING;
    input1->required = YES;
    input1->gisprompt = _("Name of VI");
    input1->description =
	_("Name of VI: sr,ndvi,ipvi,dvi,evi,pvi,wdvi,savi,msavi,msavi2,gemi,arvi,gvi,gari.");

    input1->answer = _("ndvi");
    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = _("red");
    input2->description =
	_("Name of the RED Channel surface reflectance map [0.0;1.0]");

    input3 = G_define_standard_option(G_OPT_R_INPUT);
    input3->key = _("nir");
    input3->description =
	_("Name of the NIR Channel surface reflectance map [0.0;1.0]");

    input4 = G_define_standard_option(G_OPT_R_INPUT);
    input4->key = _("green");
    input4->required = NO;
    input4->description =
	_("Name of the GREEN Channel surface reflectance map [0.0;1.0]");

    input5 = G_define_standard_option(G_OPT_R_INPUT);
    input5->key = _("blue");
    input5->required = NO;
    input5->description =
	_("Name of the BLUE Channel surface reflectance map [0.0;1.0]");

    input6 = G_define_standard_option(G_OPT_R_INPUT);
    input6->key = _("chan5");
    input6->required = NO;
    input6->description =
	_("Name of the CHAN5 Channel surface reflectance map [0.0;1.0]");

    input7 = G_define_standard_option(G_OPT_R_INPUT);
    input7->key = _("chan7");
    input7->required = NO;
    input7->description =
	_("Name of the CHAN7 Channel surface reflectance map [0.0;1.0]");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->description = _("Name of the output vi layer");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    viflag = input1->answer;
    redchan = input2->answer;
    nirchan = input3->answer;
    greenchan = input4->answer;
    bluechan = input5->answer;
    chan5chan = input6->answer;
    chan7chan = input7->answer;
    result = output->answer;

    if ((infd_redchan = G_open_cell_old(redchan, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), redchan);
    inrast_redchan = G_allocate_d_raster_buf();

    if ((infd_nirchan = G_open_cell_old(nirchan, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), nirchan);
    inrast_nirchan = G_allocate_d_raster_buf();

    if (greenchan) {
	if ((infd_greenchan = G_open_cell_old(greenchan, "")) < 0)
	    G_fatal_error(_("Cannot open cell file [%s]"), greenchan);
	inrast_greenchan = G_allocate_d_raster_buf();
    }

    if (bluechan) {
	if ((infd_bluechan = G_open_cell_old(bluechan, "")) < 0)
	    G_fatal_error(_("Cannot open cell file [%s]"), bluechan);
	inrast_bluechan = G_allocate_d_raster_buf();
    }

    if (chan5chan) {
	if ((infd_chan5chan = G_open_cell_old(chan5chan, "")) < 0)
	    G_fatal_error(_("Cannot open cell file [%s]"), chan5chan);
	inrast_chan5chan = G_allocate_d_raster_buf();
    }

    if (chan7chan) {
	if ((infd_chan7chan = G_open_cell_old(chan7chan, "")) < 0)
	    G_fatal_error(_("Cannot open cell file [%s]"), chan7chan);
	inrast_chan7chan = G_allocate_d_raster_buf();
    }

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
	DCELL d_bluechan;
	DCELL d_greenchan;
	DCELL d_redchan;
	DCELL d_nirchan;
	DCELL d_chan5chan;
	DCELL d_chan7chan;

	G_percent(row, nrows, 2);

	if (G_get_d_raster_row(infd_redchan, inrast_redchan, row) < 0)
	    G_fatal_error(_("Could not read from <%s>"), redchan);
	if (G_get_d_raster_row(infd_nirchan, inrast_nirchan, row) < 0)
	    G_fatal_error(_("Could not read from <%s>"), nirchan);
	if (greenchan) {
	    if (G_get_d_raster_row(infd_greenchan, inrast_greenchan, row) < 0)
		G_fatal_error(_("Could not read from <%s>"), greenchan);
	}
	if (bluechan) {
	    if (G_get_d_raster_row(infd_bluechan, inrast_bluechan, row) < 0)
		G_fatal_error(_("Could not read from <%s>"), bluechan);
	}
	if (chan5chan) {
	    if (G_get_d_raster_row(infd_chan5chan, inrast_chan5chan, row) < 0)
		G_fatal_error(_("Could not read from <%s>"), chan5chan);
	}
	if (chan7chan) {
	    if (G_get_d_raster_row(infd_chan7chan, inrast_chan7chan, row) < 0)
		G_fatal_error(_("Could not read from <%s>"), chan7chan);
	}

	/* process the data */ 
	for (col = 0; col < ncols; col++)
	{
	    d_redchan   = inrast_redchan[col];
	    d_nirchan   = inrast_nirchan[col];
	    d_greenchan = inrast_greenchan[col];
	    d_bluechan  = inrast_bluechan[col];
	    d_chan5chan = inrast_chan5chan[col];
	    d_chan7chan = inrast_chan7chan[col];

	    if (G_is_d_null_value(&d_redchan) ||
		G_is_d_null_value(&d_nirchan) || 
		((greenchan) && G_is_d_null_value(&d_greenchan)) ||
		((bluechan) && G_is_d_null_value(&d_bluechan)) ||
		((chan5chan) && G_is_d_null_value(&d_chan5chan)) ||
		((chan7chan) && G_is_d_null_value(&d_chan7chan))) {
		G_set_d_null_value(&outrast[col], 1);
	    }
	    else {
		/* calculate simple_ratio        */ 
		if (!strcmp(viflag, "sr"))
		    d = s_r(d_redchan, d_nirchan);

		/* calculate ndvi                    */ 
		if (!strcmp(viflag, "ndvi")) {
		    if (d_redchan + d_nirchan < 0.001)
			G_set_d_null_value(&outrast[col], 1);
		    else
			outrast[col] = nd_vi(d_redchan, d_nirchan);
		}

		if (!strcmp(viflag, "ipvi"))
		    outrast[col] = ip_vi(d_redchan, d_nirchan);

		if (!strcmp(viflag, "dvi"))
		    outrast[col] = d_vi(d_redchan, d_nirchan);

		if (!strcmp(viflag, "evi"))
		    outrast[col] = e_vi(d_bluechan, d_redchan, d_nirchan);

		if (!strcmp(viflag, "pvi"))
		    outrast[col] = p_vi(d_redchan, d_nirchan);

		if (!strcmp(viflag, "wdvi"))
		    outrast[col] = wd_vi(d_redchan, d_nirchan);

		if (!strcmp(viflag, "savi"))
		    outrast[col] = sa_vi(d_redchan, d_nirchan);

		if (!strcmp(viflag, "msavi"))
		    outrast[col] = msa_vi(d_redchan, d_nirchan);

		if (!strcmp(viflag, "msavi2"))
		    outrast[col] = msa_vi2(d_redchan, d_nirchan);

		if (!strcmp(viflag, "gemi"))
		    outrast[col] = ge_mi(d_redchan, d_nirchan);

		if (!strcmp(viflag, "arvi"))
		    outrast[col] = ar_vi(d_redchan, d_nirchan, d_bluechan);

		if (!strcmp(viflag, "gvi"))
		    outrast[col] = g_vi(d_bluechan, d_greenchan, d_redchan, d_nirchan,
					d_chan5chan, d_chan7chan);

		if (!strcmp(viflag, "gari"))
		    outrast[col] = ga_ri(d_redchan, d_nirchan, d_bluechan, d_greenchan);
	    }
	}
	if (G_put_d_raster_row(outfd, outrast) < 0)
	    G_fatal_error(_("Cannot write to output raster file"));
    }

    G_free(inrast_redchan);
    G_close_cell(infd_redchan);
    G_free(inrast_nirchan);
    G_close_cell(infd_nirchan);
    if (greenchan) {
	G_free(inrast_greenchan);
	G_close_cell(infd_greenchan);
    }
    if (bluechan) {
	G_free(inrast_bluechan);
	G_close_cell(infd_bluechan);
    }
    if (chan5chan) {
	G_free(inrast_chan5chan);
	G_close_cell(infd_chan5chan);
    }
    if (chan7chan) {
	G_free(inrast_chan7chan);
	G_close_cell(infd_chan7chan);
    }

    G_free(outrast);
    G_close_cell(outfd);

    /* Color from -1.0 to +1.0 in grey */ 
    G_init_colors(&colors);
    G_add_color_rule(-1.0, 0, 0, 0, 1.0, 255, 255, 255, &colors);
    G_short_history(result, "raster", &history);
    G_command_history(&history);
    G_write_history(result, &history);
    exit(EXIT_SUCCESS);
}

