
/****************************************************************************
 *
 * MODULE:       i.vi
 * AUTHOR(S):    Baburao Kamble baburaokamble@gmail.com
 *		 Yann Chemin - yann.chemin@gmail.com
 *		 Nikos Alexandris - nik@nikosalexandris.net
 * PURPOSE:      Calculates 15 vegetation indices
 * 		 based on biophysical parameters.
 *
 * COPYRIGHT:    (C) 2002-2013 by the GRASS Development Team
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
 * 		 Added EVI2 on 20130208 (NikosA)
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

double s_r(double redchan, double nirchan);
double nd_vi(double redchan, double nirchan);
double ip_vi(double redchan, double nirchan);
double d_vi(double redchan, double nirchan);
double e_vi(double bluechan, double redchan, double nirchan);
double e_vi2(double redchan, double nirchan);
double p_vi(double redchan, double nirchan);
double wd_vi(double redchan, double nirchan);
double sa_vi(double redchan, double nirchan);
double msa_vi(double redchan, double nirchan, double soil_line_slope, double soil_line_intercept, double soil_noise_reduction_factor);
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
    char *desc;
    struct GModule *module;
    struct Option *input1, *input2, *input3, *input4, *input5, *input6,
	*input7, *input8, *input9, *input10, *input11, *output;
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
    RASTER_MAP_TYPE data_type_redchan;
    RASTER_MAP_TYPE data_type_nirchan, data_type_greenchan;
    RASTER_MAP_TYPE data_type_bluechan;
    RASTER_MAP_TYPE data_type_chan5chan, data_type_chan7chan;
    DCELL msavip1, msavip2, msavip3, dnbits;
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
    desc = NULL;
    G_asprintf(&desc,
	       "arvi;%s;dvi;%s;evi;%s;evi2;%s;gvi;%s;gari;%s;gemi;%s;ipvi;%s;msavi;%s;"
	       "msavi2;%s;ndvi;%s;pvi;%s;savi;%s;sr;%s;vari;%s;wdvi;%s",
	       _("Atmospherically Resistant Vegetation Indices"),
	       _("Difference Vegetation Index"),
	       _("Enhanced Vegetation Index"),
	       _("Enhanced Vegetation Index 2"),
	       _("Green Vegetation Index"),
	       _("Green atmospherically resistant vegetation index"),
	       _("Global Environmental Monitoring Index"),
	       _("Infrared Percentage Vegetation IndexInfrared Percentage Vegetation Index"),
	       _("Modified Soil Adjusted Vegetation Index"),
	       _("second Modified Soil Adjusted Vegetation Index"),
	       _("Normalized Difference Vegetation Index"),
	       _("Perpendicular Vegetation Index"),
	       _("Soil Adjusted Vegetation Index"),
	       _("Simple Ratio"),
	       _("Visible Atmospherically Resistant Index"),
	       _("Weighted Difference Vegetation Index"));
    input1->descriptions = desc;
    input1->options = "arvi,dvi,evi,evi2,gvi,gari,gemi,ipvi,msavi,msavi2,ndvi,pvi,savi,sr,vari,wdvi";
    input1->answer = "ndvi";

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

    input8 = G_define_option();
    input8->key = "soil_line_slope";
    input8->type = TYPE_DOUBLE;
    input8->required = NO;
    input8->description = _("MSAVI2: Value of the slope of the soil line");

    input9 = G_define_option();
    input9->key = "soil_line_intercept";
    input9->type = TYPE_DOUBLE;
    input9->required = NO;
    input9->description = _("MSAVI2: Value of the intercept of the soil line");

    input10 = G_define_option();
    input10->key = "soil_noise_reduction_factor";
    input10->type = TYPE_DOUBLE;
    input10->required = NO;
    input10->description = _("MSAVI2: Value of the factor of reduction of soil noise");

    input11 = G_define_option();
    input11->key = "DN_storage_bit";
    input11->type = TYPE_INTEGER;
    input11->required = NO;
    input11->description = _("If your data is in Digital Numbers (i.e. integer type), give the max bits (i.e. 8 for Landsat -> [0-255])");
    input11->options = "7,8,10,16";
    input11->answer = "8";

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
    if(input8->answer)
        msavip1 = atof(input8->answer);
    if(input9->answer)
        msavip2 = atof(input9->answer);
    if(input10->answer)
        msavip3 = atof(input10->answer);
    if(input11->answer)
        dnbits = atof(input11->answer);
    result = output->answer;

    if (!strcasecmp(viflag, "sr") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("sr index requires red and nir maps"));

    if (!strcasecmp(viflag, "ndvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("ndvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "ipvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("ipvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "dvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("dvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "pvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("pvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "wdvi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("wdvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "savi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("savi index requires red and nir maps"));

    if (!strcasecmp(viflag, "msavi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("msavi index requires red and nir maps"));

    if (!strcasecmp(viflag, "msavi2") && (!(input2->answer) || !(input3->answer)||!(input8->answer) ||!(input9->answer)||!(input10->answer)) )
	G_fatal_error(_("msavi2 index requires red and nir maps, and 3 parameters related to soil line"));

    if (!strcasecmp(viflag, "gemi") && (!(input2->answer) || !(input3->answer)) )
	G_fatal_error(_("gemi index requires red and nir maps"));

    if (!strcasecmp(viflag, "arvi") && (!(input2->answer) || !(input3->answer)
                || !(input5->answer)) )
	G_fatal_error(_("arvi index requires blue, red and nir maps"));

    if (!strcasecmp(viflag, "evi") && (!(input2->answer) || !(input3->answer)
                || !(input5->answer)) )
	G_fatal_error(_("evi index requires blue, red and nir maps"));

	if (!strcasecmp(viflag, "evi2") && (!(input2->answer) || !(input3->answer) ) )
	G_fatal_error(_("evi2 index requires red and nir maps"));
	
    if (!strcasecmp(viflag, "vari") && (!(input2->answer) || !(input4->answer)
                || !(input5->answer)) )
	G_fatal_error(_("vari index requires blue, green and red maps"));

    if (!strcasecmp(viflag, "gari") && (!(input2->answer) || !(input3->answer)
                || !(input4->answer) || !(input5->answer)) )
	G_fatal_error(_("gari index requires blue, green, red and nir maps"));

    if (!strcasecmp(viflag, "gvi") && (!(input2->answer) || !(input3->answer)
                || !(input4->answer) || !(input5->answer)
                || !(input6->answer) || !(input7->answer)) )
	G_fatal_error(_("gvi index requires blue, green, red, nir, chan5 and chan7 maps"));

    infd_redchan = Rast_open_old(redchan, "");
    data_type_redchan = Rast_map_type(redchan, "");
    inrast_redchan = Rast_allocate_buf(data_type_redchan);

    if (nirchan) {
        infd_nirchan = Rast_open_old(nirchan, "");
        data_type_nirchan = Rast_map_type(nirchan, "");
        inrast_nirchan = Rast_allocate_buf(data_type_nirchan);
    }

    if (greenchan) {
        infd_greenchan = Rast_open_old(greenchan, "");
        data_type_greenchan = Rast_map_type(greenchan, "");
        inrast_greenchan = Rast_allocate_buf(data_type_greenchan);
    }

    if (bluechan) {
        infd_bluechan = Rast_open_old(bluechan, "");
        data_type_bluechan = Rast_map_type(bluechan, "");
        inrast_bluechan = Rast_allocate_buf(data_type_bluechan);
    }

    if (chan5chan) {
        infd_chan5chan = Rast_open_old(chan5chan, "");
        data_type_chan5chan = Rast_map_type(chan5chan, "");
        inrast_chan5chan = Rast_allocate_buf(data_type_chan5chan);
    }

    if (chan7chan) {
        infd_chan7chan = Rast_open_old(chan7chan, "");
        data_type_chan7chan = Rast_map_type(chan7chan, "");
        inrast_chan7chan = Rast_allocate_buf(data_type_chan7chan);
    }

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* Create New raster files */
    outfd = Rast_open_new(result, DCELL_TYPE);
    outrast = Rast_allocate_d_buf();

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

	/* read input maps */
	Rast_get_row(infd_redchan,inrast_redchan,row,data_type_redchan);
	if (nirchan) {
	    Rast_get_row(infd_nirchan,inrast_nirchan,row,data_type_nirchan);
	}
	if (bluechan) {
	    Rast_get_row(infd_bluechan,inrast_bluechan,row,data_type_bluechan);
	}
	if (greenchan) {
	    Rast_get_row(infd_greenchan,inrast_greenchan,row,data_type_greenchan);
	}
	if (chan5chan) {
	    Rast_get_row(infd_chan5chan,inrast_chan5chan,row,data_type_chan5chan);
	}
	if (chan7chan) {
	    Rast_get_row(infd_chan7chan,inrast_chan7chan,row,data_type_chan7chan);
	}
	/* process the data */
	for (col = 0; col < ncols; col++)
	{
	    switch(data_type_redchan){
		    case CELL_TYPE:
			d_redchan   = (double) ((CELL *) inrast_redchan)[col];
			if(input11->answer)
				d_redchan *= 1.0/(pow(2,dnbits)-1);	
			break;
		    case FCELL_TYPE:
			d_redchan   = (double) ((FCELL *) inrast_redchan)[col];
			break;
		    case DCELL_TYPE:
			d_redchan   = ((DCELL *) inrast_redchan)[col];
			break;
	    }
	    if (nirchan) {
		switch(data_type_nirchan){
		    case CELL_TYPE:
			d_nirchan   = (double) ((CELL *) inrast_nirchan)[col];
			if(input11->answer)
				d_nirchan *= 1.0/(pow(2,dnbits)-1);	
			break;
		    case FCELL_TYPE:
			d_nirchan   = (double) ((FCELL *) inrast_nirchan)[col];
			break;
		    case DCELL_TYPE:
			d_nirchan   = ((DCELL *) inrast_nirchan)[col];
			break;
		}
	    }
	    if (greenchan) {
		switch(data_type_greenchan){
		    case CELL_TYPE:
			d_greenchan   = (double) ((CELL *) inrast_greenchan)[col];
			if(input11->answer)
				d_greenchan *= 1.0/(pow(2,dnbits)-1);	
			break;
		    case FCELL_TYPE:
			d_greenchan   = (double) ((FCELL *) inrast_greenchan)[col];
			break;
		    case DCELL_TYPE:
			d_greenchan   = ((DCELL *) inrast_greenchan)[col];
			break;
		}
	    }
	    if (bluechan) {
		switch(data_type_bluechan){
		    case CELL_TYPE:
			d_bluechan   = (double) ((CELL *) inrast_bluechan)[col];
			if(input11->answer)
				d_bluechan *= 1.0/(pow(2,dnbits)-1);	
			break;
		    case FCELL_TYPE:
			d_bluechan   = (double) ((FCELL *) inrast_bluechan)[col];
			break;
		    case DCELL_TYPE:
			d_bluechan   = ((DCELL *) inrast_bluechan)[col];
			break;
		}
	    }
	    if (chan5chan) {
		switch(data_type_chan5chan){
		    case CELL_TYPE:
			d_chan5chan   = (double) ((CELL *) inrast_chan5chan)[col];
			if(input11->answer)
				d_chan5chan *= 1.0/(pow(2,dnbits)-1);	
			break;
		    case FCELL_TYPE:
			d_chan5chan   = (double) ((FCELL *) inrast_chan5chan)[col];
			break;
		    case DCELL_TYPE:
			d_chan5chan   = ((DCELL *) inrast_chan5chan)[col];
			break;
		}
	    }
	    if (chan7chan) {
		switch(data_type_chan7chan){
		    case CELL_TYPE:
			d_chan7chan   = (double) ((CELL *) inrast_chan7chan)[col];
			if(input11->answer)
				d_chan7chan *= 1.0/(pow(2,dnbits)-1);	
			break;
		    case FCELL_TYPE:
			d_chan7chan   = (double) ((FCELL *) inrast_chan7chan)[col];
			break;
		    case DCELL_TYPE:
			d_chan7chan   = ((DCELL *) inrast_chan7chan)[col];
			break;
		}
	    }

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
		if (!strcasecmp(viflag, "sr"))
		    outrast[col] = s_r(d_redchan, d_nirchan);

		/* calculate ndvi                    */
		if (!strcasecmp(viflag, "ndvi")) {
		    if (d_redchan + d_nirchan < 0.001)
			Rast_set_d_null_value(&outrast[col], 1);
		    else
			outrast[col] = nd_vi(d_redchan, d_nirchan);
		}

		if (!strcasecmp(viflag, "ipvi"))
		    outrast[col] = ip_vi(d_redchan, d_nirchan);

		if (!strcasecmp(viflag, "dvi"))
		    outrast[col] = d_vi(d_redchan, d_nirchan);

		if (!strcasecmp(viflag, "evi"))
		    outrast[col] = e_vi(d_bluechan, d_redchan, d_nirchan);

		if (!strcasecmp(viflag, "evi2"))
		    outrast[col] = e_vi2(d_redchan, d_nirchan);

		if (!strcasecmp(viflag, "pvi"))
		    outrast[col] = p_vi(d_redchan, d_nirchan);

		if (!strcasecmp(viflag, "wdvi"))
		    outrast[col] = wd_vi(d_redchan, d_nirchan);

		if (!strcasecmp(viflag, "savi"))
		    outrast[col] = sa_vi(d_redchan, d_nirchan);

		if (!strcasecmp(viflag, "msavi"))
		    outrast[col] = msa_vi(d_redchan, d_nirchan, msavip1, msavip2, msavip3);

		if (!strcasecmp(viflag, "msavi2"))
		    outrast[col] = msa_vi2(d_redchan, d_nirchan);

		if (!strcasecmp(viflag, "gemi"))
		    outrast[col] = ge_mi(d_redchan, d_nirchan);

		if (!strcasecmp(viflag, "arvi"))
		    outrast[col] = ar_vi(d_redchan, d_nirchan, d_bluechan);

		if (!strcasecmp(viflag, "gvi"))
		    outrast[col] = g_vi(d_bluechan, d_greenchan, d_redchan, d_nirchan, d_chan5chan, d_chan7chan);

		if (!strcasecmp(viflag, "gari"))
		    outrast[col] = ga_ri(d_redchan, d_nirchan, d_bluechan, d_greenchan);

		if (!strcasecmp(viflag, "vari"))
		    outrast[col] = va_ri(d_redchan, d_greenchan, d_bluechan);
	    }
	}
	Rast_put_d_row(outfd, outrast);
    }

    G_free(inrast_redchan);
    Rast_close(infd_redchan);
    if (nirchan) {
    	G_free(inrast_nirchan);
    	Rast_close(infd_nirchan);
    }
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
