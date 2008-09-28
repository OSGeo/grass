
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
    struct Cell_head cellhd;	/*region+header info */

    char *mapset;		/*mapset name */

    int nrows, ncols;

    int row, col;

    char *viflag;		/*Switch for particular index */

    struct GModule *module;

    struct Option *input1, *input2, *input3, *input4, *input5, *input6,
	*input7, *output;
    struct Flag *flag1;

    struct History history;	/*metadata */

    struct Colors colors;	/*Color rules */

    

	/************************************/ 
	/* FMEO Declarations**************** */ 
    char *name;			/*input raster name */

    char *result;		/*output raster name */

    
	/*File Descriptors */ 
    int infd_redchan, infd_nirchan, infd_greenchan;

    int infd_bluechan, infd_chan5chan, infd_chan7chan;

    int outfd;

    char *bluechan, *greenchan, *redchan, *nirchan, *chan5chan, *chan7chan;

    int i = 0, j = 0;

    void *inrast_redchan, *inrast_nirchan, *inrast_greenchan;

    void *inrast_bluechan, *inrast_chan5chan, *inrast_chan7chan;

    DCELL * outrast;
    RASTER_MAP_TYPE data_type_output = DCELL_TYPE;
    RASTER_MAP_TYPE data_type_redchan;
    RASTER_MAP_TYPE data_type_nirchan;
    RASTER_MAP_TYPE data_type_greenchan;
    RASTER_MAP_TYPE data_type_bluechan;
    RASTER_MAP_TYPE data_type_chan5chan;
    RASTER_MAP_TYPE data_type_chan7chan;
    

	/************************************/ 
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
    

	/********************/ 
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
    

	/***************************************************/ 
	mapset = G_find_cell2(redchan, "");
    if (mapset == NULL) {
	G_fatal_error(_("%s - cell file [%s] not found"),
			G_program_name(), redchan);
    }
    data_type_redchan = G_raster_map_type(redchan, mapset);
    if ((infd_redchan = G_open_cell_old(redchan, mapset)) < 0){
	G_fatal_error(_("%s - Cannot open cell file [%s]"),
			G_program_name(), redchan);
    }
    if (G_get_cellhd(redchan, mapset, &cellhd) < 0){
	G_fatal_error(_("%s - Cannot read file header of [%s])"),
			G_program_name(), redchan);
    }
    inrast_redchan = G_allocate_raster_buf(data_type_redchan);
    

	/***************************************************/ 
	mapset = G_find_cell2(nirchan, "");
    if (mapset == NULL) {
	G_fatal_error(_("%s - cell file [%s] not found"),
			G_program_name(), nirchan);
    }
    data_type_nirchan = G_raster_map_type(nirchan, mapset);
    if ((infd_nirchan = G_open_cell_old(nirchan, mapset)) < 0){
	G_fatal_error(_("%s - Cannot open cell file [%s]"),
			G_program_name(), nirchan);
    }
    if (G_get_cellhd(nirchan, mapset, &cellhd) < 0){
	G_fatal_error(_("%s - Cannot read file header of [%s]"),
			G_program_name(), nirchan);
    }
    inrast_nirchan = G_allocate_raster_buf(data_type_nirchan);
    

	/***************************************************/ 
	if (greenchan) {
	mapset = G_find_cell2(greenchan, "");
	if (mapset == NULL) {
	    G_fatal_error(_("%s - cell file [%s] not found"),
			G_program_name(), greenchan);
	}
	data_type_greenchan = G_raster_map_type(greenchan, mapset);
	if ((infd_greenchan = G_open_cell_old(greenchan, mapset)) < 0){
	    G_fatal_error(_("%s - Cannot open cell file [%s]"),
			G_program_name(), greenchan);
        }
	if (G_get_cellhd(greenchan, mapset, &cellhd) < 0){
	    G_fatal_error(_("%s - Cannot read file header of [%s])"),
			G_program_name(), greenchan);
        }
	inrast_greenchan = G_allocate_raster_buf(data_type_greenchan);
    }
    

	/***************************************************/ 
	if (bluechan) {
	mapset = G_find_cell2(bluechan, "");
	if (mapset == NULL) {
	    G_fatal_error(_("%s - cell file [%s] not found"),
			G_program_name(), bluechan);
	}
	data_type_bluechan = G_raster_map_type(bluechan, mapset);
	if ((infd_bluechan = G_open_cell_old(bluechan, mapset)) < 0){
	    G_fatal_error(_("%s - Cannot open cell file [%s]"),
			G_program_name(), bluechan);
        }
	if (G_get_cellhd(bluechan, mapset, &cellhd) < 0){
	    G_fatal_error(_("%s - Cannot read file header of [%s])"),
			G_program_name(), bluechan);
        }
	inrast_bluechan = G_allocate_raster_buf(data_type_bluechan);
    }
    

	/***************************************************/ 
	if (chan5chan) {
	mapset = G_find_cell2(chan5chan, "");
	if (mapset == NULL) {
	    G_fatal_error(_("%s - cell file [%s] not found"),
			G_program_name(), chan5chan);
	}
	data_type_chan5chan = G_raster_map_type(chan5chan, mapset);
	if ((infd_chan5chan = G_open_cell_old(chan5chan, mapset)) < 0){
	    G_fatal_error(_("%s - Cannot open cell file [%s]"),
			G_program_name(), chan5chan);
        }
	if (G_get_cellhd(chan5chan, mapset, &cellhd) < 0){
	    G_fatal_error(_("%s - Cannot read file header of [%s])"),
			G_program_name(), chan5chan);
        }
	inrast_chan5chan = G_allocate_raster_buf(data_type_chan5chan);
    }
    

	/***************************************************/ 
	if (chan7chan) {
	mapset = G_find_cell2(chan7chan, "");
	if (mapset == NULL) {
	    G_fatal_error(_("%s - cell file [%s] not found"),
			G_program_name(), chan7chan);
	}
	data_type_chan7chan = G_raster_map_type(chan7chan, mapset);
	if ((infd_chan7chan = G_open_cell_old(chan7chan, mapset)) < 0){
	    G_fatal_error(_("%s - Cannot open cell file [%s]"),
			G_program_name(), chan7chan);
        }
	if (G_get_cellhd(chan7chan, mapset, &cellhd) < 0){
	    G_fatal_error(_("%s - Cannot read file header of [%s])"),
			G_program_name(), chan7chan);
        }
	inrast_chan7chan = G_allocate_raster_buf(data_type_chan7chan);
    }
    

	/***************************************************/ 
	G_debug(3, "number of rows %d", cellhd.rows);
    nrows = G_window_rows();
    ncols = G_window_cols();
    outrast = G_allocate_raster_buf(data_type_output);
    
	/* Create New raster files */ 
	if ((outfd = G_open_raster_new(result, data_type_output)) < 0){
	G_fatal_error(_("%s - Could not open <%s>"),
			G_program_name(), result);
        }
    
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
	if (G_get_raster_row
	     (infd_redchan, inrast_redchan, row, data_type_redchan) < 0){
	    G_fatal_error(_("%s - Could not read from <%s>"),
			G_program_name(), redchan);
        }
	if (G_get_raster_row
	     (infd_nirchan, inrast_nirchan, row, data_type_nirchan) < 0){
	    G_fatal_error(_("%s - Could not read from <%s>"),
			G_program_name(), nirchan);
        }
	if (greenchan) {
	    if (G_get_raster_row
		 (infd_greenchan, inrast_greenchan, row,
		  data_type_greenchan) < 0){
		G_fatal_error(_("%s - Could not read from <%s>"),
			G_program_name(), greenchan);
            }
	}
	if (bluechan) {
	    if (G_get_raster_row
		 (infd_bluechan, inrast_bluechan, row,
		  data_type_bluechan) < 0){
		G_fatal_error(_("%s - Could not read from <%s>"),
			G_program_name(), bluechan);
            }
	}
	if (chan5chan) {
	    if (G_get_raster_row
		 (infd_chan5chan, inrast_chan5chan, row,
		  data_type_chan5chan) < 0){
		G_fatal_error(_("%s - Could not read from <%s>"),
			G_program_name(), chan5chan);
            }
	}
	if (chan7chan) {
	    if (G_get_raster_row
		 (infd_chan7chan, inrast_chan7chan, row,
		  data_type_chan7chan) < 0){
		G_fatal_error(_("%s - Could not read from <%s>"),
			G_program_name(), chan7chan);
            }
	}
	
	    /*process the data */ 
	    for (col = 0; col < ncols; col++)
	     {
	    switch (data_type_redchan) {
	    case CELL_TYPE:
		d_redchan = (double)((CELL *) inrast_redchan)[col];
		break;
	    case FCELL_TYPE:
		d_redchan = (double)((FCELL *) inrast_redchan)[col];
		break;
	    case DCELL_TYPE:
		d_redchan = ((DCELL *) inrast_redchan)[col];
		break;
	    }
	    switch (data_type_nirchan) {
	    case CELL_TYPE:
		d_nirchan = (double)((CELL *) inrast_nirchan)[col];
		break;
	    case FCELL_TYPE:
		d_nirchan = (double)((FCELL *) inrast_nirchan)[col];
		break;
	    case DCELL_TYPE:
		d_nirchan = ((DCELL *) inrast_nirchan)[col];
		break;
	    }
	    if (greenchan) {
		switch (data_type_greenchan) {
		case CELL_TYPE:
		    d_greenchan = (double)((CELL *) inrast_greenchan)[col];
		    break;
		case FCELL_TYPE:
		    d_greenchan = (double)((FCELL *) inrast_greenchan)[col];
		    break;
		case DCELL_TYPE:
		    d_greenchan = ((DCELL *) inrast_greenchan)[col];
		    break;
		}
	    }
	    if (bluechan) {
		switch (data_type_bluechan) {
		case CELL_TYPE:
		    d_bluechan = (double)((CELL *) inrast_bluechan)[col];
		    break;
		case FCELL_TYPE:
		    d_bluechan = (double)((FCELL *) inrast_bluechan)[col];
		    break;
		case DCELL_TYPE:
		    d_bluechan = ((DCELL *) inrast_bluechan)[col];
		    break;
		}
	    }
	    if (chan5chan) {
		switch (data_type_chan5chan) {
		case CELL_TYPE:
		    d_chan5chan = (double)((CELL *) inrast_chan5chan)[col];
		    break;
		case FCELL_TYPE:
		    d_chan5chan = (double)((FCELL *) inrast_chan5chan)[col];
		    break;
		case DCELL_TYPE:
		    d_chan5chan = ((DCELL *) inrast_chan5chan)[col];
		    break;
		}
	    }
	    if (chan7chan) {
		switch (data_type_chan7chan) {
		case CELL_TYPE:
		    d_chan7chan = (double)((CELL *) inrast_chan7chan)[col];
		    break;
		case FCELL_TYPE:
		    d_chan7chan = (double)((FCELL *) inrast_chan7chan)[col];
		    break;
		case DCELL_TYPE:
		    d_chan7chan = ((DCELL *) inrast_chan7chan)[col];
		    break;
		}
	    }
	    if (G_is_d_null_value(&d_redchan) ||
		 G_is_d_null_value(&d_nirchan) || 
                 ((greenchan) && G_is_d_null_value(&d_greenchan)) ||
		 ((bluechan) && G_is_d_null_value(&d_bluechan)) ||
		 ((chan5chan) && G_is_d_null_value(&d_chan5chan)) ||
		 ((chan7chan) && G_is_d_null_value(&d_chan7chan))) {
		G_set_d_null_value(&outrast[col], 1);
	    }
	    else {
		

		    /************************************/ 
		    /*calculate simple_ratio        */ 
		    if (!strcmp(viflag, "sr")) {
		    d = s_r(d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate ndvi                    */ 
		    if (!strcmp(viflag, "ndvi")) {
		    if (d_redchan + d_nirchan < 0.001) {
			G_set_d_null_value(&outrast[col], 1);
		    }
		    else {
			d = nd_vi(d_redchan, d_nirchan);
			((DCELL *) outrast)[col] = d;
		    }
		}
		
		    /*calculate ipvi                    */ 
		    if (!strcmp(viflag, "ipvi")) {
		    d = ip_vi(d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate dvi             */ 
		    if (!strcmp(viflag, "dvi")) {
		    d = d_vi(d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate evi             */ 
		    if (!strcmp(viflag, "evi")) {
		    d = e_vi(d_bluechan, d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate pvi             */ 
		    if (!strcmp(viflag, "pvi")) {
		    d = p_vi(d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate wdvi                    */ 
		    if (!strcmp(viflag, "wdvi")) {
		    d = wd_vi(d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate savi                    */ 
		    if (!strcmp(viflag, "savi")) {
		    d = sa_vi(d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate msavi                   */ 
		    if (!strcmp(viflag, "msavi")) {
		    d = msa_vi(d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate msavi2            */ 
		    if (!strcmp(viflag, "msavi2")) {
		    d = msa_vi2(d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate gemi                    */ 
		    if (!strcmp(viflag, "gemi")) {
		    d = ge_mi(d_redchan, d_nirchan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate arvi                    */ 
		    if (!strcmp(viflag, "arvi")) {
		    d = ar_vi(d_redchan, d_nirchan, d_bluechan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate gvi            */ 
		    if (!strcmp(viflag, "gvi")) {
		    d = g_vi(d_bluechan, d_greenchan, d_redchan, d_nirchan,
			      d_chan5chan, d_chan7chan);
		    ((DCELL *) outrast)[col] = d;
		}
		
		    /*calculate gari                    */ 
		    if (!strcmp(viflag, "gari")) {
		    d = ga_ri(d_redchan, d_nirchan, d_bluechan, d_greenchan);
		    ((DCELL *) outrast)[col] = d;
		}
	    }
	    }
	if (G_put_raster_row(outfd, outrast, data_type_output) < 0){
	    G_fatal_error(_("%s - Cannot write to output raster file"), 
			G_program_name());
        }
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


