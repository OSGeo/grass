#include<stdio.h>
#include<stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"
#include "earth_sun.h"

#define ETM_MET_SIZE    5600	/* .met file size  5516 bytes */
#define TM5_MET_SIZE    28700	/* .met file size 28686 bytes */
#define MAX_STR      127

inline void chrncpy(char *dest, char *src, int n)
{
    if (src == NULL)
	n = 1;
    else
	strncpy(dest, src, n);
    dest[n - 1] = '\0';
}

/****************************************************************************
 * PURPOSE:     Read values of Landsat-7 ETM+ from header (.met) file
 *****************************************************************************/
void get_value_met7(const char mettext[], char *text, char value[])
{
    char *ptr;

    value[0] = 0;

    ptr = strstr(mettext, text);
    if (ptr == NULL)
	return;

    while (*ptr++ != '=') ;
    sscanf(ptr, "%s", value);

    return;
}

void met_ETM(char *metfile, lsat_data * lsat)
{
    FILE *f;
    char mettext[ETM_MET_SIZE];
    char name[MAX_STR], value[MAX_STR];
    int i;

    static int band[] = { 1, 2, 3, 4, 5, 6, 6, 7, 8 };
    static int code[] = { 1, 2, 3, 4, 5, 61, 62, 7, 8 };

    static double esun[] =
	{ 1969., 1840., 1551., 1044., 225.7, 0., 82.07, 1368. };

    if ((f = fopen(metfile, "r")) == NULL)
	G_fatal_error(_("Metadata file <%s> not found"), metfile);

    fread(mettext, 1, ETM_MET_SIZE, f);

    /* --------------------------------------- */
    lsat->number = 7;

    get_value_met7(mettext, "SENSOR_ID", value);
    chrncpy(lsat->sensor, value + 1, 5);

    if (lsat->creation[0] == 0) {
	get_value_met7(mettext, "CREATION_TIME", value);
	chrncpy(lsat->creation, value, 11);
    }

    get_value_met7(mettext, "ACQUISITION_DATE", value);
    chrncpy(lsat->date, value, 11);
    lsat->dist_es = earth_sun(lsat->date);

    get_value_met7(mettext, "SUN_ELEVATION", value);
    lsat->sun_elev = atof(value);

    lsat->bands = 9;
    for (i = 0; i < lsat->bands; i++) {
	lsat->band[i].number = *(band + i);
	lsat->band[i].code = *(code + i);
	lsat->band[i].esun = *(esun + lsat->band[i].number - 1);
	snprintf(name, MAX_STR, "LMAX_BAND%d", lsat->band[i].code);
	get_value_met7(mettext, name, value);
	lsat->band[i].lmax = atof(value);
	snprintf(name, MAX_STR, "LMIN_BAND%d", lsat->band[i].code);
	get_value_met7(mettext, name, value);
	lsat->band[i].lmin = atof(value);
	snprintf(name, MAX_STR, "QCALMAX_BAND%d", lsat->band[i].code);
	get_value_met7(mettext, name, value);
	lsat->band[i].qcalmax = atof(value);
	snprintf(name, MAX_STR, "QCALMIN_BAND%d", lsat->band[i].code);
	get_value_met7(mettext, name, value);
	lsat->band[i].qcalmin = atof(value);
	if (lsat->band[i].number == 6) {
	    lsat->band[i].thermal = 1;
	    lsat->band[i].K1 = 666.09;
	    lsat->band[i].K2 = 1282.71;
	}
	else {
	    lsat->band[i].thermal = 0;
	}
    }

    (void)fclose(f);
    return;
}

/****************************************************************************
 * PURPOSE:     Read values of Landsat MSS/TM from header (.met) file
 *****************************************************************************/
void get_value_met(const char mettext[], char *text, char value[])
{
    char *ptr;
    int i;

    value[0] = 0;

    ptr = strstr(mettext, text);
    if (ptr == NULL)
	return;

    ptr = strstr(ptr, " VALUE ");
    if (ptr == NULL)
	return;

    i = 0;
    while (*ptr++ != '\"') ;
    while (*ptr != '\"' && i < MAX_STR)
	value[i++] = *ptr++;
    value[i] = '\0';

    return;
}

void met_TM5(char *metfile, lsat_data * lsat)
{
    FILE *f;
    char mettext[TM5_MET_SIZE];
    char value[MAX_STR];

    /* char metdate[MAX_STR]; */

    if ((f = fopen(metfile, "r")) == NULL)
	G_fatal_error(_("Metadata file <%s> not found"), metfile);

    fread(mettext, 1, TM5_MET_SIZE, f);

    /* --------------------------------------- */
    get_value_met(mettext, "CALENDARDATE", value);
    chrncpy(lsat->date, value, 11);

    if (lsat->creation[0] == 0) {
	get_value_met(mettext, "PRODUCTIONDATETIME", value);
	chrncpy(lsat->creation, value, 11);
    }

    if (lsat->creation[0] == 0)
	G_fatal_error(_("Product creation date not in metadata file <%s>"),
		      metfile);
    G_debug(1, "met_TM5: Product creation date = [%s]", lsat->creation);


    get_value_met(mettext, "SolarElevation", value);
    if (!value)
	G_warning("Unable to read solar elevation from metadata file");
    else
	lsat->sun_elev = atof(value);
    G_debug(1, "met_TM5: value=[%s], SolarElevation = %.2f", value,
	    lsat->sun_elev);


    get_value_met(mettext, "PLATFORMSHORTNAME", value);
    G_debug(1, "met_TM5: PLATFORMSHORTNAME=[%s]", value);
    switch (value[8]) {
    case '1':
	set_MSS1(lsat);
	break;
    case '2':
	set_MSS2(lsat);
	break;
    case '3':
	set_MSS3(lsat);
	break;
    case '4':
	get_value_met(mettext, "SENSORSHORTNAME", value);
	if (value[0] == 'M')
	    set_MSS4(lsat);
	else
	    set_TM4(lsat);
	break;
    case '5':
	get_value_met(mettext, "SENSORSHORTNAME", value);
	if (value[0] == 'M')
	    set_MSS5(lsat);
	else
	    set_TM5(lsat);
	break;
    default:
	G_warning("Unable to recognize satellite platform [%s]", value);
	break;
    }

    (void)fclose(f);
    return;
}



/****************************************************************************
 * PURPOSE:     Read values of Landsat TM5 from header (.mtl) file
 *****************************************************************************/

/****************************************************************************
 * EXPLANATION: This module is a modification of the met_ETM() found before
 *              to allow TM5 from GLOVIS to use .MTL extension that responds
 *              near to perfectly to the .met parser. While L7 files using
 *              .MTL from GLOVIS can be processed as if having .met files
 *              seemlessly, TM5 using .MTL need to read basic info and 
 *              additionally the LMIN, LMAX, QCALMIN, QCALMAX being explicitely
 *              provided in the .MTL as if in a .met file.
 *****************************************************************************/
void mtl_TM5(char *metfile, lsat_data * lsat)
{
    FILE *f;
    char mettext[ETM_MET_SIZE];
    char name[MAX_STR], value[MAX_STR];
    int i;

    static int band[] = { 1, 2, 3, 4, 5, 6, 7 };
    static int code[] = { 1, 2, 3, 4, 5, 6, 7 };

    if ((f = fopen(metfile, "r")) == NULL)
	G_fatal_error(_("Metadata file <%s> not found"), metfile);

    fread(mettext, 1, ETM_MET_SIZE, f);

    /* --------------------------------------- */
    get_value_met7(mettext, "SENSOR_ID", value);
    chrncpy(lsat->sensor, value + 1, 3);

    if (lsat->creation[0] == 0) {
	get_value_met7(mettext, "PRODUCT_CREATION_TIME", value);
	chrncpy(lsat->creation, value, 11);
    }

    get_value_met7(mettext, "ACQUISITION_DATE", value);
    chrncpy(lsat->date, value, 11);
    lsat->dist_es = earth_sun(lsat->date);

    get_value_met7(mettext, "SUN_ELEVATION", value);
    lsat->sun_elev = atof(value);

    /* We still have to initialize most of the info */
    /* So instead of rewriting a new function, we use set_TM5()... */
    set_TM5(lsat);
    /* ... and we rewrite the necessary 'a la Landsat 7' */

    lsat->bands = 7;
    for (i = 0; i < lsat->bands; i++) {
	lsat->band[i].code = *(code + i);
	snprintf(name, MAX_STR, "LMAX_BAND%d", lsat->band[i].code);
	get_value_met7(mettext, name, value);
	lsat->band[i].lmax = atof(value);
	snprintf(name, MAX_STR, "LMIN_BAND%d", lsat->band[i].code);
	get_value_met7(mettext, name, value);
	lsat->band[i].lmin = atof(value);
	snprintf(name, MAX_STR, "QCALMAX_BAND%d", lsat->band[i].code);
	get_value_met7(mettext, name, value);
	lsat->band[i].qcalmax = atof(value);
	snprintf(name, MAX_STR, "QCALMIN_BAND%d", lsat->band[i].code);
	get_value_met7(mettext, name, value);
	lsat->band[i].qcalmin = atof(value);
	if (lsat->band[i].number == 6)
	    lsat->band[i].thermal = 1;
    }
    (void)fclose(f);
    return;
}
