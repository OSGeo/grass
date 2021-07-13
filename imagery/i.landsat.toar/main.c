
/****************************************************************************
 *
 * MODULE:       i.landsat.toar
 *
 * AUTHOR(S):    E. Jorge Tizado - ej.tizado at unileon.es
 *               Hamish Bowman (small grassification cleanups)
 *               Yann Chemin (v7 + L5TM _MTL.txt support) [removed after update]
 *               Adopted for GRASS 7 by Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      Calculate TOA Radiance or Reflectance and Kinetic Temperature
 *               for Landsat 1/2/3/4/5 MS, 4/5 TM, 7 ETM+, and 8 OLI/TIRS
 *
 * COPYRIGHT:    (C) 2006-2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

#define QCALMAX   65536		/* L1-7=256 but L8=65536 */

int main(int argc, char *argv[])
{
    struct History history;
    struct GModule *module;
    char *desc;

    struct Cell_head cellhd, orig_cellhd;

    void *inrast, *outrast;
    int infd, outfd;
    void *ptr;
    int nrows, ncols, row, col;

    RASTER_MAP_TYPE in_data_type;

    struct Option *input_prefix, *output_prefix, *metfn, *sensor, *adate,
	*pdate, *elev, *bgain, *metho, *perc, *dark, *atmo, *lsatmet, *oscale;
    char *inputname, *met, *outputname, *sensorname;
    struct Flag *frad, *print_meta, *named;

    lsat_data lsat;
    char band_in[GNAME_MAX], band_out[GNAME_MAX];
    int i, j, q, method, pixel, dn_dark[MAX_BANDS], dn_mode[MAX_BANDS], dn_sat;
    int overwrite;
    double qcal, rad, ref, percent, ref_mode, rayleigh, scale;
    unsigned long hist[QCALMAX], h_max;

    struct Colors colors;
    struct FPRange range;
    double min, max;

    /* initialize GIS environment */
    G_gisinit(argv[0]);

    /* initialize module */
    module = G_define_module();
    module->description =
	_("Calculates top-of-atmosphere radiance or reflectance and temperature for Landsat MSS/TM/ETM+/OLI");
    G_add_keyword(_("imagery"));
    G_add_keyword(_("radiometric conversion"));
    G_add_keyword(_("radiance"));
    G_add_keyword(_("reflectance"));
    G_add_keyword(_("brightness temperature"));
    G_add_keyword(_("atmospheric correction"));
    G_add_keyword(_("satellite"));
    G_add_keyword(_("Landsat"));
    module->overwrite = TRUE;

    /* It defines the different parameters */
    input_prefix = G_define_standard_option(G_OPT_R_BASENAME_INPUT);
    input_prefix->label = _("Base name of input raster bands");
    input_prefix->description = _("Example: 'B.' for B.1, B.2, ...");

    output_prefix = G_define_standard_option(G_OPT_R_BASENAME_OUTPUT);
    output_prefix->label = _("Prefix for output raster maps");
    output_prefix->description =
	_("Example: 'B.toar.' generates B.toar.1, B.toar.2, ...");

    metfn = G_define_standard_option(G_OPT_F_INPUT);
    metfn->key = "metfile";
    metfn->required = NO;
    metfn->description = _("Name of Landsat metadata file (.met or MTL.txt)");
    metfn->guisection = _("Metadata");

    sensor = G_define_option();
    sensor->key = "sensor";
    sensor->type = TYPE_STRING;
    sensor->label = _("Spacecraft sensor");
    sensor->description = _("Required only if 'metfile' not given (recommended for sanity)");
    sensor->options = "mss1,mss2,mss3,mss4,mss5,tm4,tm5,tm7,oli8";
    desc = NULL;
    G_asprintf(&desc,
	        "mss1;%s;mss2;%s;mss3;%s;mss4;%s;mss5;%s;tm4;%s;tm5;%s;tm7;%s;oli8;%s",
	        _("Landsat-1 MSS"),
	        _("Landsat-2 MSS"),
	        _("Landsat-3 MSS"),
	        _("Landsat-4 MSS"),
	        _("Landsat-5 MSS"),
	        _("Landsat-4 TM"),
	        _("Landsat-5 TM"),
	        _("Landsat-7 ETM+"),
	        _("Landsat_8 OLI/TIRS"));
    sensor->descriptions = desc;
    sensor->required = NO;	/* perhaps YES for clarity */
    sensor->guisection = _("Metadata");

    metho = G_define_option();
    metho->key = "method";
    metho->type = TYPE_STRING;
    metho->required = NO;
    metho->options = "uncorrected,dos1,dos2,dos2b,dos3,dos4";
    metho->label = _("Atmospheric correction method");
    metho->answer = "uncorrected";
    metho->guisection = _("Metadata");

    adate = G_define_option();
    adate->key = "date";
    adate->type = TYPE_STRING;
    adate->required = NO;
    adate->key_desc = "yyyy-mm-dd";
    adate->label = _("Image acquisition date (yyyy-mm-dd)");
    adate->description = _("Required only if 'metfile' not given");
    adate->guisection = _("Metadata");

    elev = G_define_option();
    elev->key = "sun_elevation";
    elev->type = TYPE_DOUBLE;
    elev->required = NO;
    elev->label = _("Sun elevation in degrees");
    elev->description = _("Required only if 'metfile' not given");
    elev->guisection = _("Metadata");

    pdate = G_define_option();
    pdate->key = "product_date";
    pdate->type = TYPE_STRING;
    pdate->required = NO;
    pdate->key_desc = "yyyy-mm-dd";
    pdate->label = _("Image creation date (yyyy-mm-dd)");
    pdate->description = _("Required only if 'metfile' not given");
    pdate->guisection = _("Metadata");

    bgain = G_define_option();
    bgain->key = "gain";
    bgain->type = TYPE_STRING;
    bgain->required = NO;
    bgain->label = _("Gain (H/L) of all Landsat ETM+ bands (1-5,61,62,7,8)");
    bgain->description = _("Required only if 'metfile' not given");
    bgain->guisection = _("Settings");

    perc = G_define_option();
    perc->key = "percent";
    perc->type = TYPE_DOUBLE;
    perc->required = NO;
    perc->label = _("Percent of solar radiance in path radiance");
    perc->description = _("Required only if 'method' is any DOS");
    perc->answer = "0.01";
    perc->guisection = _("Settings");

    dark = G_define_option();
    dark->key = "pixel";
    dark->type = TYPE_INTEGER;
    dark->required = NO;
    dark->label =
	_("Minimum pixels to consider digital number as dark object");
    dark->description = _("Required only if 'method' is any DOS");
    dark->answer = "1000";
    dark->guisection = _("Settings");

    atmo = G_define_option();
    atmo->key = "rayleigh";
    atmo->type = TYPE_DOUBLE;
    atmo->required = NO;
    atmo->label = _("Rayleigh atmosphere (diffuse sky irradiance)");	/* scattering coefficient? */
    atmo->description = _("Required only if 'method' is DOS3");
    atmo->answer = "0.0";
    atmo->guisection = _("Settings");

    lsatmet = G_define_option();
    lsatmet->key = "lsatmet";
    lsatmet->type = TYPE_STRING;
    lsatmet->required = NO;
    lsatmet->multiple = YES;
    lsatmet->label = _("return value stored for a given metadata");
    lsatmet->description = _("Required only if 'metfile' and -p given");
    lsatmet->options =
	"number,creation,date,sun_elev,sensor,bands,sunaz,time";
    desc = NULL;
    G_asprintf(&desc,
	       "number;%s;creation;%s;date;%s;sun_elev;%s;sensor;%s;bands;%s;sunaz;%s;time;%s",
	       _("Landsat Number"),
	       _("Creation timestamp"),
	       _("Date"),
	       _("Sun Elevation"),
	       _("Sensor"),
	       _("Bands count"), _("Sun Azimuth Angle"), _("Time"));
    lsatmet->descriptions = desc;
    lsatmet->guisection = _("Settings");

    oscale = G_define_option();
    oscale->key = "scale";
    oscale->type = TYPE_DOUBLE;
    oscale->answer = "1.0";
    oscale->required = NO;
    oscale->description = _("Scale factor for output");

    /* define the different flags */
    frad = G_define_flag();
    frad->key = 'r';
    frad->description =
	_("Output at-sensor radiance instead of reflectance for all bands");

    named = G_define_flag();
    named->key = 'n';
    named->description =
	_("Input raster maps use as extension the number of the band instead the code");

    print_meta = G_define_flag();
    print_meta->key = 'p';
    print_meta->description = _("Print output metadata info");

    /* options and afters parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


  /*****************************************
  * ---------- START --------------------
  * Stores options and flags to variables
  *****************************************/
    met = metfn->answer;
    inputname = input_prefix->answer;
    outputname = output_prefix->answer;
    sensorname = sensor->answer ? sensor->answer : "";

    overwrite = G_check_overwrite(argc, argv);

    Rast_get_window(&orig_cellhd);

    G_zero(&lsat, sizeof(lsat));

    if (adate->answer != NULL) {
	strncpy(lsat.date, adate->answer, 11);
	lsat.date[10] = '\0';
	if (strlen(lsat.date) != 10)
	    G_fatal_error(_("Illegal date format: [%s] (yyyy-mm-dd)"),
			  lsat.date);
    }

    if (pdate->answer != NULL) {
	strncpy(lsat.creation, pdate->answer, 11);
	lsat.creation[10] = '\0';
	if (strlen(lsat.creation) != 10)
	    G_fatal_error(_("Illegal date format: [%s] (yyyy-mm-dd)"),
			  lsat.creation);
    }

    lsat.sun_elev = elev->answer == NULL ? 0. : atof(elev->answer);

    percent = atof(perc->answer);
    pixel = atoi(dark->answer);
    rayleigh = atof(atmo->answer);
    scale = atof(oscale->answer);

    /*
     * Data from metadata file
     */
    lsat.flag = NOMETADATAFILE;	/* Unnecessary because G_zero filled, but for sanity */
    if (met != NULL) {
	lsat.flag = METADATAFILE;
	lsat_metadata(met, &lsat);
	if (print_meta->answer) {
	    char *lsatmeta;

	    if (lsatmet->answer == NULL) {
		G_fatal_error(_("Please use a metadata keyword with -p"));
	    }

	    for (i = 0; lsatmet->answers[i] != NULL; i++) {
		lsatmeta = lsatmet->answers[i];

		if (strcmp(lsatmeta, "number") == 0) {
		    fprintf(stdout, "number=%d\n", lsat.number);
		}
		if (strcmp(lsatmeta, "creation") == 0) {
		    fprintf(stdout, "creation=%s\n", lsat.creation);
		}
		if (strcmp(lsatmeta, "date") == 0) {
		    fprintf(stdout, "date=%s\n", lsat.date);
		}
		if (strcmp(lsatmeta, "sun_elev") == 0) {
		    fprintf(stdout, "sun_elev=%f\n", lsat.sun_elev);
		}
		if (strcmp(lsatmeta, "sunaz") == 0) {
		    fprintf(stdout, "sunaz=%f\n", lsat.sun_az);
		}
		if (strcmp(lsatmeta, "sensor") == 0) {
		    fprintf(stdout, "sensor=%s\n", lsat.sensor);
		}
		if (strcmp(lsatmeta, "bands") == 0) {
		    fprintf(stdout, "bands=%d\n", lsat.bands);
		}
		if (strcmp(lsatmet->answer, "time") == 0) {
		    fprintf(stdout, "%f\n", lsat.time);
		}
	    }
	    exit(EXIT_SUCCESS);
	}
	G_debug(1, "lsat.number = %d, lsat.sensor = [%s]",
		lsat.number, lsat.sensor);

	if (!lsat.sensor[0] || lsat.number > 8 || lsat.number < 1)
	    G_fatal_error(_("Failed to identify satellite"));

	G_debug(1, "Landsat-%d %s with data set in metadata file [%s]",
		lsat.number, lsat.sensor, met);

	if (elev->answer != NULL) {
	    lsat.sun_elev = atof(elev->answer);
	    G_warning("Overwriting solar elevation of metadata file");
	}
    }
    /*
     * Data from command line
     */
    else if (adate->answer == NULL || elev->answer == NULL) {
	G_fatal_error(_("Lacking '%s' and/or '%s' for this satellite"),
		      adate->key, elev->key);
    }
    else {
	if (strcmp(sensorname, "tm7") == 0) {
	    if (bgain->answer == NULL || strlen(bgain->answer) != 9)
		G_fatal_error(_("Landsat-7 requires band gain with 9 (H/L) characters"));
	    set_ETM(&lsat, bgain->answer);
	}
	else if (strcmp(sensorname, "oli8") == 0)
	    set_OLI(&lsat);
	else if (strcmp(sensorname, "tm5") == 0)
	    set_TM5(&lsat);
	else if (strcmp(sensorname, "tm4") == 0)
	    set_TM4(&lsat);
	else if (strcmp(sensorname, "mss5") == 0)
	    set_MSS5(&lsat);
	else if (strcmp(sensorname, "mss4") == 0)
	    set_MSS4(&lsat);
	else if (strcmp(sensorname, "mss3") == 0)
	    set_MSS3(&lsat);
	else if (strcmp(sensorname, "mss2") == 0)
	    set_MSS2(&lsat);
	else if (strcmp(sensorname, "mss1") == 0)
	    set_MSS1(&lsat);
	else
	    G_fatal_error(_("Unknown satellite type (defined by '%s')"),
			  sensorname);
    }

	/*****************************************
	* ------------ PREPARATION --------------
	*****************************************/
    if (strcasecmp(metho->answer, "corrected") == 0)	/* deleted 2013 */
	method = CORRECTED;
    else if (strcasecmp(metho->answer, "dos1") == 0)
	method = DOS1;
    else if (strcasecmp(metho->answer, "dos2") == 0)
	method = DOS2;
    else if (strcasecmp(metho->answer, "dos2b") == 0)
	method = DOS2b;
    else if (strcasecmp(metho->answer, "dos3") == 0)
	method = DOS3;
    else if (strcasecmp(metho->answer, "dos4") == 0)
	method = DOS4;
    else
	method = UNCORRECTED;

    /*
       if (metho->answer[3] == '2') method = (metho->answer[4] == '\0') ? DOS2 : DOS2b;
       else if (metho->answer[3] == '1') method = DOS1;
       else if (metho->answer[3] == '3') method = DOS3;
       else if (metho->answer[3] == '4') method = DOS4;
       else method = UNCORRECTED;
     */

    for (i = 0; i < lsat.bands; i++) {
	dn_mode[i] = 0;
	dn_dark[i] = (int)lsat.band[i].qcalmin;
	dn_sat = (int)(0.90 * lsat.band[i].qcalmax);
	/* Begin: calculate dark pixel */
	if (method > DOS && !lsat.band[i].thermal) {
	    for (q = 0; q <= lsat.band[i].qcalmax; q++)
		hist[q] = 0L;

	    sprintf(band_in, "%s%d", inputname, lsat.band[i].code);
	    Rast_get_cellhd(band_in, "", &cellhd);
	    Rast_set_window(&cellhd);
	    if ((infd = Rast_open_old(band_in, "")) < 0)
		G_fatal_error(_("Unable to open raster map <%s>"), band_in);

	    in_data_type = Rast_get_map_type(infd);
	    if (in_data_type < 0)
		G_fatal_error(_("Unable to read data type of raster map <%s>"), band_in);
	    inrast = Rast_allocate_buf(in_data_type);

	    nrows = Rast_window_rows();
	    ncols = Rast_window_cols();

	    G_message("Calculating dark pixel of <%s>... ", band_in);
	    for (row = 0; row < nrows; row++) {
		Rast_get_row(infd, inrast, row, in_data_type);
		for (col = 0; col < ncols; col++) {
		    switch (in_data_type) {
		    case CELL_TYPE:
			ptr = (void *)((CELL *) inrast + col);
			q = (int)*((CELL *) ptr);
			break;
		    case FCELL_TYPE:
			ptr = (void *)((FCELL *) inrast + col);
			q = (int)*((FCELL *) ptr);
			break;
		    case DCELL_TYPE:
			ptr = (void *)((DCELL *) inrast + col);
			q = (int)*((DCELL *) ptr);
			break;
		    default:
			ptr = NULL;
			q = -1.;
		    }
		    if (!Rast_is_null_value(ptr, in_data_type) &&
			q >= lsat.band[i].qcalmin &&
			q <= lsat.band[i].qcalmax)
			hist[q]++;
		}
	    }
	    /* DN of dark object */
	    for (j = lsat.band[i].qcalmin; j <= lsat.band[i].qcalmax; j++) {
		if (hist[j] >= (unsigned int)pixel) {
		    dn_dark[i] = j;
		    break;
		}
	    }
	    /* Mode of DN (exclude potentially saturated) */
	    h_max = 0L;
	    for (j = lsat.band[i].qcalmin; j < dn_sat; j++) {
		/* G_debug(5, "%d-%ld", j, hist[j]); */
		if (hist[j] > h_max) {
		    h_max = hist[j];
		    dn_mode[i] = j;
		}
	    }
	    G_verbose_message
		("... DN = %.2d [%lu] : mode %.2d [%lu], excluding DN > %d",
		 dn_dark[i], hist[dn_dark[i]], dn_mode[i], hist[dn_mode[i]],
		 dn_sat);

	    G_free(inrast);
	    Rast_close(infd);
	}
	/* End: calculate dark pixel */

	/* Calculate transformation constants */
	lsat_bandctes(&lsat, i, method, percent, dn_dark[i], rayleigh);
    }

    /*
     * unnecessary or necessary with more checking as acquisition date,...
     * if (strlen(lsat.creation) == 0)
     * G_fatal_error(_("Unknown production date (defined by '%s')"), pdate->key);
     */

    if (G_verbose() > G_verbose_std()) {
	fprintf(stderr, "\n LANDSAT: %d SENSOR: %s\n", lsat.number,
		lsat.sensor);
	fprintf(stderr, " ACQUISITION DATE %s [production date %s]\n",
		lsat.date, lsat.creation);
	fprintf(stderr, "   Earth-sun distance    = %.8lf\n", lsat.dist_es);
	fprintf(stderr, "   Solar elevation angle = %.8lf\n", lsat.sun_elev);
	fprintf(stderr, "   Atmospheric correction: %s\n",
		(method == UNCORRECTED ? "UNCORRECTED" : metho->answer));
	if (method > DOS) {
	    fprintf(stderr,
		    "   Percent of solar irradiance in path radiance = %.4lf\n",
		    percent);
	}
	for (i = 0; i < lsat.bands; i++) {
	    fprintf(stderr, "-------------------\n");
	    fprintf(stderr, " BAND %d %s(code %d)\n", lsat.band[i].number,
		    (lsat.band[i].thermal ? "thermal " : ""),
		    lsat.band[i].code);
	    fprintf(stderr,
		    "   calibrated digital number (DN): %.1lf to %.1lf\n",
		    lsat.band[i].qcalmin, lsat.band[i].qcalmax);
	    fprintf(stderr, "   calibration constants (L): %.5lf to %.5lf\n",
		    lsat.band[i].lmin, lsat.band[i].lmax);
	    fprintf(stderr, "   at-%s radiance = %.8lf * DN + %.5lf\n",
		    (method > DOS ? "surface" : "sensor"), lsat.band[i].gain,
		    lsat.band[i].bias);
	    if (lsat.band[i].thermal) {
		fprintf(stderr,
			"   at-sensor temperature = %.5lf / log[(%.5lf / radiance) + 1.0]\n",
			lsat.band[i].K2, lsat.band[i].K1);
	    }
	    else {
		fprintf(stderr,
			"   mean solar exoatmospheric irradiance (ESUN): %.5lf\n",
			lsat.band[i].esun);
		fprintf(stderr, "   at-%s reflectance = radiance / %.5lf\n",
			(method > DOS ? "surface" : "sensor"),
			lsat.band[i].K1);
		if (method > DOS) {
		    fprintf(stderr,
			    "   the darkness DN with a least %d pixels is %d\n",
			    pixel, dn_dark[i]);
		    fprintf(stderr, "   the DN mode is %d\n", dn_mode[i]);
		}
	    }
	}
	fprintf(stderr, "-------------------\n");
	fflush(stderr);
    }

    /*****************************************
    * ------------ CALCULUS -----------------
    *****************************************/

    G_message(_("Calculating..."));
    for (i = 0; i < lsat.bands; i++) {
	sprintf(band_in, "%s%d", inputname,
		(named->answer ? lsat.band[i].number : lsat.band[i].code));
	sprintf(band_out, "%s%d", outputname, lsat.band[i].code);

	/* set same size as original band raster */
	Rast_get_cellhd(band_in, "", &cellhd);
	Rast_set_window(&cellhd);
	if ((infd = Rast_open_old(band_in, "")) < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"), band_in);

	if (G_find_raster2(band_out, "")) {
	    if (overwrite) {
		G_warning(_("Raster map <%s> already exists and will be overwritten"),
			  band_out);
	    }
	    else {
		G_warning(_("Raster map <%s> exists. Skipping."), band_out);
		continue;
	    }
	}

	in_data_type = Rast_get_map_type(infd);
	if (in_data_type < 0)
	    G_fatal_error(_("Unable to read data type of raster map <%s>"), band_in);

	/* controlling, if we can write the raster */
	if (G_legal_filename(band_out) < 0)
	    G_fatal_error(_("<%s> is an illegal file name"), band_out);

	if ((outfd = Rast_open_new(band_out, DCELL_TYPE)) < 0)
	    G_fatal_error(_("Unable to create raster map <%s>"), band_out);

	/* allocate input and output buffer */
	inrast = Rast_allocate_buf(in_data_type);
	outrast = Rast_allocate_buf(DCELL_TYPE);

	nrows = Rast_window_rows();
	ncols = Rast_window_cols();

	G_important_message(_("Writing %s of <%s> to <%s>..."),
			    (frad->
			     answer ? _("radiance") : (lsat.band[i].
						       thermal) ?
			     _("temperature") : _("reflectance")), band_in,
			    band_out);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);

	    Rast_get_row(infd, inrast, row, in_data_type);
	    for (col = 0; col < ncols; col++) {
		switch (in_data_type) {
		case CELL_TYPE:
		    ptr = (void *)((CELL *) inrast + col);
		    qcal = (double)((CELL *) inrast)[col];
		    break;
		case FCELL_TYPE:
		    ptr = (void *)((FCELL *) inrast + col);
		    qcal = (double)((FCELL *) inrast)[col];
		    break;
		case DCELL_TYPE:
		    ptr = (void *)((DCELL *) inrast + col);
		    qcal = (double)((DCELL *) inrast)[col];
		    break;
		default:
		    ptr = NULL;
		    qcal = -1.;
		}
		if (Rast_is_null_value(ptr, in_data_type) ||
		    qcal < lsat.band[i].qcalmin) {
		    Rast_set_d_null_value((DCELL *) outrast + col, 1);
		}
		else {
		    rad = lsat_qcal2rad(qcal, &lsat.band[i]);
		    if (frad->answer) {
			ref = rad;
		    }
		    else {
			if (lsat.band[i].thermal) {
			    ref = lsat_rad2temp(rad, &lsat.band[i]);
			}
			else {
			    ref = lsat_rad2ref(rad, &lsat.band[i]) * scale;
			    if (ref < 0. && method > DOS)
				ref = 0.;
			}
		    }
		    ((DCELL *) outrast)[col] = ref;
		}
	    }
	    Rast_put_row(outfd, outrast, DCELL_TYPE);
	}
	G_percent(1, 1, 1);

	ref_mode = 0.;
	if (method > DOS && !lsat.band[i].thermal) {
	    ref_mode = lsat_qcal2rad(dn_mode[i], &lsat.band[i]);
	    ref_mode = lsat_rad2ref(ref_mode, &lsat.band[i]);
	}


	G_free(inrast);
	Rast_close(infd);
	G_free(outrast);
	Rast_close(outfd);


	/*
	 * needed?
	 * if (out_type != CELL_TYPE)
	 * G_quantize_fp_map_range(band_out, G_mapset(), 0., 360., 0,
	 * 360);
	 */

	/* set grey255 colortable */
	Rast_init_colors(&colors);
	Rast_read_fp_range(band_out, G_mapset(), &range);
	Rast_get_fp_range_min_max(&range, &min, &max);
	Rast_make_grey_scale_fp_colors(&colors, min, max);
	Rast_write_colors(band_out, G_mapset(), &colors);

	/* Initialize the 'history' structure with basic info */
	Rast_short_history(band_out, "raster", &history);
	Rast_append_format_history(&history,
				   " %s of Landsat-%d %s (method %s)",
				   (frad->
				    answer ? "Radiance" : (lsat.band[i].
							   thermal ?
							   "Temperature" :
							   "Reflectance")),
				   lsat.number, lsat.sensor, metho->answer);
	Rast_append_history(&history,
			    "-----------------------------------------------------------------");
	Rast_append_format_history(&history,
				   " Acquisition date (and time) ........... %s (%.4lf h)",
				   lsat.date, lsat.time);
	Rast_append_format_history(&history,
				   " Production date ....................... %s\n",
				   lsat.creation);
	Rast_append_format_history(&history,
				   " Earth-sun distance (d) ................ %.7lf",
				   lsat.dist_es);
	Rast_append_format_history(&history,
				   " Sun elevation (and azimuth) ........... %.5lf (%.5lf)",
				   lsat.sun_elev, lsat.sun_az);
	Rast_append_format_history(&history,
				   " Digital number (DN) range ............. %.0lf to %.0lf",
				   lsat.band[i].qcalmin,
				   lsat.band[i].qcalmax);
	Rast_append_format_history(&history,
				   " Calibration constants (Lmin to Lmax) .. %+.5lf to %+.5lf",
				   lsat.band[i].lmin, lsat.band[i].lmax);
	Rast_append_format_history(&history,
				   " DN to Radiance (gain and bias) ........ %+.5lf and %+.5lf",
				   lsat.band[i].gain, lsat.band[i].bias);
	if (lsat.band[i].thermal) {
	    Rast_append_format_history(&history,
				       " Temperature (K1 and K2) ............... %.3lf and %.3lf",
				       lsat.band[i].K1, lsat.band[i].K2);
	}
	else {
	    Rast_append_format_history(&history,
				       " Mean solar irradiance (ESUN) .......... %.3lf",
				       lsat.band[i].esun);
	    Rast_append_format_history(&history,
				       " Radiance to Reflectance (divide by) ... %+.5lf",
				       lsat.band[i].K1);
	    if (method > DOS) {
		Rast_append_format_history(&history, " ");
		Rast_append_format_history(&history,
					   " Dark object (%4d pixels) DN = ........ %d",
					   pixel, dn_dark[i]);
		Rast_append_format_history(&history,
					   " Mode in reflectance histogram ......... %.5lf",
					   ref_mode);
	    }
	}
	Rast_append_history(&history,
			    "------------------------------------------------------------------");

	Rast_command_history(&history);
	Rast_write_history(band_out, &history);

	if (lsat.band[i].thermal)
	    Rast_write_units(band_out, "Kelvin");
	else if (frad->answer)
	    Rast_write_units(band_out, "W/(m^2 sr um)");
	else
	    Rast_write_units(band_out, "unitless");

	/*  set raster timestamp from acq date? (see r.timestamp module)  */
    }
    Rast_set_window(&orig_cellhd);

    exit(EXIT_SUCCESS);
}
