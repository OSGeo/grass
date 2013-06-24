#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"
#include "earth_sun.h"

#define PI   M_PI
#define D2R  M_PI / 180.

#define MAX_STR         127
#define METADATA_SIZE   65535	/* MTL.txt file size  65535 bytes */
#define TM5_MET_SIZE    28700	/* .met file size 28686 bytes */


inline void chrncpy(char *dest, char src[], int n)
{
    int i;

    for (i = 0; i < n && src[i] != '\0' && src[i] != '\"'; i++)
	dest[i] = src[i];
    dest[i] = '\0';
}

void get_metformat(const char metadata[], char *key, char value[])
{
    int i = 0;
    char *ptrmet = strstr(metadata, key);
    char *ptr;

    if (ptrmet != NULL) {
        ptr = strstr(ptrmet, " VALUE ");
        if (ptr != NULL) {
            while (*ptr++ != '=') ;
            while (*ptr <= ' ' || *ptr == '\"')
		ptr++;
            while (i < MAX_STR && *ptr != '\"' && *ptr >= ' ')
		value[i++] = *ptr++;
        }
    }
    value[i] = '\0';
}

void get_mtlformat(const char metadata[], char *key, char value[])
{
    int i = 0;
    char *ptr = strstr(metadata, key);

    if (ptr != NULL) {
        while (*ptr++ != '=') ;

        while (*ptr <= ' ' || *ptr == '\"')
	    ptr++;

        while (i < MAX_STR && *ptr != '\"' && *ptr > ' ')
	    value[i++] = *ptr++;
    }
    value[i] = '\0';
}


/****************************************************************************
 * PURPOSE:     Read parameters from Landsat metadata files
 *****************************************************************************/

void lsat_metadata(char *metafile, lsat_data * lsat)
{
    FILE *f;
    char metadata[METADATA_SIZE];
    char key[MAX_STR], value[MAX_STR];
    void (*get_metadata) (const char [], char *, char []);
    int i, j, ver_meta;

    if ((f = fopen(metafile, "r")) == NULL)
	G_fatal_error(_("Metadata file <%s> not found"), metafile);

    i = fread(metadata, METADATA_SIZE, 1, f);

    /* get version of the metadata file */
    get_metadata = 
	(strstr(metadata, " VALUE ") != NULL) ? get_metformat : get_mtlformat;
    ver_meta = (strstr(metadata, "QCALMAX_BAND") != NULL) ? 0 : 1;
    
    /* Fill with product metadata */
    get_metadata(metadata, "SPACECRAFT_ID", value);
    if (value[0] == '\0') {
        get_metadata(metadata, "PLATFORMSHORTNAME", value);
    }
    
    i = 0;
    while (value[i] && (value[i] < '0' || value[i] > '9'))
	i++;
    lsat->number = atoi(value + i);

    get_metadata(metadata, "SENSOR_ID", value);
    if (value[0] == '\0') {
        get_metadata(metadata, "SENSORSHORTNAME", value);
    }
    chrncpy(lsat->sensor, value, 8);

    get_metadata(metadata, "DATE_ACQUIRED", value);
    if (value[0] == '\0') {
        get_metadata(metadata, "ACQUISITION_DATE", value);
        if (value[0] == '\0') {
            get_metadata(metadata, "CALENDARDATE", value);
        }
    }
    chrncpy(lsat->date, value, 10);

    get_metadata(metadata, "FILE_DATE", value);
    if (value[0] == '\0') {
        get_metadata(metadata, "CREATION_TIME", value);
        if (value[0] == '\0') {
            get_metadata(metadata, "PRODUCTIONDATETIME", value);
        }
    }
    if (value[0] != '\0')
	chrncpy(lsat->creation, value, 10);
    else
	G_warning
	    ("Using production date from the command line 'product_date'");

    get_metadata(metadata, "SUN_AZIMUTH", value);
    lsat->sun_az = atof(value);
    if (lsat->sun_az == 0.)
        G_warning("Sun azimuth is %f", lsat->sun_az);

    get_metadata(metadata, "SUN_ELEVATION", value);
    if (value[0] == '\0') {
        get_metadata(metadata, "SolarElevation", value);
    }
    lsat->sun_elev = atof(value);
    if (lsat->sun_elev == 0.)
        G_warning("Sun elevation is %f", lsat->sun_elev);

    get_metadata(metadata, "SCENE_CENTER_TIME", value);
    if (value[0] == '\0') {
        get_metadata(metadata, "SCENE_CENTER_SCAN_TIME", value);
    }
    /* Remove trailing 'z'*/
    value[strlen(value) - 1]='\0';
    /* Cast from hh:mm:ss into hh.hhh*/
    G_llres_scan(value, &lsat->time);
    if (lsat->time == 0.)
        G_warning("Time is %f", lsat->time);

    /* Fill the data with the basic/default sensor parameters */
    switch (lsat->number) {
    case 1:
	set_MSS1(lsat);
	break;
    case 2:
	set_MSS2(lsat);
	break;
    case 3:
	set_MSS3(lsat);
	break;
    case 4:
	if (lsat->sensor[0] == 'M')
	    set_MSS4(lsat);
	else
	    set_TM4(lsat);
	break;
    case 5:
	if (lsat->sensor[0] == 'M')
	    set_MSS5(lsat);
	else
	    set_TM5(lsat);
	break;
    case 7:
	{
	    char *fmt_gain[] = { "BAND%d_GAIN%d", " GAIN_BAND_%d_VCID_%d" };
	    for (i = 1, j = 0; i < 9; i++) {
		sprintf(key, fmt_gain[ver_meta], i, 1);
		if (i != 6)
		    key[ver_meta == 0 ? 10 : 12] = '\0';
		get_metadata(metadata, key, value + j++);
		if (i == 6) {
		    sprintf(key, fmt_gain[ver_meta], i, 2);
		    get_metadata(metadata, key, value + j++);
		}
	    }
	    value[j] = '\0';
	    G_debug(1, "ETM+ gain = [%s]", value);
	    set_ETM(lsat, value);
	    break;
	}
    case 8:
	set_LDCM(lsat);
	break;

    default:
	G_warning("Unable to recognize satellite platform [%d]",
		  lsat->number);
	break;
    }

    /* Update the information from metadata file, if infile */
    if (get_metadata == get_mtlformat) {
	if (ver_meta == 0) {	/* now MTLold.txt */
	    G_verbose_message("Metada file is MTL file: old format");
	    for (i = 0; i < lsat->bands; i++) {
		sprintf(key, "LMAX_BAND%d", lsat->band[i].code);
		get_metadata(metadata, key, value);
		lsat->band[i].lmax = atof(value);
		sprintf(key, "LMIN_BAND%d", lsat->band[i].code);
		get_metadata(metadata, key, value);
		lsat->band[i].lmin = atof(value);
		sprintf(key, "QCALMAX_BAND%d", lsat->band[i].code);
		get_metadata(metadata, key, value);
		lsat->band[i].qcalmax = atof(value);
		sprintf(key, "QCALMIN_BAND%d", lsat->band[i].code);
		get_metadata(metadata, key, value);
		lsat->band[i].qcalmin = atof(value);
	    }
	}
	else {			/* now MTL.txt */

	    G_verbose_message("Metada file is MTL file: new format");
	    if (strstr(metadata, "RADIANCE_MAXIMUM_BAND") != NULL) {
		G_verbose_message
		    ("RADIANCE & QUANTIZE from data of the metadata file");
		for (i = 0; i < lsat->bands; i++) {
		    if (lsat->band[i].thermal && lsat->number == 7) {
			sprintf(key, "RADIANCE_MAXIMUM_BAND_6_VCID_%d",
				lsat->band[i].code - 60);
			get_metadata(metadata, key, value);
			lsat->band[i].lmax = atof(value);
			sprintf(key, "RADIANCE_MINIMUM_BAND_6_VCID_%d",
				lsat->band[i].code - 60);
			get_metadata(metadata, key, value);
			lsat->band[i].lmin = atof(value);
			sprintf(key, "QUANTIZE_CAL_MAX_BAND_6_VCID_%d",
				lsat->band[i].code - 60);
			get_metadata(metadata, key, value);
			lsat->band[i].qcalmax = atof(value);
			sprintf(key, "QUANTIZE_CAL_MIN_BAND_6_VCID_%d",
				lsat->band[i].code - 60);
			get_metadata(metadata, key, value);
			lsat->band[i].qcalmin = atof(value);
		    }
		    else {
			sprintf(key, "RADIANCE_MAXIMUM_BAND_%d",
				lsat->band[i].code);
			get_metadata(metadata, key, value);
			lsat->band[i].lmax = atof(value);
			sprintf(key, "RADIANCE_MINIMUM_BAND_%d",
				lsat->band[i].code);
			get_metadata(metadata, key, value);
			lsat->band[i].lmin = atof(value);
			sprintf(key, "QUANTIZE_CAL_MAX_BAND_%d",
				lsat->band[i].code);
			get_metadata(metadata, key, value);
			lsat->band[i].qcalmax = atof(value);
			sprintf(key, "QUANTIZE_CAL_MIN_BAND_%d",
				lsat->band[i].code);
			get_metadata(metadata, key, value);
			lsat->band[i].qcalmin = atof(value);
		    }
		}
	    }
	    else {
		G_verbose_message
		    ("RADIANCE & QUANTIZE from radiometric rescaling group of the metadata file");
		/* from LDCM sample file: mode = 0; from LDCM-DFCB-004.pdf file: mode = 1 */
		int mode =
		    (strstr(metadata, "RADIANCE_MULTIPLICATIVE_FACTOR_BAND") !=
		     NULL) ? 0 : 1;
		char *fmt_radmu[] =
		    { "RADIANCE_MULTIPLICATIVE_FACTOR_BAND%d",
	"RADIANCE_MULT_BAND_%d" };
		char *fmt_radad[] =
		    { "RADIANCE_ADDITIVE_FACTOR_BAND%d",
	"RADIANCE_ADD_BAND_%d" };
		char *fmt_k1cte[] =
		    { "K1_CONSTANT_BAND%d", "K1_CONSTANT_BAND_%d" };
		char *fmt_k2cte[] =
		    { "K2_CONSTANT_BAND%d", "K2_CONSTANT_BAND_%d" };
		char *fmt_refad[] =
		    { "REFLECTANCE_ADDITIVE_FACTOR_BAND%d",
	"REFLECTANCE_ADD_BAND_%d" };

		for (i = 0; i < lsat->bands; i++) {
		    sprintf(key, fmt_radmu[mode], lsat->band[i].code);
		    get_metadata(metadata, key, value);
		    lsat->band[i].gain = atof(value);
		    sprintf(key, fmt_radad[mode], lsat->band[i].code);
		    get_metadata(metadata, key, value);
		    lsat->band[i].bias = atof(value);
		    /* reverse to calculate the original values */
		    lsat->band[i].qcalmax =
			(lsat->number < 8 ? 255. : 65535.);
		    lsat->band[i].qcalmin = 1.;
		    lsat->band[i].lmin =
			lsat->band[i].gain * lsat->band[i].qcalmin +
			lsat->band[i].bias;
		    lsat->band[i].lmax =
			lsat->band[i].gain * lsat->band[i].qcalmax +
			lsat->band[i].bias;
		    /* ----- */
		    if (lsat->number == 8) {
			if (lsat->band[i].thermal) {
			    sprintf(key, fmt_k1cte[mode], lsat->band[i].code);
			    get_metadata(metadata, key, value);
			    lsat->band[i].K1 = atof(value);
			    sprintf(key, fmt_k2cte[mode], lsat->band[i].code);
			    get_metadata(metadata, key, value);
			    lsat->band[i].K2 = atof(value);
			}
			else {
			    lsat->band[i].K1 = 0.;
			    /* ESUN from metadafa file: bias/K2 seem better than gain/K1 */
			    sprintf(key, fmt_refad[mode], lsat->band[i].code);
			    get_metadata(metadata, key, value);
			    lsat->band[i].K2 = atof(value);
			    lsat->band[i].esun =
				(double)(PI * lsat->dist_es * lsat->dist_es *
					 lsat->band[i].bias) /
					 (sin(D2R * lsat->sun_elev) *
					 lsat->band[i].K2);
			}
		    }
		}
	    }
	    /* Other possible values in file */
	    get_metadata(metadata, "EARTH_SUN_DISTANCE", value);
	    if (value[0] != '\0') {
		lsat->dist_es = atof(value);
		G_verbose_message
		    ("ESUN evaluate from REFLECTANCE_ADDITIVE_FACTOR_BAND of the metadata file");
	    }
	}
    }
    else {
	G_verbose_message("Metada file is MET file");
	G_verbose_message
	    ("RADIANCE & QUANTIZE from band setting of the metadata file");
	for (i = 0; i < lsat->bands; i++) {
	    sprintf(key, "Band%dGainSetting", lsat->band[i].code);
	    get_metadata(metadata, key, value);
	    if (value[0] == '\0') {
		G_warning(key);
		continue;
	    }
	    lsat->band[i].gain = atof(value);
	    sprintf(key, "Band%dBiasSetting", lsat->band[i].code);
	    get_metadata(metadata, key, value);
	    if (value[0] == '\0') {
		G_warning(key);
		continue;
	    }
	    lsat->band[i].bias = atof(value);

	    lsat->band[i].qcalmax = 255.;
	    lsat->band[i].qcalmin = 1.;
	    lsat->band[i].lmin =
		lsat->band[i].gain * lsat->band[i].qcalmin +
		lsat->band[i].bias;
	    lsat->band[i].lmax =
		lsat->band[i].gain * lsat->band[i].qcalmax +
		lsat->band[i].bias;
	}
    }

    fclose(f);
    return;
}
