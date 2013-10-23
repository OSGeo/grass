#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"
#include "earth_sun.h"

#define PI  M_PI
#define D2R M_PI/180.

#define MAX_STR         256
#define METADATA_SIZE   65535	/* MTL.txt file size  65535 bytes */
#define TM5_MET_SIZE    28700	/* .met file size 28686 bytes */


static void chrncpy(char *dest, char src[], int n)
{
    int i;

    for (i = 0; i < n && src[i] != '\0' && src[i] != '\"'; i++)
	dest[i] = src[i];
    dest[i] = '\0';
}

static void date_replace_slash(char *str)
{
    while (*str) {
	if (*str == '/')
	    *str = '-';
	str++;
    }
}

/* OLD Metadata Files */
void get_metformat(const char metadata[], char *key, char value[])
{
    int i = 0;
    char *ptrmet = strstr(metadata, key);
    char *ptr;

    if (ptrmet != NULL) {
	ptr = strstr(ptrmet, " VALUE ");
	if (ptr != NULL) {
	    while (*ptr++ != '=') ;
	    while (*ptr <= ' ')
		ptr++;
	    if (*ptr == '\"')
		ptr++;
	    while (i < MAX_STR && *ptr != '\"' && *ptr >= ' ')
		value[i++] = *ptr++;
	}
    }
    value[i] = '\0';
}

double get_metdouble(const char metadata[], char *format, int code, char value[])
{
    char key[MAX_STR];
    
    sprintf(key, format, code);
    get_metformat(metadata, key, value);
    return atof(value);			    
}


/* NEW Metadata Files */
void get_mtlformat(const char metadata[], char *key, char value[])
{
    int i = 0;
    char *ptr = strstr(metadata, key);

    if (ptr != NULL) {
	while (*ptr++ != '=') ;
	while (*ptr <= ' ')
	    ptr++;
	if (*ptr == '\"')
	    ptr++;
	while (i < MAX_STR && *ptr != '\"' && *ptr > ' ')
	    value[i++] = *ptr++;
    }
    value[i] = '\0';
}

double get_mtldouble(const char metadata[], char *format, int code, char value[])
{
    char key[MAX_STR];
    
    sprintf(key, format, code);
    get_mtlformat(metadata, key, value);
    return atof(value);			    
}



/****************************************************************************
 * PURPOSE:     Read parameters from Landsat metadata files
 *****************************************************************************/

void lsat_metadata(char *metafile, lsat_data * lsat)
{
    FILE *f;
    char mtldata[METADATA_SIZE];
    char key[MAX_STR], value[MAX_STR];
    void (*get_mtldata) (const char[], char *, char[]);
    void (*get_mtlreal) (const char[], char *, int, char[]);
    int i, j, ver_mtl;
    double X2;

    /* store metadata in ram */
    if ((f = fopen(metafile, "r")) == NULL)
	G_fatal_error(_("Metadata file <%s> not found"), metafile);
    i = fread(mtldata, METADATA_SIZE, 1, f);
    (void)fclose(f);

    /* set version of the metadata file */
    /* get_mtldata = (strstr(mtldata, " VALUE ") != NULL) ? get_metformat : get_mtlformat; */
    if (strstr(mtldata, " VALUE ") != NULL)
    {
	get_mtldata = get_metformat;
	get_mtlreal = get_metdouble;
    }
    else
    {
	get_mtldata = get_mtlformat;
	get_mtlreal = get_mtldouble;
    }
    ver_mtl = (strstr(mtldata, "QCALMAX_BAND") != NULL) ? 0 : 1;

    /* Fill with product metadata */
    get_mtldata(mtldata, "SPACECRAFT_ID", value);
    if (value[0] == '\0') {
	get_mtldata(mtldata, "PLATFORMSHORTNAME", value);
    }
    i = 0;
    while (value[i] && (value[i] < '0' || value[i] > '9'))
	i++;
    lsat->number = atoi(value + i);

    get_mtldata(mtldata, "SENSOR_ID", value);
    if (value[0] == '\0') {
	get_mtldata(mtldata, "SENSORSHORTNAME", value);
    }
    chrncpy(lsat->sensor, value, 8);

    get_mtldata(mtldata, "DATE_ACQUIRED", value);
    if (value[0] == '\0') {
	get_mtldata(mtldata, "ACQUISITION_DATE", value);
	if (value[0] == '\0') {
	    get_mtldata(mtldata, "CALENDARDATE", value);
	}
    }
    if (value[0] != '\0') {
	date_replace_slash(value);
	chrncpy(lsat->date, value, 10);
    }
    else
	G_warning("Using adquisition date from the command line 'date'");

    get_mtldata(mtldata, "FILE_DATE", value);
    if (value[0] == '\0') {
	get_mtldata(mtldata, "CREATION_TIME", value);
	if (value[0] == '\0') {
	    get_mtldata(mtldata, "PRODUCTIONDATETIME", value);
	}
    }
    if (value[0] != '\0') {
	date_replace_slash(value);
	chrncpy(lsat->creation, value, 10);
    }
    else
	G_warning
	    ("Using production date from the command line 'product_date'");

    get_mtldata(mtldata, "SUN_AZIMUTH", value);
    lsat->sun_az = atof(value);
    if (lsat->sun_az == 0.)
	G_warning("Sun azimuth is %f", lsat->sun_az);

    get_mtldata(mtldata, "SUN_ELEVATION", value);
    if (value[0] == '\0') {
	get_mtldata(mtldata, "SolarElevation", value);
    }
    lsat->sun_elev = atof(value);
    if (lsat->sun_elev == 0.)
	G_warning("Sun elevation is %f", lsat->sun_elev);

    get_mtldata(mtldata, "SCENE_CENTER_TIME", value);
    if (value[0] == '\0') {
	get_mtldata(mtldata, "SCENE_CENTER_SCAN_TIME", value);
    }
    if (value[0] != '\0') {
	value[strlen(value) - 1] = '\0';	/* Remove trailing 'z' */
	G_llres_scan(value, &lsat->time);	/* Cast from hh:mm:ss into hh.hhh */
    }
    if (lsat->time == 0.)
	G_warning("Scene time is %f", lsat->time);

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
		sprintf(key, fmt_gain[ver_mtl], i, 1);
		if (i != 6)
		    key[ver_mtl == 0 ? 10 : 12] = '\0';
		get_mtldata(mtldata, key, value + j++);
		if (i == 6) {
		    sprintf(key, fmt_gain[ver_mtl], i, 2);
		    get_mtldata(mtldata, key, value + j++);
		}
	    }
	    value[j] = '\0';
	    G_debug(1, "ETM+ gain = [%s]", value);
	    set_ETM(lsat, value);
	    break;
	}
    case 8:
	set_OLI(lsat);
	break;
    default:
	G_warning("Unable to recognize satellite platform [%d]",
		  lsat->number);
	break;
    }

    /* Update the information from metadata file, if infile */
    if (get_mtldata == get_mtlformat) {
	if (ver_mtl == 0) {
	    G_verbose_message("Metada file is MTL file: old format");
	    for (i = 0; i < lsat->bands; i++) {
		sprintf(key, "LMAX_BAND%d", lsat->band[i].code);
		get_mtldata(mtldata, key, value);
		lsat->band[i].lmax = atof(value);
		sprintf(key, "LMIN_BAND%d", lsat->band[i].code);
		get_mtldata(mtldata, key, value);
		lsat->band[i].lmin = atof(value);
		sprintf(key, "QCALMAX_BAND%d", lsat->band[i].code);
		get_mtldata(mtldata, key, value);
		lsat->band[i].qcalmax = atof(value);
		sprintf(key, "QCALMIN_BAND%d", lsat->band[i].code);
		get_mtldata(mtldata, key, value);
		lsat->band[i].qcalmin = atof(value);
	    }
	}
	else {
	    G_verbose_message("Metada file is MTL file: new format");
	    /* Other possible values in the metadata file */
	    get_mtldata(mtldata, "EARTH_SUN_DISTANCE", value);	/* Necessary after */
	    if (value[0] != '\0')
		lsat->dist_es = atof(value);
	    if (strstr(mtldata, "RADIANCE_MAXIMUM_BAND") != NULL) 
	    {
		G_verbose_message("RADIANCE & QUANTIZE from  MIN_MAX_(RADIANCE|PIXEL_VALUE)");
		for (i = 0; i < lsat->bands; i++) {
		    if (lsat->number == 7 && lsat->band[i].thermal) {
			sprintf(key, "RADIANCE_MAXIMUM_BAND_6_VCID_%d", lsat->band[i].code - 60);
			get_mtldata(mtldata, key, value);
			lsat->band[i].lmax = atof(value);
			sprintf(key, "RADIANCE_MINIMUM_BAND_6_VCID_%d", lsat->band[i].code - 60);
			get_mtldata(mtldata, key, value);
			lsat->band[i].lmin = atof(value);
			sprintf(key, "QUANTIZE_CAL_MAX_BAND_6_VCID_%d", lsat->band[i].code - 60);
			get_mtldata(mtldata, key, value);
			lsat->band[i].qcalmax = atof(value);
			sprintf(key, "QUANTIZE_CAL_MIN_BAND_6_VCID_%d", lsat->band[i].code - 60);
			get_mtldata(mtldata, key, value);
			lsat->band[i].qcalmin = atof(value);
		    }
		    else {
			sprintf(key, "RADIANCE_MAXIMUM_BAND_%d", lsat->band[i].code);
			get_mtldata(mtldata, key, value);
			lsat->band[i].lmax = atof(value);
			sprintf(key, "RADIANCE_MINIMUM_BAND_%d", lsat->band[i].code);
			get_mtldata(mtldata, key, value);
			lsat->band[i].lmin = atof(value);
			sprintf(key, "QUANTIZE_CAL_MAX_BAND_%d", lsat->band[i].code);
			get_mtldata(mtldata, key, value);
			lsat->band[i].qcalmax = atof(value);
			sprintf(key, "QUANTIZE_CAL_MIN_BAND_%d", lsat->band[i].code);
			get_mtldata(mtldata, key, value);
			lsat->band[i].qcalmin = atof(value);
		    }
		    /* other possible values of each band */
		    if (lsat->band[i].thermal) {
			sprintf(key, "K1_CONSTANT_BAND_%d", lsat->band[i].code);
			get_mtldata(mtldata, key, value);
			lsat->band[i].K1 = atof(value);
			sprintf(key, "K2_CONSTANT_BAND_%d", lsat->band[i].code);
			get_mtldata(mtldata, key, value);
			lsat->band[i].K2 = atof(value);
		    }
		    else if (lsat->number == 8)
		    {
			/* ESUN from  REFLECTANCE and RADIANCE ADD_BAND */
			sprintf(key, "REFLECTANCE_MAXIMUM_BAND_%d", lsat->band[i].code);
			get_mtldata(mtldata, key, value);
			X2 = atof(value);
			lsat->band[i].esun = (double)(PI * lsat->dist_es * lsat->dist_es * lsat->band[i].lmax) / X2;
		    }
		}
		if (lsat->number == 8)
		    G_warning("ESUN evaluated from REFLECTANCE_MAXIMUM_BAND");
	    }
	    else {
		G_verbose_message("RADIANCE & QUANTIZE from RADIOMETRIC_RESCALING");
		for (i = 0; i < lsat->bands; i++) {
		    sprintf(key, "RADIANCE_MULT_BAND_%d", lsat->band[i].code);
		    get_mtldata(mtldata, key, value);
		    lsat->band[i].gain = atof(value);
		    sprintf(key, "RADIANCE_ADD_BAND_%d", lsat->band[i].code);
		    get_mtldata(mtldata, key, value);
		    lsat->band[i].bias = atof(value);
		    /* reversing to calculate the values of Lmin and Lmax ... */
		    lsat->band[i].lmin = lsat->band[i].gain * lsat->band[i].qcalmin + lsat->band[i].bias;
		    lsat->band[i].lmax = lsat->band[i].gain * lsat->band[i].qcalmax + lsat->band[i].bias;
		    /* ... qcalmax and qcalmin loaded in previous sensor_ function */
		    if (lsat->number == 8) {
			if (lsat->band[i].thermal) {
			    sprintf(key, "K1_CONSTANT_BAND_%d", lsat->band[i].code);
			    get_mtldata(mtldata, key, value);
			    lsat->band[i].K1 = atof(value);
			    sprintf(key, "K2_CONSTANT_BAND_%d", lsat->band[i].code);
			    get_mtldata(mtldata, key, value);
			    lsat->band[i].K2 = atof(value);
			}
			else {
			    sprintf(key, "REFLECTANCE_MULT_BAND_%d", lsat->band[i].code);
			    get_mtldata(mtldata, key, value);
			    lsat->band[i].K1 = atof(value);
			    sprintf(key, "REFLECTANCE_ADD_BAND_%d", lsat->band[i].code);
			    get_mtldata(mtldata, key, value);
			    lsat->band[i].K2 = atof(value);			    
			    /* ESUN from  REFLECTANCE_ADD_BAND */
			    lsat->band[i].esun = (double)(PI * lsat->dist_es * lsat->dist_es * lsat->band[i].bias) / lsat->band[i].K2;
			    /*
			       double esun1 = (double) (PI * lsat->dist_es * lsat->dist_es * lsat->band[i].bias) / lsat->band[i].K2;
			       double esun2 = (double) (PI * lsat->dist_es * lsat->dist_es * lsat->band[i].gain) / lsat->band[i].K1;
			       lsat->band[i].esun = (esun1 + esun2) / 2.;
			     */
			}
		    }
		}
		G_warning("ESUN evaluated from REFLECTANCE_ADDITIVE_FACTOR_BAND");
	    }
	}
    }
    else {
	G_verbose_message("Metadata file is MET file");
	G_verbose_message
	    ("RADIANCE & QUANTIZE from band setting of the metadata file");
	for (i = 0; i < lsat->bands; i++) {
	    sprintf(key, "Band%dGainSetting", lsat->band[i].code);
	    get_mtldata(mtldata, key, value);
	    if (value[0] == '\0') {
		G_warning(key);
		continue;
	    }
	    lsat->band[i].gain = atof(value);
	    sprintf(key, "Band%dBiasSetting", lsat->band[i].code);
	    get_mtldata(mtldata, key, value);
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

    return;
}
