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
    int i;
    for( i = 0 ; i < n && src[i] != '\0' && src[i] != '\"'; i++) dest[i] = src[i];
    dest[i] = '\0';
}

void get_oldformat( const char metadata[], char * key, char value[] )
{
    int i = 0;
    char * ptr = strstr(metadata, key);
    if (ptr != NULL)
    {
        ptr = strstr(ptr, " VALUE ");
        if (ptr != NULL)
        {
            while (*ptr++ != '=') ;
            while (*ptr <= ' ' || *ptr == '\"') *ptr++;
            while (i < MAX_STR && *ptr != '\"' && *ptr >= ' ') value[i++] = *ptr++;
        }
    }
    value[i] = '\0';
}

void get_newformat( const char metadata[], char * key, char value[] )
{
    int i = 0;
    char * ptr = strstr(metadata, key);
    if (ptr != NULL)
    {
        while (*ptr++ != '=') ;
        while (*ptr <= ' ' || *ptr == '\"') *ptr++;
        while (i < MAX_STR && *ptr != '\"' && *ptr > ' ') value[i++] = *ptr++;
    }
    value[i] = '\0';
}


/****************************************************************************
 * PURPOSE:     Read parameters from Landsat metadata files
 *****************************************************************************/

void lsat_metadata( char * metafile, lsat_data * lsat)
{
    FILE *f;
    char mtldata[METADATA_SIZE];
    char key[MAX_STR], value[MAX_STR];
    void (*get_mtldata)( const char [], char *, char [] );
    int i, j, version;

    if ((f = fopen(metafile, "r")) == NULL)
       G_fatal_error(_("Metadata file <%s> not found"), metafile);

    i = fread(mtldata, METADATA_SIZE, 1, f);
    get_mtldata = (strstr(mtldata, " VALUE ") != NULL) ? get_oldformat : get_newformat;
    version = (strstr(mtldata, "QCALMAX_BAND") != NULL) ? 0 : 1;
    
    /* Fill with product metadata */
    get_mtldata(mtldata, "SPACECRAFT_ID", value);
    if( value[0] == '\0' )
    {
        get_mtldata(mtldata, "PLATFORMSHORTNAME", value);
    }
    
    i = 0; while( value[i] && (value[i] < '0' || value[i] > '9') ) i++;
    lsat->number = atoi(value + i);

    get_mtldata(mtldata, "SENSOR_ID", value);
    if( value[0] == '\0' )
    {
        get_mtldata(mtldata, "SENSORSHORTNAME", value);
    }
    chrncpy(lsat->sensor, value, 8);

    get_mtldata(mtldata, "DATE_ACQUIRED", value);
    if( value[0] == '\0' )
    {
        get_mtldata(mtldata, "ACQUISITION_DATE", value);
        if( value[0] == '\0' )
        {
            get_mtldata(mtldata, "CALENDARDATE", value);
        }
    }
    chrncpy(lsat->date, value, 10);

    get_mtldata(mtldata, "FILE_DATE", value);
    if( value[0] == '\0' )
    {
        get_mtldata(mtldata, "CREATION_TIME", value);
        if( value[0] == '\0' )
        {
            get_mtldata(mtldata, "PRODUCTIONDATETIME", value);
        }
    }
    chrncpy(lsat->creation, value, 10);

    get_mtldata(mtldata, "SUN_ELEVATION", value);
    if( value[0] == '\0' )
    {
        get_mtldata(mtldata, "SolarElevation", value);
    }
    lsat->sun_elev = atof(value);
    if( lsat->sun_elev == 0. )
        G_warning("Sun elevation is %f", lsat->sun_elev);

    /* Fill data with the basic sensor parameters */
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
        {
            char * fmt_gain[] = { "BAND%d_GAIN%d ", " GAIN_BAND_%d_VCID_%d" };
            for( i = 1, j = 0; i < 9; i++ )
            {
                sprintf(key, fmt_gain[version], i, 1);
                if (i != 6) key[version == 0 ? 10 : 12] = '\0';
                get_mtldata(mtldata, key,  value + j++);
                if (i == 6)
                {
                    sprintf(key, fmt_gain[version], i, 2);
                    get_mtldata(mtldata, key,  value + j++);
                }
            }
            value[j] = '\0';
            G_debug(1, "ETM+ gain = [%s]", value);
            set_ETM(lsat, value);
            break;
        }
        default:
            G_warning("Unable to recognize satellite platform [%d]", lsat->number);
            break;
    }

    /* Update the information from metadata file, if infile */
//     if( format == NEW_FORMAT )
    if( get_mtldata == get_newformat )
    {
        char * fmt_lmax[]     = { "LMAX_BAND%d",    "RADIANCE_MAXIMUM_BAND_%d" };
        char * fmt_lmin[]     = { "LMIN_BAND%d",    "RADIANCE_MINIMUM_BAND_%d" };
        char * fmt_qcalmax[]  = { "QCALMAX_BAND%d", "QUANTIZE_CAL_MAX_BAND_%d" };
        char * fmt_qcalmin[]  = { "QCALMIN_BAND%d", "QUANTIZE_CAL_MIN_BAND_%d" };

        for (i = 0; i < lsat->bands; i++) {
            sprintf(key, fmt_lmax[version], lsat->band[i].code);
            get_mtldata(mtldata, key, value);
            lsat->band[i].lmax = atof(value);
            sprintf(key, fmt_lmin[version], lsat->band[i].code);
            get_mtldata(mtldata, key, value);
            lsat->band[i].lmin = atof(value);
            sprintf(key, fmt_qcalmax[version], lsat->band[i].code);
            get_mtldata(mtldata, key, value);
            lsat->band[i].qcalmax = atof(value);
            sprintf(key, fmt_qcalmin[version], lsat->band[i].code);
            get_mtldata(mtldata, key, value);
            lsat->band[i].qcalmin = atof(value);
        }
    }

    (void)fclose(f);
    return;
}
