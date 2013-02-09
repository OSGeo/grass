
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
    struct {
        struct Option *viname, *red, *nir, *green, *blue, *chan5,
            *chan7, *sl_slope, *sl_int, *sl_red, *bits, *output;
    } opt;
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
    opt.red = G_define_standard_option(G_OPT_R_INPUT);
    opt.red->key = "red";
    opt.red->label =
	_("Name of input red channel surface reflectance map");
    opt.red->description = _("Range: [0.0;1.0]");

    opt.viname = G_define_option();
    opt.viname->key = "viname";
    opt.viname->type = TYPE_STRING;
    opt.viname->required = YES;
    opt.viname->description = _("Type of vegetation index");
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
    opt.viname->descriptions = desc;
    opt.viname->options = "arvi,dvi,evi,evi2,gvi,gari,gemi,ipvi,msavi,msavi2,ndvi,pvi,savi,sr,vari,wdvi";
    opt.viname->answer = "ndvi";
    opt.viname->key_desc = _("type");

    opt.output = G_define_standard_option(G_OPT_R_OUTPUT);

    opt.nir = G_define_standard_option(G_OPT_R_INPUT);
    opt.nir->key = "nir";
    opt.nir->required = NO;
    opt.nir->label =
	_("Name of input nir channel surface reflectance map");
    opt.nir->description = _("Range: [0.0;1.0]");
    opt.nir->guisection = _("Optional inputs");

    opt.green = G_define_standard_option(G_OPT_R_INPUT);
    opt.green->key = "green";
    opt.green->required = NO;
    opt.green->label =
	_("Name of input green channel surface reflectance map");
    opt.green->description = _("Range: [0.0;1.0]");
    opt.green->guisection = _("Optional inputs");

    opt.blue = G_define_standard_option(G_OPT_R_INPUT);
    opt.blue->key = "blue";
    opt.blue->required = NO;
    opt.blue->label =
	_("Name of input blue channel surface reflectance map");
    opt.blue->description = _("Range: [0.0;1.0]");
    opt.blue->guisection = _("Optional inputs");

    opt.chan5 = G_define_standard_option(G_OPT_R_INPUT);
    opt.chan5->key = "chan5";
    opt.chan5->required = NO;
    opt.chan5->label =
	_("Name of input 5th channel surface reflectance map");
    opt.chan5->description = _("Range: [0.0;1.0]");
    opt.chan5->guisection = _("Optional inputs");

    opt.chan7 = G_define_standard_option(G_OPT_R_INPUT);
    opt.chan7->key = "chan7";
    opt.chan7->required = NO;
    opt.chan7->label =
	_("Name of input 7th channel surface reflectance map");
    opt.chan7->description = _("Range: [0.0;1.0]");
    opt.chan7->guisection = _("Optional inputs");

    opt.sl_slope = G_define_option();
    opt.sl_slope->key = "soil_line_slope";
    opt.sl_slope->type = TYPE_DOUBLE;
    opt.sl_slope->required = NO;
    opt.sl_slope->description = _("Value of the slope of the soil line (MSAVI2 only)");
    opt.sl_slope->guisection = _("MSAVI2 seetings");

    opt.sl_int = G_define_option();
    opt.sl_int->key = "soil_line_intercept";
    opt.sl_int->type = TYPE_DOUBLE;
    opt.sl_int->required = NO;
    opt.sl_int->description = _("Value of the intercept of the soil line (MSAVI2 only)");
    opt.sl_int->guisection = _("MSAVI2 seetings");

    opt.sl_red = G_define_option();
    opt.sl_red->key = "soil_noise_reduction";
    opt.sl_red->type = TYPE_DOUBLE;
    opt.sl_red->required = NO;
    opt.sl_red->description = _("Value of the factor of reduction of soil noise (MSAVI2 only)");
    opt.sl_red->guisection = _("MSAVI2 seetings");

    opt.bits = G_define_option();
    opt.bits->key = "storage_bit";
    opt.bits->type = TYPE_INTEGER;
    opt.bits->required = NO;
    opt.bits->label = _("Maximum bits for digital numbers");
    opt.bits->description = _("If data is in Digital Numbers (i.e. integer type), give the max bits (i.e. 8 for Landsat -> [0-255])");
    opt.bits->options = "7,8,10,16";
    opt.bits->answer = "8";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    viflag = opt.viname->answer;
    redchan = opt.red->answer;
    nirchan = opt.nir->answer;
    greenchan = opt.green->answer;
    bluechan = opt.blue->answer;
    chan5chan = opt.chan5->answer;
    chan7chan = opt.chan7->answer;
    if(opt.sl_slope->answer)
        msavip1 = atof(opt.sl_slope->answer);
    if(opt.sl_int->answer)
        msavip2 = atof(opt.sl_int->answer);
    if(opt.sl_red->answer)
        msavip3 = atof(opt.sl_red->answer);
    if(opt.bits->answer)
        dnbits = atof(opt.bits->answer);
    result = opt.output->answer;

    if (!strcasecmp(viflag, "sr") && (!(opt.red->answer) || !(opt.nir->answer)) )
	G_fatal_error(_("sr index requires red and nir maps"));

    if (!strcasecmp(viflag, "ndvi") && (!(opt.red->answer) || !(opt.nir->answer)) )
	G_fatal_error(_("ndvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "ipvi") && (!(opt.red->answer) || !(opt.nir->answer)) )
	G_fatal_error(_("ipvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "dvi") && (!(opt.red->answer) || !(opt.nir->answer)) )
	G_fatal_error(_("dvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "pvi") && (!(opt.red->answer) || !(opt.nir->answer)) )
	G_fatal_error(_("pvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "wdvi") && (!(opt.red->answer) || !(opt.nir->answer)) )
	G_fatal_error(_("wdvi index requires red and nir maps"));

    if (!strcasecmp(viflag, "savi") && (!(opt.red->answer) || !(opt.nir->answer)) )
	G_fatal_error(_("savi index requires red and nir maps"));

    if (!strcasecmp(viflag, "msavi") && (!(opt.red->answer) || !(opt.nir->answer)) )
	G_fatal_error(_("msavi index requires red and nir maps"));

    if (!strcasecmp(viflag, "msavi2") && (!(opt.red->answer) || !(opt.nir->answer) || 
                                          !(opt.sl_slope->answer) || !(opt.sl_int->answer) || 
                                          !(opt.sl_red->answer)) )
	G_fatal_error(_("msavi2 index requires red and nir maps, and 3 parameters related to soil line"));

    if (!strcasecmp(viflag, "gemi") && (!(opt.red->answer) || !(opt.nir->answer)) )
	G_fatal_error(_("gemi index requires red and nir maps"));

    if (!strcasecmp(viflag, "arvi") && (!(opt.red->answer) || !(opt.nir->answer)
                || !(opt.blue->answer)) )
	G_fatal_error(_("arvi index requires blue, red and nir maps"));

    if (!strcasecmp(viflag, "evi") && (!(opt.red->answer) || !(opt.nir->answer)
                || !(opt.blue->answer)) )
	G_fatal_error(_("evi index requires blue, red and nir maps"));

	if (!strcasecmp(viflag, "evi2") && (!(opt.red->answer) || !(opt.nir->answer) ) )
	G_fatal_error(_("evi2 index requires red and nir maps"));
	
    if (!strcasecmp(viflag, "vari") && (!(opt.red->answer) || !(opt.green->answer)
                || !(opt.blue->answer)) )
	G_fatal_error(_("vari index requires blue, green and red maps"));

    if (!strcasecmp(viflag, "gari") && (!(opt.red->answer) || !(opt.nir->answer)
                || !(opt.green->answer) || !(opt.blue->answer)) )
	G_fatal_error(_("gari index requires blue, green, red and nir maps"));

    if (!strcasecmp(viflag, "gvi") && (!(opt.red->answer) || !(opt.nir->answer)
                || !(opt.green->answer) || !(opt.blue->answer)
                || !(opt.chan5->answer) || !(opt.chan7->answer)) )
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
			if(opt.bits->answer)
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
			if(opt.bits->answer)
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
			if(opt.bits->answer)
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
			if(opt.bits->answer)
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
			if(opt.bits->answer)
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
			if(opt.bits->answer)
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
