
/****************************************************************************
 *
 * MODULE:       i.qc.modis
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Converts Quality Control indicators into human readable classes
 * 		 for Modis surface reflectance products 250m/500m
 * 		 (MOD09Q/MOD09A)
 *
 * COPYRIGHT:    (C) 2008 by the GRASS Development Team
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
#include <grass/glocale.h>
    
    /* 250m Products (MOD09Q) */ 
int qc250a(unsigned int pixel);

int qc250b(unsigned int pixel);

int qc250c(unsigned int pixel, int bandno);

int qc250d(unsigned int pixel);

int qc250e(unsigned int pixel);

int qc250f(unsigned int pixel);


    /* 500m Products (MOD09A) */ 
int qc500a(long int pixel);

int qc500c(long int pixel, int bandno);

int qc500d(long int pixel);

int qc500e(long int pixel);

int main(int argc, char *argv[]) 
{
    struct Cell_head cellhd;	/*region+header info */

    char *mapset;		/*mapset name */

    int nrows, ncols;

    int row, col;

    char *qcflag;		/*Switch for particular index */

    struct GModule *module;

    struct Option *input1, *input2, *input_band, *output;

    struct Flag *flag1, *flag2;

    struct History history;	/*metadata */

    struct Colors colors;	/*Color rules */

    

	/************************************/ 
	/* FMEO Declarations**************** */ 
    char *name;			/*input raster name */

    char *result;		/*output raster name */

    
	/*File Descriptors */ 
    int infd;

    int outfd;

    char *qcchan;

    int bandno;

    int i = 0, j = 0;

    void *inrast;

    CELL * outrast;
    RASTER_MAP_TYPE data_type_output = CELL_TYPE;
    RASTER_MAP_TYPE data_type_qcchan;
    

	/************************************/ 
	G_gisinit(argv[0]);
    module = G_define_module();
    module->keywords = _("QC, Quality Control, surface reflectance, Modis");
    module->description =
	_("Extract quality control parameters from Modis QC layers");
    
	/* Define the different options */ 
	input1 = G_define_option();
    input1->key = _("qcname");
    input1->type = TYPE_STRING;
    input1->required = YES;
    input1->gisprompt = _("Name of QC type to extract");
    input1->description =
	_("Name of QC: modland_qa_bits, cloud, data_quality, atcorr, adjcorr, diff_orbit_from_500m");
    input1->answer = _("modland_qa_bits");
    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->description =
	_("Name of the surface reflectance QC layer [bit array]");
    input_band = G_define_option();
    input_band->key = "band";
    input_band->type = TYPE_INTEGER;
    input_band->required = NO;
    input_band->gisprompt = "old,value";
    input_band->description =
	_("Band number of Modis product 250m=[1,2],500m=[1-7]");
    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->key = _("output");
    output->description =
	_("Name of the output QC type classification layer");
    output->answer = _("qc");
    flag1 = G_define_flag();
    flag1->key = 'A';
    flag1->description =
	_("QC for MOD09A product @ 500m instead of default MOD09Q@250m");
    

	/********************/ 
	if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    qcflag = input1->answer;
    qcchan = input2->answer;
    if (input_band->answer) {
	bandno = atoi(input_band->answer);
    }
    result = output->answer;
    

	/***************************************************/ 
	if ((!strcoll(qcflag, "cloud") && flag1->answer) || 
	    (!strcoll(qcflag, "diff_orbit_from_500m") && flag1->answer)) {
	G_fatal_error("Those flags cannot work with MOD09A @ 500m products");
    }
    if (!strcoll(qcflag, "data_quality")) {
	if (bandno < 1 || bandno > 7)
	    G_fatal_error("band number out of allowed range [1-7]");
	if (!flag1->answer && bandno > 2)
	    G_fatal_error("250m band number is out of allowed range [1,2]");
    }
    

	/***************************************************/ 
	mapset = G_find_cell2(qcchan, "");
    if (mapset == NULL) {
	G_fatal_error(_("cell file [%s] not found"), qcchan);
    }
    data_type_qcchan = G_raster_map_type(qcchan, mapset);
    if ((infd = G_open_cell_old(qcchan, mapset)) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), qcchan);
    if (G_get_cellhd(qcchan, mapset, &cellhd) < 0)
	G_fatal_error(_("Cannot read file header of [%s]"), qcchan);
    inrast = G_allocate_raster_buf(data_type_qcchan);
    

	/***************************************************/ 
	G_debug(3, "number of rows %d", cellhd.rows);
    nrows = G_window_rows();
    ncols = G_window_cols();
    outrast = G_allocate_raster_buf(data_type_output);
    
	/* Create New raster files */ 
	if ((outfd = G_open_raster_new(result, data_type_output)) < 0)
	G_fatal_error(_("Could not open <%s>"), result);
    
	/* Process pixels */ 
	for (row = 0; row < nrows; row++)
	 {
	CELL c;
	unsigned int qc250chan;

	CELL qc500chan;
	G_percent(row, nrows, 2);
	if (G_get_raster_row(infd, inrast, row, data_type_qcchan) < 0)
	    G_fatal_error(_("Could not read from <%s>"), qcchan);
	
	    /*process the data */ 
	    for (col = 0; col < ncols; col++)
	     {
	    switch (data_type_qcchan) {
	    case CELL_TYPE:
		c = (int)((CELL *) inrast)[col];
		break;
	    case FCELL_TYPE:
		c = (int)((FCELL *) inrast)[col];
		break;
	    case DCELL_TYPE:
		c = (int)((DCELL *) inrast)[col];
		break;
	    }
	    if (flag1->answer) {
		
		    /* This is a MOD09A@500m product, QC layer is 32-bit */ 
		    qc500chan = c;
	    }
	    else {
		
		    /* This is a MOD09A@250m product, QC layer is 16-bit */ 
		    qc250chan = (unsigned int)((CELL *) inrast)[col];
	    } if (G_is_c_null_value(&c)) {
		G_set_c_null_value(&outrast[col], 1);
	    }
	    else {
		
		    /*calculate modland QA bits extraction  */ 
		    if (!strcoll(qcflag, "modland_qa_bits")) {
		    if (flag1->answer) {
			
			    /* 500m product */ 
			    c = qc500a(qc500chan);
		    }
		    else {
			
			    /* 250m product */ 
			    c = qc250a(qc250chan);
		    }
		    outrast[col] = c;
		}
		else
		    
			/*calculate cloud state  */ 
		    if (!strcoll(qcflag, "cloud")) {
		    
			/* ONLY 250m product! */ 
			c = qc250b(qc250chan);
		    outrast[col] = c;
		}
		else
		    
			/*calculate modland QA bits extraction  */ 
		    if (!strcoll(qcflag, "data_quality")) {
		    if (flag1->answer) {
			
			    /* 500m product */ 
			    c = qc500c(qc500chan, bandno);
		    }
		    else {
			
			    /* 250m product */ 
			    c = qc250c(qc250chan, bandno);
		    }
		    outrast[col] = c;
		}
		else
		    
			/*calculate atmospheric correction flag  */ 
		    if (!strcoll(qcflag, "atcorr")) {
		    if (flag1->answer) {
			
			    /* 500m product */ 
			    c = qc500d(qc500chan);
		    }
		    else {
			
			    /* 250m product */ 
			    c = qc250d(qc250chan);
		    }
		    outrast[col] = c;
		}
		else
		    
			/*calculate adjacency correction flag  */ 
		    if (!strcoll(qcflag, "adjcorr")) {
		    if (flag1->answer) {
			
			    /* 500m product */ 
			    c = qc500e(qc500chan);
		    }
		    else {
			
			    /* 250m product */ 
			    c = qc250e(qc250chan);
		    }
		    outrast[col] = c;
		}
		else
		    
			/*calculate different orbit from 500m flag  */ 
		    if (!strcoll(qcflag, "diff_orbit_from_500m")) {
		    
			/* ONLY 250m product! */ 
			c = qc250f(qc500chan);
		    outrast[col] = c;
		}
		else {
		    
			/* Signal user that the flag name is badly written */ 
			/* therefore not understood by the application */ 
			G_fatal_error
			("Unknown flag name, please check spelling");
		}
	    }
	    }
	if (G_put_raster_row(outfd, outrast, data_type_output) < 0)
	    G_fatal_error(_("Cannot write to output raster file"));
	}
    G_free(inrast);
    G_close_cell(infd);
    G_free(outrast);
    G_close_cell(outfd);
    
	/* Color from -1.0 to +1.0 in grey */ 
	G_init_colors(&colors);
    G_add_color_rule(0, 0, 0, 0, 10, 255, 255, 255, &colors);
    G_short_history(result, "raster", &history);
    G_command_history(&history);
    G_write_history(result, &history);
    exit(EXIT_SUCCESS);
}


