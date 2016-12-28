
/****************************************************************************
 *
 * MODULE:       i.modis.qc
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Converts Quality Control indicators into human readable classes
 * 		 for Modis surface reflectance products 250m/500m
 * 		 (MOD09Q/MOD09A/MOD09GA), Modis LST (MOD11A1, MOD11A2), 
 *               Modis Vegetation (MOD13A2)
 *
 * COPYRIGHT:    (C) 2008 -2016 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 * CHANGELOG:	 Added support MOD11A1 (Markus, December 2010)
 *		 Added support MOD13A2, MCD43B2 (Yann, January 2011)
 *		 Added support MOD09GA (Yann, Vero, December 2016)
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

    /* MOD09GA Products (L2G, 500m, Daily) */ 
CELL mod09GAa(CELL pixel);
CELL mod09GAc(CELL pixel, int bandno);
CELL mod09GAd(CELL pixel);
CELL mod09GAe(CELL pixel);
    /* MOD09GA Products (L2G, 500m, Daily, State QA) */
CELL mod09GAsa(CELL pixel);
CELL mod09GAsb(CELL pixel);
CELL mod09GAsc(CELL pixel);
CELL mod09GAsd(CELL pixel);
CELL mod09GAse(CELL pixel);
CELL mod09GAsf(CELL pixel);
CELL mod09GAsg(CELL pixel);
CELL mod09GAsh(CELL pixel);
CELL mod09GAsi(CELL pixel);
CELL mod09GAsj(CELL pixel);
CELL mod09GAsk(CELL pixel);

    /* MOD09CMG Products (5000m, daily) */
CELL mod09CMGa(CELL pixel);
CELL mod09CMGc(CELL pixel, int bandno);
CELL mod09CMGd(CELL pixel);
CELL mod09CMGe(CELL pixel);
CELL mod09CMGia(CELL pixel);
CELL mod09CMGib(CELL pixel);
CELL mod09CMGic(CELL pixel);
CELL mod09CMGid(CELL pixel);
CELL mod09CMGie(CELL pixel);
CELL mod09CMGif(CELL pixel);
CELL mod09CMGig(CELL pixel);
CELL mod09CMGih(CELL pixel);
CELL mod09CMGii(CELL pixel);
CELL mod09CMGij(CELL pixel);
CELL mod09CMGik(CELL pixel);
CELL mod09CMGil(CELL pixel);
CELL mod09CMGim(CELL pixel);
CELL mod09CMGin(CELL pixel);

    /* MOD11A1 Products (1km, daily) */ 
CELL mod11A1a(CELL pixel);
CELL mod11A1b(CELL pixel);
CELL mod11A1c(CELL pixel);
CELL mod11A1d(CELL pixel);

    /* MOD11A2 Products (1km, 8-Days) */ 
CELL mod11A2a(CELL pixel);
CELL mod11A2b(CELL pixel);
CELL mod11A2c(CELL pixel);
CELL mod11A2d(CELL pixel);

    /* MOD13A2 Products (1km, 16-Days) */ 
CELL mod13A2a (CELL pixel);
CELL mod13A2b (CELL pixel);
CELL mod13A2c (CELL pixel);
CELL mod13A2d (CELL pixel);
CELL mod13A2e (CELL pixel);
CELL mod13A2f (CELL pixel);
CELL mod13A2g (CELL pixel);
CELL mod13A2h (CELL pixel);
CELL mod13A2i (CELL pixel);

    /* MOD13Q1 Products (250m, 16-Days) */
CELL mod13Q1a (CELL pixel);
CELL mod13Q1b (CELL pixel);
CELL mod13Q1c (CELL pixel);
CELL mod13Q1d (CELL pixel);
CELL mod13Q1e (CELL pixel);
CELL mod13Q1f (CELL pixel);
CELL mod13Q1g (CELL pixel);
CELL mod13Q1h (CELL pixel);
CELL mod13Q1i (CELL pixel);

    /* MCD43B2 Products (1km, 8-Days)*/

/* SDS: BRDF_Albedo_Ancilliary */
CELL mcd43B2a(CELL pixel);
CELL mcd43B2b(CELL pixel);
CELL mcd43B2c(CELL pixel);

/* SDS: BRDF_Albedo_Band_Quality */
CELL mcd43B2qa(CELL pixel, int bandno);


int main(int argc, char *argv[]) 
{
    struct Cell_head cellhd;	/*region+header info */
    int nrows, ncols;
    int row, col;
    char *qcflag;		/*Switch for particular index */
    struct GModule *module;
    struct Option *productname, *qcname, *input, *input_band, *output;
    struct History history;	/*metadata */
    struct Colors colors;	/*Color rules */
    char *desc_productname, *desc_qcname, *desc_input_band;

    char *result;		/*output raster name */

    /*File Descriptors */ 
    int infd;
    int outfd;
    char *product;
    char *qcchan;
    int bandno=0;
    CELL *inrast;
    CELL *outrast;
    RASTER_MAP_TYPE data_type_output = CELL_TYPE;
    CELL val1, val2;
    
    /************************************/ 
    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("imagery quality assessment"));
    G_add_keyword(_("reflectance"));
    G_add_keyword(_("land surface temperature"));
    G_add_keyword(_("vegetation"));
    G_add_keyword(_("satellite"));
    G_add_keyword(_("MODIS"));
    module->description =
	_("Extracts quality control parameters from MODIS QC layers.");

    /* Define the different options */ 
    input = G_define_standard_option(G_OPT_R_INPUT);
    input->description =
	_("Name of input surface reflectance QC layer [bit array]");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->key = "output";
    output->description =
	_("Name for output QC type classification layer");

    productname = G_define_option();
    productname->key = "productname";
    productname->type = TYPE_STRING;
    productname->required = YES;
    productname->description = _("Name of MODIS product type");
    desc_productname = NULL;
    G_asprintf(&desc_productname,
	       "mod09Q1;%s;"
	       "mod09A1;%s;"
	       "mod09A1s;%s;"
	       "mod09GA;%s;"
	       "mod09GAs;%s;"
	       "mod09CMG;%s;"
	       "mod09CMGs;%s;"
	       "mod09CMGi;%s;"
	       "mod11A1;%s;"
	       "mod11A2;%s;"
	       "mod13A2;%s;"
	       "mcd43B2;%s;"
	       "mcd43B2q;%s;"
	       "mod13Q1;%s;",
	       _("surf. refl. 250m 8-days"),
	       _("surf. refl. 500m 8-days"),
	       _("surf. refl. 500m 8-days, State QA"),
	       _("surf. refl. 500m daily"),
	       _("surf. refl. 500m daily, State QA"),
	       _("surf. refl. 5000m daily"),
	       _("surf. refl. 5000m daily, State QA"),
	       _("surf. refl. 5000m daily, Internal Climatology"),
	       _("LST 1km daily (Day/Night)"),
	       _("LST 1km 8-days (Day/Night)"),
	       _("VI 1km 16-days"),
	       _("Brdf-Albedo Quality (Ancillary SDS) 1km 8-days"),
	       _("Brdf-Albedo Quality (BRDF SDS) 1km 8-days"),
	       _("VI 250m 16-days"));
    productname->descriptions = desc_productname;
    productname->options = "mod09Q1,mod09A1,mod09A1s,mod09GA,mod09GAs,mod09CMG,mod09CMGs,mod09CMGi,mod11A1,mod11A2,mod13A2,mcd43B2,mcd43B2q,mod13Q1";
    
    qcname = G_define_option();
    qcname->key = "qcname";
    qcname->type = TYPE_STRING;
    qcname->required = YES;
    qcname->description = _("Name of QC type to extract");
    desc_qcname = NULL;
    G_asprintf(&desc_qcname,
               "adjcorr;%s;"
	       "atcorr;%s;"
	       "cloud;%s;"
	       "data_quality;%s;"
	       "diff_orbit_from_500m;%s;"
	       "modland_qa;%s;"
	       "mandatory_qa_11A1;%s;"
	       "data_quality_flag_11A1;%s;"
	       "emis_error_11A1;%s;"
	       "lst_error_11A1;%s;"
	       "data_quality_flag_11A2;%s;"
	       "emis_error_11A2;%s;"
	       "mandatory_qa_11A2;%s;"
	       "lst_error_11A2;%s;"
	       "aerosol_quantity;%s;"
	       "brdf_correction_performed;%s;"
	       "cirrus_detected;%s;"
	       "cloud_shadow;%s;"
	       "cloud_state;%s;"
	       "internal_cloud_algorithm;%s;"
	       "internal_fire_algorithm;%s;"
	       "internal_snow_mask;%s;"
	       "land_water;%s;"
	       "mod35_snow_ice;%s;"
	       "pixel_adjacent_to_cloud;%s;"
	       "salt_pan;%s;"
	       "icm_cloudy;%s;"
	       "icm_clear;%s;"
	       "icm_high_clouds;%s;"
	       "icm_low_clouds;%s;"
	       "icm_snow;%s;"
	       "icm_fire;%s;"
	       "icm_sun_glint;%s;"
	       "icm_dust;%s;"
	       "icm_cloud_shadow;%s;"
	       "icm_pixel_is_adjacent_to_cloud;%s;"
	       "icm_cirrus;%s;"
	       "icm_pan_flag;%s;"
	       "icm_criteria_for_aerosol_retrieval;%s;"
	       "icm_aot_has_clim_val;%s;"
	       "modland_qa;%s;"
	       "vi_usefulness;%s;"
	       "aerosol_quantity;%s;"
	       "pixel_adjacent_to_cloud;%s;"
	       "brdf_correction_performed;%s;"
	       "mixed_clouds;%s;"
	       "land_water;%s;"
	       "possible_snow_ice;%s;"
	       "possible_shadow;%s;"
	       "platform;%s;"
	       "land_water;%s;"
	       "sun_z_angle_at_local_noon;%s;"
	       "brdf_correction_performed;%s;"
	       "modland_qa;%s;"
	       "vi_usefulness;%s;"
	       "aerosol_quantity;%s;"
	       "pixel_adjacent_to_cloud;%s;"
	       "brdf_correction_performed;%s;"
	       "mixed_clouds;%s;"
	       "land_water;%s;"
	       "possible_snow_ice;%s;"
	       "possible_shadow;%s;",
	       _("mod09: Adjacency Correction"),
	       _("mod09: Atmospheric Correction"),
	       _("mod09: Cloud State"),
	       _("mod09: Band-Wise Data Quality Flag"),
	       _("mod09: 250m Band is at Different Orbit than 500m"),
	       _("mod09: MODIS Land General Quality Assessment"),
	       _("mod11A1: MODIS Land General Quality Assessment"),
	       _("mod11A1: Detailed Quality Indications"),
	       _("mod11A1: Average Emissivity Error Classes"),
	       _("mod11A1: Average LST Error Classes"),
	       _("mod11A2: Detailed Quality Indications"),
	       _("mod11A2: Average Emissivity Error Classes"),
	       _("mod11A2: MODIS Land General Quality Assessment"),
	       _("mod11A2: Average LST Error Classes"),
	       _("mod09*s: StateQA Aerosol Quantity"),
	       _("mod09*s: StateQA BRDF Correction Performed"),
	       _("mod09*s: StateQA Cirrus Detected"),
	       _("mod09*s: StateQA Cloud Shadow"),
	       _("mod09*s: StateQA Cloud State"),
	       _("mod09*s: StateQA Internal Cloud Algorithm"),
	       _("mod09*s: StateQA Internal Fire Algorithm"),
	       _("mod09*s: StateQA Internal Snow Mask"),
	       _("mod09*s: StateQA Land Water"),
	       _("mod09*s: StateQA mod35 Snow Ice"),
	       _("mod09*s: StateQA Pixel Adjacent to Cloud"),
	       _("mod09*s: StateQA Salt Pan (mod09GAs)"),
	       _("mod09*i: Internal CM: Cloudy"),
	       _("mod09*i: Internal CM: Clear"),
	       _("mod09*i: Internal CM: High Clouds"),
	       _("mod09*i: Internal CM: Low Clouds"),
	       _("mod09*i: Internal CM: Snow"),
	       _("mod09*i: Internal CM: Fire"),
	       _("mod09*i: Internal CM: Sun Glint"),
	       _("mod09*i: Internal CM: Dust"),
	       _("mod09*i: Internal CM: Cloud Shadow"),
	       _("mod09*i: Internal CM: Pixel is Adjacent to Cloud"),
	       _("mod09*i: Internal CM: Cirrus"),
	       _("mod09*i: Internal CM: Pan Flag"),
	       _("mod09*i: Internal CM: Criteria for Aerosol Retrieval"),
	       _("mod09*i: Internal CM: AOT (aerosol optical depth) has clim. val."),
	       _("mod13A2: MODIS Land General Quality Assessment"),
	       _("mod13A2: Quality estimation of the pixel"),
	       _("mod13A2: Quantity range of Aerosol"),
	       _("mod13A2: if pixel is a cloud neighbour"),
	       _("mod13A2: if BRDF correction performed"),
	       _("mod13A2: if pixel mixed with clouds"),
	       _("mod13A2: separate land from various water objects"),
	       _("mod13A2: if snow/ice present in pixel"),
	       _("mod13A2: if shadow is present in pixel"),
	       _("mcd43B2: Quality of BRDF correction performed"),
	       _("mcd43B2: Quality of BRDF correction performed"),
	       _("mcd43B2: Quality of BRDF correction performed"),
	       _("mcd43B2q: Quality of BRDF correction performed"),
	       _("mod13Q1: MODIS Land General Quality Assessment"),
	       _("mod13Q1: Quality estimation of the pixel"),
	       _("mod13Q1: Quantity range of Aerosol"),
	       _("mod13Q1: if pixel is a cloud neighbour"),
	       _("mod13Q1: if BRDF correction performed"),
	       _("mod13Q1: if pixel mixed with clouds"),
	       _("mod13Q1: separate land from various water objects"),
	       _("mod13Q1: if snow/ice present in pixel"),
	       _("mod13Q1: if shadow is present in pixel"));
    qcname->descriptions = desc_qcname;
    qcname->options = "adjcorr,atcorr,cloud,data_quality,diff_orbit_from_500m,modland_qa,mandatory_qa_11A1,data_quality_flag_11A1,emis_error_11A1,lst_error_11A1,data_quality_flag_11A2,emis_error_11A2,mandatory_qa_11A2,lst_error_11A2,aerosol_quantity,brdf_correction_performed,cirrus_detected,cloud_shadow,cloud_state,internal_cloud_algorithm,internal_fire_algorithm,internal_snow_mask,land_water,mod35_snow_ice,pixel_adjacent_to_cloud,salt_pan,icm_cloudy,icm_clear,icm_high_clouds,icm_low_clouds,icm_snow,icm_fire,icm_sun_glint,icm_dust,icm_cloud_shadow,icm_pixel_is_adjacent_to_cloud,icm_cirrus,icm_pan_flag,icm_criteria_for_aerosol_retrieval,icm_aot_has_clim_val,modland_qa,vi_usefulness,aerosol_quantity,pixel_adjacent_to_cloud,brdf_correction_performed,mixed_clouds,land_water,possible_snow_ice,possible_shadow,platform,land_water,sun_z_angle_at_local_noon,brdf_correction_performed,modland_qa,vi_usefulness,aerosol_quantity,pixel_adjacent_to_cloud,brdf_correction_performed,mixed_clouds,land_water,possible_snow_ice,possible_shadow";

    input_band = G_define_option();
    input_band->key = "band";
    input_band->type = TYPE_STRING;
    input_band->required = NO;
    input_band->description =
      _("Band number of MODIS product (mod09Q1=[1,2],mod09A1=[1-7],m[o/y]d09GA=[1-7],m[o/y]d09CMG=[1-7], mcd43B2q=[1-7])");
    desc_input_band = NULL;
    G_asprintf(&desc_input_band,
               "1;%s;2;%s;3;%s;4;%s;5;%s;6;%s;7;%s",
	       _("Band 1: Red"),
	       _("Band 2: NIR"),
	       _("Band 3: Blue"),
	       _("Band 4: Green"),
	       _("Band 5: SWIR 1"),
	       _("Band 6: SWIR 2"),
	       _("Band 7: SWIR 3"));
    input_band->descriptions = desc_input_band;
    input_band->options = "1,2,3,4,5,6,7";
    
    /********************/ 
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    product = productname->answer;
    qcflag = qcname->answer;
    qcchan = input->answer;
    if (input_band->answer)
	bandno = atoi(input_band->answer);

    result = output->answer;

    if (!product)
	G_fatal_error(_("Please specify a product to extract"));

    if (!qcflag)
	G_fatal_error(_("Please specify a valid QC flag to extract"));

    /*mod09Q1*/
    if (strcmp(product, "mod09Q1") /*if not mod09Q1 but its qcflag was entered, issue error*/
    && (!strcmp(qcflag, "cloud") || !strcmp(qcflag, "diff_orbit_from_500m")))
	G_fatal_error(_("This bit flag is only available for MOD09Q1 @ 250m products"));

    if (!strcmp(qcflag, "data_quality")) { /* verify the allowed number of bands for each product */
	if (!strcmp(product, "mod09Q1") && (bandno < 1 || bandno > 2)) 
	    G_fatal_error(_("Band number out of allowed range [1-2]"));
	if (!strcmp(product, "mod09A1") && (bandno < 1 || bandno > 7)) 
	    G_fatal_error(_("Band number out of allowed range [1-7]"));
	if (!strcmp(product, "mod09GA") && (bandno < 1 || bandno > 7)) 
	    G_fatal_error(_("Band number out of allowed range [1-7]"));
	if (!strcmp(product, "mod09CMG") && (bandno < 1 || bandno > 7)) 
	    G_fatal_error(_("Band number out of allowed range [1-7]"));
	if (!strcmp(product, "mod09CMGs") && (bandno < 1 || bandno > 7)) 
	    G_fatal_error(_("Band number out of allowed range [1-7]"));
	if (!strcmp(product, "mod09CMGi") && (bandno < 1 || bandno > 7)) 
	    G_fatal_error(_("Band number out of allowed range [1-7]"));
	if (!strcmp(product, "mod43B2") && (bandno < 1 || bandno > 7))
	    G_fatal_error(_("Band number out of allowed range [1-7]"));
	if (!strcmp(product, "mod43B2q") && (bandno < 1 || bandno > 7)) 
	    G_fatal_error(_("Band number out of allowed range [1-7]"));
	if (!strcmp(product, "mod09Q1") && bandno > 2)
	    G_fatal_error(_("mod09Q1 product only has 2 bands"));
    }
    
    /*mod09A1 stateqa or mod09CMG stateqa*/
    if ((strcmp(qcflag, "BRDF_correction_performed") && ( !(strcmp(product, "mod09A1s")) || !(strcmp(product, "mod09CMGs")) )) )
	G_fatal_error(_("This bit flag is only available for MOD09A1s @ 500m or MOD09CMG @ 5000m products"));

    /*mod09GA stateqa*/
    if ((!strcmp(qcflag, "salt_pan") && strcmp(product, "mod09GAs")) )
	G_fatal_error(_("This bit flag is only available for MOD09GAs @ 500m products"));

    /*mod09CMG*/
    if (strcmp(product, "mod09CMGi")  /*if not mod09CGMi but its qcflag was entered, issue error*/
    && (!strcmp(qcflag, "icm_cloudy") || !strcmp(qcflag, "icm_clear")  ||
	!strcmp(qcflag, "icm_high_clouds") || !strcmp(qcflag, "icm_low_clouds") ||
	!strcmp(qcflag, "icm_snow") || !strcmp(qcflag, "icm_fire") ||
	!strcmp(qcflag, "icm_sun_glint") || !strcmp(qcflag, "icm_dust") ||
	!strcmp(qcflag, "icm_cloud_shadow") || !strcmp(qcflag, "icm_pixel_is_adjacent_to_cloud") ||
	!strcmp(qcflag, "icm_cirrus") || !strcmp(qcflag, "icm_pan_flag") ||
	!strcmp(qcflag, "icm_criteria_for_aerosol_retrieval") || !strcmp(qcflag, "icm_aot_has_clim_val")))
	G_fatal_error(_("This bit flag is only available for mod09CMGi @ 5000m products"));
   
    /*mod13A2*/
    if (strcmp(product, "mod13A2") /*if not mod13A2 but its qcflag was entered, issue error*/
    && (!strcmp(qcflag, "vi_usefulness") || !strcmp(qcflag, "mixed_clouds")  ||
        !strcmp(qcflag, "possible_snow_ice") || !strcmp(qcflag, "possible_shadow")))
        G_fatal_error(_("This bit flag is only available for MOD13A2 @ 1km products"));

    /*mcd43B2*/
    if (strcmp(product, "mcd43B2") /*if not mcd43B2 but its qcflag was entered, issue error*/
    && (!strcmp(qcflag, "platform")||!strcmp(qcflag, "land_water") ||!strcmp(qcflag, "sun_z_angle_at_local_noon")))
        G_fatal_error(_("This bit flag is only available for MCD43B2 @ 1km products"));

    /*mcd43B2q*/
    if (!strcmp(product, "mcd43B2q") && (bandno < 1 || bandno > 7))
	    G_fatal_error(_("Band number out of allowed range [1-7]"));

    /*mod13Q1*/
    if (strcmp(product, "mod13Q1") /*if not mod13Q1 but its qcflag was entered, issue error*/
    && (!strcmp(qcflag, "vi_usefulness") || !strcmp(qcflag, "mixed_clouds") ||
        !strcmp(qcflag, "possible_snow_ice") || !strcmp(qcflag, "possible_shadow")))
        G_fatal_error(_("This bit flag is only available for MOD13Q1 @ 250m products"));

    infd = Rast_open_old(qcchan, "");

    Rast_get_cellhd(qcchan, "", &cellhd);

    inrast = Rast_allocate_c_buf();

    G_debug(3, "number of rows %d", cellhd.rows);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    outrast = Rast_allocate_c_buf();

    /* Create New raster files */ 
    outfd = Rast_open_new(result, data_type_output);

    G_debug(3, "Product is %s and QC flag is %s", product, qcflag);

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
	        if (!strcmp(qcflag, "modland_qa")) 
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
            else if (!strcmp(product, "mod09GA"))
            {
	        if (!strcmp(qcflag, "modland_qa")) 
		/*calculate modland QA bits extraction  */ 
		    c = mod09GAa(c);
	        if (!strcmp(qcflag, "data_quality"))
		/*calculate modland QA bits extraction  */ 
		    c = mod09GAc(c, bandno);
	        if (!strcmp(qcflag, "atcorr")) 
		/*calculate atmospheric correction flag  */ 
		    c = mod09GAd(c);
	        if (!strcmp(qcflag, "adjcorr")) 
		/*calculate adjacency correction flag  */ 
		    c = mod09GAe(c);
	    }
            else if (!strcmp(product, "mod09Q1"))
            {
	        if (!strcmp(qcflag, "modland_qa")) 
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
            else if (!strcmp(product, "mod09CMG"))
            {
	        if (!strcmp(qcflag, "modland_qa")) 
		/*calculate modland QA bits extraction  */ 
		    c = mod09CMGa(c);
	        if (!strcmp(qcflag, "data_quality"))
		/*calculate modland QA bits extraction  */ 
		    c = mod09CMGc(c, bandno);
	        if (!strcmp(qcflag, "atcorr")) 
		/*calculate atmospheric correction flag  */ 
		    c = mod09CMGd(c);
	        if (!strcmp(qcflag, "adjcorr")) 
		/*calculate adjacency correction flag  */ 
		    c = mod09CMGe(c);
	    }
            else if (!strcmp(product, "mod11A1"))
            {
                if (!strcmp(qcflag, "mandatory_qa_11A1"))
                /*calculate mod11A1 mandatory qa flags  */
                    c = mod11A1a(c);
                if (!strcmp(qcflag, "data_quality_flag_11A1"))
                /*calculate mod11A1 data quality flag  */
                    c = mod11A1b(c);
                if (!strcmp(qcflag, "emis_error_11A1"))
                /*calculate mod11A1 emissivity error flag  */
                    c = mod11A1c(c);
                if (!strcmp(qcflag, "lst_error_11A1"))
                /*calculate mod11A1 lst error flag  */
                    c = mod11A1d(c);
            }
            else if (!strcmp(product, "mod11A2"))
            {
                if (!strcmp(qcflag, "mandatory_qa_11A2")) 
		/*calculate mod11A2 mandatory qa flags  */ 
                    c = mod11A2a(c);
	        if (!strcmp(qcflag, "data_quality_flag_11A2"))
		/*calculate mod11A2 data quality flag  */ 
                    c = mod11A2b(c);
                if (!strcmp(qcflag, "emis_error_11A2")) 
		/*calculate mod11A2 emissivity error flag  */ 
                    c = mod11A2c(c);
	        if (!strcmp(qcflag, "lst_error_11A2")) 
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
            else if (!strcmp(product, "mod09GAs"))
            {
	        if (!strcmp(qcflag, "cloud_state")) 
		/*calculate mod09GAs cloud state flag  */ 
                    c = mod09GAsa(c);
	        if (!strcmp(qcflag, "cloud_shadow")) 
		/*calculate mod09GAs cloud shadow flag  */ 
                    c = mod09GAsb(c);
	        if (!strcmp(qcflag, "land_water")) 
		/*calculate mod09GAs land/water flag  */ 
                    c = mod09GAsc(c);
	        if (!strcmp(qcflag, "aerosol_quantity")) 
		/*calculate mod09GAs aerosol quantity flag  */ 
                    c = mod09GAsd(c);
	        if (!strcmp(qcflag, "cirrus_detected")) 
		/*calculate mod09GAs cirrus detected flag  */ 
                    c = mod09GAse(c);
	        if (!strcmp(qcflag, "internal_cloud_algorithm")) 
		/*calculate mod09GAs internal cloud algorithm flag  */ 
                    c = mod09GAsf(c);
	        if (!strcmp(qcflag, "internal_fire_algorithm")) 
		/*calculate mod09GAs internal fire algorithm flag  */ 
                    c = mod09GAsg(c);
	        if (!strcmp(qcflag, "mod35_snow_ice")) 
		/*calculate mod09GAs MOD35 snow/ice flag  */ 
                    c = mod09GAsh(c);
	        if (!strcmp(qcflag, "pixel_adjacent_to_cloud")) 
		/*calculate mod09GAs pixel adjacent to cloud flag  */ 
                    c = mod09GAsi(c);
	        if (!strcmp(qcflag, "salt_pan")) 
		/*calculate mod09GAs salt pan flag  */ 
                    c = mod09GAsj(c);
	        if (!strcmp(qcflag, "internal_snow_mask")) 
		/*calculate mod09GAs internal snow mask flag  */ 
                    c = mod09GAsk(c);
            }
            else if (!strcmp(product, "mod09CMGs"))
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
            else if (!strcmp(product, "mod09CMGi"))
            {
	        /* Internal Climatology for MOD09CMG or MYD09CMG  */
	        if (!strcmp(qcflag, "icm_cloudy"))
		/*calculate mod09A1i cloudy  */ 
                    c = mod09CMGia(c);
	        if (!strcmp(qcflag, "icm_clear")) 
		/*calculate mod09A1i clear flag  */ 
                    c = mod09CMGib(c);
	        if (!strcmp(qcflag, "icm_high_clouds")) 
		/*calculate mod09A1i high clouds flag  */ 
                    c = mod09CMGic(c);
	        if (!strcmp(qcflag, "icm_low_clouds")) 
		/*calculate mod09A1s low clouds flag  */ 
                    c = mod09CMGid(c);
	        if (!strcmp(qcflag, "icm_snow")) 
		/*calculate mod09A1i snow flag  */ 
                    c = mod09CMGie(c);
	        if (!strcmp(qcflag, "icm_fire")) 
		/*calculate mod09A1i fire flag  */ 
                    c = mod09CMGif(c);
	        if (!strcmp(qcflag, "icm_sun_glint")) 
		/*calculate mod09A1i sun glint flag  */ 
                    c = mod09CMGig(c);
	        if (!strcmp(qcflag, "icm_dust")) 
		/*calculate mod09A1i dust flag  */ 
                    c = mod09CMGih(c);
	        if (!strcmp(qcflag, "icm_cloud_shadow")) 
		/*calculate mod09A1i cloud shadow flag  */ 
                    c = mod09CMGii(c);
	        if (!strcmp(qcflag, "icm_pixel_is_adjacent_to_cloud")) 
		/*calculate mod09A1i pixel is adjacent to cloud flag  */ 
                    c = mod09CMGij(c);
	        if (!strcmp(qcflag, "icm_cirrus")) 
		/*calculate mod09A1i cirrus flag  */ 
                    c = mod09CMGik(c);
	        if (!strcmp(qcflag, "icm_pan_flag")) 
		/*calculate mod09A1i pan flag  */ 
                    c = mod09CMGil(c);
	        if (!strcmp(qcflag, "icm_criteria_for_aerosol_retrieval")) 
		/*calculate mod09A1i criteria for aerosol retrieval flag  */ 
                    c = mod09CMGim(c);
	        if (!strcmp(qcflag, "icm_aot_has_clim_val")) 
		/*calculate mod09A1i aot has clim val flag  */ 
                    c = mod09CMGin(c);
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
            else if (!strcmp(product, "mcd43B2"))
            {
	        if (!strcmp(qcflag, "platform")) 
		/*calculate mcd43B2: SDS: BRDF_Albedo_Ancillary */
		/* Satellite platform identification flag  */ 
                    c = mcd43B2a(c);
	        if (!strcmp(qcflag, "land_water")) 
		/*calculate mcd43B2: SDS: BRDF_Albedo_Ancillary */
		/* Land Water types in pixel flag  */ 
                    c = mcd43B2b(c);
	        if (!strcmp(qcflag, "sun_z_angle_at_local_noon")) 
		/*calculate mcd43B2: SDS: BRDF_Albedo_Ancillary */
		/* Sun Zenith Angle at Local Solar Noon flag  */ 
                    c = mcd43B2c(c);
            }
            else if (!strcmp(product, "mcd43B2q"))
            {
	        if (!strcmp(qcflag, "brdf_correction_performed")) 
		/*calculate mcd43B2: SDS: BRDF_Albedo_Band_Quality */
		/* BRDF correction performed Quality flag  */ 
                    c = mcd43B2qa(c, bandno);
            }
            else if (!strcmp(product, "mod13Q1"))
            {
                if (!strcmp(qcflag, "modland_qa")) 
		/*calculate mod11A2 MODIS Land Quality flags  */ 
                    c = mod13Q1a(c);
	        if (!strcmp(qcflag, "vi_usefulness"))
		/*calculate mod13A2 estimate of vi usefulness flag  */ 
                    c = mod13Q1b(c);
                if (!strcmp(qcflag, "aerosol_quantity")) 
		/*calculate mod13A2 aerosol quantity range flag  */ 
                    c = mod13Q1c(c);
	        if (!strcmp(qcflag, "pixel_adjacent_to_cloud")) 
		/*calculate mod13A2 adjacent cloud detected flag  */ 
                    c = mod13Q1d(c);
	        if (!strcmp(qcflag, "brdf_correction_performed")) 
		/*calculate mod13A2 BRDF correction performed flag  */ 
                    c = mod13Q1e(c);
	        if (!strcmp(qcflag, "mixed_clouds")) 
		/*calculate mod13A2 pixel has clouds flag  */ 
                    c = mod13Q1f(c);
	        if (!strcmp(qcflag, "land_water")) 
		/*calculate mod13A2 land and water types screening flag  */ 
                    c = mod13Q1g(c);
	        if (!strcmp(qcflag, "possible_snow_ice")) 
		/*calculate mod13A2 possible presence of snow or ice flag  */ 
                    c = mod13Q1h(c);
	        if (!strcmp(qcflag, "possible_shadow")) 
		/*calculate mod13A2 possible presence of shadow flag  */ 
                    c = mod13Q1i(c);
            }
	    else
                G_fatal_error(_("Unknown names and/or combination, please check spelling"));

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


