
/****************************************************************************
 *
 * MODULE:       i.modis.qc
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Converts Quality Control indicators into human readable classes
 * 		 for Modis surface reflectance products 250m/500m
 * 		 (MOD09Q/MOD09A), Modis LST (MOD11A1, MOD11A2), Modis Vegetation
 *		 (MOD13A2)
 *
 * COPYRIGHT:    (C) 2008 -2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 * CHANGELOG:	 Added support MOD11A1 (Markus, December 2010)
 *		 Added support MOD13A2 (Yann, January 2011)
 *
 *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
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
    /* MOD09A1 Products (500m, 8-Days, State QA) */
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

    /* MOD11A1 Products (1Km, daily) */ 
CELL mod11A1a(CELL pixel);
CELL mod11A1b(CELL pixel);
CELL mod11A1c(CELL pixel);
CELL mod11A1d(CELL pixel);

    /* MOD11A2 Products (1Km, 8-Days) */ 
CELL mod11A2a(CELL pixel);
CELL mod11A2b(CELL pixel);
CELL mod11A2c(CELL pixel);
CELL mod11A2d(CELL pixel);

    /* MOD13A2 Products (1Km, 16-Days) */ 
CELL mod13A2a (CELL pixel);
CELL mod13A2b (CELL pixel);
CELL mod13A2c (CELL pixel);
CELL mod13A2d (CELL pixel);
CELL mod13A2e (CELL pixel);
CELL mod13A2f (CELL pixel);
CELL mod13A2g (CELL pixel);
CELL mod13A2h (CELL pixel);
CELL mod13A2i (CELL pixel);


int main(int argc, char *argv[]) 
{
    struct Cell_head cellhd;	/*region+header info */
    int nrows, ncols;
    int row, col;
    char *qcflag;		/*Switch for particular index */
    struct GModule *module;
    struct Option *input, *input1, *input2, *input_band, *output;
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
    CELL val1, val2;
    
    /************************************/ 
    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("QC"));
    G_add_keyword(_("Quality Control"));
    G_add_keyword(_("surface reflectance"));
    G_add_keyword(_("Modis"));
    module->description =
	_("Extract quality control parameters from Modis QC layers");

    /* Define the different options */ 
    input = G_define_option();
    input->key = "productname";
    input->type = TYPE_STRING;
    input->required = YES;
    input->description = _("Name of MODIS product type");
    input->descriptions =_("mod09Q1;surf. refl. 250m 8-days;"
                            "mod09A1;surf. refl. 500m 8-days;"
                            "mod09A1s;surf. refl. 500m 8-days, State QA;"
                            "mod11A1;LST 1Km daily (Day/Night);"
                            "mod11A2;LST 1Km 8-days (Day/Night);"
                            "mod13A2;VI 1Km 16-days;");
    input->answer = "mod09Q1";

    input1 = G_define_option();
    input1->key = "qcname";
    input1->type = TYPE_STRING;
    input1->required = YES;
    input1->description = _("Name of QC type to extract");
    input1->descriptions =_("adjcorr;mod09: Adjacency Correction;"
                            "atcorr;mod09: Atmospheric Correction;"
                            "cloud;mod09: Cloud State;"
                            "data_quality;mod09: Band-Wise Data Quality Flag;"
                            "diff_orbit_from_500m;mod09: 250m Band is at Different Orbit than 500m;"
                            "modland_qa_bits;mod09: MODIS Land General Quality Assessment;"
                            "mandatory_qa_11A1;mod11A1: MODIS Land General Quality Assessment;"
                            "data_quality_flag_11A1;mod11A1: Detailed Quality Indications;"
                            "emis_error_11A1;mod11A1: Average Emissivity Error Classes;"
                            "lst_error_11A1;mod11A1: Average LST Error Classes;"
                            "data_quality_flag_11A2;mod11A2: Detailed Quality Indications;"
                            "emis_error_11A2;mod11A2: Average Emissivity Error Classes;"
                            "mandatory_qa_11A2;mod11A2: MODIS Land General Quality Assessment;"
                            "lst_error_11A2;mod11A2: Average LST Error Classes;"
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
			    "modland_qa_bits;mod13A2: MODIS Land General Quality Assessment"
			    "vi_usefulness;mod13A2: Quality estimation of the pixel"
			    "aerosol_quantity:mod13A2: Quantity range of Aerosol"
			    "pixel_adjacent_to_cloud:mod13A2: if pixel is a cloud neighbour"
			    "brdf_correction_performed:mod13A2: if BRDF correction performed"
			    "mixed_clouds:mod13A2: if pixel mixed with clouds"
			    "land_water: separate land from various water objects"
			    "possible_snow_ice:mod13A2: if snow/ice present in pixel"
			    "possible_shadow:mod13A2: if shadow is present in pixel");
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
    /*mod09Q1*/
    if ((!strcmp(qcflag, "cloud") && (strcmp(product, "mod09Q1"))) || 
	(!strcmp(qcflag, "diff_orbit_from_500m") && (strcmp(product, "mod09Q1"))))
	G_fatal_error(_("This flag is only available for MOD09Q1 @ 250m products"));

    if (!strcmp(qcflag, "data_quality")) {
	if (bandno < 1 || bandno > 7)
	    G_fatal_error(_("Band number out of allowed range [1-7]"));
	if (!strcmp(product, "mod09Q1") && bandno > 2)
	    G_fatal_error(_("mod09Q1 product only has 2 bands"));
    }
    
    /*mod11A1*/
    if ((!strcmp(qcflag, "mandatory_qa") && (strcmp(product, "mod11A1"))) ||
        (!strcmp(qcflag, "data_quality_flag") && (strcmp(product, "mod11A1"))) ||
        (!strcmp(qcflag, "emis_error") && (strcmp(product, "mod11A1"))) ||
        (!strcmp(qcflag, "lst_error") && (strcmp(product, "mod11A1"))))
        G_fatal_error(_("This flag is only available for MOD11A1 @ 1Km products"));
    /*mod11A2*/
    if ((!strcmp(qcflag, "mandatory_qa") && (strcmp(product, "mod11A2"))) || 
	(!strcmp(qcflag, "data_quality_flag") && (strcmp(product, "mod11A2"))) ||
	(!strcmp(qcflag, "emis_error") && (strcmp(product, "mod11A2"))) ||
	(!strcmp(qcflag, "lst_error") && (strcmp(product, "mod11A2"))))
	G_fatal_error(_("This flag is only available for MOD11A2 @ 1Km products"));

    /*mod09A1*/
    if (
	(!strcmp(qcflag, "cirrus_detected") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "cloud_shadow") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "cloud_state") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "internal_cloud_algorithm") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "internal_fire_algorithm") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "internal_snow_mask") && (strcmp(product, "mod09A1s"))) ||
	(!strcmp(qcflag, "mod35_snow_ice") && (strcmp(product, "mod09A1s"))))
	G_fatal_error(_("This flag is only available for MOD09A1s @ 500m products"));

    /*mod13A2*/
    if ((!strcmp(qcflag, "vi_usefulness") && (strcmp(product, "mod13A2"))) ||
        (!strcmp(qcflag, "mixed_clouds") && (strcmp(product, "mod13A2"))) ||
        (!strcmp(qcflag, "possible_snow_ice") && (strcmp(product, "mod13A2"))) ||
        (!strcmp(qcflag, "possible_shadow") && (strcmp(product, "mod13A2"))))
        G_fatal_error(_("This flag is only available for MOD13A2 @ 1Km products"));


    infd = Rast_open_old(qcchan, "");

    Rast_get_cellhd(qcchan, "", &cellhd);

    inrast = Rast_allocate_c_buf();

    G_debug(3, "number of rows %d", cellhd.rows);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast = Rast_allocate_c_buf();

    /* Create New raster files */ 
    outfd = Rast_open_new(result, data_type_output);

    /* Process pixels */ 
    for (row = 0; row < nrows; row++)
    {
	CELL c;
	G_percent(row, nrows, 2);
	Rast_get_c_row(infd, inrast, row);

	/*process the data */ 
	for (col = 0; col < ncols; col++)
	{
	    c = inrast[col];
	    if (Rast_is_c_null_value(&c))
		Rast_set_c_null_value(&outrast[col], 1);
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
            else if (!strcmp(product, "mod11A1"))
            {
                if (!strcmp(qcflag, "mandatory_qa"))
                /*calculate mod11A1 mandatory qa flags  */
                    c = mod11A1a(c);
                if (!strcmp(qcflag, "data_quality_flag"))
                /*calculate mod11A1 data quality flag  */
                    c = mod11A1b(c);
                if (!strcmp(qcflag, "emis_error"))
                /*calculate mod11A1 emissivity error flag  */
                    c = mod11A1c(c);
                if (!strcmp(qcflag, "lst_error"))
                /*calculate mod11A1 lst error flag  */
                    c = mod11A1d(c);
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
            else if (!strcmp(product, "mod13A2"))
            {
                if (!strcmp(qcflag, "modland_qa")) 
		/*calculate mod11A2 MODIS Land Quality flags  */ 
                    c = mod13A2a(c);
	        if (!strcmp(qcflag, "vi_usefulness"))
		/*calculate mod13A2 estimate of vi usefulness flag  */ 
                    c = mod13A2b(c);
                if (!strcmp(qcflag, "aerosol_quantity")) 
		/*calculate mod13A2 aerosol quantity range flag  */ 
                    c = mod13A2c(c);
	        if (!strcmp(qcflag, "pixel_adjacent_to_cloud")) 
		/*calculate mod13A2 adjacent cloud detected flag  */ 
                    c = mod13A2d(c);
	        if (!strcmp(qcflag, "brdf_correction_performed")) 
		/*calculate mod13A2 BRDF correction performed flag  */ 
                    c = mod13A2e(c);
	        if (!strcmp(qcflag, "mixed_clouds")) 
		/*calculate mod13A2 pixel has clouds flag  */ 
                    c = mod13A2f(c);
	        if (!strcmp(qcflag, "land_water")) 
		/*calculate mod13A2 land and water types screening flag  */ 
                    c = mod13A2g(c);
	        if (!strcmp(qcflag, "possible_snow_ice")) 
		/*calculate mod13A2 possible presence of snow or ice flag  */ 
                    c = mod13A2h(c);
	        if (!strcmp(qcflag, "possible_shadow")) 
		/*calculate mod13A2 possible presence of shadow flag  */ 
                    c = mod13A2i(c);
            }
	    else
                G_fatal_error(_("Unknown flag name, please check spelling"));

	    outrast[col] = c;
	}

	Rast_put_c_row(outfd, outrast);
    }

    G_free(inrast);
    Rast_close(infd);
    G_free(outrast);
    Rast_close(outfd);

    /* Color from 0 to 10 in grey */ 
    Rast_init_colors(&colors);
    val1 = 0;
    val2 = 10;
    Rast_add_c_color_rule(&val1, 0, 0, 0, &val2, 255, 255, 255, &colors);
    Rast_short_history(result, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(result, &history);
    exit(EXIT_SUCCESS);
}


