#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"
#include "earth_sun.h"

#define MAX_STR         127
#define METADATA_SIZE   65535  /* MTL.txt file size  65535 bytes */
#define TM5_MET_SIZE    28700  /* .met file size 28686 bytes */


inline void chrncpy(char *dest, char src[], int n)
{
    if (src == NULL)
    {
        dest[0] = '\0';
    }
    else
    {
        int i;
        for (i = 0; i < n && src[i] != '\0' && src[i] != '\"'; i++) dest[i] = src[i];
        dest[i] = '\0';
    }
}


/****************************************************************************
 * PURPOSE:     Read values of Landsat MSS/TM from header (.met) file
 *****************************************************************************/
void get_metdata(const char mettext[], char *text, char value[])
{
    char *ptr;



    ptr = strstr(mettext, text);
    if (ptr == NULL)
    {
        value[0] = 0;
        return;
    }

    ptr = strstr(ptr, " VALUE ");
    if (ptr == NULL) return;

    while (*ptr++ != '\"') ;
    int i = 0;
    while (*ptr != '\"' && i < MAX_STR) value[i++] = *ptr++;
    value[i] = '\0';

    return;
}

void lsat_metdata(char *metfile, lsat_data * lsat)
{
    FILE *f;
    char mettext[TM5_MET_SIZE];
    char name[MAX_STR], value[MAX_STR];

    /* char metdate[MAX_STR]; */

    if ((f = fopen(metfile, "r")) == NULL)
    G_fatal_error(_("Metadata file <%s> not found"), metfile);

    fread(mettext, TM5_MET_SIZE, 1, f);

    /* --------------------------------------- */
    get_metdata(mettext, "PLATFORMSHORTNAME", value);
    chrncpy(name, value + 8, 1);
    lsat->number = atoi(name);

    get_metdata(mettext, "SENSORSHORTNAME", value);
    chrncpy(lsat->sensor, value + 1, 4);

    get_metdata(mettext, "CALENDARDATE", value);
    chrncpy(lsat->date, value, 10);

    if (lsat->creation[0] == 0)
    {
        get_metdata(mettext, "PRODUCTIONDATETIME", value);
        if (!value[0])
            G_fatal_error(_("Product creation date not in metadata file <%s>, input this data in the command line parameters"), metfile);
        chrncpy(lsat->creation, value, 10);
    }

    if (lsat->sun_elev == 0)
    {
        get_metdata(mettext, "SolarElevation", value);
        if (!value[0])
           G_fatal_error(_("Unable to read solar elevation from metadata file in metadata file <%s>, input this data in the command line parameters"), metfile);
        lsat->sun_elev = atof(value);
    }

    /* Fill data with the sensor_XXX functions */
    switch(lsat->number)
    {
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
        default:
            G_warning("Unable to recognize satellite platform [%d]", lsat->number);
            break;
    }

    /* --------------------------------------- */
    (void)fclose(f);
    return;
}


/****************************************************************************
 * PURPOSE:     Read values of Landsat from MTL metadata (MTL.txt) file
 *****************************************************************************/

void get_mtldata(const char mtltext[], char *text, char value[])
{
    char *ptr;

    ptr = strstr(mtltext, text);
    if (ptr == NULL)
    {
        value[0] = '\0';
        return;
    }

    while (*ptr++ != '=') ;
    while (*ptr <= ' ' || *ptr == '\"') *ptr++;
    int i = 0;
    while (i < MAX_STR && *ptr != '\"' && *ptr > ' ') value[i++] = *ptr++;
    value[i] = '\0';

    return;
}

void lsat_mtldata(char *mtlfile, lsat_data * lsat)
{
    FILE *f;
    char mtldata[METADATA_SIZE];
    char name[MAX_STR], value[MAX_STR];
    int i;

    if ((f = fopen(mtlfile, "r")) == NULL)
       G_fatal_error(_("Metadata file <%s> not found"), mtlfile);

    fread(mtldata, METADATA_SIZE, 1, f);

    /* --------------------------------------- */
    get_mtldata(mtldata, "SPACECRAFT_ID", value);
    chrncpy(name, value + 7, 1);
    lsat->number = atoi(name);

    get_mtldata(mtldata, "SENSOR_ID", value);
    chrncpy(lsat->sensor, value, 4);

    get_mtldata(mtldata, "ACQUISITION_DATE", value);
    chrncpy(lsat->date, value, 10);

    get_mtldata(mtldata, "CREATION_TIME", value);
    chrncpy(lsat->creation, value, 10);

    get_mtldata(mtldata, "SUN_ELEVATION", value);
    lsat->sun_elev = atof(value);

    /* Fill data with the sensor_XXX functions */
    switch(lsat->number)
    {
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
            get_mtldata(mtldata, "BAND1_GAIN",  value);
            get_mtldata(mtldata, "BAND2_GAIN",  value + 1);
            get_mtldata(mtldata, "BAND3_GAIN",  value + 2);
            get_mtldata(mtldata, "BAND4_GAIN",  value + 3);
            get_mtldata(mtldata, "BAND5_GAIN",  value + 4);
            get_mtldata(mtldata, "BAND6_GAIN1", value + 5);
            get_mtldata(mtldata, "BAND6_GAIN2", value + 6);
            get_mtldata(mtldata, "BAND7_GAIN",  value + 7);
            get_mtldata(mtldata, "BAND8_GAIN",  value + 8);
            value[9] = '\0';
            set_ETM(lsat, value);
            break;
        default:
            G_warning("Unable to recognize satellite platform [%d]", lsat->number);
            break;
    }

    /* Update the information from metadata file */
    for (i = 0; i < lsat->bands; i++) {
        snprintf(name, MAX_STR, "LMAX_BAND%d", lsat->band[i].code);
        get_mtldata(mtldata, name, value);
        lsat->band[i].lmax = atof(value);
        snprintf(name, MAX_STR, "LMIN_BAND%d", lsat->band[i].code);
        get_mtldata(mtldata, name, value);
        lsat->band[i].lmin = atof(value);
        snprintf(name, MAX_STR, "QCALMAX_BAND%d", lsat->band[i].code);
        get_mtldata(mtldata, name, value);
        lsat->band[i].qcalmax = atof(value);
        snprintf(name, MAX_STR, "QCALMIN_BAND%d", lsat->band[i].code);
        get_mtldata(mtldata, name, value);
        lsat->band[i].qcalmin = atof(value);
    }
    /* --------------------------------------- */

    (void)fclose(f);
    return;
}

