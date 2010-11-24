
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
 * 		 Added VARI on 20081014 (Yann)
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
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
double va_ri(double redchan, double greenchan, double bluechan);

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

    char *result;		/*output raster name */
    int infd_redchan, infd_nirchan, infd_greenchan;
    int infd_bluechan, infd_chan5chan, infd_chan7chan;
    int outfd;
    char *bluechan, *greenchan, *redchan, *nirchan, *chan5chan, *chan7chan;
    DCELL *inrast_redchan, *inrast_nirchan, *inrast_greenchan;
    DCELL *inrast_bluechan, *inrast_chan5chan, *inrast_chan7chan;
    DCELL *outrast;
    CELL val1, val2;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("vegetation index"));
    G_add_keyword(_("biophysical parameters"));
    module->label =
	_("Calculates different types of vegetation indices.");
    module->description = _("Uses red and nir bands mostly, "
			    "and some indices require additional bands.");
    
    /* Define the different options */ 
    input1 = G_define_option();
    input1->key = _("viname");
    input1->type = TYPE_STRING;
    input1->required = YES;
    input1->description = _("Name of vegetation index");
    input1->descriptions =_("arvi;Atmospherically Resistant Vegetation Indices;"
			    "dvi;Difference Vegetation Index;"
			    "evi;Enhanced Vegetation Index;"
			    "gvi;Green Vegetation Index;"
			    "gari;Green atmospherically resistant vegetation index;"
			    "gemi;Global Environmental Monitoring Index;"
			    "ipvi;Infrared Percentage Vegetation Index;"
			    "msavi;Modified Soil Adjusted Vegetation Index;"
			    "msavi2;second Modified Soil Adjusted Vegetation Index;"
			    "ndvi;Normalized Difference Vegetation Index;"
			    "pvi;Perpendicular Vegetation Index;"
			    "savi;Soil Adjusted Vegetation Index;"
                            "sr;Simple Ratio;"
                            "vari;Visible Atmospherically Resistant Index;"
			    "wdvi;Weighted Difference Vegetation Index;");

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->key = "red";
    input2->label =
	_("Name of the red channel surface reflectance map");
    input2->description = _("Range: [0.0;1.0]");

    input3 = G_define_standard_option(G_OPT_R_INPUT);
    input3->key = "nir";
    input3->required = NO;
    input3->label =
	_("Name of the nir channel surface reflectance map");
    input3->description = _("Range: [0.0;1.0]");

    input4 = G_define_standard_option(G_OPT_R_INPUT);
    input4->key = "green";
    input4->required = NO;
    input4->label =
	_("Name of the green channel surface reflectance map");
    input4->description = _("Range: [0.0;1.0]");
    
    input5 = G_define_standard_option(G_OPT_R_INPUT);
    input5->key = "blue";
    input5->required = NO;
    input5->label =
	_("Name of the blue channel surface reflectance map");
    input5->description = _("Range: [0.0;1.0]");

    input6 = G_define_standard_option(G_OPT_R_INPUT);
    input6->key = "chan5";
    input6->required = NO;
    input6->label =
	_("Name of the chan5 channel surface reflectance map");
    input6->description = _("Range: [0.0;1.0]");

    input7 = G_define_standard_option(G_OPT_R_INPUT);
    input7->key = "chan7";
    input7->required = NO;
    input7->label = 
	_("Name of the chan7 channel surface reflectance map");
    input7->description = _("Range: [0.0;1.0]");

    output = G_define_standard_option(G_OPT_R_OUTPUT);

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

    if (!strcmp(viflag, "sr") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("sr index requires red and nir maps"));

    if (!strcmp(viflag, "ndvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("ndvi index requires red and nir maps"));

    if (!strcmp(viflag, "ipvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("ipvi index requires red and nir maps"));

    if (!strcmp(viflag, "dvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("dvi index requires red and nir maps"));

    if (!strcmp(viflag, "pvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("pvi index requires red and nir maps"));

    if (!strcmp(viflag, "wdvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("wdvi index requires red and nir maps"));

    if (!strcmp(viflag, "savi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("savi index requires red and nir maps"));

    if (!strcmp(viflag, "msavi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("msavi index requires red and nir maps"));

    if (!strcmp(viflag, "msavi2") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("msavi2 index requires red and nir maps"));

    if (!strcmp(viflag, "gemi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("gemi index requires red and nir maps"));

    if (!strcmp(viflag, "arvi") && (!(input2->answer) || !(input3->answer) 
                || !(input5->answer)) )
	G_fatal_error(_("arvi index requires blue, red and nir maps"));

    if (!strcmp(viflag, "evi") && (!(input2->answer) || !(input3->answer) 
                || !(input5->answer)) )
	G_fatal_error(_("evi index requires blue, red and nir maps"));

    if (!strcmp(viflag, "vari") && (!(input2->answer) || !(input4->answer) 
                || !(input5->answer)) )
	G_fatal_error(_("vari index requires blue, green and red maps"));

    if (!strcmp(viflag, "gari") && (!(input2->answer) || !(input3->answer) 
                || !(input4->answer) || !(input5->answer)) )
	G_fatal_error(_("gari index requires blue, green, red and nir maps"));

    if (!strcmp(viflag, "gvi") && (!(input2->answer) || !(input3->answer) 
                || !(input4->answer) || !(input5->answer) 
                || !(input6->answer) || !(input7->answer)) )
	G_fatal_error(_("gvi index requires blue, green, red, nir, chan5 and chan7 maps"));

    infd_redchan = Rast_open_old(redchan, "");
    inrast_redchan = Rast_allocate_d_buf();

    if (nirchan) {
        infd_nirchan = Rast_open_old(nirchan, "");
        inrast_nirchan = Rast_allocate_d_buf();
    }

    if (greenchan) {
	infd_greenchan = Rast_open_old(greenchan, "");
	inrast_greenchan = Rast_allocate_d_buf();
    }

    if (bluechan) {
	infd_bluechan = Rast_open_old(bluechan, "");
	inrast_bluechan = Rast_allocate_d_buf();
    }

    if (chan5chan) {
	infd_chan5chan = Rast_open_old(chan5chan, "");
	inrast_chan5chan = Rast_allocate_d_buf();
    }

    if (chan7chan) {
	infd_chan7chan = Rast_open_old(chan7chan, "");
	inrast_chan7chan = Rast_allocate_d_buf();
    }

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast = Rast_allocate_d_buf();

    /* Create New raster files */ 
    outfd = Rast_open_new(result, DCELL_TYPE);

    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	DCELL d_bluechan;
	DCELL d_greenchan;
	DCELL d_redchan;
	DCELL d_nirchan;
	DCELL d_chan5chan;
	DCELL d_chan7chan;

	G_percent(row, nrows, 2);

	Rast_get_d_row(infd_redchan, inrast_redchan, row);
	if (nirchan)
	    Rast_get_d_row(infd_nirchan, inrast_nirchan, row);
	if (greenchan)
	    Rast_get_d_row(infd_greenchan, inrast_greenchan, row);
	if (bluechan)
	    Rast_get_d_row(infd_bluechan, inrast_bluechan, row);
	if (chan5chan)
	    Rast_get_d_row(infd_chan5chan, inrast_chan5chan, row);
	if (chan7chan)
	    Rast_get_d_row(infd_chan7chan, inrast_chan7chan, row);

	/* process the data */ 
	for (col = 0; col < ncols; col++)
	{
	    d_redchan   = inrast_redchan[col];
            if(nirchan)
	    d_nirchan   = inrast_nirchan[col];
            if(greenchan)
	    d_greenchan = inrast_greenchan[col];
            if(bluechan)
	    d_bluechan  = inrast_bluechan[col];
            if(chan5chan)
	    d_chan5chan = inrast_chan5chan[col];
            if(chan7chan)
	    d_chan7chan = inrast_chan7chan[col];

	    if (Rast_is_d_null_value(&d_redchan) ||
		((nirchan) && Rast_is_d_null_value(&d_nirchan)) || 
		((greenchan) && Rast_is_d_null_value(&d_greenchan)) ||
		((bluechan) && Rast_is_d_null_value(&d_bluechan)) ||
		((chan5chan) && Rast_is_d_null_value(&d_chan5chan)) ||
		((chan7chan) && Rast_is_d_null_value(&d_chan7chan))) {
		Rast_set_d_null_value(&outrast[col], 1);
	    }
	    else {
		/* calculate simple_ratio        */ 
		if (!strcmp(viflag, "sr"))
		    outrast[col] = s_r(d_redchan, d_nirchan);

		/* calculate ndvi                    */ 
		if (!strcmp(viflag, "ndvi")) {
		    if (d_redchan + d_nirchan < 0.001)
			Rast_set_d_null_value(&outrast[col], 1);
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

		if (!strcmp(viflag, "vari"))
		    outrast[col] = va_ri(d_redchan, d_greenchan, d_bluechan);
	    }
	}
	Rast_put_d_row(outfd, outrast);
    }

    G_free(inrast_redchan);
    Rast_close(infd_redchan);
    G_free(inrast_nirchan);
    Rast_close(infd_nirchan);
    if (greenchan) {
	G_free(inrast_greenchan);
	Rast_close(infd_greenchan);
    }
    if (bluechan) {
	G_free(inrast_bluechan);
	Rast_close(infd_bluechan);
    }
    if (chan5chan) {
	G_free(inrast_chan5chan);
	Rast_close(infd_chan5chan);
    }
    if (chan7chan) {
	G_free(inrast_chan7chan);
	Rast_close(infd_chan7chan);
    }

    G_free(outrast);
    Rast_close(outfd);

    /* Color from -1.0 to +1.0 in grey */ 
    Rast_init_colors(&colors);
    val1 = -1;
    val2 = 1;
    Rast_add_c_color_rule(&val1, 0, 0, 0, &val2, 255, 255, 255, &colors);
    Rast_short_history(result, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result, &history);
    
    exit(EXIT_SUCCESS);
}

