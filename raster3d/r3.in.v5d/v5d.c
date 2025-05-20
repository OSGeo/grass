/* v5d.c */

/* Vis5D version 5.0 */

/*
   Vis5D system for visualizing five dimensional gridded data sets
   Copyright (C) 1990 - 1997 Bill Hibbard, Johan Kellum, Brian Paul,
   Dave Santek, and Andre Battaiola.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* this should be updated when the file version changes */
#define FILE_VERSION "4.3"

/*
 * New grid file format for VIS-5D:
 *
 * The header is a list of tagged items.  Each item has 3 parts:
 *    1. A tag which is a 4-byte integer identifying the type of item.
 *    2. A 4-byte integer indicating how many bytes of data follow.
 *    3. The binary data.
 *
 * If we need to add new information to a file header we just create a
 * new tag and add the code to read/write the information.
 *
 * If we're reading a header and find an unknown tag, we can use the
 * length field to skip ahead to the next tag.  Therefore, the file
 * format is forward (and backward) compatible.
 *
 * Grid data is stored as either:
 *     1-byte unsigned integers  (255=missing)
 *     2-byte unsigned integers  (65535=missing)
 *     4-byte IEEE floats        ( >1.0e30 = missing)
 *
 * All numeric values are stored in big endian order.  All floating point
 * values are in IEEE format.
 */

#include <grass/config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <errno.h>

#include "binio.h"
#include "v5d.h"
#include "vis5d.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

/*
 * Currently defined tags:
 * Note:  the notation a[i] doesn't mean a is an array of i elements,
 * rather it just refers to the ith element of a[].
 *
 * Tags marked as PHASED OUT should be readable but are no longer written.
 * Old tag numbers can't be reused!
 *
 */

/*      TAG NAME        VALUE       DATA (comments)                     */

/*----------------------------------------------------------------------*/
#define TAG_ID              0x5635440a /* hex encoding of "V5D\n"          */

/* general stuff 1000+ */
#define TAG_VERSION         1000 /* char*10 FileVersion              */
#define TAG_NUMTIMES        1001 /* int*4 NumTimes                   */
#define TAG_NUMVARS         1002 /* int*4 NumVars                    */
#define TAG_VARNAME         1003 /* int*4 var; char*10 VarName[var]  */

#define TAG_NR              1004 /* int*4 Nr                         */
#define TAG_NC              1005 /* int*4 Nc                         */
#define TAG_NL              1006 /* int*4 Nl  (Nl for all vars)      */
#define TAG_NL_VAR          1007 /* int*4 var; int*4 Nl[var]         */
#define TAG_LOWLEV_VAR      1008 /* int*4 var; int*4 LowLev[var]     */

#define TAG_TIME            1010 /* int*4 t;  int*4 TimeStamp[t]     */
#define TAG_DATE            1011 /* int*4 t;  int*4 DateStamp[t]     */

#define TAG_MINVAL          1012 /* int*4 var;  real*4 MinVal[var]   */
#define TAG_MAXVAL          1013 /* int*4 var;  real*4 MaxVal[var]   */

#define TAG_COMPRESS        1014 /* int*4 CompressMode; (#bytes/grid) */

#define TAG_UNITS           1015 /* int *4 var; char*20 Units[var]   */

/* vertical coordinate system 2000+ */
#define TAG_VERTICAL_SYSTEM 2000 /* int*4 VerticalSystem             */
#define TAG_VERT_ARGS       2100 /* int*4 n;  real*4 VertArgs[0..n-1] */

#define TAG_BOTTOMBOUND     2001 /* real*4 BottomBound     (PHASED OUT)  */
#define TAG_LEVINC          2002 /* real*4 LevInc      (PHASED OUT)      */
#define TAG_HEIGHT          2003 /* int*4 l;  real*4 Height[l] (PHASED OUT) */

/* projection 3000+ */
#define TAG_PROJECTION      3000 /* int*4 projection:                    */
                                 /*   0 = generic linear                 */
                                 /*   1 = cylindrical equidistant        */
                                 /*   2 = Lambert conformal/Polar Stereo */
                                 /*   3 = rotated equidistant            */
#define TAG_PROJ_ARGS       3100 /* int *4 n;  real*4 ProjArgs[0..n-1]   */

#define TAG_NORTHBOUND      3001 /* real*4 NorthBound       (PHASED OUT) */
#define TAG_WESTBOUND       3002 /* real*4 WestBound        (PHASED OUT) */
#define TAG_ROWINC          3003 /* real*4 RowInc           (PHASED OUT) */
#define TAG_COLINC          3004 /* real*4 ColInc           (PHASED OUT) */
#define TAG_LAT1            3005 /* real*4 Lat1             (PHASED OUT) */
#define TAG_LAT2            3006 /* real*4 Lat2             (PHASED OUT) */
#define TAG_POLE_ROW        3007 /* real*4 PoleRow          (PHASED OUT) */
#define TAG_POLE_COL        3008 /* real*4 PoleCol          (PHASED OUT) */
#define TAG_CENTLON         3009 /* real*4 CentralLon       (PHASED OUT) */
#define TAG_CENTLAT         3010 /* real*4 CentralLat       (PHASED OUT) */
#define TAG_CENTROW         3011 /* real*4 CentralRow       (PHASED OUT) */
#define TAG_CENTCOL         3012 /* real*4 CentralCol       (PHASED OUT) */
#define TAG_ROTATION        3013 /* real*4 Rotation         (PHASED OUT) */

#define TAG_END             9999

/**********************************************************************/

/*****                  Miscellaneous Functions                   *****/

/**********************************************************************/

float pressure_to_height(float pressure)
{
    return (float)DEFAULT_LOG_EXP * log((double)pressure / DEFAULT_LOG_SCALE);
}

float height_to_pressure(float height)
{
    return (float)DEFAULT_LOG_SCALE * exp((double)height / DEFAULT_LOG_EXP);
}

/*
 * Return current file position.
 * Input:  f - file descriptor
 */
static off_t ltell(int f)
{
    return lseek(f, 0, SEEK_CUR);
}

/*
 * Copy up to maxlen characters from src to dst stopping upon whitespace
 * in src.  Terminate dst with null character.
 * Return:  length of dst.
 */
static int copy_string2(char *dst, const char *src, int maxlen)
{
    int i;

    for (i = 0; i < maxlen; i++)
        dst[i] = src[i];
    for (i = maxlen - 1; i >= 0; i--) {
        if (dst[i] == ' ' || i == maxlen - 1)
            dst[i] = 0;
        else
            break;
    }
    return strlen(dst);
}

/*
 * Copy up to maxlen characters from src to dst stopping upon whitespace
 * in src.  Terminate dst with null character.
 * Return:  length of dst.
 */
static int copy_string(char *dst, const char *src, int maxlen)
{
    int i;

    for (i = 0; i < maxlen; i++) {
        if (src[i] == ' ' || i == maxlen - 1) {
            dst[i] = 0;
            break;
        }
        else {
            dst[i] = src[i];
        }
    }
    return i;
}

/*
 * Convert a date from YYDDD format to days since Jan 1, 1900.
 */
int v5dYYDDDtoDays(int yyddd)
{
    int iy, id, idays;

    iy = yyddd / 1000;
    id = yyddd - 1000 * iy;
    if (iy < 50)
        iy += 100; /* WLH 31 July 96 << 31 Dec 99 */
    idays = 365 * iy + (iy - 1) / 4 + id;

    return idays;
}

/*
 * Convert a time from HHMMSS format to seconds since midnight.
 */
int v5dHHMMSStoSeconds(int hhmmss)
{
    int h, m, s;

    h = hhmmss / 10000;
    m = (hhmmss / 100) % 100;
    s = hhmmss % 100;

    return s + m * 60 + h * 60 * 60;
}

/*
 * Convert a day since Jan 1, 1900 to YYDDD format.
 */
int v5dDaysToYYDDD(int days)
{
    int iy, id, iyyddd;

    iy = (4 * days) / 1461;
    id = days - (365 * iy + (iy - 1) / 4);
    if (iy > 99)
        iy = iy - 100; /* WLH 31 July 96 << 31 Dec 99 */
    /* iy = iy + 1900; is the right way to fix this, but requires
       changing all places where dates are printed - procrastinate */
    iyyddd = iy * 1000 + id;

    return iyyddd;
}

/*
 * Convert a time in seconds since midnight to HHMMSS format.
 */
int v5dSecondsToHHMMSS(int seconds)
{
    int hh, mm, ss;

    hh = seconds / (60 * 60);
    mm = (seconds / 60) % 60;
    ss = seconds % 60;
    return hh * 10000 + mm * 100 + ss;
}

void v5dPrintStruct(const v5dstruct *v)
{
    static char day[7][10] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                              "Thursday", "Friday", "Saturday"};
    int time, var, i;
    int maxnl;

    maxnl = 0;
    for (var = 0; var < v->NumVars; var++) {
        if (v->Nl[var] + v->LowLev[var] > maxnl) {
            maxnl = v->Nl[var] + v->LowLev[var];
        }
    }

    if (v->FileFormat == 0) {
        if (v->FileVersion[0] == 0) {
            printf("File format: v5d  version: (4.0 or 4.1)\n");
        }
        else {
            printf("File format: v5d  version: %s\n", v->FileVersion);
        }
    }
    else {
        printf("File format: comp5d  (VIS-5D 3.3 or older)\n");
    }

    if (v->CompressMode == 1) {
        printf("Compression:  1 byte per gridpoint.\n");
    }
    else {
        printf("Compression:  %d bytes per gridpoint.\n", v->CompressMode);
    }
    printf("header size=%d\n", (int)v->FirstGridPos);
    printf("sizeof(v5dstruct)=%d\n", (int)sizeof(v5dstruct));
    printf("\n");

    printf("NumVars = %d\n", v->NumVars);

    printf("Var  Name       Units      Rows  Cols  Levels LowLev  MinVal       "
           "MaxVal\n");
    for (var = 0; var < v->NumVars; var++) {
        printf("%3d  %-10s %-10s %3d   %3d   %3d    %3d", var + 1,
               v->VarName[var], v->Units[var], v->Nr, v->Nc, v->Nl[var],
               v->LowLev[var]);
        if (v->MinVal[var] > v->MaxVal[var]) {
            printf("     MISSING      MISSING\n");
        }
        else {
            printf("     %-12g %-12g\n", v->MinVal[var], v->MaxVal[var]);
        }
    }

    printf("\n");

    printf("NumTimes = %d\n", v->NumTimes);
    printf("Step    Date(YYDDD)    Time(HH:MM:SS)   Day\n");
    for (time = 0; time < v->NumTimes; time++) {
        int i = v->TimeStamp[time];

        printf("%3d        %05d       %5d:%02d:%02d     %s\n", time + 1,
               v->DateStamp[time], i / 10000, (i / 100) % 100, i % 100,
               day[v5dYYDDDtoDays(v->DateStamp[time]) % 7]);
    }
    printf("\n");

    switch (v->VerticalSystem) {
    case 0:
        printf("Generic linear vertical coordinate system:\n");
        printf("\tBottom Bound: %f\n", v->VertArgs[0]);
        printf("\tIncrement between levels:  %f\n", v->VertArgs[1]);
        break;
    case 1:
        printf("Equally spaced levels in km:\n");
        printf("\tBottom Bound: %f\n", v->VertArgs[0]);
        printf("\tIncrement: %f\n", v->VertArgs[1]);
        break;
    case 2:
        printf("Unequally spaced levels in km:\n");
        printf("Level\tHeight(km)\n");
        for (i = 0; i < maxnl; i++) {
            printf("%3d     %10.3f\n", i + 1, v->VertArgs[i]);
        }
        break;
    case 3:
        printf("Unequally spaced levels in mb:\n");
        printf("Level\tPressure(mb)\n");
        for (i = 0; i < maxnl; i++) {
            printf("%3d     %10.3f\n", i + 1,
                   height_to_pressure(v->VertArgs[i]));
        }
        break;
    default:
        printf("Bad VerticalSystem value: %d\n", v->VerticalSystem);
    }
    printf("\n");

    switch (v->Projection) {
    case 0:
        printf("Generic linear projection:\n");
        printf("\tNorth Boundary: %f\n", v->ProjArgs[0]);
        printf("\tWest Boundary: %f\n", v->ProjArgs[1]);
        printf("\tRow Increment: %f\n", v->ProjArgs[2]);
        printf("\tColumn Increment: %f\n", v->ProjArgs[3]);
        break;
    case 1:
        printf("Cylindrical Equidistant projection:\n");
        printf("\tNorth Boundary: %f degrees\n", v->ProjArgs[0]);
        printf("\tWest Boundary: %f degrees\n", v->ProjArgs[1]);
        printf("\tRow Increment: %f degrees\n", v->ProjArgs[2]);
        printf("\tColumn Increment: %f degrees\n", v->ProjArgs[3]);
        /*
           printf("\tSouth Boundary: %f degrees\n",
           v->NorthBound - v->RowInc * (v->Nr-1) );
           printf("\tEast Boundary: %f degrees\n",
           v->WestBound - v->ColInc * (v->Nc-1) );
         */
        break;
    case 2:
        printf("Lambert Conformal projection:\n");
        printf("\tStandard Latitude 1: %f\n", v->ProjArgs[0]);
        printf("\tStandard Latitude 2: %f\n", v->ProjArgs[1]);
        printf("\tNorth/South Pole Row: %f\n", v->ProjArgs[2]);
        printf("\tNorth/South Pole Column: %f\n", v->ProjArgs[3]);
        printf("\tCentral Longitude: %f\n", v->ProjArgs[4]);
        printf("\tColumn Increment: %f km\n", v->ProjArgs[5]);
        break;
    case 3:
        printf("Stereographic:\n");
        printf("\tCenter Latitude: %f\n", v->ProjArgs[0]);
        printf("\tCenter Longitude: %f\n", v->ProjArgs[1]);
        printf("\tCenter Row: %f\n", v->ProjArgs[2]);
        printf("\tCenter Column: %f\n", v->ProjArgs[3]);
        printf("\tColumn Spacing: %f\n", v->ProjArgs[4]);
        break;
    case 4:
        /* WLH 4-21-95 */
        printf("Rotated equidistant projection:\n");
        printf("\tLatitude of grid(0,0): %f\n", v->ProjArgs[0]);
        printf("\tLongitude of grid(0,0): %f\n", v->ProjArgs[1]);
        printf("\tRow Increment: %f degrees\n", v->ProjArgs[2]);
        printf("\tColumn Increment: %f degrees\n", v->ProjArgs[3]);
        printf("\tCenter Latitude: %f\n", v->ProjArgs[4]);
        printf("\tCenter Longitude: %f\n", v->ProjArgs[5]);
        printf("\tRotation: %f degrees\n", v->ProjArgs[6]);
        break;
    default:
        printf("Bad projection number: %d\n", v->Projection);
    }
}

/*
 * Compute the location of a compressed grid within a file.
 * Input:  v - pointer to v5dstruct describing the file header.
 *         time, var - which timestep and variable.
 * Return:  file offset in bytes
 */
static int grid_position(const v5dstruct *v, int time, int var)
{
    int i;
    off_t pos;

    assert(time >= 0);
    assert(var >= 0);
    assert(time < v->NumTimes);
    assert(var < v->NumVars);

    pos = v->FirstGridPos + time * v->SumGridSizes;
    for (i = 0; i < var; i++) {
        pos += v->GridSize[i];
    }

    return pos;
}

/*
 * Compute the ga and gb (de)compression values for a grid.
 * Input:  nr, nc, nl - size of grid
 *         data - the grid data
 *         ga, gb - arrays to store results.
 *         minval, maxval - pointer to floats to return min, max values
 *         compressmode - 1, 2 or 4 bytes per grid point
 * Output:  ga, gb - the (de)compression values
 *          minval, maxval - the min and max grid values
 * Side effect:  the MinVal[var] and MaxVal[var] fields in g may be
 *               updated with new values.
 */
static void compute_ga_gb(int nr, int nc, int nl, const float data[],
                          int compressmode, float ga[], float gb[],
                          float *minval, float *maxval)
{
#ifdef SIMPLE_COMPRESSION
    /*
     * Compute ga, gb values for whole grid.
     */
    int i, allmissing, num;
    float min, max, a, b;

    min = 1.0e30;
    max = -1.0e30;
    num = nr * nc * nl;
    allmissing = 1;
    for (i = 0; i < num; i++) {
        if (!IS_MISSING(data[i])) {
            if (data[i] < min)
                min = data[i];
            if (data[i] > max)
                max = data[i];
            allmissing = 0;
        }
    }
    if (allmissing) {
        a = 1.0;
        b = 0.0;
    }
    else {
        a = (max - min) / 254.0;
        b = min;
    }

    /* return results */
    for (i = 0; i < nl; i++) {
        ga[i] = a;
        gb[i] = b;
    }

    *minval = min;
    *maxval = max;
#else
    /*
     * Compress grid on level-by-level basis.
     */
#define SMALLVALUE -1.0e30
#define BIGVALUE   1.0e30
#define ABS(x)     (((x) < 0.0) ? -(x) : (x))
    float gridmin, gridmax;
    float levmin[MAXLEVELS], levmax[MAXLEVELS];
    float d[MAXLEVELS], dmax;
    float ival, mval;
    int j, k, lev, nrnc;

    nrnc = nr * nc;

    /* find min and max for each layer and the whole grid */
    gridmin = BIGVALUE;
    gridmax = SMALLVALUE;
    j = 0;
    for (lev = 0; lev < nl; lev++) {
        float min, max;

        min = BIGVALUE;
        max = SMALLVALUE;
        for (k = 0; k < nrnc; k++) {
            if (!IS_MISSING(data[j]) && data[j] < min)
                min = data[j];
            if (!IS_MISSING(data[j]) && data[j] > max)
                max = data[j];
            j++;
        }
        if (min < gridmin)
            gridmin = min;
        if (max > gridmax)
            gridmax = max;
        levmin[lev] = min;
        levmax[lev] = max;
    }

    /* WLH 2-2-95 */
#ifdef KLUDGE
    /* if the grid minimum is within delt of 0.0, fudge all values */
    /* within delt of 0.0 to delt, and recalculate mins and maxes */
    {
        float delt;
        int nrncnl = nrnc * nl;

        delt = (gridmax - gridmin) / 100000.0;
        if (ABS(gridmin) < delt && gridmin != 0.0 && compressmode != 4) {

            for (j = 0; j < nrncnl; j++) {
                if (!IS_MISSING(data[j]) && data[j] < delt)
                    data[j] = delt;
            }
            /* re-calculate min and max for each layer and the whole grid */
            gridmin = delt;
            for (lev = 0; lev < nl; lev++) {
                if (ABS(levmin[lev]) < delt)
                    levmin[lev] = delt;
                if (ABS(levmax[lev]) < delt)
                    levmax[lev] = delt;
            }
        }
    }
#endif

    /* find d[lev] and dmax = MAX( d[0], d[1], ... d[nl-1] ) */
    dmax = 0.0;
    for (lev = 0; lev < nl; lev++) {
        if (levmin[lev] >= BIGVALUE && levmax[lev] <= SMALLVALUE) {
            /* all values in the layer are MISSING */
            d[lev] = 0.0;
        }
        else {
            d[lev] = levmax[lev] - levmin[lev];
        }
        if (d[lev] > dmax)
            dmax = d[lev];
    }

    /*** Compute ga (scale) and gb (bias) for each grid level */
    if (dmax == 0.0) {

        /*** Special cases ***/
        if (gridmin == gridmax) {

            /*** whole grid is of same value ***/
            for (lev = 0; lev < nl; lev++) {
                ga[lev] = gridmin;
                gb[lev] = 0.0;
            }
        }
        else {

            /*** every layer is of a single value ***/
            for (lev = 0; lev < nl; lev++) {
                ga[lev] = levmin[lev];
                gb[lev] = 0.0;
            }
        }
    }
    else {

        /*** Normal cases ***/
        if (compressmode == 1) {
#define ORIGINAL
#ifdef ORIGINAL
            ival = dmax / 254.0;
            mval = gridmin;
            for (lev = 0; lev < nl; lev++) {
                ga[lev] = ival;
                gb[lev] = mval + ival * (int)((levmin[lev] - mval) / ival);
            }
#else
            for (lev = 0; lev < nl; lev++) {
                if (d[lev] == 0.0) {
                    ival = 1.0;
                }
                else {
                    ival = d[lev] / 254.0;
                }
                ga[lev] = ival;
                gb[lev] = levmin[lev];
            }
#endif
        }
        else if (compressmode == 2) {
            ival = dmax / 65534.0;
            mval = gridmin;
            for (lev = 0; lev < nl; lev++) {
                ga[lev] = ival;
                gb[lev] = mval + ival * (int)((levmin[lev] - mval) / ival);
            }
        }
        else {
            assert(compressmode == 4);
            for (lev = 0; lev < nl; lev++) {
                ga[lev] = 1.0;
                gb[lev] = 0.0;
            }
        }
    }

    /* update min, max values */
    *minval = gridmin;
    *maxval = gridmax;
#endif
}

/*
 * Compress a 3-D grid from floats to 1-byte unsigned integers.
 * Input: nr, nc, nl - size of grid
 *        compressmode - 1, 2 or 4 bytes per grid point
 *        data - array of [nr*nc*nl] floats
 *        compdata - pointer to array of [nr*nc*nl*compressmode] bytes
 *                   to put results into.
 *        ga, gb - pointer to arrays to put ga and gb decompression values
 *        minval, maxval - pointers to float to return min & max values
 * Output:  compdata - the compressed grid data
 *          ga, gb - the decompression values
 *          minval, maxval - the min and max grid values
 */
void v5dCompressGrid(int nr, int nc, int nl, int compressmode,
                     const float data[], void *compdata, float ga[], float gb[],
                     float *minval, float *maxval)
{
    int nrnc = nr * nc;
    int nrncnl = nr * nc * nl;
    V5Dubyte *compdata1 = (V5Dubyte *)compdata;
    V5Dushort *compdata2 = (V5Dushort *)compdata;

    /* compute ga, gb values */
    compute_ga_gb(nr, nc, nl, data, compressmode, ga, gb, minval, maxval);

    /* compress the data */
    if (compressmode == 1) {
        int i, lev, p;

        p = 0;
        for (lev = 0; lev < nl; lev++) {
            float one_over_a, b;

            b = gb[lev] - 0.0001; /* subtract an epsilon so the int((d-b)/a) */
            /* expr below doesn't get mis-truncated. */
            if (ga[lev] == 0.0) {
                one_over_a = 1.0;
            }
            else {
                one_over_a = 1.0 / ga[lev];
            }
            for (i = 0; i < nrnc; i++, p++) {
                if (IS_MISSING(data[p])) {
                    compdata1[p] = 255;
                }
                else {
                    compdata1[p] = (V5Dubyte)(int)((data[p] - b) * one_over_a);
                    if (compdata1[p] >= 255) {
                        compdata1[p] = (V5Dubyte)(int)(255.0 - .0001);
                    }
                }
            }
        }
    }

    else if (compressmode == 2) {
        int i, lev, p;

        p = 0;
        for (lev = 0; lev < nl; lev++) {
            float one_over_a, b;

            b = gb[lev] - 0.0001;
            if (ga[lev] == 0.0) {
                one_over_a = 1.0;
            }
            else {
                one_over_a = 1.0 / ga[lev];
            }
            for (i = 0; i < nrnc; i++, p++) {
                if (IS_MISSING(data[p])) {
                    compdata2[p] = 65535;
                }
                else {
                    compdata2[p] = (V5Dushort)(int)((data[p] - b) * one_over_a);
                }
            }
            /* TODO: byte-swapping on little endian??? */
        }
    }

    else {
        /* compressmode==4 */
        assert(sizeof(float) == 4);
        memcpy(compdata, data, nrncnl * 4);
        /* TODO: byte-swapping on little endian??? */
    }
}

/*
 * Decompress a 3-D grid from 1-byte integers to 4-byte floats.
 * Input:  nr, nc, nl - size of grid
 *         compdata - array of [nr*nr*nl*compressmode] bytes
 *         ga, gb - arrays of decompression factors
 *         compressmode - 1, 2 or 4 bytes per grid point
 *         data - address to put decompressed values
 * Output:  data - uncompressed floating point data values
 */
void v5dDecompressGrid(int nr, int nc, int nl, int compressmode, void *compdata,
                       float ga[], float gb[], float data[])
{
    int nrnc = nr * nc;
    int nrncnl = nr * nc * nl;
    V5Dubyte *compdata1 = (V5Dubyte *)compdata;
    V5Dushort *compdata2 = (V5Dushort *)compdata;

    if (compressmode == 1) {
        int p, i, lev;

        p = 0;
        for (lev = 0; lev < nl; lev++) {
            float a = ga[lev];
            float b = gb[lev];

            /* WLH 2-2-95 */
            float d, aa;
            int id;

            if (a > 0.0000000001) {
                d = b / a;
                id = floor(d);
                d = d - id;
                aa = a * 0.000001;
            }
            else {
                id = 1;
            }
            if (-254 <= id && id <= 0 && d < aa) {
                for (i = 0; i < nrnc; i++, p++) {
                    if (compdata1[p] == 255) {
                        data[p] = MISSING;
                    }
                    else {
                        data[p] = (float)(int)compdata1[p] * a + b;
                        if (fabs(data[p]) < aa)
                            data[p] = aa;
                    }
                }
            }
            else {
                for (i = 0; i < nrnc; i++, p++) {
                    if (compdata1[p] == 255) {
                        data[p] = MISSING;
                    }
                    else {
                        data[p] = (float)(int)compdata1[p] * a + b;
                    }
                }
            }
            /* end of WLH 2-2-95 */
        }
    }

    else if (compressmode == 2) {
        int p, i, lev;

        p = 0;
        for (lev = 0; lev < nl; lev++) {
            float a = ga[lev];
            float b = gb[lev];

            /* sizeof(V5Dushort)==2! */
            for (i = 0; i < nrnc; i++, p++) {
                if (compdata2[p] == 65535) {
                    data[p] = MISSING;
                }
                else {
                    data[p] = (float)(int)compdata2[p] * a + b;
                }
            }
        }
    }

    else {
        /* compressmode==4 */
        assert(sizeof(float) == 4);
        memcpy(data, compdata, nrncnl * 4);
    }
}

/*
 * Return the size (in bytes) of the 3-D grid specified by time and var.
 * Input:  v - pointer to v5dstruct describing the file
 *         time, var - which timestep and variable
 * Return:  number of data points.
 */
int v5dSizeofGrid(const v5dstruct *v, int time UNUSED, int var)
{
    return v->Nr * v->Nc * v->Nl[var] * v->CompressMode;
}

/*
 * Initialize a v5dstructure to reasonable initial values.
 * Input:  v - pointer to v5dstruct.
 */
void v5dInitStruct(v5dstruct *v)
{
    int i;

    /* set everything to zero */
    memset(v, 0, sizeof(v5dstruct));

    /* special cases */
    v->Projection = -1;
    v->VerticalSystem = -1;

    for (i = 0; i < MAXVARS; i++) {
        v->MinVal[i] = MISSING;
        v->MaxVal[i] = -MISSING;
        v->LowLev[i] = 0;
    }

    /* set file version */
    strcpy(v->FileVersion, FILE_VERSION);

    v->CompressMode = 1;
    v->FileDesc = -1;
}

/*
 * Return a pointer to a new, initialized v5dstruct.
 */
v5dstruct *v5dNewStruct(void)
{
    v5dstruct *v;

    v = (v5dstruct *)G_malloc(sizeof(v5dstruct));
    if (v) {
        v5dInitStruct(v);
    }
    return v;
}

/*
 * Free an initialized v5dstruct. (Todd Plessel)
 */
void v5dFreeStruct(v5dstruct *v)
{
    /*assert( v5dVerifyStruct( v ) ); */
    G_free(v);
    v = 0;
}

/*
 * Do some checking that the information in a v5dstruct is valid.
 * Input:  v - pointer to v5dstruct
 * Return:  1 = g is ok, 0 = g is invalid
 */
int v5dVerifyStruct(const v5dstruct *v)
{
    int var, i, invalid, maxnl;

    invalid = 0;

    if (!v)
        return 0;

    /* Number of variables */
    if (v->NumVars < 0) {
        printf("Invalid number of variables: %d\n", v->NumVars);
        invalid = 1;
    }
    else if (v->NumVars > MAXVARS) {
        printf("Too many variables: %d  (Maximum is %d)\n", v->NumVars,
               MAXVARS);
        invalid = 1;
    }

    /* Variable Names */
    for (i = 0; i < v->NumVars; i++) {
        if (v->VarName[i][0] == 0) {
            printf("Missing variable name: VarName[%d]=\"\"\n", i);
            invalid = 1;
        }
    }

    /* Number of timesteps */
    if (v->NumTimes < 0) {
        printf("Invalid number of timesteps: %d\n", v->NumTimes);
        invalid = 1;
    }
    else if (v->NumTimes > MAXTIMES) {
        printf("Too many timesteps: %d  (Maximum is %d)\n", v->NumTimes,
               MAXTIMES);
        invalid = 1;
    }

    /* Make sure timestamps are increasing */
    for (i = 1; i < v->NumTimes; i++) {
        int date0 = v5dYYDDDtoDays(v->DateStamp[i - 1]);
        int date1 = v5dYYDDDtoDays(v->DateStamp[i]);
        int time0 = v5dHHMMSStoSeconds(v->TimeStamp[i - 1]);
        int time1 = v5dHHMMSStoSeconds(v->TimeStamp[i]);

        if (time1 <= time0 && date1 <= date0) {
            printf("Timestamp for step %d must be later than step %d\n", i,
                   i - 1);
            invalid = 1;
        }
    }

    /* Rows */
    if (v->Nr < 2) {
        printf("Too few rows: %d (2 is minimum)\n", v->Nr);
        invalid = 1;
    }
    else if (v->Nr > MAXROWS) {
        printf("Too many rows: %d (%d is maximum)\n", v->Nr, MAXROWS);
        invalid = 1;
    }

    /* Columns */
    if (v->Nc < 2) {
        printf("Too few columns: %d (2 is minimum)\n", v->Nc);
        invalid = 1;
    }
    else if (v->Nc > MAXCOLUMNS) {
        printf("Too many columns: %d (%d is maximum)\n", v->Nc, MAXCOLUMNS);
        invalid = 1;
    }

    /* Levels */
    maxnl = 0;
    for (var = 0; var < v->NumVars; var++) {
        if (v->LowLev[var] < 0) {
            printf("Low level cannot be negative for var %s: %d\n",
                   v->VarName[var], v->LowLev[var]);
            invalid = 1;
        }
        if (v->Nl[var] < 1) {
            printf("Too few levels for var %s: %d (1 is minimum)\n",
                   v->VarName[var], v->Nl[var]);
            invalid = 1;
        }
        if (v->Nl[var] + v->LowLev[var] > MAXLEVELS) {
            printf("Too many levels for var %s: %d (%d is maximum)\n",
                   v->VarName[var], v->Nl[var] + v->LowLev[var], MAXLEVELS);
            invalid = 1;
        }
        if (v->Nl[var] + v->LowLev[var] > maxnl) {
            maxnl = v->Nl[var] + v->LowLev[var];
        }
    }

    if (v->CompressMode != 1 && v->CompressMode != 2 && v->CompressMode != 4) {
        printf("Bad CompressMode: %d (must be 1, 2 or 4)\n", v->CompressMode);
        invalid = 1;
    }

    switch (v->VerticalSystem) {
    case 0:
    case 1:
        if (v->VertArgs[1] == 0.0) {
            printf("Vertical level increment is zero, must be non-zero\n");
            invalid = 1;
        }
        break;
    case 2:
        /* Check that Height values increase upward */
        for (i = 1; i < maxnl; i++) {
            if (v->VertArgs[i] <= v->VertArgs[i - 1]) {
                printf("Height[%d]=%f <= Height[%d]=%f, level heights must "
                       "increase\n",
                       i, v->VertArgs[i], i - 1, v->VertArgs[i - 1]);
                invalid = 1;
                break;
            }
        }
        break;
    case 3:
        /* Check that Pressure values decrease upward */
        for (i = 1; i < maxnl; i++) {
            if (v->VertArgs[i] <= v->VertArgs[i - 1]) {
                printf("Pressure[%d]=%f >= Pressure[%d]=%f, level pressures "
                       "must decrease\n",
                       i, height_to_pressure(v->VertArgs[i]), i - 1,
                       height_to_pressure(v->VertArgs[i - 1]));
                invalid = 1;
                break;
            }
        }
        break;
    default:
        printf("VerticalSystem = %d, must be in 0..3\n", v->VerticalSystem);
        invalid = 1;
    }

    switch (v->Projection) {
    case 0: /* Generic */
        if (v->ProjArgs[2] == 0.0) {
            printf("Row Increment (ProjArgs[2]) can't be zero\n");
            invalid = 1;
        }
        if (v->ProjArgs[3] == 0.0) {
            printf("Column increment (ProjArgs[3]) can't be zero\n");
            invalid = 1;
        }
        break;
    case 1: /* Cylindrical equidistant */
        if (v->ProjArgs[2] < 0.0) {
            printf("Row Increment (ProjArgs[2]) = %g  (must be >=0.0)\n",
                   v->ProjArgs[2]);
            invalid = 1;
        }
        if (v->ProjArgs[3] <= 0.0) {
            printf("Column Increment (ProjArgs[3]) = %g  (must be >=0.0)\n",
                   v->ProjArgs[3]);
            invalid = 1;
        }
        break;
    case 2: /* Lambert Conformal */
        if (v->ProjArgs[0] < -90.0 || v->ProjArgs[0] > 90.0) {
            printf("Lat1 (ProjArgs[0]) out of range: %g\n", v->ProjArgs[0]);
            invalid = 1;
        }
        if (v->ProjArgs[1] < -90.0 || v->ProjArgs[1] > 90.0) {
            printf("Lat2 (ProjArgs[1] out of range: %g\n", v->ProjArgs[1]);
            invalid = 1;
        }
        if (v->ProjArgs[5] <= 0.0) {
            printf("ColInc (ProjArgs[5]) = %g  (must be >=0.0)\n",
                   v->ProjArgs[5]);
            invalid = 1;
        }
        break;
    case 3: /* Stereographic */
        if (v->ProjArgs[0] < -90.0 || v->ProjArgs[0] > 90.0) {
            printf("Central Latitude (ProjArgs[0]) out of range: ");
            printf("%g  (must be in +/-90)\n", v->ProjArgs[0]);
            invalid = 1;
        }
        if (v->ProjArgs[1] < -180.0 || v->ProjArgs[1] > 180.0) {
            printf("Central Longitude (ProjArgs[1]) out of range: ");
            printf("%g  (must be in +/-180)\n", v->ProjArgs[1]);
            invalid = 1;
        }
        if (v->ProjArgs[4] < 0) {
            printf("Column spacing (ProjArgs[4]) = %g  (must be positive)\n",
                   v->ProjArgs[4]);
            invalid = 1;
        }
        break;
    case 4: /* Rotated */
        /* WLH 4-21-95 */
        if (v->ProjArgs[2] <= 0.0) {
            printf("Row Increment (ProjArgs[2]) = %g  (must be >=0.0)\n",
                   v->ProjArgs[2]);
            invalid = 1;
        }
        if (v->ProjArgs[3] <= 0.0) {
            printf("Column Increment = (ProjArgs[3]) %g  (must be >=0.0)\n",
                   v->ProjArgs[3]);
            invalid = 1;
        }
        if (v->ProjArgs[4] < -90.0 || v->ProjArgs[4] > 90.0) {
            printf("Central Latitude (ProjArgs[4]) out of range: ");
            printf("%g  (must be in +/-90)\n", v->ProjArgs[4]);
            invalid = 1;
        }
        if (v->ProjArgs[5] < -180.0 || v->ProjArgs[5] > 180.0) {
            printf("Central Longitude (ProjArgs[5]) out of range: ");
            printf("%g  (must be in +/-180)\n", v->ProjArgs[5]);
            invalid = 1;
        }
        if (v->ProjArgs[6] < -180.0 || v->ProjArgs[6] > 180.0) {
            printf("Central Longitude (ProjArgs[6]) out of range: ");
            printf("%g  (must be in +/-180)\n", v->ProjArgs[6]);
            invalid = 1;
        }
        break;
    default:
        printf("Projection = %d, must be in 0..4\n", v->Projection);
        invalid = 1;
    }

    return !invalid;
}

/*
 * Get the McIDAS file number and grid number associated with the grid
 * identified by time and var.
 * Input:  v - v5d grid struct
 *         time, var - timestep and variable of grid
 * Output:  mcfile, mcgrid - McIDAS grid file number and grid number
 */
int v5dGetMcIDASgrid(v5dstruct *v, int time, int var, int *mcfile, int *mcgrid)
{
    if (time < 0 || time >= v->NumTimes) {
        printf("Bad time argument to v5dGetMcIDASgrid: %d\n", time);
        return 0;
    }
    if (var < 0 || var >= v->NumVars) {
        printf("Bad var argument to v5dGetMcIDASgrid: %d\n", var);
        return 0;
    }

    *mcfile = (int)v->McFile[time][var];
    *mcgrid = (int)v->McGrid[time][var];
    return 1;
}

/*
 * Set the McIDAS file number and grid number associated with the grid
 * identified by time and var.
 * Input:  v - v5d grid struct
 *         time, var - timestep and variable of grid
 *         mcfile, mcgrid - McIDAS grid file number and grid number
 * Return:  1 = ok, 0 = error (bad time or var)
 */
int v5dSetMcIDASgrid(v5dstruct *v, int time, int var, int mcfile, int mcgrid)
{
    if (time < 0 || time >= v->NumTimes) {
        printf("Bad time argument to v5dSetMcIDASgrid: %d\n", time);
        return 0;
    }
    if (var < 0 || var >= v->NumVars) {
        printf("Bad var argument to v5dSetMcIDASgrid: %d\n", var);
        return 0;
    }

    v->McFile[time][var] = (short)mcfile;
    v->McGrid[time][var] = (short)mcgrid;
    return 1;
}

/**********************************************************************/

/*****                    Input Functions                         *****/

/**********************************************************************/

/*
 * Read the header from a COMP* file and return results in the v5dstruct.
 * Input:  f - the file descriptor
 *         v - pointer to a v5dstruct.
 * Return:  1 = ok, 0 = error.
 */
static int read_comp_header(int f, v5dstruct *v)
{
    unsigned int id;

    /* reset file position to start of file */
    if (lseek(f, 0, SEEK_SET) == -1) {
        G_warning(_("Unable to seek: %s"), strerror(errno));
        return 0;
    }

    /* read file ID */
    read_int4(f, (int *)&id);

    if (id == 0x80808080 || id == 0x80808081) {
        /* Older COMP5D format */
        int gridtimes, gridparms;
        int i, j, it, iv, nl;
        off_t gridsize;
        float hgttop, hgtinc;

        /*char *compgrid; */

        if (id == 0x80808080) {
            /* 20 vars, 300 times */
            gridtimes = 300;
            gridparms = 20;
        }
        else {
            /* 30 vars, 400 times */
            gridtimes = 400;
            gridparms = 30;
        }

        v->FirstGridPos = 12 * 4 + 8 * gridtimes + 4 * gridparms;

        read_int4(f, &v->NumTimes);
        read_int4(f, &v->NumVars);
        read_int4(f, &v->Nr);
        read_int4(f, &v->Nc);
        read_int4(f, &nl);
        for (i = 0; i < v->NumVars; i++) {
            v->Nl[i] = nl;
            v->LowLev[i] = 0;
        }
        read_float4(f, &v->ProjArgs[0]);
        read_float4(f, &v->ProjArgs[1]);
        read_float4(f, &hgttop);
        read_float4(f, &v->ProjArgs[2]);
        read_float4(f, &v->ProjArgs[3]);
        read_float4(f, &hgtinc);
        /*
           for (i=0;i<nl;i++) {
           v->Height[nl-i-1] = hgttop - i * hgtinc;
           }
         */
        v->VerticalSystem = 1;
        v->VertArgs[0] = hgttop - hgtinc * (nl - 1);
        v->VertArgs[1] = hgtinc;

        /* read dates and times */
        for (i = 0; i < gridtimes; i++) {
            read_int4(f, &j);
            v->DateStamp[i] = v5dDaysToYYDDD(j);
        }
        for (i = 0; i < gridtimes; i++) {
            read_int4(f, &j);
            v->TimeStamp[i] = v5dSecondsToHHMMSS(j);
        }

        /* read variable names */
        for (i = 0; i < gridparms; i++) {
            char name[4];

            read_bytes(f, name, 4);
            /* remove trailing spaces, if any */
            for (j = 3; j > 0; j--) {
                if (name[j] == ' ' || name[j] == 0)
                    name[j] = 0;
                else
                    break;
            }
            strncpy(v->VarName[i], name, 4);
            v->VarName[i][4] = 0;
        }

        gridsize = (((off_t)v->Nr * v->Nc * nl + 3) / 4) * 4;
        for (i = 0; i < v->NumVars; i++) {
            v->GridSize[i] = 8 + gridsize;
        }
        v->SumGridSizes = (8 + gridsize) * v->NumVars;

        /* read the grids and their ga,gb values to find min and max values */

        for (i = 0; i < v->NumVars; i++) {
            v->MinVal[i] = 999999.9;
            v->MaxVal[i] = -999999.9;
        }

        /*compgrid = (char *) G_malloc ( gridsize ); */

        for (it = 0; it < v->NumTimes; it++) {
            for (iv = 0; iv < v->NumVars; iv++) {
                float ga, gb;
                float min, max;

                read_float4(f, &ga);
                read_float4(f, &gb);

                /* skip ahead by 'gridsize' bytes */
                if (lseek(f, gridsize, SEEK_CUR) == -1) {
                    G_warning(_("Error: Unexpected end of file, file may be "
                                "corrupted."));
                    return 0;
                }
                min = -(125.0 + gb) / ga;
                max = (125.0 - gb) / ga;
                if (min < v->MinVal[iv])
                    v->MinVal[iv] = min;
                if (max > v->MaxVal[iv])
                    v->MaxVal[iv] = max;
            }
        }

        /*G_free ( compgrid ); */

        /* done */
    }
    else if (id == 0x80808082 || id == 0x80808083) {
        /* Newer COMP5D format */
        int gridtimes;
        off_t gridsize;
        int it, iv, nl, i, j;
        float delta;

        read_int4(f, &gridtimes);
        read_int4(f, &v->NumVars);
        read_int4(f, &v->NumTimes);
        read_int4(f, &v->Nr);
        read_int4(f, &v->Nc);
        read_int4(f, &nl);
        for (i = 0; i < v->NumVars; i++) {
            v->Nl[i] = nl;
        }

        read_float4(f, &v->ProjArgs[2]);
        read_float4(f, &v->ProjArgs[3]);

        /* Read height and determine if equal spacing */
        v->VerticalSystem = 1;
        for (i = 0; i < nl; i++) {
            read_float4(f, &v->VertArgs[i]);
            if (i == 1) {
                delta = v->VertArgs[1] - v->VertArgs[0];
            }
            else if (i > 1) {
                if (delta != (v->VertArgs[i] - v->VertArgs[i - 1])) {
                    v->VerticalSystem = 2;
                }
            }
        }
        if (v->VerticalSystem == 1) {
            v->VertArgs[1] = delta;
        }

        /* read variable names */
        for (iv = 0; iv < v->NumVars; iv++) {
            char name[8];

            read_bytes(f, name, 8);

            /* remove trailing spaces, if any */
            for (j = 7; j > 0; j--) {
                if (name[j] == ' ' || name[j] == 0)
                    name[j] = 0;
                else
                    break;
            }
            strncpy(v->VarName[iv], name, 8);
            v->VarName[iv][8] = 0;
        }

        for (iv = 0; iv < v->NumVars; iv++) {
            read_float4(f, &v->MinVal[iv]);
        }
        for (iv = 0; iv < v->NumVars; iv++) {
            read_float4(f, &v->MaxVal[iv]);
        }
        for (it = 0; it < gridtimes; it++) {
            read_int4(f, &j);
            v->TimeStamp[it] = v5dSecondsToHHMMSS(j);
        }
        for (it = 0; it < gridtimes; it++) {
            read_int4(f, &j);
            v->DateStamp[it] = v5dDaysToYYDDD(j);
        }
        for (it = 0; it < gridtimes; it++) {
            float nlat;

            read_float4(f, &nlat);
            if (it == 0)
                v->ProjArgs[0] = nlat;
        }
        for (it = 0; it < gridtimes; it++) {
            float wlon;

            read_float4(f, &wlon);
            if (it == 0)
                v->ProjArgs[1] = wlon;
        }

        /* calculate grid storage sizes */
        if (id == 0x80808082) {
            gridsize = nl * 2 * 4 + (((off_t)v->Nr * v->Nc * nl + 3) / 4) * 4;
        }
        else {
            /* McIDAS grid and file numbers present */
            gridsize = 8 + nl * 2 * 4 + ((v->Nr * v->Nc * nl + 3) / 4) * 4;
        }
        for (i = 0; i < v->NumVars; i++) {
            v->GridSize[i] = gridsize;
        }
        v->SumGridSizes = gridsize * v->NumVars;

        /* read McIDAS numbers??? */

        /* size (in bytes) of all header info */
        v->FirstGridPos =
            9 * 4 + v->Nl[0] * 4 + v->NumVars * 16 + gridtimes * 16;
    }

    v->CompressMode = 1; /* one byte per grid point */
    v->Projection = 1;   /* Cylindrical equidistant */
    v->FileVersion[0] = 0;

    return 1;
}

/*
 * Read a compressed grid from a COMP* file.
 * Return:  1 = ok, 0 = error.
 */
static int read_comp_grid(v5dstruct *v, int time, int var, float *ga, float *gb,
                          void *compdata)
{
    unsigned int pos;
    V5Dubyte bias;
    int i, n, nl;
    int f;
    V5Dubyte *compdata1 = (V5Dubyte *)compdata;

    f = v->FileDesc;

    /* move to position in file */
    pos = grid_position(v, time, var);
    if (lseek(f, pos, SEEK_SET) == -1) {
        G_warning(_("Unable to seek: %s"), strerror(errno));
        return 0;
    }

    if (v->FileFormat == 0x80808083) {
        /* read McIDAS grid and file numbers */
        int mcfile, mcgrid;

        read_int4(f, &mcfile);
        read_int4(f, &mcgrid);
        v->McFile[time][var] = (short)mcfile;
        v->McGrid[time][var] = (short)mcgrid;
    }

    nl = v->Nl[var];

    if (v->FileFormat == 0x80808080 || v->FileFormat == 0x80808081) {
        /* single ga,gb pair for whole grid */
        float a, b;

        read_float4(f, &a);
        read_float4(f, &b);
        /* convert a, b to new v5d ga, gb values */
        for (i = 0; i < nl; i++) {
            if (a == 0.0) {
                ga[i] = gb[i] = 0.0;
            }
            else {
                gb[i] = (b + 128.0) / -a;
                ga[i] = 1.0 / a;
            }
        }
        bias = 128;
    }
    else {
        /* read ga, gb arrays */
        read_float4_array(f, ga, v->Nl[var]);
        read_float4_array(f, gb, v->Nl[var]);

        /* convert ga, gb values to v5d system */
        for (i = 0; i < nl; i++) {
            if (ga[i] == 0.0) {
                ga[i] = gb[i] = 0.0;
            }
            else {
                /*gb[i] = (gb[i]+125.0) / -ga[i]; */
                gb[i] = (gb[i] + 128.0) / -ga[i];
                ga[i] = 1.0 / ga[i];
            }
        }
        bias = 128; /* 125 ??? */
    }

    /* read compressed grid data */
    n = v->Nr * v->Nc * v->Nl[var];
    if (read_bytes(f, compdata1, n) != n)
        return 0;

    /* convert data values to v5d system */
    n = v->Nr * v->Nc * v->Nl[var];
    for (i = 0; i < n; i++) {
        compdata1[i] += bias;
    }

    return 1;
}

/*
 * Read a v5d file header.
 * Input:  f - file opened for reading.
 *         v - pointer to v5dstruct to store header info into.
 * Return:  1 = ok, 0 = error.
 */
static int read_v5d_header(v5dstruct *v)
{
#define SKIP(N)                                                  \
    do {                                                         \
        if (lseek(f, N, SEEK_CUR) == -1) {                       \
            G_warning(_("Unable to seek: %s"), strerror(errno)); \
            return 0;                                            \
        }                                                        \
    } while (0)
    int end_of_header = 0;
    unsigned int id;
    int idlen, var, numargs;
    int f;

    f = v->FileDesc;

    /* first try to read the header id */
    read_int4(f, (int *)&id);
    read_int4(f, &idlen);
    if (id == TAG_ID && idlen == 0) {
        /* this is a v5d file */
        v->FileFormat = 0;
    }
    else if (id >= 0x80808080 && id <= 0x80808083) {
        /* this is an old COMP* file */
        v->FileFormat = id;
        return read_comp_header(f, v);
    }
    else {
        /* unknown file type */
        printf("Error: not a v5d file\n");
        return 0;
    }

    v->CompressMode = 1; /* default */

    while (!end_of_header) {
        int tag, length;
        int i, var, time, nl, lev;

        if (read_int4(f, &tag) < 1 || read_int4(f, &length) < 1) {
            printf("Error while reading header, premature EOF\n");
            return 0;
        }

        switch (tag) {
        case TAG_VERSION:
            assert(length == 10);
            read_bytes(f, v->FileVersion, 10);
            /* Check if reading a file made by a future version of Vis5D */
            if (strcmp(v->FileVersion, FILE_VERSION) > 0) {
                G_warning("Trying to read a version %s file, you should "
                          "upgrade Vis5D",
                          v->FileVersion);
            }
            break;
        case TAG_NUMTIMES:
            assert(length == 4);
            read_int4(f, &v->NumTimes);
            break;
        case TAG_NUMVARS:
            assert(length == 4);
            read_int4(f, &v->NumVars);
            break;
        case TAG_VARNAME:
            assert(length == 14); /* 1 int + 10 char */
            read_int4(f, &var);
            read_bytes(f, v->VarName[var], 10);
            break;
        case TAG_NR:
            /* Number of rows for all variables */
            assert(length == 4);
            read_int4(f, &v->Nr);
            break;
        case TAG_NC:
            /* Number of columns for all variables */
            assert(length == 4);
            read_int4(f, &v->Nc);
            break;
        case TAG_NL:
            /* Number of levels for all variables */
            assert(length == 4);
            read_int4(f, &nl);
            for (i = 0; i < v->NumVars; i++) {
                v->Nl[i] = nl;
            }
            break;
        case TAG_NL_VAR:
            /* Number of levels for one variable */
            assert(length == 8);
            read_int4(f, &var);
            read_int4(f, &v->Nl[var]);
            break;
        case TAG_LOWLEV_VAR:
            /* Lowest level for one variable */
            assert(length == 8);
            read_int4(f, &var);
            read_int4(f, &v->LowLev[var]);
            break;

        case TAG_TIME:
            /* Time stamp for 1 timestep */
            assert(length == 8);
            read_int4(f, &time);
            read_int4(f, &v->TimeStamp[time]);
            break;
        case TAG_DATE:
            /* Date stamp for 1 timestep */
            assert(length == 8);
            read_int4(f, &time);
            read_int4(f, &v->DateStamp[time]);
            break;

        case TAG_MINVAL:
            /* Minimum value for a variable */
            assert(length == 8);
            read_int4(f, &var);
            read_float4(f, &v->MinVal[var]);
            break;
        case TAG_MAXVAL:
            /* Maximum value for a variable */
            assert(length == 8);
            read_int4(f, &var);
            read_float4(f, &v->MaxVal[var]);
            break;
        case TAG_COMPRESS:
            /* Compress mode */
            assert(length == 4);
            read_int4(f, &v->CompressMode);
            break;
        case TAG_UNITS:
            /* physical units */
            assert(length == 24);
            read_int4(f, &var);
            read_bytes(f, v->Units[var], 20);
            break;

            /*
             * Vertical coordinate system
             */
        case TAG_VERTICAL_SYSTEM:
            assert(length == 4);
            read_int4(f, &v->VerticalSystem);
            if (v->VerticalSystem < 0 || v->VerticalSystem > 3) {
                printf("Error: bad vertical coordinate system: %d\n",
                       v->VerticalSystem);
            }
            break;
        case TAG_VERT_ARGS:
            read_int4(f, &numargs);
            assert(numargs <= MAXVERTARGS);
            read_float4_array(f, v->VertArgs, numargs);
            assert(length == numargs * 4 + 4);
            break;
        case TAG_HEIGHT:
            /* height of a grid level */
            assert(length == 8);
            read_int4(f, &lev);
            read_float4(f, &v->VertArgs[lev]);
            break;
        case TAG_BOTTOMBOUND:
            assert(length == 4);
            read_float4(f, &v->VertArgs[0]);
            break;
        case TAG_LEVINC:
            assert(length == 4);
            read_float4(f, &v->VertArgs[1]);
            break;

            /*
             * Map projection information
             */
        case TAG_PROJECTION:
            assert(length == 4);
            read_int4(f, &v->Projection);
            if (v->Projection < 0 || v->Projection > 4) { /* WLH 4-21-95 */
                printf("Error while reading header, bad projection (%d)\n",
                       v->Projection);
                return 0;
            }
            break;
        case TAG_PROJ_ARGS:
            read_int4(f, &numargs);
            assert(numargs <= MAXPROJARGS);
            read_float4_array(f, v->ProjArgs, numargs);
            assert(length == 4 * numargs + 4);
            break;
        case TAG_NORTHBOUND:
            assert(length == 4);
            if (v->Projection == 0 || v->Projection == 1 ||
                v->Projection == 4) {
                read_float4(f, &v->ProjArgs[0]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_WESTBOUND:
            assert(length == 4);
            if (v->Projection == 0 || v->Projection == 1 ||
                v->Projection == 4) {
                read_float4(f, &v->ProjArgs[1]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_ROWINC:
            assert(length == 4);
            if (v->Projection == 0 || v->Projection == 1 ||
                v->Projection == 4) {
                read_float4(f, &v->ProjArgs[2]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_COLINC:
            assert(length == 4);
            if (v->Projection == 0 || v->Projection == 1 ||
                v->Projection == 4) {
                read_float4(f, &v->ProjArgs[3]);
            }
            else if (v->Projection == 2) {
                read_float4(f, &v->ProjArgs[5]);
            }
            else if (v->Projection == 3) {
                read_float4(f, &v->ProjArgs[4]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_LAT1:
            assert(length == 4);
            if (v->Projection == 2) {
                read_float4(f, &v->ProjArgs[0]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_LAT2:
            assert(length == 4);
            if (v->Projection == 2) {
                read_float4(f, &v->ProjArgs[1]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_POLE_ROW:
            assert(length == 4);
            if (v->Projection == 2) {
                read_float4(f, &v->ProjArgs[2]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_POLE_COL:
            assert(length == 4);
            if (v->Projection == 2) {
                read_float4(f, &v->ProjArgs[3]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_CENTLON:
            assert(length == 4);
            if (v->Projection == 2) {
                read_float4(f, &v->ProjArgs[4]);
            }
            else if (v->Projection == 3) {
                read_float4(f, &v->ProjArgs[1]);
            }
            else if (v->Projection == 4) { /* WLH 4-21-95 */
                read_float4(f, &v->ProjArgs[5]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_CENTLAT:
            assert(length == 4);
            if (v->Projection == 3) {
                read_float4(f, &v->ProjArgs[0]);
            }
            else if (v->Projection == 4) { /* WLH 4-21-95 */
                read_float4(f, &v->ProjArgs[4]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_CENTROW:
            assert(length == 4);
            if (v->Projection == 3) {
                read_float4(f, &v->ProjArgs[2]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_CENTCOL:
            assert(length == 4);
            if (v->Projection == 3) {
                read_float4(f, &v->ProjArgs[3]);
            }
            else {
                SKIP(4);
            }
            break;
        case TAG_ROTATION:
            assert(length == 4);
            if (v->Projection == 4) { /* WLH 4-21-95 */
                read_float4(f, &v->ProjArgs[6]);
            }
            else {
                SKIP(4);
            }
            break;

        case TAG_END:
            /* end of header */
            end_of_header = 1;
            if (lseek(f, length, SEEK_CUR) == -1) {
                G_warning(_("Unable to seek: %s"), strerror(errno));
                return 0;
            }
            break;

        default:
            /* unknown tag, skip to next tag */
            printf("Unknown tag: %d  length=%d\n", tag, length);
            if (lseek(f, length, SEEK_CUR) == -1) {
                G_warning(_("Unable to seek: %s"), strerror(errno));
                return 0;
            }
            break;
        }
    }

    v5dVerifyStruct(v);

    /* Now we're ready to read the grid data */

    /* Save current file pointer */
    v->FirstGridPos = ltell(f);

    /* compute grid sizes */
    v->SumGridSizes = 0;
    for (var = 0; var < v->NumVars; var++) {
        v->GridSize[var] = 8 * v->Nl[var] + v5dSizeofGrid(v, 0, var);
        v->SumGridSizes += v->GridSize[var];
    }

    return 1;
#undef SKIP
}

/*
 * Open a v5d file for reading.
 * Input:  filename - name of v5d file to open
 *         v - pointer to a v5dstruct in which to put header info or NULL
 *             if a struct should be dynamically allocated.
 * Return:  NULL if error, else v or a pointer to a new v5dstruct if v was NULL
 */
v5dstruct *v5dOpenFile(const char *filename, v5dstruct *v)
{
    int fd;

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        /* error */
        return 0;
    }

    if (v) {
        v5dInitStruct(v);
    }
    else {
        v = v5dNewStruct();
        if (!v) {
            return NULL;
        }
    }

    v->FileDesc = fd;
    v->Mode = 'r';
    if (read_v5d_header(v)) {
        return v;
    }
    else {
        return NULL;
    }
}

/*
 * Read a compressed grid from a v5d file.
 * Input:  v - pointer to v5dstruct describing the file
 *         time, var - which timestep and variable
 *         ga, gb - arrays to store grid (de)compression values
 *         compdata - address of where to store compressed grid data.
 * Return:  1 = ok, 0 = error.
 */
int v5dReadCompressedGrid(v5dstruct *v, int time, int var, float *ga, float *gb,
                          void *compdata)
{
    int pos, n, k = 0;

    if (time < 0 || time >= v->NumTimes) {
        printf("Error in v5dReadCompressedGrid: bad timestep argument (%d)\n",
               time);
        return 0;
    }
    if (var < 0 || var >= v->NumVars) {
        printf("Error in v5dReadCompressedGrid: bad var argument (%d)\n", var);
        return 0;
    }

    if (v->FileFormat) {
        /* old COMP* file */
        return read_comp_grid(v, time, var, ga, gb, compdata);
    }

    /* move to position in file */
    pos = grid_position(v, time, var);
    if (lseek(v->FileDesc, pos, SEEK_SET) == -1) {
        G_warning(_("Unable to seek: %s"), strerror(errno));
        return 0;
    }

    /* read ga, gb arrays */
    read_float4_array(v->FileDesc, ga, v->Nl[var]);
    read_float4_array(v->FileDesc, gb, v->Nl[var]);

    /* read compressed grid data */
    n = v->Nr * v->Nc * v->Nl[var];
    if (v->CompressMode == 1) {
        k = read_block(v->FileDesc, compdata, n, 1) == n;
    }
    else if (v->CompressMode == 2) {
        k = read_block(v->FileDesc, compdata, n, 2) == n;
    }
    else if (v->CompressMode == 4) {
        k = read_block(v->FileDesc, compdata, n, 4) == n;
    }
    if (!k) {
        /* error */
        printf("Error in v5dReadCompressedGrid: read failed, bad file?\n");
    }
    return k;

    /*
       n = v->Nr * v->Nc * v->Nl[var] * v->CompressMode;
       if (read( v->FileDesc, compdata, n )==n)
       return 1;
       else
       return 0;
     */
}

/*
 * Read a grid from a v5d file, decompress it and return it.
 * Input:  v - pointer to v5dstruct describing file header
 *         time, var - which timestep and variable.
 *         data - address of buffer to put grid data
 * Output:  data - the grid data
 * Return:  1 = ok, 0 = error.
 */
int v5dReadGrid(v5dstruct *v, int time, int var, float data[])
{
    float ga[MAXLEVELS], gb[MAXLEVELS];
    void *compdata;
    int bytes = 0;

    if (time < 0 || time >= v->NumTimes) {
        printf("Error in v5dReadGrid: bad timestep argument (%d)\n", time);
        return 0;
    }
    if (var < 0 || var >= v->NumVars) {
        printf("Error in v5dReadGrid: bad variable argument (%d)\n", var);
        return 0;
    }

    /* allocate compdata buffer */
    if (v->CompressMode == 1) {
        bytes = v->Nr * v->Nc * v->Nl[var] * (int)sizeof(unsigned char);
    }
    else if (v->CompressMode == 2) {
        bytes = v->Nr * v->Nc * v->Nl[var] * (int)sizeof(unsigned short);
    }
    else if (v->CompressMode == 4) {
        bytes = v->Nr * v->Nc * v->Nl[var] * (int)sizeof(float);
    }
    compdata = (void *)G_malloc(bytes);
    if (!compdata) {
        printf("Error in v5dReadGrid: out of memory (needed %d bytes)\n",
               bytes);
        return 0;
    }

    /* read the compressed data */
    if (!v5dReadCompressedGrid(v, time, var, ga, gb, compdata)) {
        return 0;
    }

    /* decompress the data */
    v5dDecompressGrid(v->Nr, v->Nc, v->Nl[var], v->CompressMode, compdata, ga,
                      gb, data);

    /* free compdata */
    G_free(compdata);
    return 1;
}

/**********************************************************************/

/*****                   Output Functions                         *****/

/**********************************************************************/

static int write_tag(v5dstruct *v, int tag, int length, int newfile)
{
    if (!newfile) {
        /* have to check that there's room in header to write this tagged item
         */
        if (v->CurPos + 8 + length > v->FirstGridPos) {
            printf("Error: out of header space!\n");
            /* Out of header space! */
            return 0;
        }
    }

    if (write_int4(v->FileDesc, tag) == 0)
        return 0;
    if (write_int4(v->FileDesc, length) == 0)
        return 0;
    v->CurPos += 8 + length;
    return 1;
}

/*
 * Write the information in the given v5dstruct as a v5d file header.
 * Note that the current file position is restored when this function
 * returns normally.
 * Input:  f - file already open for writing
 *         v - pointer to v5dstruct
 * Return:  1 = ok, 0 = error.
 */
static int write_v5d_header(v5dstruct *v)
{
    int var, time, filler, maxnl;
    int f;
    int newfile;

    if (v->FileFormat != 0) {
        printf("Error: v5d library can't write comp5d format files.\n");
        return 0;
    }

    f = v->FileDesc;

    if (!v5dVerifyStruct(v))
        return 0;

    /* Determine if we're writing to a new file */
    if (v->FirstGridPos == 0) {
        newfile = 1;
    }
    else {
        newfile = 0;
    }

    /* compute grid sizes */
    v->SumGridSizes = 0;
    for (var = 0; var < v->NumVars; var++) {
        v->GridSize[var] = 8 * v->Nl[var] + v5dSizeofGrid(v, 0, var);
        v->SumGridSizes += v->GridSize[var];
    }

    /* set file pointer to start of file */
    if (lseek(f, 0, SEEK_SET) == -1) {
        G_warning(_("Unable to seek: %s"), strerror(errno));
        return 0;
    }
    v->CurPos = 0;

    /*
     * Write the tagged header info
     */
#define WRITE_TAG(V, T, L)            \
    if (!write_tag(V, T, L, newfile)) \
        return 0;

    /* ID */
    WRITE_TAG(v, TAG_ID, 0);

    /* File Version */
    WRITE_TAG(v, TAG_VERSION, 10);
    write_bytes(f, FILE_VERSION, 10);

    /* Number of timesteps */
    WRITE_TAG(v, TAG_NUMTIMES, 4);
    write_int4(f, v->NumTimes);

    /* Number of variables */
    WRITE_TAG(v, TAG_NUMVARS, 4);
    write_int4(f, v->NumVars);

    /* Names of variables */
    for (var = 0; var < v->NumVars; var++) {
        WRITE_TAG(v, TAG_VARNAME, 14);
        write_int4(f, var);
        write_bytes(f, v->VarName[var], 10);
    }

    /* Physical Units */
    for (var = 0; var < v->NumVars; var++) {
        WRITE_TAG(v, TAG_UNITS, 24);
        write_int4(f, var);
        write_bytes(f, v->Units[var], 20);
    }

    /* Date and time of each timestep */
    for (time = 0; time < v->NumTimes; time++) {
        WRITE_TAG(v, TAG_TIME, 8);
        write_int4(f, time);
        write_int4(f, v->TimeStamp[time]);
        WRITE_TAG(v, TAG_DATE, 8);
        write_int4(f, time);
        write_int4(f, v->DateStamp[time]);
    }

    /* Number of rows */
    WRITE_TAG(v, TAG_NR, 4);
    write_int4(f, v->Nr);

    /* Number of columns */
    WRITE_TAG(v, TAG_NC, 4);
    write_int4(f, v->Nc);

    /* Number of levels, compute maxnl */
    maxnl = 0;
    for (var = 0; var < v->NumVars; var++) {
        WRITE_TAG(v, TAG_NL_VAR, 8);
        write_int4(f, var);
        write_int4(f, v->Nl[var]);
        WRITE_TAG(v, TAG_LOWLEV_VAR, 8);
        write_int4(f, var);
        write_int4(f, v->LowLev[var]);
        if (v->Nl[var] + v->LowLev[var] > maxnl) {
            maxnl = v->Nl[var] + v->LowLev[var];
        }
    }

    /* Min/Max values */
    for (var = 0; var < v->NumVars; var++) {
        WRITE_TAG(v, TAG_MINVAL, 8);
        write_int4(f, var);
        write_float4(f, v->MinVal[var]);
        WRITE_TAG(v, TAG_MAXVAL, 8);
        write_int4(f, var);
        write_float4(f, v->MaxVal[var]);
    }

    /* Compress mode */
    WRITE_TAG(v, TAG_COMPRESS, 4);
    write_int4(f, v->CompressMode);

    /* Vertical Coordinate System */
    WRITE_TAG(v, TAG_VERTICAL_SYSTEM, 4);
    write_int4(f, v->VerticalSystem);
    WRITE_TAG(v, TAG_VERT_ARGS, 4 + 4 * MAXVERTARGS);
    write_int4(f, MAXVERTARGS);
    write_float4_array(f, v->VertArgs, MAXVERTARGS);

    /* Map Projection */
    WRITE_TAG(v, TAG_PROJECTION, 4);
    write_int4(f, v->Projection);
    WRITE_TAG(v, TAG_PROJ_ARGS, 4 + 4 * MAXPROJARGS);
    write_int4(f, MAXPROJARGS);
    write_float4_array(f, v->ProjArgs, MAXPROJARGS);

    /* write END tag */
    if (newfile) {
        /* We're writing to a brand new file.  Reserve 10000 bytes */
        /* for future header growth. */
        WRITE_TAG(v, TAG_END, 10000);
        if (lseek(f, 10000, SEEK_CUR) == -1) {
            G_warning(_("Unable to seek: %s"), strerror(errno));
            return 0;
        }

        /* Let file pointer indicate where first grid is stored */
        v->FirstGridPos = ltell(f);
    }
    else {
        /* we're rewriting a header */
        filler = v->FirstGridPos - ltell(f);
        WRITE_TAG(v, TAG_END, filler - 8);
    }

#undef WRITE_TAG

    return 1;
}

/*
 * Open a v5d file for writing.  If the named file already exists,
 * it will be deleted.
 * Input:  filename - name of v5d file to create.
 *         v - pointer to v5dstruct with the header info to write.
 * Return:  1 = ok, 0 = error.
 */
int v5dCreateFile(const char *filename, v5dstruct *v)
{
    mode_t mask;
    int fd;

    mask = 0666;
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mask);
    if (fd == -1) {
        printf("Error in v5dCreateFile: open failed\n");
        v->FileDesc = -1;
        v->Mode = 0;
        return 0;
    }
    else {
        /* ok */
        v->FileDesc = fd;
        v->Mode = 'w';
        /* write header and return status */
        return write_v5d_header(v);
    }
}

/*
 * Open a v5d file for updating/appending and read the header info.
 * Input:  filename - name of v5d file to open for updating.
 *         v - pointer to v5dstruct in which the file header info will be
 *             put.  If v is NULL a v5dstruct will be allocated and returned.
 * Return:  NULL if error, else v or a pointer to a new v5dstruct if v as NULL
 */
v5dstruct *v5dUpdateFile(const char *filename, v5dstruct *v)
{
    int fd;

    fd = open(filename, O_RDWR);
    if (fd == -1) {
        return NULL;
    }

    if (!v) {
        v = v5dNewStruct();
        if (!v) {
            return NULL;
        }
    }

    v->FileDesc = fd;
    v->Mode = 'w';

    if (read_v5d_header(v)) {
        return v;
    }
    else {
        return NULL;
    }
}

/*
 * Write a compressed grid to a v5d file.
 * Input:  v - pointer to v5dstruct describing the file
 *         time, var - which timestep and variable
 *         ga, gb - the GA and GB (de)compression value arrays
 *         compdata - address of array of compressed data values
 * Return:  1 = ok, 0 = error.
 */
int v5dWriteCompressedGrid(const v5dstruct *v, int time, int var,
                           const float *ga, const float *gb,
                           const void *compdata)
{
    int pos, n, k;

    /* simple error checks */
    if (v->Mode != 'w') {
        printf("Error in v5dWriteCompressedGrid: file opened for reading,");
        printf(" not writing.\n");
        return 0;
    }
    if (time < 0 || time >= v->NumTimes) {
        printf("Error in v5dWriteCompressedGrid: bad timestep argument (%d)\n",
               time);
        return 0;
    }
    if (var < 0 || var >= v->NumVars) {
        printf("Error in v5dWriteCompressedGrid: bad variable argument (%d)\n",
               var);
        return 0;
    }

    /* move to position in file */
    pos = grid_position(v, time, var);
    if (lseek(v->FileDesc, pos, SEEK_SET) < 0) {
        /* lseek failed, return error */
        G_warning(_("Unable to seek: %s"), strerror(errno));
        return 0;
    }

    /* write ga, gb arrays */
    k = 0;
    if (write_float4_array(v->FileDesc, ga, v->Nl[var]) == v->Nl[var] &&
        write_float4_array(v->FileDesc, gb, v->Nl[var]) == v->Nl[var]) {
        /* write compressed grid data (k=1=OK, k=0=Error) */
        n = v->Nr * v->Nc * v->Nl[var];
        if (v->CompressMode == 1) {
            k = write_block(v->FileDesc, compdata, n, 1) == n;
        }
        else if (v->CompressMode == 2) {
            k = write_block(v->FileDesc, compdata, n, 2) == n;
        }
        else if (v->CompressMode == 4) {
            k = write_block(v->FileDesc, compdata, n, 4) == n;
        }
    }

    if (k == 0) {
        /* Error while writing */
        printf("Error in v5dWrite[Compressed]Grid: write failed, disk full?\n");
    }
    return k;

    /*
       n = v->Nr * v->Nc * v->Nl[var] * v->CompressMode;
       if (write_bytes( v->FileDesc, compdata, n )!=n) {
       printf("Error in v5dWrite[Compressed]Grid: write failed, disk full?\n");
       return 0;
       }
       else {
       return 1;
       }
     */
}

/*
 * Compress a grid and write it to a v5d file.
 * Input:  v - pointer to v5dstruct describing the file
 *         time, var - which timestep and variable (starting at 0)
 *         data - address of uncompressed grid data
 * Return:  1 = ok, 0 = error.
 */
int v5dWriteGrid(v5dstruct *v, int time, int var, const float data[])
{
    float ga[MAXLEVELS], gb[MAXLEVELS];
    void *compdata;
    int n, bytes = 0;
    float min, max;

    if (v->Mode != 'w') {
        printf("Error in v5dWriteGrid: file opened for reading,");
        printf(" not writing.\n");
        return 0;
    }
    if (time < 0 || time >= v->NumTimes) {
        printf("Error in v5dWriteGrid: bad timestep argument (%d)\n", time);
        return 0;
    }
    if (var < 0 || var >= v->NumVars) {
        printf("Error in v5dWriteGrid: bad variable argument (%d)\n", var);
        return 0;
    }

    /* allocate compdata buffer */
    if (v->CompressMode == 1) {
        bytes = v->Nr * v->Nc * v->Nl[var] * (int)sizeof(unsigned char);
    }
    else if (v->CompressMode == 2) {
        bytes = v->Nr * v->Nc * v->Nl[var] * (int)sizeof(unsigned short);
    }
    else if (v->CompressMode == 4) {
        bytes = v->Nr * v->Nc * v->Nl[var] * (int)sizeof(float);
    }
    compdata = (void *)G_malloc(bytes);
    if (!compdata) {
        printf("Error in v5dWriteGrid: out of memory (needed %d bytes)\n",
               bytes);
        return 0;
    }

    /* compress the grid data */
    v5dCompressGrid(v->Nr, v->Nc, v->Nl[var], v->CompressMode, data, compdata,
                    ga, gb, &min, &max);

    /* update min and max value */
    if (min < v->MinVal[var])
        v->MinVal[var] = min;
    if (max > v->MaxVal[var])
        v->MaxVal[var] = max;

    /* write the compressed grid */
    n = v5dWriteCompressedGrid(v, time, var, ga, gb, compdata);

    /* free compdata */
    G_free(compdata);

    return n;
}

/*
 * Close a v5d file which was opened with open_v5d_file() or
 * create_v5d_file().
 * Input: f - file descriptor
 * Return:  1 = ok, 0 = error
 */
int v5dCloseFile(v5dstruct *v)
{
    int status = 1;

    if (v->Mode == 'w') {
        /* rewrite header because writing grids updates the minval and */
        /* maxval fields */
        if (lseek(v->FileDesc, 0, SEEK_SET) == -1) {
            G_warning(_("Unable to seek: %s"), strerror(errno));
            return 0;
        }
        status = write_v5d_header(v);
        if (lseek(v->FileDesc, 0, SEEK_END) == -1) {
            G_warning(_("Unable to seek: %s"), strerror(errno));
            return 0;
        }
        close(v->FileDesc);
    }
    else if (v->Mode == 'r') {
        /* just close the file */
        close(v->FileDesc);
    }
    else {
        printf("Error in v5dCloseFile: bad v5dstruct argument\n");
        return 0;
    }
    v->FileDesc = -1;
    v->Mode = 0;
    return status;
}

/**********************************************************************/

/*****           Simple v5d file writing functions.               *****/

/**********************************************************************/

static v5dstruct *Simple = NULL;

/*
 * Create a new v5d file specifying both a map projection and vertical
 * coordinate system.  See README file for argument details.
 * Return:  1 = ok, 0 = error.
 */
int v5dCreate(const char *name, int numtimes, int numvars, int nr, int nc,
              const int nl[], const char varname[MAXVARS][10],
              const int timestamp[], const int datestamp[], int compressmode,
              int projection, const float proj_args[], int vertical,
              const float vert_args[])
{
    int var, time, maxnl, i;

    /* initialize the v5dstruct */
    Simple = v5dNewStruct();

    Simple->NumTimes = numtimes;
    Simple->NumVars = numvars;
    Simple->Nr = nr;
    Simple->Nc = nc;
    maxnl = nl[0];
    for (var = 0; var < numvars; var++) {
        if (nl[var] > maxnl) {
            maxnl = nl[var];
        }
        Simple->Nl[var] = nl[var];
        Simple->LowLev[var] = 0;
        strncpy(Simple->VarName[var], varname[var], 10);
        Simple->VarName[var][9] = 0;
    }

    /* time and date for each timestep */
    for (time = 0; time < numtimes; time++) {
        Simple->TimeStamp[time] = timestamp[time];
        Simple->DateStamp[time] = datestamp[time];
    }

    Simple->CompressMode = compressmode;

    /* Map projection and vertical coordinate system */
    Simple->Projection = projection;
    memcpy(Simple->ProjArgs, proj_args, MAXPROJARGS * sizeof(float));

    Simple->VerticalSystem = vertical;
    if (vertical == 3) {
        /* convert pressures to heights */
        for (i = 0; i < MAXVERTARGS; i++) {
            if (vert_args[i] > 0.000001) {
                Simple->VertArgs[i] = pressure_to_height(vert_args[i]);
            }
            else
                Simple->VertArgs[i] = 0.0;
        }
    }
    else {
        memcpy(Simple->VertArgs, vert_args, MAXVERTARGS * sizeof(float));
    }

    /* create the file */
    if (v5dCreateFile(name, Simple) == 0) {
        printf("Error in v5dCreateSimpleFile: unable to create %s\n", name);
        return 0;
    }
    else {
        return 1;
    }
}

/*
 * Create a new v5d file using minimal information.
 * Return:  1 = ok, 0 = error.  See README file for argument details.
 */
int v5dCreateSimple(const char *name, int numtimes, int numvars, int nr, int nc,
                    int nl, const char varname[MAXVARS][10],
                    const int timestamp[], const int datestamp[],
                    float northlat, float latinc, float westlon, float loninc,
                    float bottomhgt, float hgtinc)
{
    int nlvar[MAXVARS];
    int compressmode, projection, vertical;
    float proj_args[100], vert_args[MAXLEVELS];
    int i;

    for (i = 0; i < numvars; i++) {
        nlvar[i] = nl;
    }

    compressmode = 1;

    projection = 1;
    proj_args[0] = northlat;
    proj_args[1] = westlon;
    proj_args[2] = latinc;
    proj_args[3] = loninc;

    vertical = 1;
    vert_args[0] = bottomhgt;
    vert_args[1] = hgtinc;

    return v5dCreate(name, numtimes, numvars, nr, nc, nlvar, varname, timestamp,
                     datestamp, compressmode, projection, proj_args, vertical,
                     vert_args);
}

/*
 * Set lowest levels for each variable (other than default of 0).
 * Input: lowlev - array [NumVars] of ints
 * Return:  1 = ok, 0 = error
 */
int v5dSetLowLev(int lowlev[])
{
    int var;

    if (Simple) {
        for (var = 0; var < Simple->NumVars; var++) {
            Simple->LowLev[var] = lowlev[var];
        }
        return 1;
    }
    else {
        printf("Error: must call v5dCreate before v5dSetLowLev\n");
        return 0;
    }
}

/*
 * Set the units for a variable.
 * Input:  var - a variable in [1,NumVars]
 *         units - a string
 * Return:  1 = ok, 0 = error
 */
int v5dSetUnits(int var, const char *units)
{
    if (Simple) {
        if (var >= 1 && var <= Simple->NumVars) {
            strncpy(Simple->Units[var - 1], units, 19);
            Simple->Units[var - 1][19] = 0;
            return 1;
        }
        else {
            printf("Error: bad variable number in v5dSetUnits\n");
            return 0;
        }
    }
    else {
        printf("Error: must call v5dCreate before v5dSetUnits\n");
        return 0;
    }
}

/*
 * Write a grid to a v5d file.
 * Input:  time - timestep in [1,NumTimes]
 *         var - timestep in [1,NumVars]
 *         data - array [nr*nc*nl] of floats
 * Return:  1 = ok, 0 = error
 */
int v5dWrite(int time, int var, const float data[])
{
    if (Simple) {
        if (time < 1 || time > Simple->NumTimes) {
            printf("Error in v5dWrite: bad timestep number: %d\n", time);
            return 0;
        }
        if (var < 1 || var > Simple->NumVars) {
            printf("Error in v5dWrite: bad variable number: %d\n", var);
        }
        return v5dWriteGrid(Simple, time - 1, var - 1, data);
    }
    else {
        printf("Error: must call v5dCreate before v5dWrite\n");
        return 0;
    }
}

/*
 * Close a v5d file after the last grid has been written to it.
 * Return:  1 = ok, 0 = error
 */
int v5dClose(void)
{
    if (Simple) {
        int ok = v5dCloseFile(Simple);

        v5dFreeStruct(Simple);
        return ok;
    }
    else {
        printf("Error: v5dClose: no file to close\n");
        return 0;
    }
}

/**********************************************************************/

/*****                FORTRAN-callable simple output              *****/

/**********************************************************************/

/*
 * Create a v5d file.  See README file for argument descriptions.
 * Return:  1 = ok, 0 = error.
 */
#ifdef UNDERSCORE
int v5dcreate_
#else
int v5dcreate
#endif

    (const char *name, const int *numtimes, const int *numvars, const int *nr,
     const int *nc, const int nl[], const char varname[][10],
     const int timestamp[], const int datestamp[], const int *compressmode,
     const int *projection, const float proj_args[], const int *vertical,
     const float vert_args[])
{
    char filename[100];
    char names[MAXVARS][10];
    int i, maxnl, args;

    /* copy name to filename and remove trailing spaces if any */
    copy_string(filename, name, 100);

    /*
     * Check for uninitialized arguments
     */
    if (*numtimes < 1) {
        printf("Error: numtimes invalid\n");
        return 0;
    }
    if (*numvars < 1) {
        printf("Error: numvars invalid\n");
        return 0;
    }
    if (*nr < 2) {
        printf("Error: nr invalid\n");
        return 0;
    }
    if (*nc < 2) {
        printf("Error: nc invalid\n");
        return 0;
    }
    maxnl = 0;
    for (i = 0; i < *numvars; i++) {
        if (nl[i] < 1) {
            printf("Error: nl(%d) invalid\n", i + 1);
            return 0;
        }
        if (nl[i] > maxnl) {
            maxnl = nl[i];
        }
    }

    for (i = 0; i < *numvars; i++) {
        if (copy_string2(names[i], varname[i], 10) == 0) {
            printf("Error: uninitialized varname(%d)\n", i + 1);
            return 0;
        }
    }

    for (i = 0; i < *numtimes; i++) {
        if (timestamp[i] < 0) {
            printf("Error: times(%d) invalid\n", i + 1);
            return 0;
        }
        if (datestamp[i] < 0) {
            printf("Error: dates(%d) invalid\n", i + 1);
            return 0;
        }
    }

    if (*compressmode != 1 && *compressmode != 2 && *compressmode != 4) {
        printf("Error: compressmode invalid\n");
        return 0;
    }

    switch (*projection) {
    case 0:
        args = 4;
        break;
    case 1:
        args = 0;
        if (IS_MISSING(proj_args[0])) {
            printf("Error: northlat (proj_args(1)) invalid\n");
            return 0;
        }
        if (IS_MISSING(proj_args[1])) {
            printf("Error: westlon (proj_args(2)) invalid\n");
            return 0;
        }
        if (IS_MISSING(proj_args[2])) {
            printf("Error: latinc (proj_args(3)) invalid\n");
            return 0;
        }
        if (IS_MISSING(proj_args[3])) {
            printf("Error: loninc (proj_args(4)) invalid\n");
            return 0;
        }
        break;
    case 2:
        args = 6;
        break;
    case 3:
        args = 5;
        break;
    case 4:
        args = 7;
        break;
    default:
        args = 0;
        printf("Error: projection invalid\n");
        return 0;
    }
    for (i = 0; i < args; i++) {
        if (IS_MISSING(proj_args[i])) {
            printf("Error: proj_args(%d) invalid\n", i + 1);
            return 0;
        }
    }

    switch (*vertical) {
    case 0:
        /* WLH 31 Oct 96  -  just fall through
           args = 4;
           break;
         */
    case 1:
        args = 0;
        if (IS_MISSING(vert_args[0])) {
            printf("Error: bottomhgt (vert_args(1)) invalid\n");
            return 0;
        }
        if (IS_MISSING(vert_args[1])) {
            printf("Error: hgtinc (vert_args(2)) invalid\n");
            return 0;
        }
        break;
    case 2:
    case 3:
        args = maxnl;
        break;
    default:
        args = 0;
        printf("Error: vertical invalid\n");
        return 0;
    }
    for (i = 0; i < args; i++) {
        if (IS_MISSING(vert_args[i])) {
            printf("Error: vert_args(%d) invalid\n", i + 1);
            return 0;
        }
    }

    return v5dCreate(filename, *numtimes, *numvars, *nr, *nc, nl,
                     (const char(*)[10])names, timestamp, datestamp,
                     *compressmode, *projection, proj_args, *vertical,
                     vert_args);
}

/*
 * Create a simple v5d file.  See README file for argument descriptions.
 * Return:  1 = ok, 0 = error.
 */
#ifdef UNDERSCORE
int v5dcreatesimple_
#else
int v5dcreatesimple
#endif

    (const char *name, const int *numtimes, const int *numvars, const int *nr,
     const int *nc, const int *nl, const char varname[][10],
     const int timestamp[], const int datestamp[], const float *northlat,
     const float *latinc, const float *westlon, const float *loninc,
     const float *bottomhgt, const float *hgtinc)
{
    int compressmode, projection, vertical;
    float projarg[100], vertarg[MAXLEVELS];
    int varnl[MAXVARS];
    int i;

    for (i = 0; i < MAXVARS; i++) {
        varnl[i] = *nl;
    }

    compressmode = 1;

    projection = 1;
    projarg[0] = *northlat;
    projarg[1] = *westlon;
    projarg[2] = *latinc;
    projarg[3] = *loninc;

    vertical = 1;
    vertarg[0] = *bottomhgt;
    vertarg[1] = *hgtinc;

#ifdef UNDERSCORE
    return v5dcreate_
#else
    return v5dcreate
#endif
        (name, numtimes, numvars, nr, nc, varnl, varname, timestamp, datestamp,
         &compressmode, &projection, projarg, &vertical, vertarg);
}

/*
 * Set lowest levels for each variable (other than default of 0).
 * Input: lowlev - array [NumVars] of ints
 * Return:  1 = ok, 0 = error
 */
#ifdef UNDERSCORE
int v5dsetlowlev_
#else
int v5dsetlowlev
#endif
    (int *lowlev)
{
    return v5dSetLowLev(lowlev);
}

/*
 * Set the units for a variable.
 * Input: var - variable number in [1,NumVars]
 *        units - a character string
 * Return:  1 = ok, 0 = error
 */
#ifdef UNDERSCORE
int v5dsetunits_
#else
int v5dsetunits
#endif
    (int *var, char *name)
{
    return v5dSetUnits(*var, name);
}

/*
 * Write a grid of data to the file.
 * Input:  time - timestep in [1,NumTimes]
 *         var - timestep in [1,NumVars]
 *         data - array [nr*nc*nl] of floats
 * Return:  1 = ok, 0 = error
 */
#ifdef UNDERSCORE
int v5dwrite_
#else
int v5dwrite
#endif
    (const int *time, const int *var, const float *data)
{
    return v5dWrite(*time, *var, data);
}

/*
 * Specify the McIDAS GR3D file number and grid number which correspond
 * to the grid specified by time and var.
 * Input:  time, var - timestep and variable of grid (starting at 1)
 *         mcfile, mcgrid - McIDAS grid file number and grid number
 * Return:  1 = ok, 0 = error (bad time or var)
 */
#ifdef UNDERSCORE
int v5dmcfile_
#else
int v5dmcfile
#endif
    (const int *time, const int *var, const int *mcfile, const int *mcgrid)
{
    if (*time < 1 || *time > Simple->NumTimes) {
        printf("Bad time argument to v5dSetMcIDASgrid: %d\n", *time);
        return 0;
    }
    if (*var < 1 || *var > Simple->NumVars) {
        printf("Bad var argument to v5dSetMcIDASgrid: %d\n", *var);
        return 0;
    }

    Simple->McFile[*time - 1][*var - 1] = (short)*mcfile;
    Simple->McGrid[*time - 1][*var - 1] = (short)*mcgrid;
    return 1;
}

/*
 * Close a simple v5d file.
 */
#ifdef UNDERSCORE
int v5dclose_(void)
#else
int v5dclose(void)
#endif
{
    return v5dClose();
}
