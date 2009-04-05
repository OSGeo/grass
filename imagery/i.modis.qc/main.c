
/****************************************************************************
 *
 * MODULE:       i.modis.qc
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

    /* MOD09Q1 Products (250m, 8-Days) */ 
CELL mod09Q1a(CELL pixel);
CELL mod09Q1b(CELL pixel);
CELL mod09Q1c(CELL pixel, int bandno);
CELL mod09Q1d(CELL pixel);
CELL mod09Q1e(CELL pixel);
CELL mod09Q1f(CELL pixel);

    /* MOD09A1 Products (500m, 8-Days) */ 
CELL mod09A1a(CELL pixel);
CELL mod09A1c(CELL pixel, int bandno);
CELL mod09A1d(CELL pixel);
CELL mod09A1e(CELL pixel);
    /* MOD09A1 Products (500m, 8Days, State QA) */
CELL mod09A1sa(CELL pixel);
CELL mod09A1sb(CELL pixel);
CELL mod09A1sc(CELL pixel);
CELL mod09A1sd(CELL pixel);
CELL mod09A1se(CELL pixel);
CELL mod09A1sf(CELL pixel);
CELL mod09A1sg(CELL pixel);
CELL mod09A1sh(CELL pixel);
CELL mod09A1si(CELL pixel);
CELL mod09A1sj(CELL pixel);
CELL mod09A1sk(CELL pixel);

    /* MOD11A2 Products (1Km, 8-Days) */ 
CELL mod11A2a(CELL pixel);
CELL mod11A2b(CELL pixel);
CELL mod11A2c(CELL pixel);
CELL mod11A2d(CELL pixel);

int main(int argc, char *argv[]) 
{
    struct Cell_head cellhd;	/*region+header info */
    int nrows, ncols;
    int row, col;
    char *qcflag;		/*Switch for particular index */
    struct GModule *module;
    struct Option *input, *input1, *input2, *input_band, *output;
    struct Flag *flag1;
    struct History history;	/*metadata */
    struct Colors colors;	/*Color rules */

    char *result;		/*output raster name */

    /*File Descriptors */ 
    int infd;
    int outfd;
    char *product;
    char *qcchan;
    int bandno;
    CELL *inrast;
    CELL *outrast;
    RASTER_MAP_TYPE data_type_output = CELL_TYPE;

    /************************************/ 
    G_gisinit(argv[0]);
    module = G_define_module();
    module->keywords = _("QC, Quality Control, surface reflectance, Modis");
    module->description =
	_("Extract quality control parameters from Modis QC layers");

    /* Define the different options */ 
    input = G_define_option();
    input->key = "productname";
    input->type = TYPE_STRING;
    input->required = YES;
    input->gisprompt = _("Name of MODIS product type");
    input->description = _("Name of MODIS product");
    input->descriptions =_("mod09Q1;surf. refl. 250m 8-days;"
                            "mod09A1;surf. refl. 500m 8-days;"
                            "mod09A1s;surf. refl. 500m 8-days, State QA;"
                            "mod11A2;LST 1Km 8-days (Day/Night);");
    input->answer = "mod09Q1";

    input1 = G_define_option();
    input1->key = "qcname";
    input1->type = TYPE_STRING;
    input1->required = YES;
    input1->gisprompt = _("Name of QC type to extract");
    input1->description = _("Name of QC");
    input1->descriptions =_("adjcorr;mod09: Adjacency Correction;"
                            "atcorr;mod09: Atmospheric Correction;"
                            "cloud;mod09: Cloud State;"
                            "data_quality;mod09: Band-Wise Data Quality Flag;"
                            "diff_orbit_from_500m;mod09: 250m Band is at Different Orbit than 500m;"
                            "modland_qa_bits;mod09: MODIS Land General Quality Assessment;"
                            "data_quality_flag;mod11A2: Detailed Quality Indications;"
                            "emis_error;mod11A2: Average Emissivity Error Classes;"
                            "mandatory_qa;mod11A2: MODIS Land General Quality Assessment;"
                            "lst_error;mod11A2: Average LST Error Classes;"
                            "aerosol_quantity;mod09A1s: StateQA Internal Snow Mask"
                            "brdf_correction_performed;mod09A1s: StateQA Internal Snow Mask"
                            "cirrus_detected;mod09A1s: StateQA Internal Snow Mask"
                            "cloud_shadow;mod09A1s: StateQA Internal Snow Mask"
                            "cloud_state;mod09A1s: StateQA Internal Snow Mask"
                            "internal_cloud_algorithm;mod09A1s: StateQA Internal Snow Mask"
                            "internal_fire_algorithm;mod09A1s: StateQA Internal Snow Mask"
                            "internal_snow_mask;mod09A1s: StateQA Internal Snow Mask"
                            "land_water;mod09A1s: StateQA Internal Snow Mask"
                            "mod35_snow_ice;mod09A1s: StateQA Internal Snow Mask"
                            "pixel_adjacent_to_cloud;mod09A1s: StateQA Internal Snow Mask"
);
    input1->answer = "modland_qa_bits";

    input2 = G_define_standard_option(G_OPT_R_INPUT);
    input2->description =
	_("Name of the surface reflectance QC layer [bit array]");

    input_band = G_define_option();
    input_band->key = "band";
    input_band->type = TYPE_INTEGER;
    input_band->required = NO;
    input_band->gisprompt = "old,value";
    input_band->description =
	_("Band number of Modis product mod09Q1=[1,2],mod09A1=[1-7]");
    input_band->descriptions =_("1;mod09Q1/A1 Band 1: Red;"
                                "2;mod09Q1/A1 Band 2: NIR;"
                                "3;mod09A1 Band 3: Blue;"
                                "4;mod09A1 Band 4: Green;"
                                "5;mod09A1 Band 5: SWIR 1;"
                                "6;mod09A1 Band 6: SWIR 2;"
                                "7;mod09A1 Band 7: SWIR 3;");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->key = "output";
    output->description =
	_("Name of the output QC type classification layer");
    output->answer = "qc";

    /********************/ 
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    product = input->answer;
    qcflag = input1->answer;
    qcchan = input2->answer;
    if (input_band->answer)
	bandno = atoi(input_band->answer);

    result = output->answer;

    if ((!strcmp(qcflag, "cloud") && (strcmp(product, "mod09Q1"))) || 
	(!strcmp(qcflag, "diff_orbit_from_500m") && (strcmp(product, "mod09Q1"))))
	G_fatal_error(_("This flag is only available for MOD09Q1 @ 250m products"));

    if (!strcmp(qcflag, "data_quality")) {
	if (bandno < 1 || bandno > 7)
	    G_fatal_error(_("band number out of allowed range [1-7]"));
	if (!strcmp(product, "mod09Q1") && bandno > 2)
	    G_fatal_error(_("mod09Q1 product only has 2 bands"));
    }
    
    if ((!strcmp(qcflag, "mandatory_qa") && (strcmp(product, "mod11A2"))) || 
	(!strcmp(qcflag, "data_quality_flag") && (strcmp(product, "mod11A2"))) ||
	(!strcmp(qcflag, "emis_error") && (strcmp(product, "mod11A2"))) ||
	(!strcmp(qcflag, "lst_error") && (strcmp(product, "mod11A2"))))
	G_fatal_error(_("This flag is only available for MOD11A2 @ 1Km products"));

    if ((!strcmp(qcflag, "aerosol_quantity") && (strcmp(product, "mod09A1s"))) || 
	(!strcmp(qcflag, "brdf_correction_performed") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "cirrus_detected") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "cloud_shadow") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "cloud_state") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "internal_cloud_algorithm") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "internal_fire_algorithm") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "internal_snow_mask") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "land_water") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "mod35_snow_ice") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "pixel_adjacent_to_cloud") && (strcmp(product, "mod09A1s"))))
	G_fatal_error(_("This flag is only available for MOD09A1s @ 500m products"));

    if ((infd = G_open_cell_old(qcchan, "")) < 0)
	G_fatal_error(_("Cannot open cell file [%s]"), qcchan);

    if (G_get_cellhd(qcchan, "", &cellhd) < 0)
	G_fatal_error(_("Cannot read file header of [%s]"), qcchan);

    inrast = G_allocate_c_raster_buf();

    G_debug(3, "number of rows %d", cellhd.rows);
    nrows = G_window_rows();
    ncols = G_window_cols();
    outrast = G_allocate_c_raster_buf();

    /* Create New raster files */ 
    if ((outfd = G_open_raster_new(result, data_type_output)) < 0)
	G_fatal_error(_("Could not open <%s>"), result);

    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	CELL c, out;
	G_percent(row, nrows, 2);
	if (G_get_c_raster_row(infd, inrast, row) < 0)
	    G_fatal_error(_("Could not read from <%s>"), qcchan);

	/*process the data */ 
	for (col = 0; col < ncols; col++)
	{
	    c = inrast[col];
	    if (G_is_c_null_value(&c))
		G_set_c_null_value(&outrast[col], 1);
            else if (!strcmp(product, "mod09A1"))
            {
	        if (!strcmp(qcflag, "modland_qa_bits")) 
		/*calculate modland QA bits extraction  */ 
		    c = mod09A1a(c);
	        if (!strcmp(qcflag, "data_quality"))
		/*calculate modland QA bits extraction  */ 
		    c = mod09A1c(c, bandno);
	        if (!strcmp(qcflag, "atcorr")) 
		/*calculate atmospheric correction flag  */ 
		    c = mod09A1d(c);
	        if (!strcmp(qcflag, "adjcorr")) 
		/*calculate adjacency correction flag  */ 
		    c = mod09A1e(c);
	    }
            else if (!strcmp(product, "mod09Q1"))
            {
	        if (!strcmp(qcflag, "modland_qa_bits")) 
		/*calculate modland QA bits extraction  */ 
		    c = mod09Q1a(c);
	        if (!strcmp(qcflag, "cloud"))
		/*calculate cloud state  */ 
		/* ONLY 250m product! */ 
		    c = mod09Q1b(c);
	        if (!strcmp(qcflag, "data_quality"))
		/*calculate modland QA bits extraction  */ 
		    c = mod09Q1c(c, bandno);
	        if (!strcmp(qcflag, "atcorr")) 
		/*calculate atmospheric correction flag  */ 
		    c = mod09Q1d(c);
	        if (!strcmp(qcflag, "adjcorr")) 
		/*calculate adjacency correction flag  */ 
		    c = mod09Q1e(c);
	        if (!strcmp(qcflag, "diff_orbit_from_500m"))
                /*calculate different orbit from 500m flag  */ 
                    c = mod09Q1f(c);
            }
            else if (!strcmp(product, "mod11A2"))
            {
                if (!strcmp(qcflag, "mandatory_qa")) 
		/*calculate mod11A2 mandatory qa flags  */ 
                    c = mod11A2a(c);
	        if (!strcmp(qcflag, "data_quality_flag"))
		/*calculate mod11A2 data quality flag  */ 
                    c = mod11A2b(c);
                if (!strcmp(qcflag, "emis_error")) 
		/*calculate mod11A2 emissivity error flag  */ 
                    c = mod11A2c(c);
	        if (!strcmp(qcflag, "lst_error")) 
		/*calculate mod11A2 lst error flag  */ 
                    c = mod11A2d(c);
            }
            else if (!strcmp(product, "mod09A1s"))
            {
	        if (!strcmp(qcflag, "cloud_state")) 
		/*calculate mod09A1s cloud state flag  */ 
                    c = mod09A1sa(c);
	        if (!strcmp(qcflag, "cloud_shadow")) 
		/*calculate mod09A1s cloud shadow flag  */ 
                    c = mod09A1sb(c);
	        if (!strcmp(qcflag, "land_water")) 
		/*calculate mod09A1s land/water flag  */ 
                    c = mod09A1sc(c);
	        if (!strcmp(qcflag, "aerosol_quantity")) 
		/*calculate mod09A1s aerosol quantity flag  */ 
                    c = mod09A1sd(c);
	        if (!strcmp(qcflag, "cirrus_detected")) 
		/*calculate mod09A1s cirrus detected flag  */ 
                    c = mod09A1se(c);
	        if (!strcmp(qcflag, "internal_cloud_algorithm")) 
		/*calculate mod09A1s internal cloud algorithm flag  */ 
                    c = mod09A1sf(c);
	        if (!strcmp(qcflag, "internal_fire_algorithm")) 
		/*calculate mod09A1s internal fire algorithm flag  */ 
                    c = mod09A1sg(c);
	        if (!strcmp(qcflag, "mod35_snow_ice")) 
		/*calculate mod09A1s MOD35 snow/ice flag  */ 
                    c = mod09A1sh(c);
	        if (!strcmp(qcflag, "pixel_adjacent_to_cloud")) 
		/*calculate mod09A1s pixel adjacent to cloud flag  */ 
                    c = mod09A1si(c);
	        if (!strcmp(qcflag, "brdf_correction_performed")) 
		/*calculate mod09A1s BRDF correction performed flag  */ 
                    c = mod09A1sj(c);
	        if (!strcmp(qcflag, "internal_snow_mask")) 
		/*calculate mod09A1s internal snow mask flag  */ 
                    c = mod09A1sk(c);
            }
	    else
                G_fatal_error(_("Unknown flag name, please check spelling"));

	    outrast[col] = c;
	}

	if (G_put_c_raster_row(outfd, outrast) < 0)
	    G_fatal_error(_("Cannot write to output raster file"));
    }

    G_free(inrast);
    G_close_cell(infd);
    G_free(outrast);
    G_close_cell(outfd);

    /* Color from 0 to 10 in grey */ 
    G_init_colors(&colors);
    G_add_color_rule(0, 0, 0, 0, 10, 255, 255, 255, &colors);
    G_short_history(result, "raster", &history);
    G_command_history(&history);
    G_write_history(result, &history);
    exit(EXIT_SUCCESS);
}


