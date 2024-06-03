/******************************************************************************
 *
 * Project:  Shapelib
 * Purpose:  Implementation of core Shapefile read/write functions.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, 2001, Frank Warmerdam
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * Copyright (c) 2011-2019, Even Rouault <even dot rouault at spatialys.com>
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * Copyright (c) 2011-2019, Even Rouault <even dot rouault at spatialys.com>
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * Copyright (c) 2011-2019, Even Rouault <even dot rouault at spatialys.com>
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
 * Copyright (c) 2011-2019, Even Rouault <even dot rouault at spatialys.com>
=======
>>>>>>> osgeo-main
 * Copyright (c) 2011-2013, Even Rouault <even dot rouault at mines-paris dot
 *org>
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
 * Copyright (c) 2011-2013, Even Rouault <even dot rouault at mines-paris dot
 *org>
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
 * Copyright (c) 2011-2019, Even Rouault <even dot rouault at spatialys.com>
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
 * Copyright (c) 2011-2013, Even Rouault <even dot rouault at mines-paris dot
 *org>
=======
 * Copyright (c) 2011-2019, Even Rouault <even dot rouault at spatialys.com>
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
 *
 * This software is available under the following "MIT Style" license,
 * or at the option of the licensee under the LGPL (see COPYING).  This
 * option is discussed in more detail in shapelib.html.
 *
 * --
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#include "shapefil.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SHP_CVSID("$Id$")
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
#ifndef CPL_UNUSED
#define CPL_UNUSED
#endif
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
#ifndef CPL_UNUSED
#define CPL_UNUSED
#endif
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======

>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
#ifndef CPL_UNUSED
#define CPL_UNUSED
#endif
=======

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
typedef unsigned char uchar;

#if UINT_MAX == 65535
typedef unsigned long int32;
#else
typedef unsigned int int32;
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#define ByteCopy(a, b, c) memcpy(b, a, c)
#ifndef MAX
#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)
#endif

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
#ifndef USE_CPL
#if defined(_MSC_VER)
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#elif defined(WIN32) || defined(_WIN32)
=======
#if defined(WIN32) || defined(_WIN32)
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
#ifndef snprintf
#define snprintf _snprintf
#endif
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
#ifndef USE_CPL
#if defined(_MSC_VER)
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#elif defined(WIN32) || defined(_WIN32)
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif
#endif

#ifndef CPL_UNUSED
#if defined(__GNUC__) && __GNUC__ >= 4
#define CPL_UNUSED __attribute((__unused__))
#else
#define CPL_UNUSED
#endif
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
#endif
#endif

#ifndef CPL_UNUSED
#if defined(__GNUC__) && __GNUC__ >= 4
#define CPL_UNUSED __attribute((__unused__))
#else
#define CPL_UNUSED
#endif
<<<<<<< HEAD
=======
#if defined(WIN32) || defined(_WIN32)
#ifndef snprintf
#define snprintf _snprintf
#endif
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
#endif

#if defined(CPL_LSB)
#define bBigEndian false
#elif defined(CPL_MSB)
#define bBigEndian true
#else
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
static bool bBigEndian;
#endif

#ifdef __cplusplus
#define STATIC_CAST(type, x) static_cast<type>(x)
#define SHPLIB_NULLPTR       nullptr
#else
#define STATIC_CAST(type, x) ((type)(x))
#define SHPLIB_NULLPTR       NULL
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
static int bBigEndian;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
static int bBigEndian;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
static int bBigEndian;
=======
static bool bBigEndian;
#endif

#ifdef __cplusplus
#define STATIC_CAST(type, x) static_cast<type>(x)
#define SHPLIB_NULLPTR       nullptr
#else
#define STATIC_CAST(type, x) ((type)(x))
#define SHPLIB_NULLPTR       NULL
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
#endif

/************************************************************************/
/*                              SwapWord()                              */
/*                                                                      */
/*      Swap a 2, 4 or 8 byte word.                                     */
/************************************************************************/

static void SwapWord(int length, void *wordP)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    for (int i = 0; i < length / 2; i++) {
        const uchar temp = STATIC_CAST(uchar *, wordP)[i];
        STATIC_CAST(uchar *, wordP)
        [i] = STATIC_CAST(uchar *, wordP)[length - i - 1];
        STATIC_CAST(uchar *, wordP)[length - i - 1] = temp;
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    int i;
    uchar temp;

    for (i = 0; i < length / 2; i++) {
        temp = ((uchar *)wordP)[i];
        ((uchar *)wordP)[i] = ((uchar *)wordP)[length - i - 1];
        ((uchar *)wordP)[length - i - 1] = temp;
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    for (int i = 0; i < length / 2; i++) {
        const uchar temp = STATIC_CAST(uchar *, wordP)[i];
        STATIC_CAST(uchar *, wordP)
        [i] = STATIC_CAST(uchar *, wordP)[length - i - 1];
        STATIC_CAST(uchar *, wordP)[length - i - 1] = temp;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }
}

/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/

static void *SfRealloc(void *pMem, int nNewSize)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    if (pMem == SHPLIB_NULLPTR)
        return malloc(nNewSize);
    else
        return realloc(pMem, nNewSize);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (pMem == NULL)
        return ((void *)malloc(nNewSize));
    else
        return ((void *)realloc(pMem, nNewSize));
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (pMem == NULL)
        return ((void *)malloc(nNewSize));
    else
        return ((void *)realloc(pMem, nNewSize));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    if (pMem == SHPLIB_NULLPTR)
        return malloc(nNewSize);
    else
        return realloc(pMem, nNewSize);
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
}

/************************************************************************/
/*                          SHPWriteHeader()                            */
/*                                                                      */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*      Write out a header for the .shp and .shx files as well as the    */
/*    contents of the index (.shx) file.                */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*      Write out a header for the .shp and .shx files as well as the    */
/*    contents of the index (.shx) file.                */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*      Write out a header for the .shp and .shx files as well as the    */
/*    contents of the index (.shx) file.                */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
/*      Write out a header for the .shp and .shx files as well as the    */
/*    contents of the index (.shx) file.                */
=======
>>>>>>> osgeo-main
/*      Write out a header for the .shp and .shx files as well as the   */
/*      contents of the index (.shx) file.                              */

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
/*      Write out a header for the .shp and .shx files as well as the   */
/*      contents of the index (.shx) file.                              */

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
/*      Write out a header for the .shp and .shx files as well as the    */
/*    contents of the index (.shx) file.                */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
/*      Write out a header for the .shp and .shx files as well as the   */
/*      contents of the index (.shx) file.                              */

=======
/*      Write out a header for the .shp and .shx files as well as the    */
/*    contents of the index (.shx) file.                */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
/************************************************************************/

void SHPAPI_CALL SHPWriteHeader(SHPHandle psSHP)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    uchar abyHeader[100];
    int i;
    int32 i32;
    double dValue;
    int32 *panSHX;

    if (psSHP->fpSHX == NULL) {
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psSHP->sHooks.Error("SHPWriteHeader failed : SHX file is closed");
        return;
    }

    /* -------------------------------------------------------------------- */
    /*      Prepare header block for .shp file.                             */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main

    uchar abyHeader[100] = {0};
    abyHeader[2] = 0x27; /* magic cookie */
    abyHeader[3] = 0x0a;

    int32 i32 = psSHP->nFileSize / 2; /* file size */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    ByteCopy(&i32, abyHeader + 24, 4);
    if (!bBigEndian)
        SwapWord(4, abyHeader + 24);

    i32 = 1000; /* version */
    ByteCopy(&i32, abyHeader + 28, 4);
    if (bBigEndian)
        SwapWord(4, abyHeader + 28);

    i32 = psSHP->nShapeType; /* shape type */
    ByteCopy(&i32, abyHeader + 32, 4);
    if (bBigEndian)
        SwapWord(4, abyHeader + 32);

    double dValue = psSHP->adBoundsMin[0]; /* set bounds */
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    for (i = 0; i < 100; i++)
        abyHeader[i] = 0;

    abyHeader[2] = 0x27; /* magic cookie */
    abyHeader[3] = 0x0a;

    i32 = psSHP->nFileSize / 2; /* file size */
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======

    uchar abyHeader[100] = {0};
    abyHeader[2] = 0x27; /* magic cookie */
    abyHeader[3] = 0x0a;

    int32 i32 = psSHP->nFileSize / 2; /* file size */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    ByteCopy(&i32, abyHeader + 24, 4);
    if (!bBigEndian)
        SwapWord(4, abyHeader + 24);

    i32 = 1000; /* version */
    ByteCopy(&i32, abyHeader + 28, 4);
    if (bBigEndian)
        SwapWord(4, abyHeader + 28);

    i32 = psSHP->nShapeType; /* shape type */
    ByteCopy(&i32, abyHeader + 32, 4);
    if (bBigEndian)
        SwapWord(4, abyHeader + 32);

<<<<<<< HEAD
<<<<<<< HEAD
    double dValue = psSHP->adBoundsMin[0]; /* set bounds */
=======
    dValue = psSHP->adBoundsMin[0]; /* set bounds */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    for (i = 0; i < 100; i++)
        abyHeader[i] = 0;
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    uchar abyHeader[100] = {0};
    abyHeader[2] = 0x27; /* magic cookie */
    abyHeader[3] = 0x0a;

    int32 i32 = psSHP->nFileSize / 2; /* file size */
    ByteCopy(&i32, abyHeader + 24, 4);
    if (!bBigEndian)
        SwapWord(4, abyHeader + 24);

    i32 = 1000; /* version */
    ByteCopy(&i32, abyHeader + 28, 4);
    if (bBigEndian)
        SwapWord(4, abyHeader + 28);

    i32 = psSHP->nShapeType; /* shape type */
    ByteCopy(&i32, abyHeader + 32, 4);
    if (bBigEndian)
        SwapWord(4, abyHeader + 32);

<<<<<<< HEAD
    dValue = psSHP->adBoundsMin[0]; /* set bounds */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    double dValue = psSHP->adBoundsMin[0]; /* set bounds */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    dValue = psSHP->adBoundsMin[0]; /* set bounds */
=======
    double dValue = psSHP->adBoundsMin[0]; /* set bounds */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    ByteCopy(&dValue, abyHeader + 36, 8);
    if (bBigEndian)
        SwapWord(8, abyHeader + 36);

    dValue = psSHP->adBoundsMin[1];
    ByteCopy(&dValue, abyHeader + 44, 8);
    if (bBigEndian)
        SwapWord(8, abyHeader + 44);

    dValue = psSHP->adBoundsMax[0];
    ByteCopy(&dValue, abyHeader + 52, 8);
    if (bBigEndian)
        SwapWord(8, abyHeader + 52);

    dValue = psSHP->adBoundsMax[1];
    ByteCopy(&dValue, abyHeader + 60, 8);
    if (bBigEndian)
        SwapWord(8, abyHeader + 60);

    dValue = psSHP->adBoundsMin[2]; /* z */
    ByteCopy(&dValue, abyHeader + 68, 8);
    if (bBigEndian)
        SwapWord(8, abyHeader + 68);

    dValue = psSHP->adBoundsMax[2];
    ByteCopy(&dValue, abyHeader + 76, 8);
    if (bBigEndian)
        SwapWord(8, abyHeader + 76);

    dValue = psSHP->adBoundsMin[3]; /* m */
    ByteCopy(&dValue, abyHeader + 84, 8);
    if (bBigEndian)
        SwapWord(8, abyHeader + 84);

    dValue = psSHP->adBoundsMax[3];
    ByteCopy(&dValue, abyHeader + 92, 8);
    if (bBigEndian)
        SwapWord(8, abyHeader + 92);

    /* -------------------------------------------------------------------- */
    /*      Write .shp file header.                                         */
    /* -------------------------------------------------------------------- */
    if (psSHP->sHooks.FSeek(psSHP->fpSHP, 0, 0) != 0 ||
        psSHP->sHooks.FWrite(abyHeader, 100, 1, psSHP->fpSHP) != 1) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failure writing .shp header: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psSHP->sHooks.Error("Failure writing .shp header");
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        psSHP->sHooks.Error("Failure writing .shp header");
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        psSHP->sHooks.Error("Failure writing .shp header");
=======
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failure writing .shp header: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        return;
    }

    /* -------------------------------------------------------------------- */
    /*      Prepare, and write .shx file header.                            */
    /* -------------------------------------------------------------------- */
    i32 = (psSHP->nRecords * 2 * sizeof(int32) + 100) / 2; /* file size */
    ByteCopy(&i32, abyHeader + 24, 4);
    if (!bBigEndian)
        SwapWord(4, abyHeader + 24);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
>>>>>>> osgeo-main

    if (psSHP->sHooks.FSeek(psSHP->fpSHX, 0, 0) != 0 ||
        psSHP->sHooks.FWrite(abyHeader, 100, 1, psSHP->fpSHX) != 1) {
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failure writing .shx header: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);

=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

    if (psSHP->sHooks.FSeek(psSHP->fpSHX, 0, 0) != 0 ||
        psSHP->sHooks.FWrite(abyHeader, 100, 1, psSHP->fpSHX) != 1) {
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failure writing .shx header: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);

=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

    if (psSHP->sHooks.FSeek(psSHP->fpSHX, 0, 0) != 0 ||
        psSHP->sHooks.FWrite(abyHeader, 100, 1, psSHP->fpSHX) != 1) {
<<<<<<< HEAD
        psSHP->sHooks.Error("Failure writing .shx header");
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    if (psSHP->sHooks.FSeek(psSHP->fpSHX, 0, 0) != 0 ||
        psSHP->sHooks.FWrite(abyHeader, 100, 1, psSHP->fpSHX) != 1) {
        psSHP->sHooks.Error("Failure writing .shx header");
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======

    if (psSHP->sHooks.FSeek(psSHP->fpSHX, 0, 0) != 0 ||
        psSHP->sHooks.FWrite(abyHeader, 100, 1, psSHP->fpSHX) != 1) {
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failure writing .shx header: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);

<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        return;
    }

    /* -------------------------------------------------------------------- */
    /*      Write out the .shx contents.                                    */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    int32 *panSHX =
        STATIC_CAST(int32 *, malloc(sizeof(int32) * 2 * psSHP->nRecords));
    if (panSHX == SHPLIB_NULLPTR) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    int32 *panSHX =
        STATIC_CAST(int32 *, malloc(sizeof(int32) * 2 * psSHP->nRecords));
    if (panSHX == SHPLIB_NULLPTR) {
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    panSHX = (int32 *)malloc(sizeof(int32) * 2 * psSHP->nRecords);
    if (panSHX == NULL) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    panSHX = (int32 *)malloc(sizeof(int32) * 2 * psSHP->nRecords);
    if (panSHX == NULL) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    panSHX = (int32 *)malloc(sizeof(int32) * 2 * psSHP->nRecords);
    if (panSHX == NULL) {
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    int32 *panSHX =
        STATIC_CAST(int32 *, malloc(sizeof(int32) * 2 * psSHP->nRecords));
    if (panSHX == SHPLIB_NULLPTR) {
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psSHP->sHooks.Error("Failure allocatin panSHX");
        return;
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psSHP->nRecords; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psSHP->nRecords; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psSHP->nRecords; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psSHP->nRecords; i++) {
=======
>>>>>>> osgeo-main
    for (i = 0; i < psSHP->nRecords; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    for (i = 0; i < psSHP->nRecords; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    for (int i = 0; i < psSHP->nRecords; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    for (i = 0; i < psSHP->nRecords; i++) {
=======
    for (int i = 0; i < psSHP->nRecords; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        panSHX[i * 2] = psSHP->panRecOffset[i] / 2;
        panSHX[i * 2 + 1] = psSHP->panRecSize[i] / 2;
        if (!bBigEndian)
            SwapWord(4, panSHX + i * 2);
        if (!bBigEndian)
            SwapWord(4, panSHX + i * 2 + 1);
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    if (STATIC_CAST(int, psSHP->sHooks.FWrite(panSHX, sizeof(int32) * 2,
                                              psSHP->nRecords, psSHP->fpSHX)) !=
        psSHP->nRecords) {
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failure writing .shx contents: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    if ((int)psSHP->sHooks.FWrite(panSHX, sizeof(int32) * 2, psSHP->nRecords,
                                  psSHP->fpSHX) != psSHP->nRecords) {
        psSHP->sHooks.Error("Failure writing .shx contents");
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if ((int)psSHP->sHooks.FWrite(panSHX, sizeof(int32) * 2, psSHP->nRecords,
                                  psSHP->fpSHX) != psSHP->nRecords) {
        psSHP->sHooks.Error("Failure writing .shx contents");
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if ((int)psSHP->sHooks.FWrite(panSHX, sizeof(int32) * 2, psSHP->nRecords,
                                  psSHP->fpSHX) != psSHP->nRecords) {
        psSHP->sHooks.Error("Failure writing .shx contents");
=======
    if (STATIC_CAST(int, psSHP->sHooks.FWrite(panSHX, sizeof(int32) * 2,
                                              psSHP->nRecords, psSHP->fpSHX)) !=
        psSHP->nRecords) {
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failure writing .shx contents: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    free(panSHX);

    /* -------------------------------------------------------------------- */
    /*      Flush to disk.                                                  */
    /* -------------------------------------------------------------------- */
    psSHP->sHooks.FFlush(psSHP->fpSHP);
    psSHP->sHooks.FFlush(psSHP->fpSHX);
}

/************************************************************************/
/*                              SHPOpen()                               */
/************************************************************************/

SHPHandle SHPAPI_CALL SHPOpen(const char *pszLayer, const char *pszAccess)
{
    SAHooks sHooks;

    SASetupDefaultHooks(&sHooks);

    return SHPOpenLL(pszLayer, pszAccess, &sHooks);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
}

/************************************************************************/
/*                      SHPGetLenWithoutExtension()                     */
/************************************************************************/

static int SHPGetLenWithoutExtension(const char *pszBasename)
{
    const int nLen = STATIC_CAST(int, strlen(pszBasename));
    for (int i = nLen - 1;
         i > 0 && pszBasename[i] != '/' && pszBasename[i] != '\\'; i--) {
        if (pszBasename[i] == '.') {
            return i;
        }
    }
    return nLen;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                      SHPGetLenWithoutExtension()                     */
/************************************************************************/

static int SHPGetLenWithoutExtension(const char *pszBasename)
{
    const int nLen = STATIC_CAST(int, strlen(pszBasename));
    for (int i = nLen - 1;
         i > 0 && pszBasename[i] != '/' && pszBasename[i] != '\\'; i--) {
        if (pszBasename[i] == '.') {
            return i;
        }
    }
    return nLen;
<<<<<<< HEAD
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
}

/************************************************************************/
/*                              SHPOpen()                               */
/*                                                                      */
/*      Open the .shp and .shx files based on the basename of the       */
/*      files or either file name.                                      */
/************************************************************************/

SHPHandle SHPAPI_CALL SHPOpenLL(const char *pszLayer, const char *pszAccess,
                                SAHooks *psHooks)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    char *pszFullname, *pszBasename;
    SHPHandle psSHP;

    uchar *pabyBuf;
    int i;
    double dValue;
    int bLazySHXLoading = FALSE;
    size_t nFullnameLen;

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /* -------------------------------------------------------------------- */
    /*      Ensure the access string is one of the legal ones.  We          */
    /*      ensure the result string indicates binary to avoid common       */
    /*      problems on Windows.                                            */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    bool bLazySHXLoading = false;
    if (strcmp(pszAccess, "rb+") == 0 || strcmp(pszAccess, "r+b") == 0 ||
        strcmp(pszAccess, "r+") == 0) {
        pszAccess = "r+b";
    }
    else {
        bLazySHXLoading = strchr(pszAccess, 'l') != SHPLIB_NULLPTR;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
    bool bLazySHXLoading = false;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    if (strcmp(pszAccess, "rb+") == 0 || strcmp(pszAccess, "r+b") == 0 ||
        strcmp(pszAccess, "r+") == 0) {
        pszAccess = "r+b";
<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (strcmp(pszAccess, "rb+") == 0 || strcmp(pszAccess, "r+b") == 0 ||
        strcmp(pszAccess, "r+") == 0)
        pszAccess = "r+b";
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    else {
        bLazySHXLoading = strchr(pszAccess, 'l') != NULL;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    }
    else {
        bLazySHXLoading = strchr(pszAccess, 'l') != SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    bool bLazySHXLoading = false;
    if (strcmp(pszAccess, "rb+") == 0 || strcmp(pszAccess, "r+b") == 0 ||
        strcmp(pszAccess, "r+") == 0) {
        pszAccess = "r+b";
    }
    else {
        bLazySHXLoading = strchr(pszAccess, 'l') != SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        pszAccess = "rb";
    }

/* -------------------------------------------------------------------- */
/*  Establish the byte order on this machine.           */
/* -------------------------------------------------------------------- */
#if !defined(bBigEndian)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
>>>>>>> osgeo-main
    {
        int i = 1;
        if (*((uchar *)&i) == 1)
            bBigEndian = false;
        else
            bBigEndian = true;
    }
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    i = 1;
    if (*((uchar *)&i) == 1)
        bBigEndian = FALSE;
    else
        bBigEndian = TRUE;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    {
        int i = 1;
        if (*((uchar *)&i) == 1)
            bBigEndian = false;
        else
            bBigEndian = true;
    }
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
#endif

    /* -------------------------------------------------------------------- */
    /*  Initialize the info structure.                  */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    SHPHandle psSHP = STATIC_CAST(SHPHandle, calloc(sizeof(SHPInfo), 1));
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    SHPHandle psSHP = STATIC_CAST(SHPHandle, calloc(sizeof(SHPInfo), 1));
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    SHPHandle psSHP = STATIC_CAST(SHPHandle, calloc(sizeof(SHPInfo), 1));
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    SHPHandle psSHP = STATIC_CAST(SHPHandle, calloc(sizeof(SHPInfo), 1));
=======
>>>>>>> osgeo-main
    psSHP = (SHPHandle)calloc(sizeof(SHPInfo), 1);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    psSHP = (SHPHandle)calloc(sizeof(SHPInfo), 1);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    SHPHandle psSHP = STATIC_CAST(SHPHandle, calloc(sizeof(SHPInfo), 1));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    psSHP = (SHPHandle)calloc(sizeof(SHPInfo), 1);
=======
    SHPHandle psSHP = STATIC_CAST(SHPHandle, calloc(sizeof(SHPInfo), 1));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

    psSHP->bUpdated = FALSE;
    memcpy(&(psSHP->sHooks), psHooks, sizeof(SAHooks));

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    /*  Open the .shp and .shx files.  Note that files pulled from  */
    /*  a PC to Unix with upper case filenames won't work!      */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHP", 5);
        psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
<<<<<<< HEAD
    }

    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        free(psSHP);
        free(pszFullname);

        return SHPLIB_NULLPTR;
    }

=======
    }

    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        free(psSHP);
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /*  Compute the base (layer) name.  If there is any extension   */
    /*  on the passed in filename we will strip it off.         */
    /* -------------------------------------------------------------------- */
    pszBasename = (char *)malloc(strlen(pszLayer) + 5);
    strcpy(pszBasename, pszLayer);
    for (i = (int)strlen(pszBasename) - 1;
         i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' &&
         pszBasename[i] != '\\';
         i--) {
<<<<<<< HEAD
    }

    if (pszBasename[i] == '.')
        pszBasename[i] = '\0';

    /* -------------------------------------------------------------------- */
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /*  Open the .shp and .shx files.  Note that files pulled from  */
    /*  a PC to Unix with upper case filenames won't work!      */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHP", 5);
        psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

<<<<<<< HEAD
    if (psSHP->fpSHP == NULL) {
        size_t nMessageLen = strlen(pszBasename) * 2 + 256;
        char *pszMessage = (char *)malloc(nMessageLen);

=======
    }

    if (pszBasename[i] == '.')
        pszBasename[i] = '\0';

    /* -------------------------------------------------------------------- */
    /*  Open the .shp and .shx files.  Note that files pulled from  */
    /*  a PC to Unix with upper case filenames won't work!      */
    /* -------------------------------------------------------------------- */
    nFullnameLen = strlen(pszBasename) + 5;
    pszFullname = (char *)malloc(nFullnameLen);
    snprintf(pszFullname, nFullnameLen, "%s.shp", pszBasename);
    psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHP == NULL) {
        snprintf(pszFullname, nFullnameLen, "%s.SHP", pszBasename);
        psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHP == NULL) {
        size_t nMessageLen = strlen(pszBasename) * 2 + 256;
        char *pszMessage = (char *)malloc(nMessageLen);

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);
<<<<<<< HEAD
=======

        free(psSHP);
        free(pszFullname);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

<<<<<<< HEAD
        free(psSHP);
        free(pszBasename);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
        free(pszFullname);

        return SHPLIB_NULLPTR;
    }

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
    psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHX", 5);
        psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen,
                 "Unable to open %s.shx or %s.SHX. "
                 "Set SHAPE_RESTORE_SHX config option to YES to restore or "
                 "create it.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psSHP->sHooks.FClose(psSHP->fpSHP);
        free(psSHP);
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    free(pszFullname);
<<<<<<< HEAD

    /* -------------------------------------------------------------------- */
    /*  Read the file size from the SHP file.               */
    /* -------------------------------------------------------------------- */
    uchar *pabyBuf = STATIC_CAST(uchar *, malloc(100));
    if (psSHP->sHooks.FRead(pabyBuf, 100, 1, psSHP->fpSHP) != 1) {
        psSHP->sHooks.Error(".shp file is unreadable, or corrupt.");
        psSHP->sHooks.FClose(psSHP->fpSHP);
        psSHP->sHooks.FClose(psSHP->fpSHX);
        free(pabyBuf);
        free(psSHP);

=======
=======
=======
        free(pszBasename);
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    /*  Open the .shp and .shx files.  Note that files pulled from  */
    /*  a PC to Unix with upper case filenames won't work!      */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHP", 5);
        psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        free(psSHP);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
=======
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /*  Compute the base (layer) name.  If there is any extension   */
    /*  on the passed in filename we will strip it off.         */
    /* -------------------------------------------------------------------- */
    pszBasename = (char *)malloc(strlen(pszLayer) + 5);
    strcpy(pszBasename, pszLayer);
    for (i = (int)strlen(pszBasename) - 1;
         i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' &&
         pszBasename[i] != '\\';
         i--) {
<<<<<<< HEAD
    }

    if (pszBasename[i] == '.')
        pszBasename[i] = '\0';

    /* -------------------------------------------------------------------- */
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /*  Open the .shp and .shx files.  Note that files pulled from  */
    /*  a PC to Unix with upper case filenames won't work!      */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHP", 5);
        psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

<<<<<<< HEAD
    if (psSHP->fpSHP == NULL) {
        size_t nMessageLen = strlen(pszBasename) * 2 + 256;
        char *pszMessage = (char *)malloc(nMessageLen);

=======
    }

    if (pszBasename[i] == '.')
        pszBasename[i] = '\0';

    /* -------------------------------------------------------------------- */
    /*  Open the .shp and .shx files.  Note that files pulled from  */
    /*  a PC to Unix with upper case filenames won't work!      */
    /* -------------------------------------------------------------------- */
    nFullnameLen = strlen(pszBasename) + 5;
    pszFullname = (char *)malloc(nFullnameLen);
    snprintf(pszFullname, nFullnameLen, "%s.shp", pszBasename);
    psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHP == NULL) {
        snprintf(pszFullname, nFullnameLen, "%s.SHP", pszBasename);
        psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHP == NULL) {
        size_t nMessageLen = strlen(pszBasename) * 2 + 256;
        char *pszMessage = (char *)malloc(nMessageLen);

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);
<<<<<<< HEAD
=======

        free(psSHP);
        free(pszFullname);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

<<<<<<< HEAD
        free(psSHP);
        free(pszBasename);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        free(pszFullname);

        return SHPLIB_NULLPTR;
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    snprintf(pszFullname, nFullnameLen, "%s.shx", pszBasename);
    psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHX == NULL) {
        snprintf(pszFullname, nFullnameLen, "%s.SHX", pszBasename);
        psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHX == NULL) {
        size_t nMessageLen = strlen(pszBasename) * 2 + 256;
        char *pszMessage = (char *)malloc(nMessageLen);

        snprintf(pszMessage, nMessageLen,
                 "Unable to open %s.shx or %s.SHX."
                 "Try --config SHAPE_RESTORE_SHX true to restore or create it",
                 pszBasename, pszBasename);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psSHP->sHooks.FClose(psSHP->fpSHP);
        free(psSHP);
        free(pszBasename);
        free(pszFullname);
        return (NULL);
    }

    free(pszFullname);
    free(pszBasename);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
    psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHX", 5);
        psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen,
                 "Unable to open %s.shx or %s.SHX. "
                 "Set SHAPE_RESTORE_SHX config option to YES to restore or "
                 "create it.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psSHP->sHooks.FClose(psSHP->fpSHP);
        free(psSHP);
<<<<<<< HEAD
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    free(pszFullname);
=======
=======
        free(pszBasename);
=======
    /*  Open the .shp and .shx files.  Note that files pulled from  */
    /*  a PC to Unix with upper case filenames won't work!      */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHP", 5);
        psSHP->fpSHP = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHP == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        free(psSHP);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
        free(pszFullname);

        return SHPLIB_NULLPTR;
    }

<<<<<<< HEAD
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    snprintf(pszFullname, nFullnameLen, "%s.shx", pszBasename);
    psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHX == NULL) {
        snprintf(pszFullname, nFullnameLen, "%s.SHX", pszBasename);
        psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHX == NULL) {
        size_t nMessageLen = strlen(pszBasename) * 2 + 256;
        char *pszMessage = (char *)malloc(nMessageLen);

        snprintf(pszMessage, nMessageLen,
                 "Unable to open %s.shx or %s.SHX."
                 "Try --config SHAPE_RESTORE_SHX true to restore or create it",
                 pszBasename, pszBasename);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psSHP->sHooks.FClose(psSHP->fpSHP);
        free(psSHP);
        free(pszBasename);
        free(pszFullname);
        return (NULL);
    }

    free(pszFullname);
    free(pszBasename);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
    psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHX", 5);
        psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen,
                 "Unable to open %s.shx or %s.SHX. "
                 "Set SHAPE_RESTORE_SHX config option to YES to restore or "
                 "create it.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psSHP->sHooks.FClose(psSHP->fpSHP);
        free(psSHP);
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    free(pszFullname);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    /* -------------------------------------------------------------------- */
    /*  Read the file size from the SHP file.               */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
    uchar *pabyBuf = STATIC_CAST(uchar *, malloc(100));
    if (psSHP->sHooks.FRead(pabyBuf, 100, 1, psSHP->fpSHP) != 1) {
        psSHP->sHooks.Error(".shp file is unreadable, or corrupt.");
        psSHP->sHooks.FClose(psSHP->fpSHP);
        psSHP->sHooks.FClose(psSHP->fpSHX);
        free(pabyBuf);
        free(psSHP);

        return SHPLIB_NULLPTR;
    }

    psSHP->nFileSize = (STATIC_CAST(unsigned int, pabyBuf[24]) << 24) |
                       (pabyBuf[25] << 16) | (pabyBuf[26] << 8) | pabyBuf[27];
    if (psSHP->nFileSize < UINT_MAX / 2)
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    pabyBuf = (uchar *)malloc(100);
    psSHP->sHooks.FRead(pabyBuf, 100, 1, psSHP->fpSHP);

=======
    snprintf(pszFullname, nFullnameLen, "%s.shx", pszBasename);
=======
        return SHPLIB_NULLPTR;
    }

    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHX", 5);
        psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen,
                 "Unable to open %s.shx or %s.SHX. "
                 "Set SHAPE_RESTORE_SHX config option to YES to restore or "
                 "create it.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psSHP->sHooks.FClose(psSHP->fpSHP);
        free(psSHP);
<<<<<<< HEAD
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    free(pszFullname);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    /* -------------------------------------------------------------------- */
    /*  Read the file size from the SHP file.               */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
    uchar *pabyBuf = STATIC_CAST(uchar *, malloc(100));
    if (psSHP->sHooks.FRead(pabyBuf, 100, 1, psSHP->fpSHP) != 1) {
        psSHP->sHooks.Error(".shp file is unreadable, or corrupt.");
        psSHP->sHooks.FClose(psSHP->fpSHP);
        psSHP->sHooks.FClose(psSHP->fpSHX);
        free(pabyBuf);
        free(psSHP);

>>>>>>> osgeo-main
        return SHPLIB_NULLPTR;
    }

    psSHP->nFileSize = (STATIC_CAST(unsigned int, pabyBuf[24]) << 24) |
                       (pabyBuf[25] << 16) | (pabyBuf[26] << 8) | pabyBuf[27];
    if (psSHP->nFileSize < UINT_MAX / 2)
<<<<<<< HEAD
=======
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    pabyBuf = (uchar *)malloc(100);
    psSHP->sHooks.FRead(pabyBuf, 100, 1, psSHP->fpSHP);

=======
    snprintf(pszFullname, nFullnameLen, "%s.shx", pszBasename);
=======
        return SHPLIB_NULLPTR;
    }

    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHX", 5);
        psSHP->fpSHX = psSHP->sHooks.FOpen(pszFullname, pszAccess);
    }

    if (psSHP->fpSHX == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen,
                 "Unable to open %s.shx or %s.SHX. "
                 "Set SHAPE_RESTORE_SHX config option to YES to restore or "
                 "create it.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psSHP->sHooks.FClose(psSHP->fpSHP);
        free(psSHP);
=======
>>>>>>> osgeo-main
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    free(pszFullname);

    /* -------------------------------------------------------------------- */
    /*  Read the file size from the SHP file.               */
    /* -------------------------------------------------------------------- */
    uchar *pabyBuf = STATIC_CAST(uchar *, malloc(100));
    if (psSHP->sHooks.FRead(pabyBuf, 100, 1, psSHP->fpSHP) != 1) {
        psSHP->sHooks.Error(".shp file is unreadable, or corrupt.");
        psSHP->sHooks.FClose(psSHP->fpSHP);
        psSHP->sHooks.FClose(psSHP->fpSHX);
        free(pabyBuf);
        free(psSHP);

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    psSHP->nFileSize =
        ((unsigned int)pabyBuf[24] * 256 * 256 * 256 +
         (unsigned int)pabyBuf[25] * 256 * 256 +
         (unsigned int)pabyBuf[26] * 256 + (unsigned int)pabyBuf[27]);
    if (psSHP->nFileSize < 0xFFFFFFFFU / 2)
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
    uchar *pabyBuf = STATIC_CAST(uchar *, malloc(100));
    if (psSHP->sHooks.FRead(pabyBuf, 100, 1, psSHP->fpSHP) != 1) {
        psSHP->sHooks.Error(".shp file is unreadable, or corrupt.");
        psSHP->sHooks.FClose(psSHP->fpSHP);
        psSHP->sHooks.FClose(psSHP->fpSHX);
        free(pabyBuf);
        free(psSHP);

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        return SHPLIB_NULLPTR;
    }

    psSHP->nFileSize = (STATIC_CAST(unsigned int, pabyBuf[24]) << 24) |
                       (pabyBuf[25] << 16) | (pabyBuf[26] << 8) | pabyBuf[27];
    if (psSHP->nFileSize < UINT_MAX / 2)
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psSHP->nFileSize *= 2;
    else
        psSHP->nFileSize = (UINT_MAX / 2) * 2;

    /* -------------------------------------------------------------------- */
    /*  Read SHX file Header info                                           */
    /* -------------------------------------------------------------------- */
    if (psSHP->sHooks.FRead(pabyBuf, 100, 1, psSHP->fpSHX) != 1 ||
        pabyBuf[0] != 0 || pabyBuf[1] != 0 || pabyBuf[2] != 0x27 ||
        (pabyBuf[3] != 0x0a && pabyBuf[3] != 0x0d)) {
        psSHP->sHooks.Error(".shx file is unreadable, or corrupt.");
        psSHP->sHooks.FClose(psSHP->fpSHP);
        psSHP->sHooks.FClose(psSHP->fpSHX);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        free(pabyBuf);
        free(psSHP);

        return SHPLIB_NULLPTR;
    }

    psSHP->nRecords = pabyBuf[27] | (pabyBuf[26] << 8) | (pabyBuf[25] << 16) |
                      ((pabyBuf[24] & 0x7F) << 24);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
        free(pabyBuf);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        free(psSHP);

        return SHPLIB_NULLPTR;
    }

<<<<<<< HEAD
    psSHP->nRecords = pabyBuf[27] + pabyBuf[26] * 256 +
                      pabyBuf[25] * 256 * 256 +
                      (pabyBuf[24] & 0x7F) * 256 * 256 * 256;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        free(psSHP);

        return (NULL);
    }

    psSHP->nRecords = pabyBuf[27] + pabyBuf[26] * 256 +
                      pabyBuf[25] * 256 * 256 +
                      (pabyBuf[24] & 0x7F) * 256 * 256 * 256;
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    psSHP->nRecords = pabyBuf[27] | (pabyBuf[26] << 8) | (pabyBuf[25] << 16) |
                      ((pabyBuf[24] & 0x7F) << 24);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        free(pabyBuf);
        free(psSHP);

        return SHPLIB_NULLPTR;
    }

    psSHP->nRecords = pabyBuf[27] | (pabyBuf[26] << 8) | (pabyBuf[25] << 16) |
                      ((pabyBuf[24] & 0x7F) << 24);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    psSHP->nRecords = (psSHP->nRecords - 50) / 4;

    psSHP->nShapeType = pabyBuf[32];

    if (psSHP->nRecords < 0 || psSHP->nRecords > 256000000) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        char szErrorMsg[200];
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
        char szErrorMsg[200];
=======
        char szError[200];
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Record count in .shx header is %d, which seems\n"
                 "unreasonable.  Assuming header is corrupt.",
                 psSHP->nRecords);
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        char szError[200];
=======
        char szErrorMsg[200];
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Record count in .shx header is %d, which seems\n"
                 "unreasonable.  Assuming header is corrupt.",
                 psSHP->nRecords);
<<<<<<< HEAD
        psSHP->sHooks.Error(szError);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Record count in .shx header is %d, which seems\n"
                 "unreasonable.  Assuming header is corrupt.",
                 psSHP->nRecords);
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psSHP->sHooks.FClose(psSHP->fpSHP);
        psSHP->sHooks.FClose(psSHP->fpSHX);
        free(psSHP);
        free(pabyBuf);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
        return (NULL);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        return (NULL);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        return (NULL);
=======
        return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    /* If a lot of records are advertized, check that the file is big enough */
    /* to hold them */
    if (psSHP->nRecords >= 1024 * 1024) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        psSHP->sHooks.FSeek(psSHP->fpSHX, 0, 2);
        const SAOffset nFileSize = psSHP->sHooks.FTell(psSHP->fpSHX);
        if (nFileSize > 100 &&
            nFileSize / 2 < STATIC_CAST(SAOffset, psSHP->nRecords * 4 + 50)) {
            psSHP->nRecords = STATIC_CAST(int, (nFileSize - 100) / 8);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        SAOffset nFileSize;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        psSHP->sHooks.FSeek(psSHP->fpSHX, 0, 2);
        const SAOffset nFileSize = psSHP->sHooks.FTell(psSHP->fpSHX);
        if (nFileSize > 100 &&
<<<<<<< HEAD
            nFileSize / 2 < (SAOffset)(psSHP->nRecords * 4 + 50)) {
            psSHP->nRecords = (int)((nFileSize - 100) / 8);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            nFileSize / 2 < STATIC_CAST(SAOffset, psSHP->nRecords * 4 + 50)) {
            psSHP->nRecords = STATIC_CAST(int, (nFileSize - 100) / 8);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        psSHP->sHooks.FSeek(psSHP->fpSHX, 0, 2);
        const SAOffset nFileSize = psSHP->sHooks.FTell(psSHP->fpSHX);
        if (nFileSize > 100 &&
            nFileSize / 2 < STATIC_CAST(SAOffset, psSHP->nRecords * 4 + 50)) {
            psSHP->nRecords = STATIC_CAST(int, (nFileSize - 100) / 8);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        }
        psSHP->sHooks.FSeek(psSHP->fpSHX, 100, 0);
    }

    /* -------------------------------------------------------------------- */
    /*      Read the bounds.                                                */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    double dValue;

=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    double dValue;

=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    double dValue;

=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    double dValue;

=======
>>>>>>> osgeo-main
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    double dValue;

>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    double dValue;

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    if (bBigEndian)
        SwapWord(8, pabyBuf + 36);
    memcpy(&dValue, pabyBuf + 36, 8);
    psSHP->adBoundsMin[0] = dValue;

    if (bBigEndian)
        SwapWord(8, pabyBuf + 44);
    memcpy(&dValue, pabyBuf + 44, 8);
    psSHP->adBoundsMin[1] = dValue;

    if (bBigEndian)
        SwapWord(8, pabyBuf + 52);
    memcpy(&dValue, pabyBuf + 52, 8);
    psSHP->adBoundsMax[0] = dValue;

    if (bBigEndian)
        SwapWord(8, pabyBuf + 60);
    memcpy(&dValue, pabyBuf + 60, 8);
    psSHP->adBoundsMax[1] = dValue;

    if (bBigEndian)
        SwapWord(8, pabyBuf + 68); /* z */
    memcpy(&dValue, pabyBuf + 68, 8);
    psSHP->adBoundsMin[2] = dValue;

    if (bBigEndian)
        SwapWord(8, pabyBuf + 76);
    memcpy(&dValue, pabyBuf + 76, 8);
    psSHP->adBoundsMax[2] = dValue;

    if (bBigEndian)
        SwapWord(8, pabyBuf + 84); /* z */
    memcpy(&dValue, pabyBuf + 84, 8);
    psSHP->adBoundsMin[3] = dValue;

    if (bBigEndian)
        SwapWord(8, pabyBuf + 92);
    memcpy(&dValue, pabyBuf + 92, 8);
    psSHP->adBoundsMax[3] = dValue;

    free(pabyBuf);

    /* -------------------------------------------------------------------- */
    /*  Read the .shx file to get the offsets to each record in     */
    /*  the .shp file.                          */
    /* -------------------------------------------------------------------- */
    psSHP->nMaxRecords = psSHP->nRecords;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    psSHP->panRecOffset =
        STATIC_CAST(unsigned int *,
                    malloc(sizeof(unsigned int) * MAX(1, psSHP->nMaxRecords)));
    psSHP->panRecSize =
        STATIC_CAST(unsigned int *,
                    malloc(sizeof(unsigned int) * MAX(1, psSHP->nMaxRecords)));
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    if (bLazySHXLoading)
        pabyBuf = SHPLIB_NULLPTR;
    else
        pabyBuf = STATIC_CAST(uchar *, malloc(8 * MAX(1, psSHP->nRecords)));

    if (psSHP->panRecOffset == SHPLIB_NULLPTR ||
        psSHP->panRecSize == SHPLIB_NULLPTR ||
        (!bLazySHXLoading && pabyBuf == SHPLIB_NULLPTR)) {
        char szErrorMsg[200];

        snprintf(
            szErrorMsg, sizeof(szErrorMsg),
            "Not enough memory to allocate requested memory (nRecords=%d).\n"
            "Probably broken SHP file",
            psSHP->nRecords);
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    psSHP->panRecOffset = (unsigned int *)malloc(sizeof(unsigned int) *
                                                 MAX(1, psSHP->nMaxRecords));
    psSHP->panRecSize = (unsigned int *)malloc(sizeof(unsigned int) *
                                               MAX(1, psSHP->nMaxRecords));
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    if (bLazySHXLoading)
        pabyBuf = SHPLIB_NULLPTR;
    else
        pabyBuf = STATIC_CAST(uchar *, malloc(8 * MAX(1, psSHP->nRecords)));

    if (psSHP->panRecOffset == SHPLIB_NULLPTR ||
        psSHP->panRecSize == SHPLIB_NULLPTR ||
        (!bLazySHXLoading && pabyBuf == SHPLIB_NULLPTR)) {
        char szErrorMsg[200];

        snprintf(
            szErrorMsg, sizeof(szErrorMsg),
            "Not enough memory to allocate requested memory (nRecords=%d).\n"
            "Probably broken SHP file",
            psSHP->nRecords);
<<<<<<< HEAD
        psSHP->sHooks.Error(szError);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    psSHP->panRecOffset =
        STATIC_CAST(unsigned int *,
                    malloc(sizeof(unsigned int) * MAX(1, psSHP->nMaxRecords)));
    psSHP->panRecSize =
        STATIC_CAST(unsigned int *,
                    malloc(sizeof(unsigned int) * MAX(1, psSHP->nMaxRecords)));
    if (bLazySHXLoading)
        pabyBuf = SHPLIB_NULLPTR;
    else
        pabyBuf = STATIC_CAST(uchar *, malloc(8 * MAX(1, psSHP->nRecords)));

    if (psSHP->panRecOffset == SHPLIB_NULLPTR ||
        psSHP->panRecSize == SHPLIB_NULLPTR ||
        (!bLazySHXLoading && pabyBuf == SHPLIB_NULLPTR)) {
        char szErrorMsg[200];

        snprintf(
            szErrorMsg, sizeof(szErrorMsg),
            "Not enough memory to allocate requested memory (nRecords=%d).\n"
            "Probably broken SHP file",
            psSHP->nRecords);
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psSHP->sHooks.FClose(psSHP->fpSHP);
        psSHP->sHooks.FClose(psSHP->fpSHX);
        if (psSHP->panRecOffset)
            free(psSHP->panRecOffset);
        if (psSHP->panRecSize)
            free(psSHP->panRecSize);
        if (pabyBuf)
            free(pabyBuf);
        free(psSHP);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
        return (NULL);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        return (NULL);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        return (NULL);
=======
        return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    if (bLazySHXLoading) {
        memset(psSHP->panRecOffset, 0,
               sizeof(unsigned int) * MAX(1, psSHP->nMaxRecords));
        memset(psSHP->panRecSize, 0,
               sizeof(unsigned int) * MAX(1, psSHP->nMaxRecords));
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        free(pabyBuf); // sometimes make cppcheck happy, but
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        free(pabyBuf); // sometimes make cppcheck happy, but
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        return (psSHP);
    }

    if (STATIC_CAST(int, psSHP->sHooks.FRead(pabyBuf, 8, psSHP->nRecords,
                                             psSHP->fpSHX)) !=
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psSHP->nRecords) {
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failed to read all values for %d records in .shx file: %s.",
                 psSHP->nRecords, strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
        return (psSHP);
    }

=======
        return (psSHP);
    }

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if ((int)psSHP->sHooks.FRead(pabyBuf, 8, psSHP->nRecords, psSHP->fpSHX) !=
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        psSHP->nRecords) {
        char szErrorMsg[200];

<<<<<<< HEAD
        snprintf(szError, sizeof(szError),
                 "Failed to read all values for %d records in .shx file.",
                 psSHP->nRecords);
        psSHP->sHooks.Error(szError);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
        free(pabyBuf); // sometimes make cppcheck happy, but
        return (psSHP);
    }

    if (STATIC_CAST(int, psSHP->sHooks.FRead(pabyBuf, 8, psSHP->nRecords,
                                             psSHP->fpSHX)) !=
        psSHP->nRecords) {
        char szErrorMsg[200];

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failed to read all values for %d records in .shx file: %s.",
                 psSHP->nRecords, strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

        /* SHX is short or unreadable for some reason. */
        psSHP->sHooks.FClose(psSHP->fpSHP);
        psSHP->sHooks.FClose(psSHP->fpSHX);
        free(psSHP->panRecOffset);
        free(psSHP->panRecSize);
        free(pabyBuf);
        free(psSHP);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
        return (NULL);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        return (NULL);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        return (NULL);
=======
        return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    /* In read-only mode, we can close the SHX now */
    if (strcmp(pszAccess, "rb") == 0) {
        psSHP->sHooks.FClose(psSHP->fpSHX);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        psSHP->fpSHX = SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
        psSHP->fpSHX = SHPLIB_NULLPTR;
=======
        psSHP->fpSHX = NULL;
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    for (int i = 0; i < psSHP->nRecords; i++) {
        unsigned int nOffset;
        memcpy(&nOffset, pabyBuf + i * 8, 4);
        if (!bBigEndian)
            SwapWord(4, &nOffset);

        unsigned int nLength;
        memcpy(&nLength, pabyBuf + i * 8 + 4, 4);
        if (!bBigEndian)
            SwapWord(4, &nLength);

        if (nOffset > STATIC_CAST(unsigned int, INT_MAX)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid offset for entity %d", i);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            SHPClose(psSHP);
            free(pabyBuf);
            return SHPLIB_NULLPTR;
        }
        if (nLength > STATIC_CAST(unsigned int, INT_MAX / 2 - 4)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid length for entity %d", i);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            SHPClose(psSHP);
            free(pabyBuf);
            return SHPLIB_NULLPTR;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        }
        psSHP->panRecOffset[i] = nOffset * 2;
        psSHP->panRecSize[i] = nLength * 2;
    }
    free(pabyBuf);

=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        psSHP->fpSHX = NULL;
=======
        psSHP->fpSHX = SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    }

    for (int i = 0; i < psSHP->nRecords; i++) {
        unsigned int nOffset;
        memcpy(&nOffset, pabyBuf + i * 8, 4);
        if (!bBigEndian)
            SwapWord(4, &nOffset);

        unsigned int nLength;
        memcpy(&nLength, pabyBuf + i * 8 + 4, 4);
        if (!bBigEndian)
            SwapWord(4, &nLength);

        if (nOffset > STATIC_CAST(unsigned int, INT_MAX)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid offset for entity %d", i);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            SHPClose(psSHP);
            free(pabyBuf);
            return SHPLIB_NULLPTR;
        }
        if (nLength > STATIC_CAST(unsigned int, INT_MAX / 2 - 4)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid length for entity %d", i);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            SHPClose(psSHP);
            free(pabyBuf);
<<<<<<< HEAD
            return NULL;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        psSHP->fpSHX = SHPLIB_NULLPTR;
    }

    for (int i = 0; i < psSHP->nRecords; i++) {
        unsigned int nOffset;
        memcpy(&nOffset, pabyBuf + i * 8, 4);
        if (!bBigEndian)
            SwapWord(4, &nOffset);

        unsigned int nLength;
        memcpy(&nLength, pabyBuf + i * 8 + 4, 4);
        if (!bBigEndian)
            SwapWord(4, &nLength);

        if (nOffset > STATIC_CAST(unsigned int, INT_MAX)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid offset for entity %d", i);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            SHPClose(psSHP);
            free(pabyBuf);
            return SHPLIB_NULLPTR;
        }
        if (nLength > STATIC_CAST(unsigned int, INT_MAX / 2 - 4)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid length for entity %d", i);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            SHPClose(psSHP);
            free(pabyBuf);
            return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }
        psSHP->panRecOffset[i] = nOffset * 2;
        psSHP->panRecSize[i] = nLength * 2;
    }
    free(pabyBuf);

>>>>>>> osgeo-main
    return (psSHP);
}

/************************************************************************/
/*                              SHPOpenLLEx()                           */
/*                                                                      */
/*      Open the .shp and .shx files based on the basename of the       */
/*      files or either file name. It generally invokes SHPRestoreSHX() */
/*      in case when bRestoreSHX equals true.                           */
/************************************************************************/

SHPHandle SHPAPI_CALL SHPOpenLLEx(const char *pszLayer, const char *pszAccess,
                                  SAHooks *psHooks, int bRestoreSHX)
{
    if (!bRestoreSHX)
        return SHPOpenLL(pszLayer, pszAccess, psHooks);
    else {
        if (SHPRestoreSHX(pszLayer, pszAccess, psHooks)) {
            return SHPOpenLL(pszLayer, pszAccess, psHooks);
        }
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    return SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
    return (NULL);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    return (NULL);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    return (NULL);
=======
    return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
}

/************************************************************************/
/*                              SHPRestoreSHX()                         */
/*                                                                      */
/*      Restore .SHX file using associated .SHP file.                   */
/*                                                                      */
/************************************************************************/

int SHPAPI_CALL SHPRestoreSHX(const char *pszLayer, const char *pszAccess,
                              SAHooks *psHooks)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    char *pszFullname, *pszBasename;
    SAFile fpSHP, fpSHX;

    uchar *pabyBuf;
    int i;
    size_t nFullnameLen;
    unsigned int nSHPFilesize;

    size_t nMessageLen;
    char *pszMessage;

    unsigned int nCurrentSHPOffset = 100;
    size_t nRealSHXContentSize = 100;

    const char pszSHXAccess[] = "w+b";
    char *pabySHXHeader;
    char abyReadedRecord[8];
    unsigned int niRecord = 0;
    unsigned int nRecordLength = 0;
    unsigned int nRecordOffset = 50;

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /* -------------------------------------------------------------------- */
    /*      Ensure the access string is one of the legal ones.  We          */
    /*      ensure the result string indicates binary to avoid common       */
    /*      problems on Windows.                                            */
    /* -------------------------------------------------------------------- */
    if (strcmp(pszAccess, "rb+") == 0 || strcmp(pszAccess, "r+b") == 0 ||
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        strcmp(pszAccess, "r+") == 0) {
        pszAccess = "r+b";
    }
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        strcmp(pszAccess, "r+") == 0) {
        pszAccess = "r+b";
    }
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        strcmp(pszAccess, "r+") == 0)
        pszAccess = "r+b";
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        strcmp(pszAccess, "r+") == 0)
        pszAccess = "r+b";
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        strcmp(pszAccess, "r+") == 0)
        pszAccess = "r+b";
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        strcmp(pszAccess, "r+") == 0) {
        pszAccess = "r+b";
    }
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    else {
        pszAccess = "rb";
    }

/* -------------------------------------------------------------------- */
/*  Establish the byte order on this machine.                           */
/* -------------------------------------------------------------------- */
#if !defined(bBigEndian)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
>>>>>>> osgeo-main
    {
        int i = 1;
        if (*((uchar *)&i) == 1)
            bBigEndian = false;
        else
            bBigEndian = true;
<<<<<<< HEAD
    }
#endif

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
    }
#endif

    /* -------------------------------------------------------------------- */
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /*  Open the .shp file.  Note that files pulled from                    */
    /*  a PC to Unix with upper case filenames won't work!                  */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    SAFile fpSHP = psHooks->FOpen(pszFullname, pszAccess);
    if (fpSHP == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHP", 5);
        fpSHP = psHooks->FOpen(pszFullname, pszAccess);
    }

    if (fpSHP == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));

        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        free(pszFullname);

=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    i = 1;
    if (*((uchar *)&i) == 1)
        bBigEndian = FALSE;
    else
        bBigEndian = TRUE;
#endif

    /* -------------------------------------------------------------------- */
    /*  Compute the base (layer) name.  If there is any extension           */
    /*  on the passed in filename we will strip it off.                     */
    /* -------------------------------------------------------------------- */
    pszBasename = (char *)malloc(strlen(pszLayer) + 5);
    strcpy(pszBasename, pszLayer);
    for (i = (int)strlen(pszBasename) - 1;
         i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' &&
         pszBasename[i] != '\\';
         i--) {
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    }
#endif

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /*  Open the .shp file.  Note that files pulled from                    */
    /*  a PC to Unix with upper case filenames won't work!                  */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    SAFile fpSHP = psHooks->FOpen(pszFullname, pszAccess);
    if (fpSHP == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHP", 5);
        fpSHP = psHooks->FOpen(pszFullname, pszAccess);
    }

    if (fpSHP == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        free(pszFullname);

        return (0);
    }

    /* -------------------------------------------------------------------- */
    /*  Read the file size from the SHP file.                               */
    /* -------------------------------------------------------------------- */
    uchar *pabyBuf = STATIC_CAST(uchar *, malloc(100));
    if (psHooks->FRead(pabyBuf, 100, 1, fpSHP) != 1) {
        psHooks->Error(".shp file is unreadable, or corrupt.");
        psHooks->FClose(fpSHP);

=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        free(pszFullname);

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    {
        int i = 1;
        if (*((uchar *)&i) == 1)
            bBigEndian = false;
        else
            bBigEndian = true;
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  Open the .shp file.  Note that files pulled from                    */
    /*  a PC to Unix with upper case filenames won't work!                  */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    SAFile fpSHP = psHooks->FOpen(pszFullname, pszAccess);
    if (fpSHP == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".SHP", 5);
        fpSHP = psHooks->FOpen(pszFullname, pszAccess);
    }

    if (fpSHP == SHPLIB_NULLPTR) {
        const size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));

        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen, "Unable to open %s.shp or %s.SHP.",
                 pszFullname, pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        free(pszFullname);

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        return (0);
    }

    /* -------------------------------------------------------------------- */
    /*  Read the file size from the SHP file.                               */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    uchar *pabyBuf = STATIC_CAST(uchar *, malloc(100));
    if (psHooks->FRead(pabyBuf, 100, 1, fpSHP) != 1) {
        psHooks->Error(".shp file is unreadable, or corrupt.");
        psHooks->FClose(fpSHP);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

>>>>>>> osgeo-main
        free(pabyBuf);
        free(pszFullname);

        return (0);
    }

    unsigned int nSHPFilesize = (STATIC_CAST(unsigned int, pabyBuf[24]) << 24) |
                                (pabyBuf[25] << 16) | (pabyBuf[26] << 8) |
                                pabyBuf[27];
    if (nSHPFilesize < UINT_MAX / 2)
<<<<<<< HEAD
=======
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    pabyBuf = (uchar *)malloc(100);
    psHooks->FRead(pabyBuf, 100, 1, fpSHP);

=======
    pabyBuf = (uchar *)malloc(100);
    psHooks->FRead(pabyBuf, 100, 1, fpSHP);

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    nSHPFilesize =
        ((unsigned int)pabyBuf[24] * 256 * 256 * 256 +
         (unsigned int)pabyBuf[25] * 256 * 256 +
         (unsigned int)pabyBuf[26] * 256 + (unsigned int)pabyBuf[27]);
    if (nSHPFilesize < 0xFFFFFFFFU / 2)
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
    uchar *pabyBuf = STATIC_CAST(uchar *, malloc(100));
    if (psHooks->FRead(pabyBuf, 100, 1, fpSHP) != 1) {
        psHooks->Error(".shp file is unreadable, or corrupt.");
        psHooks->FClose(fpSHP);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

        free(pabyBuf);
        free(pszFullname);

        return (0);
    }

    unsigned int nSHPFilesize = (STATIC_CAST(unsigned int, pabyBuf[24]) << 24) |
                                (pabyBuf[25] << 16) | (pabyBuf[26] << 8) |
                                pabyBuf[27];
    if (nSHPFilesize < UINT_MAX / 2)
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    pabyBuf = (uchar *)malloc(100);
    psHooks->FRead(pabyBuf, 100, 1, fpSHP);

=======
    pabyBuf = (uchar *)malloc(100);
    psHooks->FRead(pabyBuf, 100, 1, fpSHP);

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    nSHPFilesize =
        ((unsigned int)pabyBuf[24] * 256 * 256 * 256 +
         (unsigned int)pabyBuf[25] * 256 * 256 +
         (unsigned int)pabyBuf[26] * 256 + (unsigned int)pabyBuf[27]);
    if (nSHPFilesize < 0xFFFFFFFFU / 2)
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
    uchar *pabyBuf = STATIC_CAST(uchar *, malloc(100));
    if (psHooks->FRead(pabyBuf, 100, 1, fpSHP) != 1) {
        psHooks->Error(".shp file is unreadable, or corrupt.");
        psHooks->FClose(fpSHP);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

        free(pabyBuf);
        free(pszFullname);

        return (0);
    }

    unsigned int nSHPFilesize = (STATIC_CAST(unsigned int, pabyBuf[24]) << 24) |
                                (pabyBuf[25] << 16) | (pabyBuf[26] << 8) |
                                pabyBuf[27];
    if (nSHPFilesize < UINT_MAX / 2)
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        nSHPFilesize *= 2;
    else
        nSHPFilesize = (UINT_MAX / 2) * 2;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
    const char pszSHXAccess[] = "w+b";
    SAFile fpSHX = psHooks->FOpen(pszFullname, pszSHXAccess);
    if (fpSHX == SHPLIB_NULLPTR) {
        size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        snprintf(pszMessage, nMessageLen,
                 "Error opening file %s.shx for writing", pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psHooks->FClose(fpSHP);

        free(pabyBuf);
        free(pszFullname);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
    snprintf(pszFullname, nFullnameLen, "%s.shx", pszBasename);
    fpSHX = psHooks->FOpen(pszFullname, pszSHXAccess);

    if (fpSHX == NULL) {
        nMessageLen = strlen(pszBasename) * 2 + 256;
        pszMessage = (char *)malloc(nMessageLen);
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        snprintf(pszMessage, nMessageLen,
                 "Error opening file %s.shx for writing", pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psHooks->FClose(fpSHP);

        free(pabyBuf);
        free(pszFullname);

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    snprintf(pszFullname, nFullnameLen, "%s.shx", pszBasename);
    fpSHX = psHooks->FOpen(pszFullname, pszSHXAccess);

    if (fpSHX == NULL) {
        nMessageLen = strlen(pszBasename) * 2 + 256;
        pszMessage = (char *)malloc(nMessageLen);
        snprintf(pszMessage, nMessageLen,
                 "Error opening file %s.shx for writing", pszBasename);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psHooks->FClose(fpSHX);

        free(pabyBuf);
        free(pszBasename);
        free(pszFullname);

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
    const char pszSHXAccess[] = "w+b";
    SAFile fpSHX = psHooks->FOpen(pszFullname, pszSHXAccess);
    if (fpSHX == SHPLIB_NULLPTR) {
        size_t nMessageLen = strlen(pszFullname) * 2 + 256;
        char *pszMessage = STATIC_CAST(char *, malloc(nMessageLen));
        pszFullname[nLenWithoutExtension] = 0;
        snprintf(pszMessage, nMessageLen,
                 "Error opening file %s.shx for writing", pszFullname);
        psHooks->Error(pszMessage);
        free(pszMessage);

        psHooks->FClose(fpSHP);

        free(pabyBuf);
        free(pszFullname);

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        return (0);
    }

    /* -------------------------------------------------------------------- */
    /*  Open SHX and create it using SHP file content.                      */
    /* -------------------------------------------------------------------- */
    psHooks->FSeek(fpSHP, 100, 0);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    char *pabySHXHeader = STATIC_CAST(char *, malloc(100));
    memcpy(pabySHXHeader, pabyBuf, 100);
    psHooks->FWrite(pabySHXHeader, 100, 1, fpSHX);
    free(pabyBuf);

    // unsigned int nCurrentRecordOffset = 0;
    unsigned int nCurrentSHPOffset = 100;
    unsigned int nRealSHXContentSize = 100;
    unsigned int niRecord = 0;
    unsigned int nRecordLength = 0;
    unsigned int nRecordOffset = 50;
    char abyReadRecord[8];

    while (nCurrentSHPOffset < nSHPFilesize) {
        if (psHooks->FRead(&niRecord, 4, 1, fpSHP) == 1 &&
            psHooks->FRead(&nRecordLength, 4, 1, fpSHP) == 1) {
            if (!bBigEndian)
                SwapWord(4, &nRecordOffset);
            memcpy(abyReadRecord, &nRecordOffset, 4);
            memcpy(abyReadRecord + 4, &nRecordLength, 4);

            psHooks->FWrite(abyReadRecord, 8, 1, fpSHX);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
    pabySHXHeader = (char *)malloc(100);
=======
    char *pabySHXHeader = STATIC_CAST(char *, malloc(100));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    memcpy(pabySHXHeader, pabyBuf, 100);
    psHooks->FWrite(pabySHXHeader, 100, 1, fpSHX);
    free(pabyBuf);

    // unsigned int nCurrentRecordOffset = 0;
    unsigned int nCurrentSHPOffset = 100;
    unsigned int nRealSHXContentSize = 100;
    unsigned int niRecord = 0;
    unsigned int nRecordLength = 0;
    unsigned int nRecordOffset = 50;
    char abyReadRecord[8];

    while (nCurrentSHPOffset < nSHPFilesize) {
        if (psHooks->FRead(&niRecord, 4, 1, fpSHP) == 1 &&
            psHooks->FRead(&nRecordLength, 4, 1, fpSHP) == 1) {
            if (!bBigEndian)
                SwapWord(4, &nRecordOffset);
            memcpy(abyReadRecord, &nRecordOffset, 4);
            memcpy(abyReadRecord + 4, &nRecordLength, 4);

            psHooks->FWrite(abyReadRecord, 8, 1, fpSHX);

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    pabySHXHeader = (char *)malloc(100);
    memcpy(pabySHXHeader, pabyBuf, 100);
    psHooks->FWrite(pabySHXHeader, 100, 1, fpSHX);

    while (nCurrentSHPOffset < nSHPFilesize) {
        if (psHooks->FRead(&niRecord, 4, 1, fpSHP) == 1 &&
            psHooks->FRead(&nRecordLength, 4, 1, fpSHP) == 1) {
            if (!bBigEndian)
                SwapWord(4, &nRecordOffset);
            memcpy(abyReadedRecord, &nRecordOffset, 4);
            memcpy(abyReadedRecord + 4, &nRecordLength, 4);

            psHooks->FWrite(abyReadedRecord, 8, 1, fpSHX);

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    char *pabySHXHeader = STATIC_CAST(char *, malloc(100));
    memcpy(pabySHXHeader, pabyBuf, 100);
    psHooks->FWrite(pabySHXHeader, 100, 1, fpSHX);
    free(pabyBuf);

    // unsigned int nCurrentRecordOffset = 0;
    unsigned int nCurrentSHPOffset = 100;
    unsigned int nRealSHXContentSize = 100;
    unsigned int niRecord = 0;
    unsigned int nRecordLength = 0;
    unsigned int nRecordOffset = 50;
    char abyReadRecord[8];

    while (nCurrentSHPOffset < nSHPFilesize) {
        if (psHooks->FRead(&niRecord, 4, 1, fpSHP) == 1 &&
            psHooks->FRead(&nRecordLength, 4, 1, fpSHP) == 1) {
            if (!bBigEndian)
                SwapWord(4, &nRecordOffset);
            memcpy(abyReadRecord, &nRecordOffset, 4);
            memcpy(abyReadRecord + 4, &nRecordLength, 4);

            psHooks->FWrite(abyReadRecord, 8, 1, fpSHX);

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            if (!bBigEndian)
                SwapWord(4, &nRecordOffset);
            if (!bBigEndian)
                SwapWord(4, &nRecordLength);
            nRecordOffset += nRecordLength + 4;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            // nCurrentRecordOffset += 8;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            // nCurrentRecordOffset += 8;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            // nCurrentRecordOffset += 8;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            // nCurrentRecordOffset += 8;
=======
>>>>>>> osgeo-main
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            // nCurrentRecordOffset += 8;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
            // nCurrentRecordOffset += 8;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            nCurrentSHPOffset += 8 + nRecordLength * 2;

            psHooks->FSeek(fpSHP, nCurrentSHPOffset, 0);
            nRealSHXContentSize += 8;
        }
        else {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psHooks->Error("Error parsing .shp to restore .shx");
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psHooks->Error("Error parsing .shp to restore .shx");
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psHooks->Error("Error parsing .shp to restore .shx");
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            psHooks->Error("Error parsing .shp to restore .shx");
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            nMessageLen = strlen(pszBasename) * 2 + 256;
            pszMessage = (char *)malloc(nMessageLen);
            snprintf(pszMessage, nMessageLen,
                     "Error parsing .shp to restore .shx");
            psHooks->Error(pszMessage);
            free(pszMessage);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            psHooks->Error("Error parsing .shp to restore .shx");
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
            psHooks->Error("Error parsing .shp to restore .shx");
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

            psHooks->FClose(fpSHX);
            psHooks->FClose(fpSHP);

            free(pabySHXHeader);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
            free(pszBasename);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            free(pszBasename);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            free(pszBasename);
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            free(pszFullname);

            return (0);
        }
    }

    nRealSHXContentSize /= 2; // Bytes counted -> WORDs
    if (!bBigEndian)
        SwapWord(4, &nRealSHXContentSize);
    psHooks->FSeek(fpSHX, 24, 0);
    psHooks->FWrite(&nRealSHXContentSize, 4, 1, fpSHX);

    psHooks->FClose(fpSHP);
    psHooks->FClose(fpSHX);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    free(pszFullname);
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    free(pszFullname);
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    free(pszFullname);
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    free(pszFullname);
=======
>>>>>>> osgeo-main
    free(pabyBuf);
    free(pszFullname);
    free(pszBasename);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    free(pabyBuf);
    free(pszFullname);
    free(pszBasename);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    free(pszFullname);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    free(pabyBuf);
    free(pszFullname);
    free(pszBasename);
=======
    free(pszFullname);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    free(pabySHXHeader);

    return (1);
}

/************************************************************************/
/*                              SHPClose()                              */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*                                           */
/*    Close the .shp and .shx files.                    */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*                                           */
/*    Close the .shp and .shx files.                    */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*                                           */
/*    Close the .shp and .shx files.                    */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
/*                                           */
/*    Close the .shp and .shx files.                    */
=======
>>>>>>> osgeo-main
/*                                                                      */
/*      Close the .shp and .shx files.                                  */

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
/*                                                                      */
/*      Close the .shp and .shx files.                                  */

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
/*                                           */
/*    Close the .shp and .shx files.                    */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
/*                                                                      */
/*      Close the .shp and .shx files.                                  */

=======
/*                                           */
/*    Close the .shp and .shx files.                    */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
/************************************************************************/

void SHPAPI_CALL SHPClose(SHPHandle psSHP)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    if (psSHP == SHPLIB_NULLPTR)
        return;

    /* -------------------------------------------------------------------- */
    /*    Update the header if we have modified anything.            */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /* -------------------------------------------------------------------- */
    if (psSHP->bUpdated)
        SHPWriteHeader(psSHP);

    /* -------------------------------------------------------------------- */
    /*      Free all resources, and close files.                            */
    /* -------------------------------------------------------------------- */
    free(psSHP->panRecOffset);
    free(psSHP->panRecSize);

    if (psSHP->fpSHX != SHPLIB_NULLPTR)
        psSHP->sHooks.FClose(psSHP->fpSHX);
    psSHP->sHooks.FClose(psSHP->fpSHP);

=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psSHP == NULL)
        return;

    /* -------------------------------------------------------------------- */
    /*      Update the header if we have modified anything.                 */
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    if (psSHP == SHPLIB_NULLPTR)
        return;

    /* -------------------------------------------------------------------- */
    /*    Update the header if we have modified anything.            */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* -------------------------------------------------------------------- */
    if (psSHP->bUpdated)
        SHPWriteHeader(psSHP);

    /* -------------------------------------------------------------------- */
    /*      Free all resources, and close files.                            */
    /* -------------------------------------------------------------------- */
    free(psSHP->panRecOffset);
    free(psSHP->panRecSize);

<<<<<<< HEAD
<<<<<<< HEAD
    if (psSHP->fpSHX != SHPLIB_NULLPTR)
        psSHP->sHooks.FClose(psSHP->fpSHX);
    psSHP->sHooks.FClose(psSHP->fpSHP);

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    if (psSHP->pabyRec != SHPLIB_NULLPTR) {
        free(psSHP->pabyRec);
    }

    if (psSHP->pabyObjectBuf != SHPLIB_NULLPTR) {
        free(psSHP->pabyObjectBuf);
    }
    if (psSHP->psCachedObject != SHPLIB_NULLPTR) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        free(psSHP->psCachedObject);
    }

=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psSHP->fpSHX != NULL)
        psSHP->sHooks.FClose(psSHP->fpSHX);
    psSHP->sHooks.FClose(psSHP->fpSHP);

    if (psSHP->pabyRec != NULL) {
        free(psSHP->pabyRec);
    }

    if (psSHP->pabyObjectBuf != NULL) {
        free(psSHP->pabyObjectBuf);
    }
    if (psSHP->psCachedObject != NULL) {
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    if (psSHP->fpSHX != SHPLIB_NULLPTR)
        psSHP->sHooks.FClose(psSHP->fpSHX);
    psSHP->sHooks.FClose(psSHP->fpSHP);

    if (psSHP->pabyRec != SHPLIB_NULLPTR) {
        free(psSHP->pabyRec);
    }

    if (psSHP->pabyObjectBuf != SHPLIB_NULLPTR) {
        free(psSHP->pabyObjectBuf);
    }
    if (psSHP->psCachedObject != SHPLIB_NULLPTR) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        free(psSHP->psCachedObject);
    }

=======
    if (psSHP == NULL)
=======
    if (psSHP == SHPLIB_NULLPTR)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        return;

    /* -------------------------------------------------------------------- */
    /*    Update the header if we have modified anything.            */
    /* -------------------------------------------------------------------- */
    if (psSHP->bUpdated)
        SHPWriteHeader(psSHP);

    /* -------------------------------------------------------------------- */
    /*      Free all resources, and close files.                            */
    /* -------------------------------------------------------------------- */
    free(psSHP->panRecOffset);
    free(psSHP->panRecSize);

    if (psSHP->fpSHX != SHPLIB_NULLPTR)
        psSHP->sHooks.FClose(psSHP->fpSHX);
    psSHP->sHooks.FClose(psSHP->fpSHP);

    if (psSHP->pabyRec != SHPLIB_NULLPTR) {
        free(psSHP->pabyRec);
    }

    if (psSHP->pabyObjectBuf != SHPLIB_NULLPTR) {
        free(psSHP->pabyObjectBuf);
    }
    if (psSHP->psCachedObject != SHPLIB_NULLPTR) {
        free(psSHP->psCachedObject);
    }

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    free(psSHP);
}

/************************************************************************/
/*                    SHPSetFastModeReadObject()                        */
/************************************************************************/

/* If setting bFastMode = TRUE, the content of SHPReadObject() is owned by the
 * SHPHandle. */
/* So you cannot have 2 valid instances of SHPReadObject() simultaneously. */
/* The SHPObject padfZ and padfM members may be NULL depending on the geometry
 */
/* type. It is illegal to free at hand any of the pointer members of the
 * SHPObject structure */
void SHPAPI_CALL SHPSetFastModeReadObject(SHPHandle hSHP, int bFastMode)
{
    if (bFastMode) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        if (hSHP->psCachedObject == SHPLIB_NULLPTR) {
            hSHP->psCachedObject =
                STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
            assert(hSHP->psCachedObject != SHPLIB_NULLPTR);
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        if (hSHP->psCachedObject == NULL) {
            hSHP->psCachedObject = (SHPObject *)calloc(1, sizeof(SHPObject));
            assert(hSHP->psCachedObject != NULL);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        if (hSHP->psCachedObject == NULL) {
            hSHP->psCachedObject = (SHPObject *)calloc(1, sizeof(SHPObject));
            assert(hSHP->psCachedObject != NULL);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        if (hSHP->psCachedObject == NULL) {
            hSHP->psCachedObject = (SHPObject *)calloc(1, sizeof(SHPObject));
            assert(hSHP->psCachedObject != NULL);
=======
        if (hSHP->psCachedObject == SHPLIB_NULLPTR) {
            hSHP->psCachedObject =
                STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
            assert(hSHP->psCachedObject != SHPLIB_NULLPTR);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        }
    }

    hSHP->bFastModeReadObject = bFastMode;
}

/************************************************************************/
/*                             SHPGetInfo()                             */
/*                                                                      */
/*      Fetch general information about the shape file.                 */
/************************************************************************/

void SHPAPI_CALL SHPGetInfo(SHPHandle psSHP, int *pnEntities, int *pnShapeType,
                            double *padfMinBound, double *padfMaxBound)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    if (psSHP == SHPLIB_NULLPTR)
        return;

    if (pnEntities != SHPLIB_NULLPTR)
        *pnEntities = psSHP->nRecords;

    if (pnShapeType != SHPLIB_NULLPTR)
        *pnShapeType = psSHP->nShapeType;

    for (int i = 0; i < 4; i++) {
        if (padfMinBound != SHPLIB_NULLPTR)
            padfMinBound[i] = psSHP->adBoundsMin[i];
        if (padfMaxBound != SHPLIB_NULLPTR)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
    int i;

    if (psSHP == NULL)
=======
    if (psSHP == SHPLIB_NULLPTR)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        return;

    if (pnEntities != SHPLIB_NULLPTR)
        *pnEntities = psSHP->nRecords;

    if (pnShapeType != SHPLIB_NULLPTR)
        *pnShapeType = psSHP->nShapeType;

    for (int i = 0; i < 4; i++) {
        if (padfMinBound != SHPLIB_NULLPTR)
            padfMinBound[i] = psSHP->adBoundsMin[i];
<<<<<<< HEAD
        if (padfMaxBound != NULL)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    int i;

    if (psSHP == NULL)
        return;

    if (pnEntities != NULL)
        *pnEntities = psSHP->nRecords;

    if (pnShapeType != NULL)
        *pnShapeType = psSHP->nShapeType;

    for (i = 0; i < 4; i++) {
        if (padfMinBound != NULL)
            padfMinBound[i] = psSHP->adBoundsMin[i];
        if (padfMaxBound != NULL)
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        if (padfMaxBound != SHPLIB_NULLPTR)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    if (psSHP == SHPLIB_NULLPTR)
        return;

    if (pnEntities != SHPLIB_NULLPTR)
        *pnEntities = psSHP->nRecords;

    if (pnShapeType != SHPLIB_NULLPTR)
        *pnShapeType = psSHP->nShapeType;

    for (int i = 0; i < 4; i++) {
        if (padfMinBound != SHPLIB_NULLPTR)
            padfMinBound[i] = psSHP->adBoundsMin[i];
        if (padfMaxBound != SHPLIB_NULLPTR)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            padfMaxBound[i] = psSHP->adBoundsMax[i];
    }
}

/************************************************************************/
/*                             SHPCreate()                              */
/*                                                                      */
/*      Create a new shape file and return a handle to the open         */
/*      shape file with read/write access.                              */
/************************************************************************/

SHPHandle SHPAPI_CALL SHPCreate(const char *pszLayer, int nShapeType)
{
    SAHooks sHooks;

    SASetupDefaultHooks(&sHooks);

    return SHPCreateLL(pszLayer, nShapeType, &sHooks);
}

/************************************************************************/
/*                             SHPCreate()                              */
/*                                                                      */
/*      Create a new shape file and return a handle to the open         */
/*      shape file with read/write access.                              */
/************************************************************************/

SHPHandle SHPAPI_CALL SHPCreateLL(const char *pszLayer, int nShapeType,
                                  SAHooks *psHooks)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
/* -------------------------------------------------------------------- */
/*      Establish the byte order on this system.                        */
/* -------------------------------------------------------------------- */
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    char *pszBasename = NULL, *pszFullname = NULL;
    int i;
    SAFile fpSHP = NULL, fpSHX = NULL;
    uchar abyHeader[100];
    int32 i32;
    double dValue;
    size_t nFullnameLen;

    /* -------------------------------------------------------------------- */
    /*      Establish the byte order on this system.                        */
    /* -------------------------------------------------------------------- */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
#if !defined(bBigEndian)
    {
        int i = 1;
        if (*((uchar *)&i) == 1)
            bBigEndian = false;
        else
            bBigEndian = true;
    }
#endif

    /* -------------------------------------------------------------------- */
    /*      Open the two files so we can write their headers.               */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    SAFile fpSHP = psHooks->FOpen(pszFullname, "wb");
    if (fpSHP == SHPLIB_NULLPTR) {
        char szErrorMsg[200];
        snprintf(szErrorMsg, sizeof(szErrorMsg), "Failed to create file %s: %s",
                 pszFullname, strerror(errno));
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        return NULL;
    }

    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
    SAFile fpSHX = psHooks->FOpen(pszFullname, "wb");
    if (fpSHX == SHPLIB_NULLPTR) {
        char szErrorMsg[200];
        snprintf(szErrorMsg, sizeof(szErrorMsg), "Failed to create file %s: %s",
                 pszFullname, strerror(errno));
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        psHooks->FClose(fpSHP);
        return NULL;
    }

    free(pszFullname);
    pszFullname = SHPLIB_NULLPTR;
=======
    char *pszBasename = NULL, *pszFullname = NULL;
    int i;
    SAFile fpSHP = NULL, fpSHX = NULL;
    uchar abyHeader[100];
    int32 i32;
    double dValue;
    size_t nFullnameLen;

    /* -------------------------------------------------------------------- */
    /*      Establish the byte order on this system.                        */
    /* -------------------------------------------------------------------- */
=======
/* -------------------------------------------------------------------- */
/*      Establish the byte order on this system.                        */
/* -------------------------------------------------------------------- */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
#if !defined(bBigEndian)
    {
        int i = 1;
        if (*((uchar *)&i) == 1)
            bBigEndian = false;
        else
            bBigEndian = true;
    }
#endif

    /* -------------------------------------------------------------------- */
    /*      Open the two files so we can write their headers.               */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    SAFile fpSHP = psHooks->FOpen(pszFullname, "wb");
    if (fpSHP == SHPLIB_NULLPTR) {
        char szErrorMsg[200];
        snprintf(szErrorMsg, sizeof(szErrorMsg), "Failed to create file %s: %s",
                 pszFullname, strerror(errno));
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        return NULL;
    }

    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
    SAFile fpSHX = psHooks->FOpen(pszFullname, "wb");
    if (fpSHX == SHPLIB_NULLPTR) {
        char szErrorMsg[200];
        snprintf(szErrorMsg, sizeof(szErrorMsg), "Failed to create file %s: %s",
                 pszFullname, strerror(errno));
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        psHooks->FClose(fpSHP);
        return NULL;
    }

    free(pszFullname);
<<<<<<< HEAD
    pszFullname = NULL;
    free(pszBasename);
    pszBasename = NULL;
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    pszFullname = SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
/* -------------------------------------------------------------------- */
/*      Establish the byte order on this system.                        */
/* -------------------------------------------------------------------- */
#if !defined(bBigEndian)
    {
        int i = 1;
        if (*((uchar *)&i) == 1)
            bBigEndian = false;
        else
            bBigEndian = true;
    }
#endif

    /* -------------------------------------------------------------------- */
    /*      Open the two files so we can write their headers.               */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = SHPGetLenWithoutExtension(pszLayer);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszLayer, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".shp", 5);
    SAFile fpSHP = psHooks->FOpen(pszFullname, "wb");
    if (fpSHP == SHPLIB_NULLPTR) {
        char szErrorMsg[200];
        snprintf(szErrorMsg, sizeof(szErrorMsg), "Failed to create file %s: %s",
                 pszFullname, strerror(errno));
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        return NULL;
    }

    memcpy(pszFullname + nLenWithoutExtension, ".shx", 5);
    SAFile fpSHX = psHooks->FOpen(pszFullname, "wb");
    if (fpSHX == SHPLIB_NULLPTR) {
        char szErrorMsg[200];
        snprintf(szErrorMsg, sizeof(szErrorMsg), "Failed to create file %s: %s",
                 pszFullname, strerror(errno));
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        psHooks->FClose(fpSHP);
        return NULL;
    }

    free(pszFullname);
    pszFullname = SHPLIB_NULLPTR;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main

    /* -------------------------------------------------------------------- */
    /*      Prepare header block for .shp file.                             */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    uchar abyHeader[100];
    memset(abyHeader, 0, sizeof(abyHeader));
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    uchar abyHeader[100];
    memset(abyHeader, 0, sizeof(abyHeader));
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    uchar abyHeader[100];
    memset(abyHeader, 0, sizeof(abyHeader));
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    uchar abyHeader[100];
    memset(abyHeader, 0, sizeof(abyHeader));
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    for (i = 0; i < 100; i++)
        abyHeader[i] = 0;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    uchar abyHeader[100];
    memset(abyHeader, 0, sizeof(abyHeader));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    for (i = 0; i < 100; i++)
        abyHeader[i] = 0;
=======
    uchar abyHeader[100];
    memset(abyHeader, 0, sizeof(abyHeader));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

    abyHeader[2] = 0x27; /* magic cookie */
    abyHeader[3] = 0x0a;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    int32 i32 = 50; /* file size */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    int32 i32 = 50; /* file size */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    int32 i32 = 50; /* file size */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    int32 i32 = 50; /* file size */
=======
>>>>>>> osgeo-main
    i32 = 50; /* file size */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    i32 = 50; /* file size */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    int32 i32 = 50; /* file size */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    i32 = 50; /* file size */
=======
    int32 i32 = 50; /* file size */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    ByteCopy(&i32, abyHeader + 24, 4);
    if (!bBigEndian)
        SwapWord(4, abyHeader + 24);

    i32 = 1000; /* version */
    ByteCopy(&i32, abyHeader + 28, 4);
    if (bBigEndian)
        SwapWord(4, abyHeader + 28);

    i32 = nShapeType; /* shape type */
    ByteCopy(&i32, abyHeader + 32, 4);
    if (bBigEndian)
        SwapWord(4, abyHeader + 32);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    double dValue = 0.0; /* set bounds */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    double dValue = 0.0; /* set bounds */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    double dValue = 0.0; /* set bounds */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    double dValue = 0.0; /* set bounds */
=======
>>>>>>> osgeo-main
    dValue = 0.0; /* set bounds */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    dValue = 0.0; /* set bounds */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    double dValue = 0.0; /* set bounds */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    dValue = 0.0; /* set bounds */
=======
    double dValue = 0.0; /* set bounds */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    ByteCopy(&dValue, abyHeader + 36, 8);
    ByteCopy(&dValue, abyHeader + 44, 8);
    ByteCopy(&dValue, abyHeader + 52, 8);
    ByteCopy(&dValue, abyHeader + 60, 8);

    /* -------------------------------------------------------------------- */
    /*      Write .shp file header.                                         */
    /* -------------------------------------------------------------------- */
    if (psHooks->FWrite(abyHeader, 100, 1, fpSHP) != 1) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failed to write .shp header: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        psHooks->FClose(fpSHP);
        psHooks->FClose(fpSHX);
        return NULL;
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        psHooks->Error("Failed to write .shp header.");
        goto error;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        psHooks->Error("Failed to write .shp header.");
        goto error;
=======
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failed to write .shp header: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        psHooks->FClose(fpSHP);
        psHooks->FClose(fpSHX);
        return NULL;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    /* -------------------------------------------------------------------- */
    /*      Prepare, and write .shx file header.                            */
    /* -------------------------------------------------------------------- */
    i32 = 50; /* file size */
    ByteCopy(&i32, abyHeader + 24, 4);
    if (!bBigEndian)
        SwapWord(4, abyHeader + 24);

    if (psHooks->FWrite(abyHeader, 100, 1, fpSHX) != 1) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failure writing .shx header: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        psHooks->FClose(fpSHP);
        psHooks->FClose(fpSHX);
        return NULL;
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        psHooks->Error("Failed to write .shx header.");
        goto error;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        psHooks->Error("Failed to write .shx header.");
        goto error;
=======
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Failure writing .shx header: %s", strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psHooks->Error(szErrorMsg);

        free(pszFullname);
        psHooks->FClose(fpSHP);
        psHooks->FClose(fpSHX);
        return NULL;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    /* -------------------------------------------------------------------- */
    /*      Close the files, and then open them as regular existing files.  */
    /* -------------------------------------------------------------------- */
    psHooks->FClose(fpSHP);
    psHooks->FClose(fpSHX);

    return (SHPOpenLL(pszLayer, "r+b", psHooks));
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

error:
    if (pszFullname)
        free(pszFullname);
    if (pszBasename)
        free(pszBasename);
    if (fpSHP)
        psHooks->FClose(fpSHP);
    if (fpSHX)
        psHooks->FClose(fpSHX);
    return NULL;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
}

/************************************************************************/
/*                           _SHPSetBounds()                            */
/*                                                                      */
/*      Compute a bounds rectangle for a shape, and set it into the     */
/*      indicated location in the record.                               */
/************************************************************************/

static void _SHPSetBounds(uchar *pabyRec, SHPObject *psShape)
{
    ByteCopy(&(psShape->dfXMin), pabyRec + 0, 8);
    ByteCopy(&(psShape->dfYMin), pabyRec + 8, 8);
    ByteCopy(&(psShape->dfXMax), pabyRec + 16, 8);
    ByteCopy(&(psShape->dfYMax), pabyRec + 24, 8);

    if (bBigEndian) {
        SwapWord(8, pabyRec + 0);
        SwapWord(8, pabyRec + 8);
        SwapWord(8, pabyRec + 16);
        SwapWord(8, pabyRec + 24);
    }
}

/************************************************************************/
/*                         SHPComputeExtents()                          */
/*                                                                      */
/*      Recompute the extents of a shape.  Automatically done by        */
/*      SHPCreateObject().                                              */
/************************************************************************/

void SHPAPI_CALL SHPComputeExtents(SHPObject *psObject)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
    int i;

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    int i;

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    int i;

=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /* -------------------------------------------------------------------- */
    /*      Build extents for this object.                                  */
    /* -------------------------------------------------------------------- */
    if (psObject->nVertices > 0) {
        psObject->dfXMin = psObject->dfXMax = psObject->padfX[0];
        psObject->dfYMin = psObject->dfYMax = psObject->padfY[0];
        psObject->dfZMin = psObject->dfZMax = psObject->padfZ[0];
        psObject->dfMMin = psObject->dfMMax = psObject->padfM[0];
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psObject->nVertices; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psObject->nVertices; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
    for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    for (i = 0; i < psObject->nVertices; i++) {
=======
    for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psObject->dfXMin = MIN(psObject->dfXMin, psObject->padfX[i]);
        psObject->dfYMin = MIN(psObject->dfYMin, psObject->padfY[i]);
        psObject->dfZMin = MIN(psObject->dfZMin, psObject->padfZ[i]);
        psObject->dfMMin = MIN(psObject->dfMMin, psObject->padfM[i]);

        psObject->dfXMax = MAX(psObject->dfXMax, psObject->padfX[i]);
        psObject->dfYMax = MAX(psObject->dfYMax, psObject->padfY[i]);
        psObject->dfZMax = MAX(psObject->dfZMax, psObject->padfZ[i]);
        psObject->dfMMax = MAX(psObject->dfMMax, psObject->padfM[i]);
    }
}

/************************************************************************/
/*                          SHPCreateObject()                           */
/*                                                                      */
/*      Create a shape object.  It should be freed with                 */
/*      SHPDestroyObject().                                             */
/************************************************************************/

SHPObject SHPAPI_CALL1(*)
    SHPCreateObject(int nSHPType, int nShapeId, int nParts,
                    const int *panPartStart, const int *panPartType,
                    int nVertices, const double *padfX, const double *padfY,
                    const double *padfZ, const double *padfM)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    SHPObject *psObject =
        STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    SHPObject *psObject =
        STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    SHPObject *psObject =
        STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    SHPObject *psObject =
        STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    SHPObject *psObject;
    int i, bHasM, bHasZ;

    psObject = (SHPObject *)calloc(1, sizeof(SHPObject));
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    SHPObject *psObject =
        STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    SHPObject *psObject =
        STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    psObject->nSHPType = nSHPType;
    psObject->nShapeId = nShapeId;
    psObject->bMeasureIsUsed = FALSE;

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
    /*    Establish whether this shape type has M, and Z values.        */
=======
    /*      Establish whether this shape type has M, and Z values.          */
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* -------------------------------------------------------------------- */
    bool bHasM;
    bool bHasZ;

    if (nSHPType == SHPT_ARCM || nSHPType == SHPT_POINTM ||
        nSHPType == SHPT_POLYGONM || nSHPType == SHPT_MULTIPOINTM) {
        bHasM = true;
        bHasZ = false;
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    /*      Establish whether this shape type has M, and Z values.          */
=======
    /*    Establish whether this shape type has M, and Z values.        */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    bool bHasM;
    bool bHasZ;

    if (nSHPType == SHPT_ARCM || nSHPType == SHPT_POINTM ||
        nSHPType == SHPT_POLYGONM || nSHPType == SHPT_MULTIPOINTM) {
<<<<<<< HEAD
        bHasM = TRUE;
        bHasZ = FALSE;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        bHasM = true;
        bHasZ = false;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /*    Establish whether this shape type has M, and Z values.        */
    /* -------------------------------------------------------------------- */
    bool bHasM;
    bool bHasZ;

    if (nSHPType == SHPT_ARCM || nSHPType == SHPT_POINTM ||
        nSHPType == SHPT_POLYGONM || nSHPType == SHPT_MULTIPOINTM) {
        bHasM = true;
        bHasZ = false;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
    }
    else if (nSHPType == SHPT_ARCZ || nSHPType == SHPT_POINTZ ||
             nSHPType == SHPT_POLYGONZ || nSHPType == SHPT_MULTIPOINTZ ||
             nSHPType == SHPT_MULTIPATCH) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        bHasM = true;
        bHasZ = true;
    }
    else {
        bHasM = false;
        bHasZ = false;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        bHasM = true;
        bHasZ = true;
    }
    else {
        bHasM = false;
        bHasZ = false;
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        bHasM = TRUE;
        bHasZ = TRUE;
    }
    else {
        bHasM = FALSE;
        bHasZ = FALSE;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        bHasM = true;
        bHasZ = true;
    }
    else {
        bHasM = false;
        bHasZ = false;
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    /* -------------------------------------------------------------------- */
    /*      Capture parts.  Note that part type is optional, and            */
    /*      defaults to ring.                                               */
    /* -------------------------------------------------------------------- */
    if (nSHPType == SHPT_ARC || nSHPType == SHPT_POLYGON ||
        nSHPType == SHPT_ARCM || nSHPType == SHPT_POLYGONM ||
        nSHPType == SHPT_ARCZ || nSHPType == SHPT_POLYGONZ ||
        nSHPType == SHPT_MULTIPATCH) {
        psObject->nParts = MAX(1, nParts);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psObject->panPartStart =
            STATIC_CAST(int *, calloc(sizeof(int), psObject->nParts));
        psObject->panPartType =
            STATIC_CAST(int *, malloc(sizeof(int) * psObject->nParts));
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psObject->panPartStart = (int *)calloc(sizeof(int), psObject->nParts);
        psObject->panPartType = (int *)malloc(sizeof(int) * psObject->nParts);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        psObject->panPartStart = (int *)calloc(sizeof(int), psObject->nParts);
        psObject->panPartType = (int *)malloc(sizeof(int) * psObject->nParts);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        psObject->panPartStart = (int *)calloc(sizeof(int), psObject->nParts);
        psObject->panPartType = (int *)malloc(sizeof(int) * psObject->nParts);
=======
        psObject->panPartStart =
            STATIC_CAST(int *, calloc(sizeof(int), psObject->nParts));
        psObject->panPartType =
            STATIC_CAST(int *, malloc(sizeof(int) * psObject->nParts));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

        psObject->panPartStart[0] = 0;
        psObject->panPartType[0] = SHPP_RING;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        for (int i = 0; i < nParts; i++) {
            if (panPartStart != SHPLIB_NULLPTR)
                psObject->panPartStart[i] = panPartStart[i];

            if (panPartType != SHPLIB_NULLPTR)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        for (i = 0; i < nParts; i++) {
            if (panPartStart != NULL)
                psObject->panPartStart[i] = panPartStart[i];

            if (panPartType != NULL)
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = 0; i < nParts; i++) {
            if (panPartStart != NULL)
                psObject->panPartStart[i] = panPartStart[i];

            if (panPartType != NULL)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        for (int i = 0; i < nParts; i++) {
            if (panPartStart != SHPLIB_NULLPTR)
                psObject->panPartStart[i] = panPartStart[i];

            if (panPartType != SHPLIB_NULLPTR)
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                psObject->panPartType[i] = panPartType[i];
            else
                psObject->panPartType[i] = SHPP_RING;
        }

        if (psObject->panPartStart[0] != 0)
            psObject->panPartStart[0] = 0;
    }

    /* -------------------------------------------------------------------- */
    /*      Capture vertices.  Note that X, Y, Z and M are optional.        */
    /* -------------------------------------------------------------------- */
    if (nVertices > 0) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        const size_t nSize = sizeof(double) * nVertices;
        psObject->padfX =
            STATIC_CAST(double *, padfX ? malloc(nSize)
                                        : calloc(sizeof(double), nVertices));
        psObject->padfY =
            STATIC_CAST(double *, padfY ? malloc(nSize)
                                        : calloc(sizeof(double), nVertices));
        psObject->padfZ = STATIC_CAST(
            double *,
            padfZ &&bHasZ ? malloc(nSize) : calloc(sizeof(double), nVertices));
        psObject->padfM = STATIC_CAST(
            double *,
            padfM &&bHasM ? malloc(nSize) : calloc(sizeof(double), nVertices));
        if (padfX != SHPLIB_NULLPTR)
            memcpy(psObject->padfX, padfX, nSize);
        if (padfY != SHPLIB_NULLPTR)
            memcpy(psObject->padfY, padfY, nSize);
        if (padfZ != SHPLIB_NULLPTR && bHasZ)
            memcpy(psObject->padfZ, padfZ, nSize);
        if (padfM != SHPLIB_NULLPTR && bHasM) {
            memcpy(psObject->padfM, padfM, nSize);
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        psObject->padfX = (double *)calloc(sizeof(double), nVertices);
        psObject->padfY = (double *)calloc(sizeof(double), nVertices);
        psObject->padfZ = (double *)calloc(sizeof(double), nVertices);
        psObject->padfM = (double *)calloc(sizeof(double), nVertices);

        for (i = 0; i < nVertices; i++) {
            if (padfX != NULL)
                psObject->padfX[i] = padfX[i];
            if (padfY != NULL)
                psObject->padfY[i] = padfY[i];
            if (padfZ != NULL && bHasZ)
                psObject->padfZ[i] = padfZ[i];
            if (padfM != NULL && bHasM)
                psObject->padfM[i] = padfM[i];
        }
        if (padfM != NULL && bHasM)
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        const size_t nSize = sizeof(double) * nVertices;
        psObject->padfX =
            STATIC_CAST(double *, padfX ? malloc(nSize)
                                        : calloc(sizeof(double), nVertices));
        psObject->padfY =
            STATIC_CAST(double *, padfY ? malloc(nSize)
                                        : calloc(sizeof(double), nVertices));
        psObject->padfZ = STATIC_CAST(
            double *,
            padfZ &&bHasZ ? malloc(nSize) : calloc(sizeof(double), nVertices));
        psObject->padfM = STATIC_CAST(
            double *,
            padfM &&bHasM ? malloc(nSize) : calloc(sizeof(double), nVertices));
        if (padfX != SHPLIB_NULLPTR)
            memcpy(psObject->padfX, padfX, nSize);
        if (padfY != SHPLIB_NULLPTR)
            memcpy(psObject->padfY, padfY, nSize);
        if (padfZ != SHPLIB_NULLPTR && bHasZ)
            memcpy(psObject->padfZ, padfZ, nSize);
        if (padfM != SHPLIB_NULLPTR && bHasM) {
            memcpy(psObject->padfM, padfM, nSize);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psObject->bMeasureIsUsed = TRUE;
        }
    }

    /* -------------------------------------------------------------------- */
    /*      Compute the extents.                                            */
    /* -------------------------------------------------------------------- */
    psObject->nVertices = nVertices;
    SHPComputeExtents(psObject);

    return (psObject);
}

/************************************************************************/
/*                       SHPCreateSimpleObject()                        */
/*                                                                      */
/*      Create a simple (common) shape object.  Destroy with            */
/*      SHPDestroyObject().                                             */
/************************************************************************/

SHPObject SHPAPI_CALL1(*)
    SHPCreateSimpleObject(int nSHPType, int nVertices, const double *padfX,
                          const double *padfY, const double *padfZ)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    return (SHPCreateObject(nSHPType, -1, 0, SHPLIB_NULLPTR, SHPLIB_NULLPTR,
                            nVertices, padfX, padfY, padfZ, SHPLIB_NULLPTR));
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    return (SHPCreateObject(nSHPType, -1, 0, SHPLIB_NULLPTR, SHPLIB_NULLPTR,
                            nVertices, padfX, padfY, padfZ, SHPLIB_NULLPTR));
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    return (SHPCreateObject(nSHPType, -1, 0, SHPLIB_NULLPTR, SHPLIB_NULLPTR,
                            nVertices, padfX, padfY, padfZ, SHPLIB_NULLPTR));
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    return (SHPCreateObject(nSHPType, -1, 0, SHPLIB_NULLPTR, SHPLIB_NULLPTR,
                            nVertices, padfX, padfY, padfZ, SHPLIB_NULLPTR));
=======
>>>>>>> osgeo-main
    return (SHPCreateObject(nSHPType, -1, 0, NULL, NULL, nVertices, padfX,
                            padfY, padfZ, NULL));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    return (SHPCreateObject(nSHPType, -1, 0, NULL, NULL, nVertices, padfX,
                            padfY, padfZ, NULL));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    return (SHPCreateObject(nSHPType, -1, 0, SHPLIB_NULLPTR, SHPLIB_NULLPTR,
                            nVertices, padfX, padfY, padfZ, SHPLIB_NULLPTR));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    return (SHPCreateObject(nSHPType, -1, 0, NULL, NULL, nVertices, padfX,
                            padfY, padfZ, NULL));
=======
    return (SHPCreateObject(nSHPType, -1, 0, SHPLIB_NULLPTR, SHPLIB_NULLPTR,
                            nVertices, padfX, padfY, padfZ, SHPLIB_NULLPTR));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
}

/************************************************************************/
/*                           SHPWriteObject()                           */
/*                                                                      */
/*      Write out the vertices of a new structure.  Note that it is     */
/*      only possible to write vertices at the end of the file.         */
/************************************************************************/

int SHPAPI_CALL SHPWriteObject(SHPHandle psSHP, int nShapeId,
                               SHPObject *psObject)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    unsigned int nRecordOffset, nRecordSize = 0;
    int i;
    uchar *pabyRec;
    int32 i32;
    int bExtendFile = FALSE;

<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    psSHP->bUpdated = TRUE;

    /* -------------------------------------------------------------------- */
    /*      Ensure that shape object matches the type of the file it is     */
    /*      being written to.                                               */
    /* -------------------------------------------------------------------- */
    assert(psObject->nSHPType == psSHP->nShapeType ||
           psObject->nSHPType == SHPT_NULL);

    /* -------------------------------------------------------------------- */
    /*      Ensure that -1 is used for appends.  Either blow an             */
    /*      assertion, or if they are disabled, set the shapeid to -1       */
    /*      for appends.                                                    */
    /* -------------------------------------------------------------------- */
    assert(nShapeId == -1 || (nShapeId >= 0 && nShapeId < psSHP->nRecords));

    if (nShapeId != -1 && nShapeId >= psSHP->nRecords)
        nShapeId = -1;

    /* -------------------------------------------------------------------- */
    /*      Add the new entity to the in memory index.                      */
    /* -------------------------------------------------------------------- */
    if (nShapeId == -1 && psSHP->nRecords + 1 > psSHP->nMaxRecords) {
        int nNewMaxRecords = psSHP->nMaxRecords + psSHP->nMaxRecords / 3 + 100;
        unsigned int *panRecOffsetNew;
        unsigned int *panRecSizeNew;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        panRecOffsetNew = STATIC_CAST(
            unsigned int *, SfRealloc(psSHP->panRecOffset,
                                      sizeof(unsigned int) * nNewMaxRecords));
        if (panRecOffsetNew == SHPLIB_NULLPTR)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            return -1;
        psSHP->panRecOffset = panRecOffsetNew;

        panRecSizeNew = STATIC_CAST(
            unsigned int *, SfRealloc(psSHP->panRecSize,
                                      sizeof(unsigned int) * nNewMaxRecords));
        if (panRecSizeNew == SHPLIB_NULLPTR)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        panRecOffsetNew = (unsigned int *)SfRealloc(
            psSHP->panRecOffset, sizeof(unsigned int) * nNewMaxRecords);
        if (panRecOffsetNew == NULL)
            return -1;
        psSHP->panRecOffset = panRecOffsetNew;

        panRecSizeNew = (unsigned int *)SfRealloc(
            psSHP->panRecSize, sizeof(unsigned int) * nNewMaxRecords);
        if (panRecSizeNew == NULL)
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        panRecOffsetNew = (unsigned int *)SfRealloc(
            psSHP->panRecOffset, sizeof(unsigned int) * nNewMaxRecords);
        if (panRecOffsetNew == NULL)
            return -1;
        psSHP->panRecOffset = panRecOffsetNew;

        panRecSizeNew = (unsigned int *)SfRealloc(
            psSHP->panRecSize, sizeof(unsigned int) * nNewMaxRecords);
        if (panRecSizeNew == NULL)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
        panRecOffsetNew = STATIC_CAST(
            unsigned int *, SfRealloc(psSHP->panRecOffset,
                                      sizeof(unsigned int) * nNewMaxRecords));
        if (panRecOffsetNew == SHPLIB_NULLPTR)
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            return -1;
        psSHP->panRecOffset = panRecOffsetNew;

        panRecSizeNew = STATIC_CAST(
            unsigned int *, SfRealloc(psSHP->panRecSize,
                                      sizeof(unsigned int) * nNewMaxRecords));
        if (panRecSizeNew == SHPLIB_NULLPTR)
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            return -1;
        psSHP->panRecSize = panRecSizeNew;

        psSHP->nMaxRecords = nNewMaxRecords;
    }

    /* -------------------------------------------------------------------- */
    /*      Initialize record.                                              */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    uchar *pabyRec =
        STATIC_CAST(uchar *, malloc(psObject->nVertices * 4 * sizeof(double) +
                                    psObject->nParts * 8 + 128));
    if (pabyRec == SHPLIB_NULLPTR)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
=======
>>>>>>> osgeo-main
        return -1;

    /* -------------------------------------------------------------------- */
    /*  Extract vertices for a Polygon or Arc.                */
    /* -------------------------------------------------------------------- */
    unsigned int nRecordSize = 0;
    const bool bFirstFeature = psSHP->nRecords == 0;
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    pabyRec = (uchar *)malloc(psObject->nVertices * 4 * sizeof(double) +
                              psObject->nParts * 8 + 128);
    if (pabyRec == NULL)
        return -1;
=======
    pabyRec = (uchar *)malloc(psObject->nVertices * 4 * sizeof(double) +
                              psObject->nParts * 8 + 128);
    if (pabyRec == NULL)
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
>>>>>>> osgeo-main
        return -1;

    /* -------------------------------------------------------------------- */
    /*  Extract vertices for a Polygon or Arc.                */
    /* -------------------------------------------------------------------- */
    unsigned int nRecordSize = 0;
    const bool bFirstFeature = psSHP->nRecords == 0;
<<<<<<< HEAD

    if (psObject->nSHPType == SHPT_POLYGON ||
        psObject->nSHPType == SHPT_POLYGONZ ||
        psObject->nSHPType == SHPT_POLYGONM || psObject->nSHPType == SHPT_ARC ||
        psObject->nSHPType == SHPT_ARCZ || psObject->nSHPType == SHPT_ARCM ||
        psObject->nSHPType == SHPT_MULTIPATCH) {
<<<<<<< HEAD
        int32 nPoints, nParts;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    /* -------------------------------------------------------------------- */
    /*  Extract vertices for a Polygon or Arc.                              */
    /* -------------------------------------------------------------------- */
    if (psObject->nSHPType == SHPT_POLYGON ||
        psObject->nSHPType == SHPT_POLYGONZ ||
        psObject->nSHPType == SHPT_POLYGONM || psObject->nSHPType == SHPT_ARC ||
        psObject->nSHPType == SHPT_ARCZ || psObject->nSHPType == SHPT_ARCM ||
        psObject->nSHPType == SHPT_MULTIPATCH) {
        int32 nPoints, nParts;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

<<<<<<< HEAD
=======
=======
    uchar *pabyRec =
        STATIC_CAST(uchar *, malloc(psObject->nVertices * 4 * sizeof(double) +
                                    psObject->nParts * 8 + 128));
    if (pabyRec == SHPLIB_NULLPTR)
        return -1;

    /* -------------------------------------------------------------------- */
    /*  Extract vertices for a Polygon or Arc.                */
    /* -------------------------------------------------------------------- */
    unsigned int nRecordSize = 0;
    const bool bFirstFeature = psSHP->nRecords == 0;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psObject->nSHPType == SHPT_POLYGON ||
        psObject->nSHPType == SHPT_POLYGONZ ||
        psObject->nSHPType == SHPT_POLYGONM || psObject->nSHPType == SHPT_ARC ||
        psObject->nSHPType == SHPT_ARCZ || psObject->nSHPType == SHPT_ARCM ||
        psObject->nSHPType == SHPT_MULTIPATCH) {
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        int32 nPoints = psObject->nVertices;
        int32 nParts = psObject->nParts;

        _SHPSetBounds(pabyRec + 12, psObject);

        if (bBigEndian)
            SwapWord(4, &nPoints);
        if (bBigEndian)
            SwapWord(4, &nParts);

=======
<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    pabyRec = (uchar *)malloc(psObject->nVertices * 4 * sizeof(double) +
                              psObject->nParts * 8 + 128);
    if (pabyRec == NULL)
        return -1;
=======
    pabyRec = (uchar *)malloc(psObject->nVertices * 4 * sizeof(double) +
                              psObject->nParts * 8 + 128);
    if (pabyRec == NULL)
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> osgeo-main
        return -1;

    /* -------------------------------------------------------------------- */
    /*  Extract vertices for a Polygon or Arc.                */
    /* -------------------------------------------------------------------- */
    unsigned int nRecordSize = 0;
    const bool bFirstFeature = psSHP->nRecords == 0;

    if (psObject->nSHPType == SHPT_POLYGON ||
        psObject->nSHPType == SHPT_POLYGONZ ||
        psObject->nSHPType == SHPT_POLYGONM || psObject->nSHPType == SHPT_ARC ||
        psObject->nSHPType == SHPT_ARCZ || psObject->nSHPType == SHPT_ARCM ||
        psObject->nSHPType == SHPT_MULTIPATCH) {
<<<<<<< HEAD
        int32 nPoints, nParts;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    /* -------------------------------------------------------------------- */
    /*  Extract vertices for a Polygon or Arc.                              */
    /* -------------------------------------------------------------------- */
    if (psObject->nSHPType == SHPT_POLYGON ||
        psObject->nSHPType == SHPT_POLYGONZ ||
        psObject->nSHPType == SHPT_POLYGONM || psObject->nSHPType == SHPT_ARC ||
        psObject->nSHPType == SHPT_ARCZ || psObject->nSHPType == SHPT_ARCM ||
        psObject->nSHPType == SHPT_MULTIPATCH) {
        int32 nPoints, nParts;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

<<<<<<< HEAD
=======
=======
    uchar *pabyRec =
        STATIC_CAST(uchar *, malloc(psObject->nVertices * 4 * sizeof(double) +
                                    psObject->nParts * 8 + 128));
    if (pabyRec == SHPLIB_NULLPTR)
        return -1;

    /* -------------------------------------------------------------------- */
    /*  Extract vertices for a Polygon or Arc.                */
    /* -------------------------------------------------------------------- */
    unsigned int nRecordSize = 0;
    const bool bFirstFeature = psSHP->nRecords == 0;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psObject->nSHPType == SHPT_POLYGON ||
        psObject->nSHPType == SHPT_POLYGONZ ||
        psObject->nSHPType == SHPT_POLYGONM || psObject->nSHPType == SHPT_ARC ||
        psObject->nSHPType == SHPT_ARCZ || psObject->nSHPType == SHPT_ARCM ||
        psObject->nSHPType == SHPT_MULTIPATCH) {
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        int32 nPoints = psObject->nVertices;
        int32 nParts = psObject->nParts;

        _SHPSetBounds(pabyRec + 12, psObject);

        if (bBigEndian)
            SwapWord(4, &nPoints);
        if (bBigEndian)
            SwapWord(4, &nParts);

=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        _SHPSetBounds(pabyRec + 12, psObject);

        if (bBigEndian)
            SwapWord(4, &nPoints);
        if (bBigEndian)
            SwapWord(4, &nParts);

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        ByteCopy(&nPoints, pabyRec + 40 + 8, 4);
        ByteCopy(&nParts, pabyRec + 36 + 8, 4);

        nRecordSize = 52;

        /*
         * Write part start positions.
         */
        ByteCopy(psObject->panPartStart, pabyRec + 44 + 8,
                 4 * psObject->nParts);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psObject->nParts; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psObject->nParts; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psObject->nParts; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psObject->nParts; i++) {
=======
>>>>>>> osgeo-main
        for (i = 0; i < psObject->nParts; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = 0; i < psObject->nParts; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        for (int i = 0; i < psObject->nParts; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        for (i = 0; i < psObject->nParts; i++) {
=======
        for (int i = 0; i < psObject->nParts; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            if (bBigEndian)
                SwapWord(4, pabyRec + 44 + 8 + 4 * i);
            nRecordSize += 4;
        }

        /*
         * Write multipatch part types if needed.
         */
        if (psObject->nSHPType == SHPT_MULTIPATCH) {
            memcpy(pabyRec + nRecordSize, psObject->panPartType,
                   4 * psObject->nParts);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nParts; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nParts; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nParts; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nParts; i++) {
=======
>>>>>>> osgeo-main
            for (i = 0; i < psObject->nParts; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (i = 0; i < psObject->nParts; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; i < psObject->nParts; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; i < psObject->nParts; i++) {
=======
            for (int i = 0; i < psObject->nParts; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                if (bBigEndian)
                    SwapWord(4, pabyRec + nRecordSize);
                nRecordSize += 4;
            }
        }

        /*
         * Write the (x,y) vertex values.
         */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psObject->nVertices; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psObject->nVertices; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
        for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        for (i = 0; i < psObject->nVertices; i++) {
=======
        for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            ByteCopy(psObject->padfX + i, pabyRec + nRecordSize, 8);
            ByteCopy(psObject->padfY + i, pabyRec + nRecordSize + 8, 8);

            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);

            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize + 8);

            nRecordSize += 2 * 8;
        }

        /*
         * Write the Z coordinates (if any).
         */
        if (psObject->nSHPType == SHPT_POLYGONZ ||
            psObject->nSHPType == SHPT_ARCZ ||
            psObject->nSHPType == SHPT_MULTIPATCH) {
            ByteCopy(&(psObject->dfZMin), pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;

            ByteCopy(&(psObject->dfZMax), pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
            for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; i < psObject->nVertices; i++) {
=======
            for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                ByteCopy(psObject->padfZ + i, pabyRec + nRecordSize, 8);
                if (bBigEndian)
                    SwapWord(8, pabyRec + nRecordSize);
                nRecordSize += 8;
            }
        }

        /*
         * Write the M values, if any.
         */
        if (psObject->bMeasureIsUsed &&
            (psObject->nSHPType == SHPT_POLYGONM ||
             psObject->nSHPType == SHPT_ARCM
#ifndef DISABLE_MULTIPATCH_MEASURE
             || psObject->nSHPType == SHPT_MULTIPATCH
#endif
             || psObject->nSHPType == SHPT_POLYGONZ ||
             psObject->nSHPType == SHPT_ARCZ)) {
            ByteCopy(&(psObject->dfMMin), pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;

            ByteCopy(&(psObject->dfMMax), pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
            for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; i < psObject->nVertices; i++) {
=======
            for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                ByteCopy(psObject->padfM + i, pabyRec + nRecordSize, 8);
                if (bBigEndian)
                    SwapWord(8, pabyRec + nRecordSize);
                nRecordSize += 8;
            }
        }
    }

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a MultiPoint.                    */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a MultiPoint.                    */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a MultiPoint.                    */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a MultiPoint.                    */
=======
>>>>>>> osgeo-main
    /*  Extract vertices for a MultiPoint.                                  */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    /*  Extract vertices for a MultiPoint.                                  */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    /*  Extract vertices for a MultiPoint.                    */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    /*  Extract vertices for a MultiPoint.                                  */
=======
    /*  Extract vertices for a MultiPoint.                    */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /* -------------------------------------------------------------------- */
    else if (psObject->nSHPType == SHPT_MULTIPOINT ||
             psObject->nSHPType == SHPT_MULTIPOINTZ ||
             psObject->nSHPType == SHPT_MULTIPOINTM) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        int32 nPoints = psObject->nVertices;
=======
        int32 nPoints;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        int32 nPoints;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        _SHPSetBounds(pabyRec + 12, psObject);
=======
        int32 nPoints = psObject->nVertices;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

<<<<<<< HEAD
<<<<<<< HEAD
        if (bBigEndian)
            SwapWord(4, &nPoints);
        ByteCopy(&nPoints, pabyRec + 44, 4);

        for (int i = 0; i < psObject->nVertices; i++) {
            ByteCopy(psObject->padfX + i, pabyRec + 48 + i * 16, 8);
            ByteCopy(psObject->padfY + i, pabyRec + 48 + i * 16 + 8, 8);

=======
        _SHPSetBounds(pabyRec + 12, psObject);
<<<<<<< HEAD

=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int32 nPoints = psObject->nVertices;
=======
        int32 nPoints;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        int32 nPoints;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        _SHPSetBounds(pabyRec + 12, psObject);
=======
        int32 nPoints = psObject->nVertices;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        if (bBigEndian)
            SwapWord(4, &nPoints);
        ByteCopy(&nPoints, pabyRec + 44, 4);

        for (int i = 0; i < psObject->nVertices; i++) {
            ByteCopy(psObject->padfX + i, pabyRec + 48 + i * 16, 8);
            ByteCopy(psObject->padfY + i, pabyRec + 48 + i * 16 + 8, 8);

<<<<<<< HEAD
=======
=======
        _SHPSetBounds(pabyRec + 12, psObject);
=======
>>>>>>> osgeo-main

        if (bBigEndian)
            SwapWord(4, &nPoints);
        ByteCopy(&nPoints, pabyRec + 44, 4);

        for (int i = 0; i < psObject->nVertices; i++) {
            ByteCopy(psObject->padfX + i, pabyRec + 48 + i * 16, 8);
            ByteCopy(psObject->padfY + i, pabyRec + 48 + i * 16 + 8, 8);

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        int32 nPoints;
=======
        int32 nPoints = psObject->nVertices;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))

        _SHPSetBounds(pabyRec + 12, psObject);

<<<<<<< HEAD
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        _SHPSetBounds(pabyRec + 12, psObject);

        if (bBigEndian)
            SwapWord(4, &nPoints);
        ByteCopy(&nPoints, pabyRec + 44, 4);

        for (i = 0; i < psObject->nVertices; i++) {
            ByteCopy(psObject->padfX + i, pabyRec + 48 + i * 16, 8);
            ByteCopy(psObject->padfY + i, pabyRec + 48 + i * 16 + 8, 8);

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        if (bBigEndian)
            SwapWord(4, &nPoints);
        ByteCopy(&nPoints, pabyRec + 44, 4);

        for (int i = 0; i < psObject->nVertices; i++) {
            ByteCopy(psObject->padfX + i, pabyRec + 48 + i * 16, 8);
            ByteCopy(psObject->padfY + i, pabyRec + 48 + i * 16 + 8, 8);

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            if (bBigEndian)
                SwapWord(8, pabyRec + 48 + i * 16);
            if (bBigEndian)
                SwapWord(8, pabyRec + 48 + i * 16 + 8);
        }

        nRecordSize = 48 + 16 * psObject->nVertices;

        if (psObject->nSHPType == SHPT_MULTIPOINTZ) {
            ByteCopy(&(psObject->dfZMin), pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;

            ByteCopy(&(psObject->dfZMax), pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
            for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; i < psObject->nVertices; i++) {
=======
            for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                ByteCopy(psObject->padfZ + i, pabyRec + nRecordSize, 8);
                if (bBigEndian)
                    SwapWord(8, pabyRec + nRecordSize);
                nRecordSize += 8;
            }
        }

        if (psObject->bMeasureIsUsed &&
            (psObject->nSHPType == SHPT_MULTIPOINTZ ||
             psObject->nSHPType == SHPT_MULTIPOINTM)) {
            ByteCopy(&(psObject->dfMMin), pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;

            ByteCopy(&(psObject->dfMMax), pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
            for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; i < psObject->nVertices; i++) {
=======
            for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                ByteCopy(psObject->padfM + i, pabyRec + nRecordSize, 8);
                if (bBigEndian)
                    SwapWord(8, pabyRec + nRecordSize);
                nRecordSize += 8;
            }
        }
    }

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*      Write point.                            */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*      Write point.                            */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*      Write point.                            */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    /*      Write point.                            */
=======
>>>>>>> osgeo-main
    /*      Write point.                                                    */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    /*      Write point.                                                    */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    /*      Write point.                            */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    /*      Write point.                                                    */
=======
    /*      Write point.                            */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /* -------------------------------------------------------------------- */
    else if (psObject->nSHPType == SHPT_POINT ||
             psObject->nSHPType == SHPT_POINTZ ||
             psObject->nSHPType == SHPT_POINTM) {
        ByteCopy(psObject->padfX, pabyRec + 12, 8);
        ByteCopy(psObject->padfY, pabyRec + 20, 8);

        if (bBigEndian)
            SwapWord(8, pabyRec + 12);
        if (bBigEndian)
            SwapWord(8, pabyRec + 20);

        nRecordSize = 28;

        if (psObject->nSHPType == SHPT_POINTZ) {
            ByteCopy(psObject->padfZ, pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;
        }

        if (psObject->bMeasureIsUsed && (psObject->nSHPType == SHPT_POINTZ ||
                                         psObject->nSHPType == SHPT_POINTM)) {
            ByteCopy(psObject->padfM, pabyRec + nRecordSize, 8);
            if (bBigEndian)
                SwapWord(8, pabyRec + nRecordSize);
            nRecordSize += 8;
        }
    }

    /* -------------------------------------------------------------------- */
    /*      Not much to do for null geometries.                             */
    /* -------------------------------------------------------------------- */
    else if (psObject->nSHPType == SHPT_NULL) {
        nRecordSize = 12;
    }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    else {
        /* unknown type */
        assert(false);
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    else {
        /* unknown type */
        assert(false);
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    else {
        /* unknown type */
        assert(FALSE);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======

    else {
        /* unknown type */
        assert(FALSE);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    else {
        /* unknown type */
        assert(false);
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    /* -------------------------------------------------------------------- */
    /*      Establish where we are going to put this record. If we are      */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /*      rewriting the last record of the file, then we can update it in */
    /*      place. Otherwise if rewriting an existing record, and it will   */
    /*      fit, then put it  back where the original came from.  Otherwise */
    /*      write at the end.                                               */
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
    /*      rewriting and existing record, and it will fit, then put it     */
    /*      back where the original came from.  Otherwise write at the end. */
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
=======
    /*      rewriting and existing record, and it will fit, then put it     */
    /*      back where the original came from.  Otherwise write at the end. */
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
=======
    /*      rewriting and existing record, and it will fit, then put it     */
    /*      back where the original came from.  Otherwise write at the end. */
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
    /* -------------------------------------------------------------------- */
    SAOffset nRecordOffset;
    bool bAppendToLastRecord = false;
    bool bAppendToFile = false;
    if (nShapeId != -1 &&
        psSHP->panRecOffset[nShapeId] + psSHP->panRecSize[nShapeId] + 8 ==
            psSHP->nFileSize) {
        nRecordOffset = psSHP->panRecOffset[nShapeId];
        bAppendToLastRecord = true;
    }
    else if (nShapeId == -1 || psSHP->panRecSize[nShapeId] < nRecordSize - 8) {
        if (psSHP->nFileSize > UINT_MAX - nRecordSize) {
            char str[255];
            snprintf(str, sizeof(str),
                     "Failed to write shape object. "
                     "The maximum file size of %u has been reached. "
                     "The current record of size %u cannot be added.",
                     psSHP->nFileSize, nRecordSize);
<<<<<<< HEAD
            str[sizeof(str) - 1] = '\0';
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            str[sizeof(str) - 1] = '\0';
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    /*      rewriting and existing record, and it will fit, then put it     */
    /*      back where the original came from.  Otherwise write at the end. */
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    /*      rewriting the last record of the file, then we can update it in */
    /*      place. Otherwise if rewriting an existing record, and it will   */
    /*      fit, then put it  back where the original came from.  Otherwise */
    /*      write at the end.                                               */
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* -------------------------------------------------------------------- */
    SAOffset nRecordOffset;
    bool bAppendToLastRecord = false;
    bool bAppendToFile = false;
    if (nShapeId != -1 &&
        psSHP->panRecOffset[nShapeId] + psSHP->panRecSize[nShapeId] + 8 ==
            psSHP->nFileSize) {
        nRecordOffset = psSHP->panRecOffset[nShapeId];
        bAppendToLastRecord = true;
    }
    else if (nShapeId == -1 || psSHP->panRecSize[nShapeId] < nRecordSize - 8) {
        if (psSHP->nFileSize > UINT_MAX - nRecordSize) {
            char str[255];
            snprintf(str, sizeof(str),
                     "Failed to write shape object. "
                     "The maximum file size of %u has been reached. "
                     "The current record of size %u cannot be added.",
                     psSHP->nFileSize, nRecordSize);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            str[sizeof(str) - 1] = '\0';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            str[sizeof(str) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->sHooks.Error(str);
            free(pabyRec);
            return -1;
        }

        bAppendToFile = true;
        nRecordOffset = psSHP->nFileSize;
    }
    else {
        nRecordOffset = psSHP->panRecOffset[nShapeId];
    }

    /* -------------------------------------------------------------------- */
    /*      Set the shape type, record number, and record size.             */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    int32 i32 =
        (nShapeId < 0) ? psSHP->nRecords + 1 : nShapeId + 1; /* record # */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    int32 i32 =
        (nShapeId < 0) ? psSHP->nRecords + 1 : nShapeId + 1; /* record # */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    int32 i32 =
        (nShapeId < 0) ? psSHP->nRecords + 1 : nShapeId + 1; /* record # */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    int32 i32 =
        (nShapeId < 0) ? psSHP->nRecords + 1 : nShapeId + 1; /* record # */
=======
>>>>>>> osgeo-main
    i32 = (nShapeId < 0) ? psSHP->nRecords + 1 : nShapeId + 1; /* record # */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    i32 = (nShapeId < 0) ? psSHP->nRecords + 1 : nShapeId + 1; /* record # */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    int32 i32 =
        (nShapeId < 0) ? psSHP->nRecords + 1 : nShapeId + 1; /* record # */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    i32 = (nShapeId < 0) ? psSHP->nRecords + 1 : nShapeId + 1; /* record # */
=======
    int32 i32 =
        (nShapeId < 0) ? psSHP->nRecords + 1 : nShapeId + 1; /* record # */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    if (!bBigEndian)
        SwapWord(4, &i32);
    ByteCopy(&i32, pabyRec, 4);

    i32 = (nRecordSize - 8) / 2; /* record size */
    if (!bBigEndian)
        SwapWord(4, &i32);
    ByteCopy(&i32, pabyRec + 4, 4);

    i32 = psObject->nSHPType; /* shape type */
    if (bBigEndian)
        SwapWord(4, &i32);
    ByteCopy(&i32, pabyRec + 8, 4);

    /* -------------------------------------------------------------------- */
    /*      Write out record.                                               */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

    /* -------------------------------------------------------------------- */
    /*      Guard FSeek with check for whether we're already at position;   */
    /*      no-op FSeeks defeat network filesystems' write buffering.       */
    /* -------------------------------------------------------------------- */
    if (psSHP->sHooks.FTell(psSHP->fpSHP) != nRecordOffset) {
        if (psSHP->sHooks.FSeek(psSHP->fpSHP, nRecordOffset, 0) != 0) {
            char szErrorMsg[200];

            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Error in psSHP->sHooks.FSeek() while writing object to "
                     ".shp file: %s",
                     strerror(errno));
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
            psSHP->sHooks.Error(szErrorMsg);

            free(pabyRec);
            return -1;
        }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    }
    if (psSHP->sHooks.FWrite(pabyRec, nRecordSize, 1, psSHP->fpSHP) < 1) {
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }
    if (psSHP->sHooks.FWrite(pabyRec, nRecordSize, 1, psSHP->fpSHP) < 1) {
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Error in psSHP->sHooks.FWrite() while writing object of %u "
                 "bytes to .shp file: %s",
                 nRecordSize, strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);

=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psSHP->sHooks.FSeek(psSHP->fpSHP, nRecordOffset, 0) != 0) {
        psSHP->sHooks.Error("Error in psSHP->sHooks.FSeek() while writing "
                            "object to .shp file.");
        free(pabyRec);
        return -1;
    }
    if (psSHP->sHooks.FWrite(pabyRec, nRecordSize, 1, psSHP->fpSHP) < 1) {
        psSHP->sHooks.Error("Error in psSHP->sHooks.Fwrite() while writing "
                            "object to .shp file.");
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======

    /* -------------------------------------------------------------------- */
    /*      Guard FSeek with check for whether we're already at position;   */
    /*      no-op FSeeks defeat network filesystems' write buffering.       */
    /* -------------------------------------------------------------------- */
    if (psSHP->sHooks.FTell(psSHP->fpSHP) != nRecordOffset) {
        if (psSHP->sHooks.FSeek(psSHP->fpSHP, nRecordOffset, 0) != 0) {
            char szErrorMsg[200];

            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Error in psSHP->sHooks.FSeek() while writing object to "
                     ".shp file: %s",
                     strerror(errno));
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
            psSHP->sHooks.Error(szErrorMsg);

            free(pabyRec);
            return -1;
        }
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }
    if (psSHP->sHooks.FWrite(pabyRec, nRecordSize, 1, psSHP->fpSHP) < 1) {
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        char szErrorMsg[200];

        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Error in psSHP->sHooks.FWrite() while writing object of %u "
                 "bytes to .shp file: %s",
                 nRecordSize, strerror(errno));
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        free(pabyRec);
        return -1;
    }

    free(pabyRec);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    if (bAppendToLastRecord) {
        psSHP->nFileSize = psSHP->panRecOffset[nShapeId] + nRecordSize;
    }
    else if (bAppendToFile) {
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    if (bExtendFile) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (bExtendFile) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (bExtendFile) {
=======
    if (bAppendToLastRecord) {
        psSHP->nFileSize = psSHP->panRecOffset[nShapeId] + nRecordSize;
    }
    else if (bAppendToFile) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        if (nShapeId == -1)
            nShapeId = psSHP->nRecords++;

        psSHP->panRecOffset[nShapeId] = psSHP->nFileSize;
        psSHP->nFileSize += nRecordSize;
    }
    psSHP->panRecSize[nShapeId] = nRecordSize - 8;

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    /*    Expand file wide bounds based on this shape.            */
    /* -------------------------------------------------------------------- */
    if (bFirstFeature) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*    Expand file wide bounds based on this shape.            */
    /* -------------------------------------------------------------------- */
    if (bFirstFeature) {
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /*      Expand file wide bounds based on this shape.                    */
    /* -------------------------------------------------------------------- */
    if (psSHP->adBoundsMin[0] == 0.0 && psSHP->adBoundsMax[0] == 0.0 &&
        psSHP->adBoundsMin[1] == 0.0 && psSHP->adBoundsMax[1] == 0.0) {
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    /*    Expand file wide bounds based on this shape.            */
    /* -------------------------------------------------------------------- */
    if (bFirstFeature) {
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        if (psObject->nSHPType == SHPT_NULL || psObject->nVertices == 0) {
            psSHP->adBoundsMin[0] = psSHP->adBoundsMax[0] = 0.0;
            psSHP->adBoundsMin[1] = psSHP->adBoundsMax[1] = 0.0;
            psSHP->adBoundsMin[2] = psSHP->adBoundsMax[2] = 0.0;
            psSHP->adBoundsMin[3] = psSHP->adBoundsMax[3] = 0.0;
        }
        else {
            psSHP->adBoundsMin[0] = psSHP->adBoundsMax[0] = psObject->padfX[0];
            psSHP->adBoundsMin[1] = psSHP->adBoundsMax[1] = psObject->padfY[0];
            psSHP->adBoundsMin[2] = psSHP->adBoundsMax[2] =
                psObject->padfZ ? psObject->padfZ[0] : 0.0;
            psSHP->adBoundsMin[3] = psSHP->adBoundsMax[3] =
                psObject->padfM ? psObject->padfM[0] : 0.0;
        }
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psObject->nVertices; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psObject->nVertices; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = 0; i < psObject->nVertices; i++) {
=======
>>>>>>> osgeo-main
    for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    for (i = 0; i < psObject->nVertices; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    for (i = 0; i < psObject->nVertices; i++) {
=======
    for (int i = 0; i < psObject->nVertices; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psSHP->adBoundsMin[0] = MIN(psSHP->adBoundsMin[0], psObject->padfX[i]);
        psSHP->adBoundsMin[1] = MIN(psSHP->adBoundsMin[1], psObject->padfY[i]);
        psSHP->adBoundsMax[0] = MAX(psSHP->adBoundsMax[0], psObject->padfX[i]);
        psSHP->adBoundsMax[1] = MAX(psSHP->adBoundsMax[1], psObject->padfY[i]);
        if (psObject->padfZ) {
            psSHP->adBoundsMin[2] =
                MIN(psSHP->adBoundsMin[2], psObject->padfZ[i]);
            psSHP->adBoundsMax[2] =
                MAX(psSHP->adBoundsMax[2], psObject->padfZ[i]);
        }
        if (psObject->padfM) {
            psSHP->adBoundsMin[3] =
                MIN(psSHP->adBoundsMin[3], psObject->padfM[i]);
            psSHP->adBoundsMax[3] =
                MAX(psSHP->adBoundsMax[3], psObject->padfM[i]);
        }
    }

    return (nShapeId);
}

/************************************************************************/
/*                         SHPAllocBuffer()                             */
/************************************************************************/

static void *SHPAllocBuffer(unsigned char **pBuffer, int nSize)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    if (pBuffer == SHPLIB_NULLPTR)
        return calloc(1, nSize);

    unsigned char *pRet = *pBuffer;
    if (pRet == SHPLIB_NULLPTR)
        return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (pBuffer == SHPLIB_NULLPTR)
        return calloc(1, nSize);

    unsigned char *pRet = *pBuffer;
    if (pRet == SHPLIB_NULLPTR)
        return SHPLIB_NULLPTR;
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    unsigned char *pRet;

=======
    unsigned char *pRet;

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if (pBuffer == NULL)
        return calloc(1, nSize);

    pRet = *pBuffer;
    if (pRet == NULL)
        return NULL;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    if (pBuffer == SHPLIB_NULLPTR)
        return calloc(1, nSize);

    unsigned char *pRet = *pBuffer;
    if (pRet == SHPLIB_NULLPTR)
        return SHPLIB_NULLPTR;
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

    (*pBuffer) += nSize;
    return pRet;
}

/************************************************************************/
/*                    SHPReallocObjectBufIfNecessary()                  */
/************************************************************************/

static unsigned char *SHPReallocObjectBufIfNecessary(SHPHandle psSHP,
                                                     int nObjectBufSize)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    if (nObjectBufSize == 0) {
        nObjectBufSize = 4 * sizeof(double);
    }

    unsigned char *pBuffer;
    if (nObjectBufSize > psSHP->nObjectBufSize) {
        pBuffer = STATIC_CAST(unsigned char *,
                              realloc(psSHP->pabyObjectBuf, nObjectBufSize));
        if (pBuffer != SHPLIB_NULLPTR) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    unsigned char *pBuffer;

    if (nObjectBufSize == 0) {
        nObjectBufSize = 4 * sizeof(double);
    }
=======
    unsigned char *pBuffer;

    if (nObjectBufSize == 0) {
        nObjectBufSize = 4 * sizeof(double);
    }
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if (nObjectBufSize > psSHP->nObjectBufSize) {
        pBuffer =
            (unsigned char *)realloc(psSHP->pabyObjectBuf, nObjectBufSize);
        if (pBuffer != NULL) {
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    if (nObjectBufSize == 0) {
        nObjectBufSize = 4 * sizeof(double);
    }

    unsigned char *pBuffer;
    if (nObjectBufSize > psSHP->nObjectBufSize) {
        pBuffer = STATIC_CAST(unsigned char *,
                              realloc(psSHP->pabyObjectBuf, nObjectBufSize));
        if (pBuffer != SHPLIB_NULLPTR) {
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->pabyObjectBuf = pBuffer;
            psSHP->nObjectBufSize = nObjectBufSize;
        }
    }
    else {
        pBuffer = psSHP->pabyObjectBuf;
    }

    return pBuffer;
}

/************************************************************************/
/*                          SHPReadObject()                             */
/*                                                                      */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*      Read the vertices, parts, and other non-attribute information    */
/*    for one shape.                            */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*      Read the vertices, parts, and other non-attribute information    */
/*    for one shape.                            */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*      Read the vertices, parts, and other non-attribute information    */
/*    for one shape.                            */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
/*      Read the vertices, parts, and other non-attribute information    */
/*    for one shape.                            */
=======
>>>>>>> osgeo-main
/*      Read the vertices, parts, and other non-attribute information   */
/*      for one shape.                                                  */

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
/*      Read the vertices, parts, and other non-attribute information   */
/*      for one shape.                                                  */

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
/*      Read the vertices, parts, and other non-attribute information    */
/*    for one shape.                            */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
/*      Read the vertices, parts, and other non-attribute information   */
/*      for one shape.                                                  */

=======
/*      Read the vertices, parts, and other non-attribute information    */
/*    for one shape.                            */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
/************************************************************************/

SHPObject SHPAPI_CALL1(*) SHPReadObject(SHPHandle psSHP, int hEntity)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    /* -------------------------------------------------------------------- */
    /*      Validate the record/entity number.                              */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity >= psSHP->nRecords)
        return SHPLIB_NULLPTR;

    /* -------------------------------------------------------------------- */
    /*      Read offset/length from SHX loading if necessary.               */
    /* -------------------------------------------------------------------- */
    if (psSHP->panRecOffset[hEntity] == 0 && psSHP->fpSHX != SHPLIB_NULLPTR) {
        unsigned int nOffset;
        unsigned int nLength;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
    int nEntitySize, nRequiredSize;
    SHPObject *psShape;
    char szErrorMsg[128];
    int nSHPType;
    int nBytesRead;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    /*      Validate the record/entity number.                              */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity >= psSHP->nRecords)
        return SHPLIB_NULLPTR;

    /* -------------------------------------------------------------------- */
    /*      Read offset/length from SHX loading if necessary.               */
    /* -------------------------------------------------------------------- */
    if (psSHP->panRecOffset[hEntity] == 0 && psSHP->fpSHX != SHPLIB_NULLPTR) {
        unsigned int nOffset;
        unsigned int nLength;

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    int nEntitySize, nRequiredSize;
    SHPObject *psShape;
    char szErrorMsg[128];
    int nSHPType;
    int nBytesRead;

    /* -------------------------------------------------------------------- */
    /*      Validate the record/entity number.                              */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity >= psSHP->nRecords)
        return (NULL);

    /* -------------------------------------------------------------------- */
    /*      Read offset/length from SHX loading if necessary.               */
    /* -------------------------------------------------------------------- */
    if (psSHP->panRecOffset[hEntity] == 0 && psSHP->fpSHX != NULL) {
        unsigned int nOffset, nLength;

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    /* -------------------------------------------------------------------- */
    /*      Validate the record/entity number.                              */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity >= psSHP->nRecords)
        return SHPLIB_NULLPTR;

    /* -------------------------------------------------------------------- */
    /*      Read offset/length from SHX loading if necessary.               */
    /* -------------------------------------------------------------------- */
    if (psSHP->panRecOffset[hEntity] == 0 && psSHP->fpSHX != SHPLIB_NULLPTR) {
        unsigned int nOffset;
        unsigned int nLength;

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        if (psSHP->sHooks.FSeek(psSHP->fpSHX, 100 + 8 * hEntity, 0) != 0 ||
            psSHP->sHooks.FRead(&nOffset, 1, 4, psSHP->fpSHX) != 4 ||
            psSHP->sHooks.FRead(&nLength, 1, 4, psSHP->fpSHX) != 4) {
            char str[128];
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            snprintf(str, sizeof(str),
                     "Error in fseek()/fread() reading object from .shx file "
                     "at offset %d",
                     100 + 8 * hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
=======

=======

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
            snprintf(str, sizeof(str),
                     "Error in fseek()/fread() reading object from .shx file "
                     "at offset %d",
                     100 + 8 * hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
<<<<<<< HEAD
<<<<<<< HEAD
            return NULL;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
            snprintf(str, sizeof(str),
                     "Error in fseek()/fread() reading object from .shx file "
                     "at offset %d",
                     100 + 8 * hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }
        if (!bBigEndian)
            SwapWord(4, &nOffset);
        if (!bBigEndian)
            SwapWord(4, &nLength);

<<<<<<< HEAD
<<<<<<< HEAD
        if (nOffset > STATIC_CAST(unsigned int, INT_MAX)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid offset for entity %d", hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
        }
        if (nLength > STATIC_CAST(unsigned int, INT_MAX / 2 - 4)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid length for entity %d", hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            snprintf(str, sizeof(str),
                     "Error in fseek()/fread() reading object from .shx file "
                     "at offset %d",
                     100 + 8 * hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
=======

=======

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
            snprintf(str, sizeof(str),
                     "Error in fseek()/fread() reading object from .shx file "
                     "at offset %d",
                     100 + 8 * hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
<<<<<<< HEAD
<<<<<<< HEAD
            return NULL;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
            snprintf(str, sizeof(str),
                     "Error in fseek()/fread() reading object from .shx file "
                     "at offset %d",
                     100 + 8 * hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }
        if (!bBigEndian)
            SwapWord(4, &nOffset);
        if (!bBigEndian)
            SwapWord(4, &nLength);

<<<<<<< HEAD
<<<<<<< HEAD
        if (nOffset > STATIC_CAST(unsigned int, INT_MAX)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid offset for entity %d", hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
        }
        if (nLength > STATIC_CAST(unsigned int, INT_MAX / 2 - 4)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid length for entity %d", hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        if (nOffset > (unsigned int)INT_MAX) {
            char str[128];

            snprintf(str, sizeof(str), "Invalid offset for entity %d", hEntity);

            psSHP->sHooks.Error(str);
            return NULL;
        }
        if (nLength > (unsigned int)(INT_MAX / 2 - 4)) {
            char str[128];

            snprintf(str, sizeof(str), "Invalid length for entity %d", hEntity);

=======
            return NULL;
=======
<<<<<<< HEAD
            return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        }
        if (!bBigEndian)
            SwapWord(4, &nOffset);
        if (!bBigEndian)
            SwapWord(4, &nLength);

=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        if (nOffset > STATIC_CAST(unsigned int, INT_MAX)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid offset for entity %d", hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
        }
        if (nLength > STATIC_CAST(unsigned int, INT_MAX / 2 - 4)) {
            char str[128];
            snprintf(str, sizeof(str), "Invalid length for entity %d", hEntity);
            str[sizeof(str) - 1] = '\0';

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            psSHP->sHooks.Error(str);
<<<<<<< HEAD
            return NULL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        }

        psSHP->panRecOffset[hEntity] = nOffset * 2;
        psSHP->panRecSize[hEntity] = nLength * 2;
    }

    /* -------------------------------------------------------------------- */
    /*      Ensure our record buffer is large enough.                       */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    const int nEntitySize = psSHP->panRecSize[hEntity] + 8;
    if (nEntitySize > psSHP->nBufSize) {
        int nNewBufSize = nEntitySize;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    const int nEntitySize = psSHP->panRecSize[hEntity] + 8;
    if (nEntitySize > psSHP->nBufSize) {
        int nNewBufSize = nEntitySize;
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    nEntitySize = psSHP->panRecSize[hEntity] + 8;
=======
    const int nEntitySize = psSHP->panRecSize[hEntity] + 8;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    if (nEntitySize > psSHP->nBufSize) {
        int nNewBufSize = nEntitySize;
<<<<<<< HEAD

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    nEntitySize = psSHP->panRecSize[hEntity] + 8;
    if (nEntitySize > psSHP->nBufSize) {
        uchar *pabyRecNew;
        int nNewBufSize = nEntitySize;

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    const int nEntitySize = psSHP->panRecSize[hEntity] + 8;
    if (nEntitySize > psSHP->nBufSize) {
        int nNewBufSize = nEntitySize;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        if (nNewBufSize < INT_MAX - nNewBufSize / 3)
            nNewBufSize += nNewBufSize / 3;
        else
            nNewBufSize = INT_MAX;

        /* Before allocating too much memory, check that the file is big enough
         */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        /* and do not trust the file size in the header the first time we */
        /* need to allocate more than 10 MB */
        if (nNewBufSize >= 10 * 1024 * 1024) {
            if (psSHP->nBufSize < 10 * 1024 * 1024) {
                SAOffset nFileSize;
                psSHP->sHooks.FSeek(psSHP->fpSHP, 0, 2);
                nFileSize = psSHP->sHooks.FTell(psSHP->fpSHP);
                if (nFileSize >= UINT_MAX)
                    psSHP->nFileSize = UINT_MAX;
                else
                    psSHP->nFileSize = STATIC_CAST(unsigned int, nFileSize);
            }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

            if (psSHP->panRecOffset[hEntity] >= psSHP->nFileSize ||
                /* We should normally use nEntitySize instead of*/
                /* psSHP->panRecSize[hEntity] in the below test, but because of
                 */
                /* the case of non conformant .shx files detailed a bit below,
                 */
                /* let be more tolerant */
                psSHP->panRecSize[hEntity] >
                    psSHP->nFileSize - psSHP->panRecOffset[hEntity]) {
                char str[128];
                snprintf(str, sizeof(str),
                         "Error in fread() reading object of size %d at offset "
                         "%u from .shp file",
                         nEntitySize, psSHP->panRecOffset[hEntity]);
                str[sizeof(str) - 1] = '\0';

                psSHP->sHooks.Error(str);
                return SHPLIB_NULLPTR;
            }
        }

        uchar *pabyRecNew =
            STATIC_CAST(uchar *, SfRealloc(psSHP->pabyRec, nNewBufSize));
        if (pabyRecNew == SHPLIB_NULLPTR) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        if (nEntitySize >= 10 * 1024 * 1024 &&
            (psSHP->panRecOffset[hEntity] >= psSHP->nFileSize ||
             (unsigned int)nEntitySize >
                 psSHP->nFileSize - psSHP->panRecOffset[hEntity])) {
            /* We do as is we didn't trust the file size in the header */
            SAOffset nFileSize;

            psSHP->sHooks.FSeek(psSHP->fpSHP, 0, 2);
            nFileSize = psSHP->sHooks.FTell(psSHP->fpSHP);
            if (nFileSize >= 0xFFFFFFFFU)
                psSHP->nFileSize = 0xFFFFFFFFU;
            else
                psSHP->nFileSize = (unsigned int)nFileSize;
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

            if (psSHP->panRecOffset[hEntity] >= psSHP->nFileSize ||
                /* We should normally use nEntitySize instead of*/
                /* psSHP->panRecSize[hEntity] in the below test, but because of
                 */
                /* the case of non conformant .shx files detailed a bit below,
                 */
                /* let be more tolerant */
                psSHP->panRecSize[hEntity] >
                    psSHP->nFileSize - psSHP->panRecOffset[hEntity]) {
                char str[128];
                snprintf(str, sizeof(str),
                         "Error in fread() reading object of size %d at offset "
                         "%u from .shp file",
                         nEntitySize, psSHP->panRecOffset[hEntity]);
                str[sizeof(str) - 1] = '\0';

                psSHP->sHooks.Error(str);
                return SHPLIB_NULLPTR;
            }
        }

<<<<<<< HEAD
        pabyRecNew = (uchar *)SfRealloc(psSHP->pabyRec, nNewBufSize);
        if (pabyRecNew == NULL) {
            char szError[200];

            snprintf(szError, sizeof(szError),
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
        /* and do not trust the file size in the header the first time we */
        /* need to allocate more than 10 MB */
        if (nNewBufSize >= 10 * 1024 * 1024) {
            if (psSHP->nBufSize < 10 * 1024 * 1024) {
                SAOffset nFileSize;
                psSHP->sHooks.FSeek(psSHP->fpSHP, 0, 2);
                nFileSize = psSHP->sHooks.FTell(psSHP->fpSHP);
                if (nFileSize >= UINT_MAX)
                    psSHP->nFileSize = UINT_MAX;
                else
                    psSHP->nFileSize = STATIC_CAST(unsigned int, nFileSize);
            }

            if (psSHP->panRecOffset[hEntity] >= psSHP->nFileSize ||
                /* We should normally use nEntitySize instead of*/
                /* psSHP->panRecSize[hEntity] in the below test, but because of
                 */
                /* the case of non conformant .shx files detailed a bit below,
                 */
                /* let be more tolerant */
                psSHP->panRecSize[hEntity] >
                    psSHP->nFileSize - psSHP->panRecOffset[hEntity]) {
                char str[128];
                snprintf(str, sizeof(str),
                         "Error in fread() reading object of size %d at offset "
                         "%u from .shp file",
                         nEntitySize, psSHP->panRecOffset[hEntity]);
                str[sizeof(str) - 1] = '\0';

                psSHP->sHooks.Error(str);
                return SHPLIB_NULLPTR;
            }
        }

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        uchar *pabyRecNew =
            STATIC_CAST(uchar *, SfRealloc(psSHP->pabyRec, nNewBufSize));
        if (pabyRecNew == SHPLIB_NULLPTR) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                     "Not enough memory to allocate requested memory "
                     "(nNewBufSize=%d). "
                     "Probably broken SHP file",
                     nNewBufSize);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
            psSHP->sHooks.Error(szErrorMsg);
            return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
            psSHP->sHooks.Error(szErrorMsg);
            return SHPLIB_NULLPTR;
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            psSHP->sHooks.Error(szError);
            return NULL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            psSHP->sHooks.Error(szError);
            return NULL;
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
            psSHP->sHooks.Error(szErrorMsg);
            return SHPLIB_NULLPTR;
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        }

        /* Only set new buffer size after successful alloc */
        psSHP->pabyRec = pabyRecNew;
        psSHP->nBufSize = nNewBufSize;
    }

    /* In case we were not able to reallocate the buffer on a previous step */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psSHP->pabyRec == SHPLIB_NULLPTR) {
        return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psSHP->pabyRec == SHPLIB_NULLPTR) {
        return SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psSHP->pabyRec == SHPLIB_NULLPTR) {
        return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    if (psSHP->pabyRec == SHPLIB_NULLPTR) {
        return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if (psSHP->pabyRec == NULL) {
        return NULL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psSHP->pabyRec == SHPLIB_NULLPTR) {
        return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (psSHP->pabyRec == NULL) {
        return NULL;
=======
    if (psSHP->pabyRec == SHPLIB_NULLPTR) {
        return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    /* -------------------------------------------------------------------- */
    /*      Read the record.                                                */
    /* -------------------------------------------------------------------- */
    if (psSHP->sHooks.FSeek(psSHP->fpSHP, psSHP->panRecOffset[hEntity], 0) !=
        0) {
        /*
         * TODO - mloskot: Consider detailed diagnostics of shape file,
         * for example to detect if file is truncated.
         */
        char str[128];
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======

=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        snprintf(str, sizeof(str),
                 "Error in fseek() reading object from .shp file at offset %u",
                 psSHP->panRecOffset[hEntity]);
        str[sizeof(str) - 1] = '\0';

        psSHP->sHooks.Error(str);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        return SHPLIB_NULLPTR;
    }

    const int nBytesRead = STATIC_CAST(
        int, psSHP->sHooks.FRead(psSHP->pabyRec, 1, nEntitySize, psSHP->fpSHP));
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        return NULL;
    }
<<<<<<< HEAD
=======

    nBytesRead =
        (int)psSHP->sHooks.FRead(psSHP->pabyRec, 1, nEntitySize, psSHP->fpSHP);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        return SHPLIB_NULLPTR;
    }

    const int nBytesRead = STATIC_CAST(
        int, psSHP->sHooks.FRead(psSHP->pabyRec, 1, nEntitySize, psSHP->fpSHP));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main

    nBytesRead =
        (int)psSHP->sHooks.FRead(psSHP->pabyRec, 1, nEntitySize, psSHP->fpSHP);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        return SHPLIB_NULLPTR;
    }

    const int nBytesRead = STATIC_CAST(
        int, psSHP->sHooks.FRead(psSHP->pabyRec, 1, nEntitySize, psSHP->fpSHP));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
    }

    const int nBytesRead = STATIC_CAST(
        int, psSHP->sHooks.FRead(psSHP->pabyRec, 1, nEntitySize, psSHP->fpSHP));
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        return NULL;
    }

    nBytesRead =
        (int)psSHP->sHooks.FRead(psSHP->pabyRec, 1, nEntitySize, psSHP->fpSHP);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        return SHPLIB_NULLPTR;
    }

    const int nBytesRead = STATIC_CAST(
        int, psSHP->sHooks.FRead(psSHP->pabyRec, 1, nEntitySize, psSHP->fpSHP));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

>>>>>>> osgeo-main
    /* Special case for a shapefile whose .shx content length field is not equal
     */
    /* to the content length field of the .shp, which is a violation of "The */
    /* content length stored in the index record is the same as the value stored
     * in the main */
    /* file record header."
     * (http://www.esri.com/library/whitepapers/pdfs/shapefile.pdf, page 24) */
    /* Actually in that case the .shx content length is equal to the .shp
     * content length + */
    /* 4 (16 bit words), representing the 8 bytes of the record header... */
    if (nBytesRead >= 8 && nBytesRead == nEntitySize - 8) {
        /* Do a sanity check */
        int nSHPContentLength;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        memcpy(&nSHPContentLength, psSHP->pabyRec + 4, 4);
        if (!bBigEndian)
            SwapWord(4, &(nSHPContentLength));
        if (nSHPContentLength < 0 || nSHPContentLength > INT_MAX / 2 - 4 ||
            2 * nSHPContentLength + 8 != nBytesRead) {
            char str[128];
            snprintf(str, sizeof(str),
                     "Sanity check failed when trying to recover from "
                     "inconsistent .shx/.shp with shape %d",
                     hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        memcpy(&nSHPContentLength, psSHP->pabyRec + 4, 4);
        if (!bBigEndian)
            SwapWord(4, &(nSHPContentLength));
        if (nSHPContentLength < 0 || nSHPContentLength > INT_MAX / 2 - 4 ||
            2 * nSHPContentLength + 8 != nBytesRead) {
            char str[128];
            snprintf(str, sizeof(str),
                     "Sanity check failed when trying to recover from "
                     "inconsistent .shx/.shp with shape %d",
                     hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

=======

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        memcpy(&nSHPContentLength, psSHP->pabyRec + 4, 4);
        if (!bBigEndian)
            SwapWord(4, &(nSHPContentLength));
        if (nSHPContentLength < 0 || nSHPContentLength > INT_MAX / 2 - 4 ||
            2 * nSHPContentLength + 8 != nBytesRead) {
            char str[128];
            snprintf(str, sizeof(str),
                     "Sanity check failed when trying to recover from "
                     "inconsistent .shx/.shp with shape %d",
                     hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
<<<<<<< HEAD
            return NULL;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        memcpy(&nSHPContentLength, psSHP->pabyRec + 4, 4);
        if (!bBigEndian)
            SwapWord(4, &(nSHPContentLength));
        if (nSHPContentLength < 0 || nSHPContentLength > INT_MAX / 2 - 4 ||
            2 * nSHPContentLength + 8 != nBytesRead) {
            char str[128];
            snprintf(str, sizeof(str),
                     "Sanity check failed when trying to recover from "
                     "inconsistent .shx/.shp with shape %d",
                     hEntity);
            str[sizeof(str) - 1] = '\0';

            psSHP->sHooks.Error(str);
            return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        }
    }
    else if (nBytesRead != nEntitySize) {
        /*
         * TODO - mloskot: Consider detailed diagnostics of shape file,
         * for example to detect if file is truncated.
         */
        char str[128];
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        snprintf(str, sizeof(str),
                 "Error in fread() reading object of size %d at offset %u from "
                 ".shp file",
                 nEntitySize, psSHP->panRecOffset[hEntity]);
        str[sizeof(str) - 1] = '\0';

        psSHP->sHooks.Error(str);
        return SHPLIB_NULLPTR;
    }

    if (8 + 4 > nEntitySize) {
        char szErrorMsg[160];
        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Corrupted .shp file : shape %d : nEntitySize = %d", hEntity,
                 nEntitySize);
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
        return SHPLIB_NULLPTR;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    }
    int nSHPType;
    memcpy(&nSHPType, psSHP->pabyRec + 8, 4);

    if (bBigEndian)
        SwapWord(4, &(nSHPType));

    /* -------------------------------------------------------------------- */
    /*    Allocate and minimally initialize the object.            */
    /* -------------------------------------------------------------------- */
    SHPObject *psShape;
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }
    int nSHPType;
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

        snprintf(str, sizeof(str),
                 "Error in fread() reading object of size %u at offset %u from "
                 ".shp file",
                 nEntitySize, psSHP->panRecOffset[hEntity]);

        psSHP->sHooks.Error(str);
        return NULL;
    }

    if (8 + 4 > nEntitySize) {
        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Corrupted .shp file : shape %d : nEntitySize = %d", hEntity,
                 nEntitySize);
        psSHP->sHooks.Error(szErrorMsg);
        return NULL;
    }
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        snprintf(str, sizeof(str),
                 "Error in fread() reading object of size %d at offset %u from "
                 ".shp file",
                 nEntitySize, psSHP->panRecOffset[hEntity]);
        str[sizeof(str) - 1] = '\0';

        psSHP->sHooks.Error(str);
        return SHPLIB_NULLPTR;
    }

    if (8 + 4 > nEntitySize) {
        char szErrorMsg[160];
        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Corrupted .shp file : shape %d : nEntitySize = %d", hEntity,
                 nEntitySize);
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
        return SHPLIB_NULLPTR;
    }
    int nSHPType;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    memcpy(&nSHPType, psSHP->pabyRec + 8, 4);

    if (bBigEndian)
        SwapWord(4, &(nSHPType));

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
    /*    Allocate and minimally initialize the object.            */
    /* -------------------------------------------------------------------- */
    SHPObject *psShape;
=======
    /*      Allocate and minimally initialize the object.                   */
    /* -------------------------------------------------------------------- */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        snprintf(str, sizeof(str),
                 "Error in fread() reading object of size %d at offset %u from "
                 ".shp file",
                 nEntitySize, psSHP->panRecOffset[hEntity]);
        str[sizeof(str) - 1] = '\0';

        psSHP->sHooks.Error(str);
        return SHPLIB_NULLPTR;
    }

    if (8 + 4 > nEntitySize) {
        char szErrorMsg[160];
        snprintf(szErrorMsg, sizeof(szErrorMsg),
                 "Corrupted .shp file : shape %d : nEntitySize = %d", hEntity,
                 nEntitySize);
        szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
        psSHP->sHooks.Error(szErrorMsg);
        return SHPLIB_NULLPTR;
    }
    int nSHPType;
    memcpy(&nSHPType, psSHP->pabyRec + 8, 4);

    if (bBigEndian)
        SwapWord(4, &(nSHPType));

    /* -------------------------------------------------------------------- */
    /*    Allocate and minimally initialize the object.            */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    SHPObject *psShape;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    /*      Allocate and minimally initialize the object.                   */
    /* -------------------------------------------------------------------- */
=======
    /*    Allocate and minimally initialize the object.            */
    /* -------------------------------------------------------------------- */
    SHPObject *psShape;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    if (psSHP->bFastModeReadObject) {
        if (psSHP->psCachedObject->bFastModeReadObject) {
            psSHP->sHooks.Error("Invalid read pattern in fast read mode. "
                                "SHPDestroyObject() should be called.");
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            return SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            return SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            return SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            return NULL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            return NULL;
=======
            return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        }

        psShape = psSHP->psCachedObject;
        memset(psShape, 0, sizeof(SHPObject));
    }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
    else {
        psShape = STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
    }
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    else {
        psShape = STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
    }
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    else
        psShape = (SHPObject *)calloc(1, sizeof(SHPObject));
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    else
        psShape = (SHPObject *)calloc(1, sizeof(SHPObject));
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    else {
        psShape = STATIC_CAST(SHPObject *, calloc(1, sizeof(SHPObject)));
    }
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    psShape->nShapeId = hEntity;
    psShape->nSHPType = nSHPType;
    psShape->bMeasureIsUsed = FALSE;
    psShape->bFastModeReadObject = psSHP->bFastModeReadObject;

    /* ==================================================================== */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a Polygon or Arc.                */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a Polygon or Arc.                */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a Polygon or Arc.                */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a Polygon or Arc.                */
=======
>>>>>>> osgeo-main
    /*  Extract vertices for a Polygon or Arc.                              */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    /*  Extract vertices for a Polygon or Arc.                              */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    /*  Extract vertices for a Polygon or Arc.                */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    /*  Extract vertices for a Polygon or Arc.                              */
=======
    /*  Extract vertices for a Polygon or Arc.                */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /* ==================================================================== */
    if (psShape->nSHPType == SHPT_POLYGON || psShape->nSHPType == SHPT_ARC ||
        psShape->nSHPType == SHPT_POLYGONZ ||
        psShape->nSHPType == SHPT_POLYGONM || psShape->nSHPType == SHPT_ARCZ ||
        psShape->nSHPType == SHPT_ARCM ||
        psShape->nSHPType == SHPT_MULTIPATCH) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        if (40 + 8 + 4 > nEntitySize) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
        int32 nPoints, nParts;
        int i, nOffset;
        unsigned char *pBuffer = NULL;
        unsigned char **ppBuffer = NULL;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        if (40 + 8 + 4 > nEntitySize) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        int32 nPoints, nParts;
        int i, nOffset;
        unsigned char *pBuffer = NULL;
        unsigned char **ppBuffer = NULL;

        if (40 + 8 + 4 > nEntitySize) {
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        if (40 + 8 + 4 > nEntitySize) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }
        /* --------------------------------------------------------------------
         */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        /*    Get the X/Y bounds.                        */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        /*    Get the X/Y bounds.                        */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        /*    Get the X/Y bounds.                        */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        /*    Get the X/Y bounds.                        */
=======
>>>>>>> osgeo-main
        /*      Get the X/Y bounds. */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        /*      Get the X/Y bounds. */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        /*    Get the X/Y bounds.                        */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        /*      Get the X/Y bounds. */
=======
        /*    Get the X/Y bounds.                        */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        /* --------------------------------------------------------------------
         */
        memcpy(&(psShape->dfXMin), psSHP->pabyRec + 8 + 4, 8);
        memcpy(&(psShape->dfYMin), psSHP->pabyRec + 8 + 12, 8);
        memcpy(&(psShape->dfXMax), psSHP->pabyRec + 8 + 20, 8);
        memcpy(&(psShape->dfYMax), psSHP->pabyRec + 8 + 28, 8);

        if (bBigEndian)
            SwapWord(8, &(psShape->dfXMin));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfYMin));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfXMax));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfYMax));

        /* --------------------------------------------------------------------
         */
        /*      Extract part/point count, and build vertex and part arrays */
        /*      to proper size. */
        /* --------------------------------------------------------------------
         */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        int32 nPoints;
        memcpy(&nPoints, psSHP->pabyRec + 40 + 8, 4);
        int32 nParts;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int32 nPoints;
        memcpy(&nPoints, psSHP->pabyRec + 40 + 8, 4);
        int32 nParts;
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        memcpy(&nPoints, psSHP->pabyRec + 40 + 8, 4);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        memcpy(&nPoints, psSHP->pabyRec + 40 + 8, 4);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        memcpy(&nPoints, psSHP->pabyRec + 40 + 8, 4);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        int32 nPoints;
        memcpy(&nPoints, psSHP->pabyRec + 40 + 8, 4);
        int32 nParts;
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        memcpy(&nParts, psSHP->pabyRec + 36 + 8, 4);

        if (bBigEndian)
            SwapWord(4, &nPoints);
        if (bBigEndian)
            SwapWord(4, &nParts);

        /* nPoints and nParts are unsigned */
        if (/* nPoints < 0 || nParts < 0 || */
            nPoints > 50 * 1000 * 1000 || nParts > 10 * 1000 * 1000) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
            char szErrorMsg[160];
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
            char szErrorMsg[160];
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d, nPoints=%u, nParts=%u.",
                     hEntity, nPoints, nParts);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d, nPoints=%u, nParts=%u.",
                     hEntity, nPoints, nParts);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            char szErrorMsg[160];
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
            char szErrorMsg[160];
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d, nPoints=%u, nParts=%u.",
                     hEntity, nPoints, nParts);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d, nPoints=%u, nParts=%u.",
                     hEntity, nPoints, nParts);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }

        /* With the previous checks on nPoints and nParts, */
        /* we should not overflow here and after */
        /* since 50 M * (16 + 8 + 8) = 1 600 MB */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int nRequiredSize = 44 + 8 + 4 * nParts + 16 * nPoints;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int nRequiredSize = 44 + 8 + 4 * nParts + 16 * nPoints;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
        int nRequiredSize = 44 + 8 + 4 * nParts + 16 * nPoints;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
        int nRequiredSize = 44 + 8 + 4 * nParts + 16 * nPoints;
=======
>>>>>>> osgeo-main
        nRequiredSize = 44 + 8 + 4 * nParts + 16 * nPoints;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        int nRequiredSize = 44 + 8 + 4 * nParts + 16 * nPoints;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        nRequiredSize = 44 + 8 + 4 * nParts + 16 * nPoints;
=======
        int nRequiredSize = 44 + 8 + 4 * nParts + 16 * nPoints;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        if (psShape->nSHPType == SHPT_POLYGONZ ||
            psShape->nSHPType == SHPT_ARCZ ||
            psShape->nSHPType == SHPT_MULTIPATCH) {
            nRequiredSize += 16 + 8 * nPoints;
        }
        if (psShape->nSHPType == SHPT_MULTIPATCH) {
            nRequiredSize += 4 * nParts;
        }
        if (nRequiredSize > nEntitySize) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d, nPoints=%u, nParts=%u, "
                     "nEntitySize=%d.",
                     hEntity, nPoints, nParts, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
            char szErrorMsg[160];
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d, nPoints=%u, nParts=%u, "
                     "nEntitySize=%d.",
                     hEntity, nPoints, nParts, nEntitySize);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d, nPoints=%d, nParts=%d, "
                     "nEntitySize=%d.",
                     hEntity, nPoints, nParts, nEntitySize);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d, nPoints=%u, nParts=%u, "
                     "nEntitySize=%d.",
                     hEntity, nPoints, nParts, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        unsigned char *pBuffer = SHPLIB_NULLPTR;
        unsigned char **ppBuffer = SHPLIB_NULLPTR;

        if (psShape->bFastModeReadObject) {
            const int nObjectBufSize =
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
        if (psShape->bFastModeReadObject) {
            int nObjectBufSize =
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        if (psShape->bFastModeReadObject) {
            int nObjectBufSize =
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        if (psShape->bFastModeReadObject) {
            int nObjectBufSize =
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        unsigned char *pBuffer = SHPLIB_NULLPTR;
        unsigned char **ppBuffer = SHPLIB_NULLPTR;

        if (psShape->bFastModeReadObject) {
            const int nObjectBufSize =
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                4 * sizeof(double) * nPoints + 2 * sizeof(int) * nParts;
            pBuffer = SHPReallocObjectBufIfNecessary(psSHP, nObjectBufSize);
            ppBuffer = &pBuffer;
        }

        psShape->nVertices = nPoints;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psShape->padfX = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfY = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfZ = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfM = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

        psShape->nParts = nParts;
        psShape->panPartStart =
            STATIC_CAST(int *, SHPAllocBuffer(ppBuffer, nParts * sizeof(int)));
        psShape->panPartType =
            STATIC_CAST(int *, SHPAllocBuffer(ppBuffer, nParts * sizeof(int)));

        if (psShape->padfX == SHPLIB_NULLPTR ||
            psShape->padfY == SHPLIB_NULLPTR ||
            psShape->padfZ == SHPLIB_NULLPTR ||
            psShape->padfM == SHPLIB_NULLPTR ||
            psShape->panPartStart == SHPLIB_NULLPTR ||
            psShape->panPartType == SHPLIB_NULLPTR) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Not enough memory to allocate requested memory "
                     "(nPoints=%u, nParts=%u) for shape %d. "
                     "Probably broken SHP file",
                     nPoints, nParts, hEntity);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
        psShape->padfX =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfY =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfZ =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfM =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

        psShape->nParts = nParts;
        psShape->panPartStart =
            STATIC_CAST(int *, SHPAllocBuffer(ppBuffer, nParts * sizeof(int)));
        psShape->panPartType =
            STATIC_CAST(int *, SHPAllocBuffer(ppBuffer, nParts * sizeof(int)));

        if (psShape->padfX == SHPLIB_NULLPTR ||
            psShape->padfY == SHPLIB_NULLPTR ||
            psShape->padfZ == SHPLIB_NULLPTR ||
            psShape->padfM == SHPLIB_NULLPTR ||
            psShape->panPartStart == SHPLIB_NULLPTR ||
            psShape->panPartType == SHPLIB_NULLPTR) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Not enough memory to allocate requested memory "
                     "(nPoints=%u, nParts=%u) for shape %d. "
                     "Probably broken SHP file",
<<<<<<< HEAD
                     hEntity, nPoints, nParts);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        psShape->padfX =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfY =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfZ =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfM =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);

        psShape->nParts = nParts;
        psShape->panPartStart =
            (int *)SHPAllocBuffer(ppBuffer, nParts * sizeof(int));
        psShape->panPartType =
            (int *)SHPAllocBuffer(ppBuffer, nParts * sizeof(int));

        if (psShape->padfX == NULL || psShape->padfY == NULL ||
            psShape->padfZ == NULL || psShape->padfM == NULL ||
            psShape->panPartStart == NULL || psShape->panPartType == NULL) {
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Not enough memory to allocate requested memory "
                     "(nPoints=%d, nParts=%d) for shape %d. "
                     "Probably broken SHP file",
                     hEntity, nPoints, nParts);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                     nPoints, nParts, hEntity);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        psShape->padfX = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfY = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfZ = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfM = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));

        psShape->nParts = nParts;
        psShape->panPartStart =
            STATIC_CAST(int *, SHPAllocBuffer(ppBuffer, nParts * sizeof(int)));
        psShape->panPartType =
            STATIC_CAST(int *, SHPAllocBuffer(ppBuffer, nParts * sizeof(int)));

        if (psShape->padfX == SHPLIB_NULLPTR ||
            psShape->padfY == SHPLIB_NULLPTR ||
            psShape->padfZ == SHPLIB_NULLPTR ||
            psShape->padfM == SHPLIB_NULLPTR ||
            psShape->panPartStart == SHPLIB_NULLPTR ||
            psShape->panPartType == SHPLIB_NULLPTR) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Not enough memory to allocate requested memory "
                     "(nPoints=%u, nParts=%u) for shape %d. "
                     "Probably broken SHP file",
                     nPoints, nParts, hEntity);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++)
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++)
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++)
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++)
=======
>>>>>>> osgeo-main
        for (i = 0; (int32)i < nParts; i++)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = 0; (int32)i < nParts; i++)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        for (i = 0; (int32)i < nParts; i++)
=======
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psShape->panPartType[i] = SHPP_RING;

        /* --------------------------------------------------------------------
         */
        /*      Copy out the part array from the record. */
        /* --------------------------------------------------------------------
         */
        memcpy(psShape->panPartStart, psSHP->pabyRec + 44 + 8, 4 * nParts);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
=======
>>>>>>> osgeo-main
        for (i = 0; (int32)i < nParts; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = 0; (int32)i < nParts; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        for (i = 0; (int32)i < nParts; i++) {
=======
        for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            if (bBigEndian)
                SwapWord(4, psShape->panPartStart + i);

            /* We check that the offset is inside the vertex array */
            if (psShape->panPartStart[i] < 0 ||
                (psShape->panPartStart[i] >= psShape->nVertices &&
                 psShape->nVertices > 0) ||
                (psShape->panPartStart[i] > 0 && psShape->nVertices == 0)) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                char szErrorMsg[160];
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                char szErrorMsg[160];
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                char szErrorMsg[160];
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
                char szErrorMsg[160];
=======
>>>>>>> osgeo-main
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                char szErrorMsg[160];
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
                char szErrorMsg[160];
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                snprintf(szErrorMsg, sizeof(szErrorMsg),
                         "Corrupted .shp file : shape %d : panPartStart[%d] = "
                         "%d, nVertices = %d",
                         hEntity, i, psShape->panPartStart[i],
                         psShape->nVertices);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
>>>>>>> osgeo-main
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                psSHP->sHooks.Error(szErrorMsg);
                SHPDestroyObject(psShape);
                return SHPLIB_NULLPTR;
            }
            if (i > 0 &&
                psShape->panPartStart[i] <= psShape->panPartStart[i - 1]) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                char szErrorMsg[160];
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                char szErrorMsg[160];
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                char szErrorMsg[160];
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
                char szErrorMsg[160];
=======
>>>>>>> osgeo-main
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                char szErrorMsg[160];
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
                char szErrorMsg[160];
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                snprintf(szErrorMsg, sizeof(szErrorMsg),
                         "Corrupted .shp file : shape %d : panPartStart[%d] = "
                         "%d, panPartStart[%d] = %d",
                         hEntity, i, psShape->panPartStart[i], i - 1,
                         psShape->panPartStart[i - 1]);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
=======
>>>>>>> osgeo-main
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
                szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                psSHP->sHooks.Error(szErrorMsg);
                SHPDestroyObject(psShape);
                return SHPLIB_NULLPTR;
            }
        }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int nOffset = 44 + 8 + 4 * nParts;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int nOffset = 44 + 8 + 4 * nParts;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int nOffset = 44 + 8 + 4 * nParts;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        int nOffset = 44 + 8 + 4 * nParts;
=======
>>>>>>> osgeo-main
        nOffset = 44 + 8 + 4 * nParts;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        nOffset = 44 + 8 + 4 * nParts;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        int nOffset = 44 + 8 + 4 * nParts;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        nOffset = 44 + 8 + 4 * nParts;
=======
        int nOffset = 44 + 8 + 4 * nParts;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

        /* --------------------------------------------------------------------
         */
        /*      If this is a multipatch, we will also have parts types. */
        /* --------------------------------------------------------------------
         */
        if (psShape->nSHPType == SHPT_MULTIPATCH) {
            memcpy(psShape->panPartType, psSHP->pabyRec + nOffset, 4 * nParts);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
=======
>>>>>>> osgeo-main
            for (i = 0; (int32)i < nParts; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (i = 0; (int32)i < nParts; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; (int32)i < nParts; i++) {
=======
            for (int i = 0; STATIC_CAST(int32, i) < nParts; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                if (bBigEndian)
                    SwapWord(4, psShape->panPartType + i);
            }

            nOffset += 4 * nParts;
        }

        /* --------------------------------------------------------------------
         */
        /*      Copy out the vertices from the record. */
        /* --------------------------------------------------------------------
         */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
        for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        for (i = 0; (int32)i < nPoints; i++) {
=======
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            memcpy(psShape->padfX + i, psSHP->pabyRec + nOffset + i * 16, 8);

            memcpy(psShape->padfY + i, psSHP->pabyRec + nOffset + i * 16 + 8,
                   8);

            if (bBigEndian)
                SwapWord(8, psShape->padfX + i);
            if (bBigEndian)
                SwapWord(8, psShape->padfY + i);
        }

        nOffset += 16 * nPoints;

        /* --------------------------------------------------------------------
         */
        /*      If we have a Z coordinate, collect that now. */
        /* --------------------------------------------------------------------
         */
        if (psShape->nSHPType == SHPT_POLYGONZ ||
            psShape->nSHPType == SHPT_ARCZ ||
            psShape->nSHPType == SHPT_MULTIPATCH) {
            memcpy(&(psShape->dfZMin), psSHP->pabyRec + nOffset, 8);
            memcpy(&(psShape->dfZMax), psSHP->pabyRec + nOffset + 8, 8);

            if (bBigEndian)
                SwapWord(8, &(psShape->dfZMin));
            if (bBigEndian)
                SwapWord(8, &(psShape->dfZMax));

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
            for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; (int32)i < nPoints; i++) {
=======
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                memcpy(psShape->padfZ + i,
                       psSHP->pabyRec + nOffset + 16 + i * 8, 8);
                if (bBigEndian)
                    SwapWord(8, psShape->padfZ + i);
            }

            nOffset += 16 + 8 * nPoints;
        }
        else if (psShape->bFastModeReadObject) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfZ = SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfZ = SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfZ = SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfZ = SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            psShape->padfZ = NULL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            psShape->padfZ = SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            psShape->padfZ = NULL;
=======
            psShape->padfZ = SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        }

        /* --------------------------------------------------------------------
         */
        /*      If we have a M measure value, then read it now.  We assume */
        /*      that the measure can be present for any shape if the size is */
        /*      big enough, but really it will only occur for the Z shapes */
        /*      (options), and the M shapes. */
        /* --------------------------------------------------------------------
         */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
=======
>>>>>>> osgeo-main
        if (nEntitySize >= (int)(nOffset + 16 + 8 * nPoints)) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        if (nEntitySize >= (int)(nOffset + 16 + 8 * nPoints)) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        if (nEntitySize >= (int)(nOffset + 16 + 8 * nPoints)) {
=======
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            memcpy(&(psShape->dfMMin), psSHP->pabyRec + nOffset, 8);
            memcpy(&(psShape->dfMMax), psSHP->pabyRec + nOffset + 8, 8);

            if (bBigEndian)
                SwapWord(8, &(psShape->dfMMin));
            if (bBigEndian)
                SwapWord(8, &(psShape->dfMMax));

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
            for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; (int32)i < nPoints; i++) {
=======
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                memcpy(psShape->padfM + i,
                       psSHP->pabyRec + nOffset + 16 + i * 8, 8);
                if (bBigEndian)
                    SwapWord(8, psShape->padfM + i);
            }
            psShape->bMeasureIsUsed = TRUE;
        }
        else if (psShape->bFastModeReadObject) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfM = SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfM = SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfM = SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfM = SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            psShape->padfM = NULL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            psShape->padfM = SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            psShape->padfM = NULL;
=======
            psShape->padfM = SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        }
    }

    /* ==================================================================== */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a MultiPoint.                    */
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a MultiPoint.                    */
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a MultiPoint.                    */
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    /*  Extract vertices for a MultiPoint.                    */
=======
>>>>>>> osgeo-main
    /*  Extract vertices for a MultiPoint.                                  */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    /*  Extract vertices for a MultiPoint.                                  */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    /*  Extract vertices for a MultiPoint.                    */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    /*  Extract vertices for a MultiPoint.                                  */
=======
    /*  Extract vertices for a MultiPoint.                    */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /* ==================================================================== */
    else if (psShape->nSHPType == SHPT_MULTIPOINT ||
             psShape->nSHPType == SHPT_MULTIPOINTM ||
             psShape->nSHPType == SHPT_MULTIPOINTZ) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        if (44 + 4 > nEntitySize) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }
        int32 nPoints;
        memcpy(&nPoints, psSHP->pabyRec + 44, 4);

=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        int32 nPoints;
        int i, nOffset;
        unsigned char *pBuffer = NULL;
        unsigned char **ppBuffer = NULL;

        if (44 + 4 > nEntitySize) {
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        if (44 + 4 > nEntitySize) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }
<<<<<<< HEAD
<<<<<<< HEAD
        int32 nPoints;
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        int32 nPoints;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        memcpy(&nPoints, psSHP->pabyRec + 44, 4);

=======
        int32 nPoints;
        int i, nOffset;
        unsigned char *pBuffer = NULL;
        unsigned char **ppBuffer = NULL;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        if (44 + 4 > nEntitySize) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }
        int32 nPoints;
        memcpy(&nPoints, psSHP->pabyRec + 44, 4);

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        if (bBigEndian)
            SwapWord(4, &nPoints);

        /* nPoints is unsigned */
        if (/* nPoints < 0 || */ nPoints > 50 * 1000 * 1000) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nPoints = %u", hEntity,
                     nPoints);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
            char szErrorMsg[160];
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nPoints = %u", hEntity,
                     nPoints);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nPoints = %d", hEntity,
                     nPoints);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nPoints = %d", hEntity,
                     nPoints);
=======
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nPoints = %u", hEntity,
                     nPoints);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int nRequiredSize = 48 + nPoints * 16;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int nRequiredSize = 48 + nPoints * 16;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
        int nRequiredSize = 48 + nPoints * 16;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
        int nRequiredSize = 48 + nPoints * 16;
=======
>>>>>>> osgeo-main
        nRequiredSize = 48 + nPoints * 16;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        int nRequiredSize = 48 + nPoints * 16;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        nRequiredSize = 48 + nPoints * 16;
=======
        int nRequiredSize = 48 + nPoints * 16;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        if (psShape->nSHPType == SHPT_MULTIPOINTZ) {
            nRequiredSize += 16 + nPoints * 8;
        }
        if (nRequiredSize > nEntitySize) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nPoints = %u, "
                     "nEntitySize = %d",
                     hEntity, nPoints, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
            char szErrorMsg[160];
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nPoints = %u, "
                     "nEntitySize = %d",
                     hEntity, nPoints, nEntitySize);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nPoints = %d, "
                     "nEntitySize = %d",
                     hEntity, nPoints, nEntitySize);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nPoints = %u, "
                     "nEntitySize = %d",
                     hEntity, nPoints, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        unsigned char *pBuffer = SHPLIB_NULLPTR;
        unsigned char **ppBuffer = SHPLIB_NULLPTR;

        if (psShape->bFastModeReadObject) {
            const int nObjectBufSize = 4 * sizeof(double) * nPoints;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
        if (psShape->bFastModeReadObject) {
            int nObjectBufSize = 4 * sizeof(double) * nPoints;

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        if (psShape->bFastModeReadObject) {
            int nObjectBufSize = 4 * sizeof(double) * nPoints;

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        if (psShape->bFastModeReadObject) {
            int nObjectBufSize = 4 * sizeof(double) * nPoints;

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        unsigned char *pBuffer = SHPLIB_NULLPTR;
        unsigned char **ppBuffer = SHPLIB_NULLPTR;

        if (psShape->bFastModeReadObject) {
            const int nObjectBufSize = 4 * sizeof(double) * nPoints;
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            pBuffer = SHPReallocObjectBufIfNecessary(psSHP, nObjectBufSize);
            ppBuffer = &pBuffer;
        }

        psShape->nVertices = nPoints;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psShape->padfX = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfY = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfZ = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfM = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

        if (psShape->padfX == SHPLIB_NULLPTR ||
            psShape->padfY == SHPLIB_NULLPTR ||
            psShape->padfZ == SHPLIB_NULLPTR ||
            psShape->padfM == SHPLIB_NULLPTR) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Not enough memory to allocate requested memory "
                     "(nPoints=%u) for shape %d. "
                     "Probably broken SHP file",
                     nPoints, hEntity);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
        psShape->padfX =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfY =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfZ =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfM =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

        if (psShape->padfX == SHPLIB_NULLPTR ||
            psShape->padfY == SHPLIB_NULLPTR ||
            psShape->padfZ == SHPLIB_NULLPTR ||
            psShape->padfM == SHPLIB_NULLPTR) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Not enough memory to allocate requested memory "
                     "(nPoints=%u) for shape %d. "
                     "Probably broken SHP file",
<<<<<<< HEAD
                     hEntity, nPoints);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        psShape->padfX =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfY =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfZ =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);
        psShape->padfM =
            (double *)SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints);

        if (psShape->padfX == NULL || psShape->padfY == NULL ||
            psShape->padfZ == NULL || psShape->padfM == NULL) {
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Not enough memory to allocate requested memory "
                     "(nPoints=%d) for shape %d. "
                     "Probably broken SHP file",
                     hEntity, nPoints);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                     nPoints, hEntity);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        psShape->padfX = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfY = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfZ = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));
        psShape->padfM = STATIC_CAST(
            double *, SHPAllocBuffer(ppBuffer, sizeof(double) * nPoints));

        if (psShape->padfX == SHPLIB_NULLPTR ||
            psShape->padfY == SHPLIB_NULLPTR ||
            psShape->padfZ == SHPLIB_NULLPTR ||
            psShape->padfM == SHPLIB_NULLPTR) {
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Not enough memory to allocate requested memory "
                     "(nPoints=%u) for shape %d. "
                     "Probably broken SHP file",
                     nPoints, hEntity);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
        for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        for (i = 0; (int32)i < nPoints; i++) {
=======
        for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            memcpy(psShape->padfX + i, psSHP->pabyRec + 48 + 16 * i, 8);
            memcpy(psShape->padfY + i, psSHP->pabyRec + 48 + 16 * i + 8, 8);

            if (bBigEndian)
                SwapWord(8, psShape->padfX + i);
            if (bBigEndian)
                SwapWord(8, psShape->padfY + i);
        }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
        int nOffset = 48 + 16 * nPoints;

        /* --------------------------------------------------------------------
         */
        /*    Get the X/Y bounds.                        */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        /* --------------------------------------------------------------------
         */
        memcpy(&(psShape->dfXMin), psSHP->pabyRec + 8 + 4, 8);
        memcpy(&(psShape->dfYMin), psSHP->pabyRec + 8 + 12, 8);
        memcpy(&(psShape->dfXMax), psSHP->pabyRec + 8 + 20, 8);
        memcpy(&(psShape->dfYMax), psSHP->pabyRec + 8 + 28, 8);

        if (bBigEndian)
            SwapWord(8, &(psShape->dfXMin));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfYMin));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfXMax));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfYMax));

        /* --------------------------------------------------------------------
         */
        /*      If we have a Z coordinate, collect that now. */
        /* --------------------------------------------------------------------
         */
        if (psShape->nSHPType == SHPT_MULTIPOINTZ) {
            memcpy(&(psShape->dfZMin), psSHP->pabyRec + nOffset, 8);
            memcpy(&(psShape->dfZMax), psSHP->pabyRec + nOffset + 8, 8);

            if (bBigEndian)
                SwapWord(8, &(psShape->dfZMin));
            if (bBigEndian)
                SwapWord(8, &(psShape->dfZMax));

            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        nOffset = 48 + 16 * nPoints;

        /* --------------------------------------------------------------------
         */
        /*      Get the X/Y bounds. */
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        int nOffset = 48 + 16 * nPoints;

        /* --------------------------------------------------------------------
         */
        /*    Get the X/Y bounds.                        */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        /* --------------------------------------------------------------------
         */
        memcpy(&(psShape->dfXMin), psSHP->pabyRec + 8 + 4, 8);
        memcpy(&(psShape->dfYMin), psSHP->pabyRec + 8 + 12, 8);
        memcpy(&(psShape->dfXMax), psSHP->pabyRec + 8 + 20, 8);
        memcpy(&(psShape->dfYMax), psSHP->pabyRec + 8 + 28, 8);

        if (bBigEndian)
            SwapWord(8, &(psShape->dfXMin));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfYMin));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfXMax));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfYMax));

        /* --------------------------------------------------------------------
         */
        /*      If we have a Z coordinate, collect that now. */
        /* --------------------------------------------------------------------
         */
        if (psShape->nSHPType == SHPT_MULTIPOINTZ) {
            memcpy(&(psShape->dfZMin), psSHP->pabyRec + nOffset, 8);
            memcpy(&(psShape->dfZMax), psSHP->pabyRec + nOffset + 8, 8);

            if (bBigEndian)
                SwapWord(8, &(psShape->dfZMin));
            if (bBigEndian)
                SwapWord(8, &(psShape->dfZMax));

<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
            for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        nOffset = 48 + 16 * nPoints;
=======
        int nOffset = 48 + 16 * nPoints;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

        /* --------------------------------------------------------------------
         */
        /*    Get the X/Y bounds.                        */
        /* --------------------------------------------------------------------
         */
        memcpy(&(psShape->dfXMin), psSHP->pabyRec + 8 + 4, 8);
        memcpy(&(psShape->dfYMin), psSHP->pabyRec + 8 + 12, 8);
        memcpy(&(psShape->dfXMax), psSHP->pabyRec + 8 + 20, 8);
        memcpy(&(psShape->dfYMax), psSHP->pabyRec + 8 + 28, 8);

        if (bBigEndian)
            SwapWord(8, &(psShape->dfXMin));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfYMin));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfXMax));
        if (bBigEndian)
            SwapWord(8, &(psShape->dfYMax));

        /* --------------------------------------------------------------------
         */
        /*      If we have a Z coordinate, collect that now. */
        /* --------------------------------------------------------------------
         */
        if (psShape->nSHPType == SHPT_MULTIPOINTZ) {
            memcpy(&(psShape->dfZMin), psSHP->pabyRec + nOffset, 8);
            memcpy(&(psShape->dfZMax), psSHP->pabyRec + nOffset + 8, 8);

            if (bBigEndian)
                SwapWord(8, &(psShape->dfZMin));
            if (bBigEndian)
                SwapWord(8, &(psShape->dfZMax));

<<<<<<< HEAD
            for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; (int32)i < nPoints; i++) {
=======
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                memcpy(psShape->padfZ + i,
                       psSHP->pabyRec + nOffset + 16 + i * 8, 8);
                if (bBigEndian)
                    SwapWord(8, psShape->padfZ + i);
            }

            nOffset += 16 + 8 * nPoints;
        }
        else if (psShape->bFastModeReadObject)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfZ = SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfZ = SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfZ = SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfZ = SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            psShape->padfZ = NULL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            psShape->padfZ = SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            psShape->padfZ = NULL;
=======
            psShape->padfZ = SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

        /* --------------------------------------------------------------------
         */
        /*      If we have a M measure value, then read it now.  We assume */
        /*      that the measure can be present for any shape if the size is */
        /*      big enough, but really it will only occur for the Z shapes */
        /*      (options), and the M shapes. */
        /* --------------------------------------------------------------------
         */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
=======
>>>>>>> osgeo-main
        if (nEntitySize >= (int)(nOffset + 16 + 8 * nPoints)) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        if (nEntitySize >= (int)(nOffset + 16 + 8 * nPoints)) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        if (nEntitySize >= (int)(nOffset + 16 + 8 * nPoints)) {
=======
        if (nEntitySize >= STATIC_CAST(int, nOffset + 16 + 8 * nPoints)) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            memcpy(&(psShape->dfMMin), psSHP->pabyRec + nOffset, 8);
            memcpy(&(psShape->dfMMax), psSHP->pabyRec + nOffset + 8, 8);

            if (bBigEndian)
                SwapWord(8, &(psShape->dfMMin));
            if (bBigEndian)
                SwapWord(8, &(psShape->dfMMax));

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
=======
>>>>>>> osgeo-main
            for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (i = 0; (int32)i < nPoints; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            for (i = 0; (int32)i < nPoints; i++) {
=======
            for (int i = 0; STATIC_CAST(int32, i) < nPoints; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                memcpy(psShape->padfM + i,
                       psSHP->pabyRec + nOffset + 16 + i * 8, 8);
                if (bBigEndian)
                    SwapWord(8, psShape->padfM + i);
            }
            psShape->bMeasureIsUsed = TRUE;
        }
        else if (psShape->bFastModeReadObject)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfM = SHPLIB_NULLPTR;
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfM = SHPLIB_NULLPTR;
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfM = SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
            psShape->padfM = SHPLIB_NULLPTR;
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            psShape->padfM = NULL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            psShape->padfM = SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            psShape->padfM = NULL;
=======
            psShape->padfM = SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    }

    /* ==================================================================== */
    /*      Extract vertices for a point.                                   */
    /* ==================================================================== */
    else if (psShape->nSHPType == SHPT_POINT ||
             psShape->nSHPType == SHPT_POINTM ||
             psShape->nSHPType == SHPT_POINTZ) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        int nOffset;

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        int nOffset;

=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        psShape->nVertices = 1;
        if (psShape->bFastModeReadObject) {
            psShape->padfX = &(psShape->dfXMin);
            psShape->padfY = &(psShape->dfYMin);
            psShape->padfZ = &(psShape->dfZMin);
            psShape->padfM = &(psShape->dfMMin);
            psShape->padfZ[0] = 0.0;
            psShape->padfM[0] = 0.0;
        }
        else {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psShape->padfX = STATIC_CAST(double *, calloc(1, sizeof(double)));
            psShape->padfY = STATIC_CAST(double *, calloc(1, sizeof(double)));
            psShape->padfZ = STATIC_CAST(double *, calloc(1, sizeof(double)));
            psShape->padfM = STATIC_CAST(double *, calloc(1, sizeof(double)));
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            psShape->padfX = (double *)calloc(1, sizeof(double));
            psShape->padfY = (double *)calloc(1, sizeof(double));
            psShape->padfZ = (double *)calloc(1, sizeof(double));
            psShape->padfM = (double *)calloc(1, sizeof(double));
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
            psShape->padfX = STATIC_CAST(double *, calloc(1, sizeof(double)));
            psShape->padfY = STATIC_CAST(double *, calloc(1, sizeof(double)));
            psShape->padfZ = STATIC_CAST(double *, calloc(1, sizeof(double)));
            psShape->padfM = STATIC_CAST(double *, calloc(1, sizeof(double)));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        }

        if (20 + 8 + ((psShape->nSHPType == SHPT_POINTZ) ? 8 : 0) >
            nEntitySize) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
=======
            char szErrorMsg[160];
            snprintf(szErrorMsg, sizeof(szErrorMsg),
                     "Corrupted .shp file : shape %d : nEntitySize = %d",
                     hEntity, nEntitySize);
            szErrorMsg[sizeof(szErrorMsg) - 1] = '\0';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            psSHP->sHooks.Error(szErrorMsg);
            SHPDestroyObject(psShape);
            return SHPLIB_NULLPTR;
        }
        memcpy(psShape->padfX, psSHP->pabyRec + 12, 8);
        memcpy(psShape->padfY, psSHP->pabyRec + 20, 8);

        if (bBigEndian)
            SwapWord(8, psShape->padfX);
        if (bBigEndian)
            SwapWord(8, psShape->padfY);

        int nOffset = 20 + 8;

        /* --------------------------------------------------------------------
         */
        /*      If we have a Z coordinate, collect that now. */
        /* --------------------------------------------------------------------
         */
        if (psShape->nSHPType == SHPT_POINTZ) {
            memcpy(psShape->padfZ, psSHP->pabyRec + nOffset, 8);

            if (bBigEndian)
                SwapWord(8, psShape->padfZ);

            nOffset += 8;
        }

        /* --------------------------------------------------------------------
         */
        /*      If we have a M measure value, then read it now.  We assume */
        /*      that the measure can be present for any shape if the size is */
        /*      big enough, but really it will only occur for the Z shapes */
        /*      (options), and the M shapes. */
        /* --------------------------------------------------------------------
         */
        if (nEntitySize >= nOffset + 8) {
            memcpy(psShape->padfM, psSHP->pabyRec + nOffset, 8);

            if (bBigEndian)
                SwapWord(8, psShape->padfM);
            psShape->bMeasureIsUsed = TRUE;
        }

        /* --------------------------------------------------------------------
         */
        /*      Since no extents are supplied in the record, we will apply */
        /*      them from the single vertex. */
        /* --------------------------------------------------------------------
         */
        psShape->dfXMin = psShape->dfXMax = psShape->padfX[0];
        psShape->dfYMin = psShape->dfYMax = psShape->padfY[0];
        psShape->dfZMin = psShape->dfZMax = psShape->padfZ[0];
        psShape->dfMMin = psShape->dfMMax = psShape->padfM[0];
    }

    return (psShape);
}

/************************************************************************/
/*                            SHPTypeName()                             */
/************************************************************************/

const char SHPAPI_CALL1(*) SHPTypeName(int nSHPType)
{
    switch (nSHPType) {
    case SHPT_NULL:
        return "NullShape";

    case SHPT_POINT:
        return "Point";

    case SHPT_ARC:
        return "Arc";

    case SHPT_POLYGON:
        return "Polygon";

    case SHPT_MULTIPOINT:
        return "MultiPoint";

    case SHPT_POINTZ:
        return "PointZ";

    case SHPT_ARCZ:
        return "ArcZ";

    case SHPT_POLYGONZ:
        return "PolygonZ";

    case SHPT_MULTIPOINTZ:
        return "MultiPointZ";

    case SHPT_POINTM:
        return "PointM";

    case SHPT_ARCM:
        return "ArcM";

    case SHPT_POLYGONM:
        return "PolygonM";

    case SHPT_MULTIPOINTM:
        return "MultiPointM";

    case SHPT_MULTIPATCH:
        return "MultiPatch";

    default:
        return "UnknownShapeType";
    }
}

/************************************************************************/
/*                          SHPPartTypeName()                           */
/************************************************************************/

const char SHPAPI_CALL1(*) SHPPartTypeName(int nPartType)
{
    switch (nPartType) {
    case SHPP_TRISTRIP:
        return "TriangleStrip";

    case SHPP_TRIFAN:
        return "TriangleFan";

    case SHPP_OUTERRING:
        return "OuterRing";

    case SHPP_INNERRING:
        return "InnerRing";

    case SHPP_FIRSTRING:
        return "FirstRing";

    case SHPP_RING:
        return "Ring";

    default:
        return "UnknownPartType";
    }
}

/************************************************************************/
/*                          SHPDestroyObject()                          */
/************************************************************************/

void SHPAPI_CALL SHPDestroyObject(SHPObject *psShape)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psShape == SHPLIB_NULLPTR)
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psShape == SHPLIB_NULLPTR)
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psShape == SHPLIB_NULLPTR)
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
    if (psShape == SHPLIB_NULLPTR)
=======
>>>>>>> osgeo-main
    if (psShape == NULL)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psShape == NULL)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psShape == SHPLIB_NULLPTR)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (psShape == NULL)
=======
    if (psShape == SHPLIB_NULLPTR)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        return;

    if (psShape->bFastModeReadObject) {
        psShape->bFastModeReadObject = FALSE;
        return;
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
    if (psShape->padfX != SHPLIB_NULLPTR)
        free(psShape->padfX);
    if (psShape->padfY != SHPLIB_NULLPTR)
        free(psShape->padfY);
    if (psShape->padfZ != SHPLIB_NULLPTR)
        free(psShape->padfZ);
    if (psShape->padfM != SHPLIB_NULLPTR)
        free(psShape->padfM);

    if (psShape->panPartStart != SHPLIB_NULLPTR)
        free(psShape->panPartStart);
    if (psShape->panPartType != SHPLIB_NULLPTR)
        free(psShape->panPartType);

    free(psShape);
}

/************************************************************************/
/*                       SHPGetPartVertexCount()                        */
/************************************************************************/

static int SHPGetPartVertexCount(const SHPObject *psObject, int iPart)
{
    if (iPart == psObject->nParts - 1)
        return psObject->nVertices - psObject->panPartStart[iPart];
    else
        return psObject->panPartStart[iPart + 1] -
               psObject->panPartStart[iPart];
}

/************************************************************************/
/*                      SHPRewindIsInnerRing()                          */
/************************************************************************/

/* Return -1 in case of ambiguity */
static int SHPRewindIsInnerRing(const SHPObject *psObject, int iOpRing,
                                double dfTestX, double dfTestY,
                                double dfRelativeTolerance, int bSameZ,
                                double dfTestZ)
{
    /* -------------------------------------------------------------------- */
    /*      Determine if this ring is an inner ring or an outer ring        */
    /*      relative to all the other rings.  For now we assume the         */
    /*      first ring is outer and all others are inner, but eventually    */
    /*      we need to fix this to handle multiple island polygons and      */
    /*      unordered sets of rings.                                        */
    /*                                                                      */
    /* -------------------------------------------------------------------- */

    bool bInner = false;
    for (int iCheckRing = 0; iCheckRing < psObject->nParts; iCheckRing++) {
        if (iCheckRing == iOpRing)
            continue;

        const int nVertStartCheck = psObject->panPartStart[iCheckRing];
        const int nVertCountCheck = SHPGetPartVertexCount(psObject, iCheckRing);

        /* Ignore rings that don't have the same (constant) Z value as the
         * point. */
        /* As noted in SHPRewindObject(), this is a simplification */
        /* of what we should ideally do. */
        if (!bSameZ) {
            int bZTestOK = TRUE;
            for (int iVert = nVertStartCheck + 1;
                 iVert < nVertStartCheck + nVertCountCheck; ++iVert) {
                if (psObject->padfZ[iVert] != dfTestZ) {
                    bZTestOK = FALSE;
                    break;
                }
            }
            if (!bZTestOK)
                continue;
        }

        for (int iEdge = 0; iEdge < nVertCountCheck; iEdge++) {
            int iNext;
            if (iEdge < nVertCountCheck - 1)
                iNext = iEdge + 1;
            else
                iNext = 0;

            const double y0 = psObject->padfY[iEdge + nVertStartCheck];
            const double y1 = psObject->padfY[iNext + nVertStartCheck];
            /* Rule #1:
             * Test whether the edge 'straddles' the horizontal ray from
             * the test point (dfTestY,dfTestY)
             * The rule #1 also excludes edges colinear with the ray.
             */
            if ((y0 < dfTestY && dfTestY <= y1) ||
                (y1 < dfTestY && dfTestY <= y0)) {
                /* Rule #2:
                 * Test if edge-ray intersection is on the right from the
                 * test point (dfTestY,dfTestY)
                 */
                const double x0 = psObject->padfX[iEdge + nVertStartCheck];
                const double x1 = psObject->padfX[iNext + nVertStartCheck];
                const double intersect_minus_testX =
                    (x0 - dfTestX) + (dfTestY - y0) / (y1 - y0) * (x1 - x0);

                if (fabs(intersect_minus_testX) <=
                    dfRelativeTolerance * fabs(dfTestX)) {
                    /* Potential shared edge, or slightly overlapping polygons
                     */
                    return -1;
                }
                else if (intersect_minus_testX < 0) {
                    bInner = !bInner;
                }
            }
        }
    } /* for iCheckRing */
    return bInner;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
    if (psShape->padfX != NULL)
=======
    if (psShape->padfX != SHPLIB_NULLPTR)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        free(psShape->padfX);
    if (psShape->padfY != SHPLIB_NULLPTR)
        free(psShape->padfY);
    if (psShape->padfZ != SHPLIB_NULLPTR)
        free(psShape->padfZ);
    if (psShape->padfM != SHPLIB_NULLPTR)
        free(psShape->padfM);

    if (psShape->panPartStart != SHPLIB_NULLPTR)
        free(psShape->panPartStart);
    if (psShape->panPartType != SHPLIB_NULLPTR)
        free(psShape->panPartType);

    free(psShape);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psShape->padfX != NULL)
        free(psShape->padfX);
    if (psShape->padfY != NULL)
        free(psShape->padfY);
    if (psShape->padfZ != NULL)
        free(psShape->padfZ);
    if (psShape->padfM != NULL)
        free(psShape->padfM);

    if (psShape->panPartStart != NULL)
        free(psShape->panPartStart);
    if (psShape->panPartType != NULL)
        free(psShape->panPartType);

    free(psShape);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    if (psShape->padfX != SHPLIB_NULLPTR)
        free(psShape->padfX);
    if (psShape->padfY != SHPLIB_NULLPTR)
        free(psShape->padfY);
    if (psShape->padfZ != SHPLIB_NULLPTR)
        free(psShape->padfZ);
    if (psShape->padfM != SHPLIB_NULLPTR)
        free(psShape->padfM);

    if (psShape->panPartStart != SHPLIB_NULLPTR)
        free(psShape->panPartStart);
    if (psShape->panPartType != SHPLIB_NULLPTR)
        free(psShape->panPartType);

    free(psShape);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                       SHPGetPartVertexCount()                        */
/************************************************************************/

static int SHPGetPartVertexCount(const SHPObject *psObject, int iPart)
{
    if (iPart == psObject->nParts - 1)
        return psObject->nVertices - psObject->panPartStart[iPart];
    else
        return psObject->panPartStart[iPart + 1] -
               psObject->panPartStart[iPart];
}

/************************************************************************/
/*                      SHPRewindIsInnerRing()                          */
/************************************************************************/

/* Return -1 in case of ambiguity */
static int SHPRewindIsInnerRing(const SHPObject *psObject, int iOpRing,
                                double dfTestX, double dfTestY,
                                double dfRelativeTolerance, int bSameZ,
                                double dfTestZ)
{
    /* -------------------------------------------------------------------- */
    /*      Determine if this ring is an inner ring or an outer ring        */
    /*      relative to all the other rings.  For now we assume the         */
    /*      first ring is outer and all others are inner, but eventually    */
    /*      we need to fix this to handle multiple island polygons and      */
    /*      unordered sets of rings.                                        */
    /*                                                                      */
    /* -------------------------------------------------------------------- */

    bool bInner = false;
    for (int iCheckRing = 0; iCheckRing < psObject->nParts; iCheckRing++) {
        if (iCheckRing == iOpRing)
            continue;

        const int nVertStartCheck = psObject->panPartStart[iCheckRing];
        const int nVertCountCheck = SHPGetPartVertexCount(psObject, iCheckRing);

        /* Ignore rings that don't have the same (constant) Z value as the
         * point. */
        /* As noted in SHPRewindObject(), this is a simplification */
        /* of what we should ideally do. */
        if (!bSameZ) {
            int bZTestOK = TRUE;
            for (int iVert = nVertStartCheck + 1;
                 iVert < nVertStartCheck + nVertCountCheck; ++iVert) {
                if (psObject->padfZ[iVert] != dfTestZ) {
                    bZTestOK = FALSE;
                    break;
                }
            }
            if (!bZTestOK)
                continue;
        }

        for (int iEdge = 0; iEdge < nVertCountCheck; iEdge++) {
            int iNext;
            if (iEdge < nVertCountCheck - 1)
                iNext = iEdge + 1;
            else
                iNext = 0;

            const double y0 = psObject->padfY[iEdge + nVertStartCheck];
            const double y1 = psObject->padfY[iNext + nVertStartCheck];
            /* Rule #1:
             * Test whether the edge 'straddles' the horizontal ray from
             * the test point (dfTestY,dfTestY)
             * The rule #1 also excludes edges colinear with the ray.
             */
            if ((y0 < dfTestY && dfTestY <= y1) ||
                (y1 < dfTestY && dfTestY <= y0)) {
                /* Rule #2:
                 * Test if edge-ray intersection is on the right from the
                 * test point (dfTestY,dfTestY)
                 */
                const double x0 = psObject->padfX[iEdge + nVertStartCheck];
                const double x1 = psObject->padfX[iNext + nVertStartCheck];
                const double intersect_minus_testX =
                    (x0 - dfTestX) + (dfTestY - y0) / (y1 - y0) * (x1 - x0);

                if (fabs(intersect_minus_testX) <=
                    dfRelativeTolerance * fabs(dfTestX)) {
                    /* Potential shared edge, or slightly overlapping polygons
                     */
                    return -1;
                }
                else if (intersect_minus_testX < 0) {
                    bInner = !bInner;
                }
            }
        }
    } /* for iCheckRing */
    return bInner;
<<<<<<< HEAD
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
}

/************************************************************************/
/*                          SHPRewindObject()                           */
/*                                                                      */
/*      Reset the winding of polygon objects to adhere to the           */
/*      specification.                                                  */
/************************************************************************/

int SHPAPI_CALL SHPRewindObject(CPL_UNUSED SHPHandle hSHP, SHPObject *psObject)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
    int iOpRing, bAltered = 0;

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    int iOpRing, bAltered = 0;

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    int iOpRing, bAltered = 0;

=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /* -------------------------------------------------------------------- */
    /*      Do nothing if this is not a polygon object.                     */
    /* -------------------------------------------------------------------- */
    if (psObject->nSHPType != SHPT_POLYGON &&
        psObject->nSHPType != SHPT_POLYGONZ &&
        psObject->nSHPType != SHPT_POLYGONM)
        return 0;

    if (psObject->nVertices == 0 || psObject->nParts == 0)
        return 0;

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    /*      Test if all points have the same Z value.                       */
    /* -------------------------------------------------------------------- */
    int bSameZ = TRUE;
    if (psObject->nSHPType == SHPT_POLYGONZ ||
        psObject->nSHPType == SHPT_POLYGONM) {
        for (int iVert = 1; iVert < psObject->nVertices; ++iVert) {
            if (psObject->padfZ[iVert] != psObject->padfZ[0]) {
                bSameZ = FALSE;
                break;
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> osgeo-main
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> osgeo-main
            }
        }
    }

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
    /*      Process each of the rings.                                      */
    /* -------------------------------------------------------------------- */
    int bAltered = 0;
    for (int iOpRing = 0; iOpRing < psObject->nParts; iOpRing++) {
        const int nVertStart = psObject->panPartStart[iOpRing];
        const int nVertCount = SHPGetPartVertexCount(psObject, iOpRing);

        if (nVertCount < 2)
            continue;

        /* If a ring has a non-constant Z value, then consider it as an outer */
        /* ring. */
        /* NOTE: this is a rough approximation. If we were smarter, */
        /* we would check that all points of the ring are coplanar, and compare
         */
        /* that to other rings in the same (oblique) plane. */
        int bDoIsInnerRingTest = TRUE;
        if (!bSameZ) {
            int bPartSameZ = TRUE;
            for (int iVert = nVertStart + 1; iVert < nVertStart + nVertCount;
                 ++iVert) {
                if (psObject->padfZ[iVert] != psObject->padfZ[nVertStart]) {
                    bPartSameZ = FALSE;
                    break;
                }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            }
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
            }
<<<<<<< HEAD
        }
    }
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
            if (!bPartSameZ)
                bDoIsInnerRingTest = FALSE;
        }

        int bInner = FALSE;
        if (bDoIsInnerRingTest) {
            for (int iTolerance = 0; iTolerance < 2; iTolerance++) {
                /* In a first attempt, use a relaxed criterion to decide if a
                 * point */
                /* is inside another ring. If all points of the current ring are
                 * in the */
                /* "grey" zone w.r.t that criterion, which seems really
                 * unlikely, */
                /* then use the strict criterion for another pass. */
                const double dfRelativeTolerance = (iTolerance == 0) ? 1e-9 : 0;
                for (int iVert = nVertStart;
                     iVert + 1 < nVertStart + nVertCount; ++iVert) {
                    /* Use point in the middle of segment to avoid testing
                     * common points of rings.
                     */
                    const double dfTestX =
                        (psObject->padfX[iVert] + psObject->padfX[iVert + 1]) /
                        2;
                    const double dfTestY =
                        (psObject->padfY[iVert] + psObject->padfY[iVert + 1]) /
                        2;
                    const double dfTestZ =
                        !bSameZ ? psObject->padfZ[nVertStart] : 0;

                    bInner = SHPRewindIsInnerRing(psObject, iOpRing, dfTestX,
                                                  dfTestY, dfRelativeTolerance,
                                                  bSameZ, dfTestZ);
                    if (bInner >= 0)
                        break;
                }
                if (bInner >= 0)
                    break;
            }
            if (bInner < 0) {
                /* Completely degenerate case. Do not bother touching order. */
                continue;
            }
        }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

<<<<<<< HEAD
<<<<<<< HEAD
    /* -------------------------------------------------------------------- */
    /*      Process each of the rings.                                      */
    /* -------------------------------------------------------------------- */
    int bAltered = 0;
    for (int iOpRing = 0; iOpRing < psObject->nParts; iOpRing++) {
        const int nVertStart = psObject->panPartStart[iOpRing];
        const int nVertCount = SHPGetPartVertexCount(psObject, iOpRing);
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    /*      Test if all points have the same Z value.                       */
    /* -------------------------------------------------------------------- */
    int bSameZ = TRUE;
    if (psObject->nSHPType == SHPT_POLYGONZ ||
        psObject->nSHPType == SHPT_POLYGONM) {
        for (int iVert = 1; iVert < psObject->nVertices; ++iVert) {
            if (psObject->padfZ[iVert] != psObject->padfZ[0]) {
                bSameZ = FALSE;
                break;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
            }
        }
    }

<<<<<<< HEAD
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        /* --------------------------------------------------------------------
         */
        /*      Determine the current order of this ring so we will know if */
        /*      it has to be reversed. */
        /* --------------------------------------------------------------------
         */

<<<<<<< HEAD
        if (iOpRing == psObject->nParts - 1)
            nVertCount = psObject->nVertices - psObject->panPartStart[iOpRing];
        else
            nVertCount = psObject->panPartStart[iOpRing + 1] -
                         psObject->panPartStart[iOpRing];
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    /* -------------------------------------------------------------------- */
    /*      Process each of the rings.                                      */
    /* -------------------------------------------------------------------- */
    int bAltered = 0;
    for (int iOpRing = 0; iOpRing < psObject->nParts; iOpRing++) {
        const int nVertStart = psObject->panPartStart[iOpRing];
        const int nVertCount = SHPGetPartVertexCount(psObject, iOpRing);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

        if (nVertCount < 2)
            continue;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        /* If a ring has a non-constant Z value, then consider it as an outer */
        /* ring. */
        /* NOTE: this is a rough approximation. If we were smarter, */
        /* we would check that all points of the ring are coplanar, and compare
         */
        /* that to other rings in the same (oblique) plane. */
        int bDoIsInnerRingTest = TRUE;
        if (!bSameZ) {
            int bPartSameZ = TRUE;
            for (int iVert = nVertStart + 1; iVert < nVertStart + nVertCount;
                 ++iVert) {
                if (psObject->padfZ[iVert] != psObject->padfZ[nVertStart]) {
                    bPartSameZ = FALSE;
                    break;
                }
            }
            if (!bPartSameZ)
                bDoIsInnerRingTest = FALSE;
        }

        int bInner = FALSE;
        if (bDoIsInnerRingTest) {
            for (int iTolerance = 0; iTolerance < 2; iTolerance++) {
                /* In a first attempt, use a relaxed criterion to decide if a
                 * point */
                /* is inside another ring. If all points of the current ring are
                 * in the */
                /* "grey" zone w.r.t that criterion, which seems really
                 * unlikely, */
                /* then use the strict criterion for another pass. */
                const double dfRelativeTolerance = (iTolerance == 0) ? 1e-9 : 0;
                for (int iVert = nVertStart;
                     iVert + 1 < nVertStart + nVertCount; ++iVert) {
                    /* Use point in the middle of segment to avoid testing
                     * common points of rings.
                     */
                    const double dfTestX =
                        (psObject->padfX[iVert] + psObject->padfX[iVert + 1]) /
                        2;
                    const double dfTestY =
                        (psObject->padfY[iVert] + psObject->padfY[iVert + 1]) /
                        2;
                    const double dfTestZ =
                        !bSameZ ? psObject->padfZ[nVertStart] : 0;

                    bInner = SHPRewindIsInnerRing(psObject, iOpRing, dfTestX,
                                                  dfTestY, dfRelativeTolerance,
                                                  bSameZ, dfTestZ);
                    if (bInner >= 0)
                        break;
                }
                if (bInner >= 0)
                    break;
            }
            if (bInner < 0) {
                /* Completely degenerate case. Do not bother touching order. */
                continue;
            }
        }
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        dfSum = psObject->padfX[nVertStart] *
                (psObject->padfY[nVertStart + 1] -
                 psObject->padfY[nVertStart + nVertCount - 1]);
        for (iVert = nVertStart + 1; iVert < nVertStart + nVertCount - 1;
             iVert++) {
            dfSum += psObject->padfX[iVert] *
                     (psObject->padfY[iVert + 1] - psObject->padfY[iVert - 1]);
        }
=======
        dfSum = psObject->padfX[nVertStart] *
                (psObject->padfY[nVertStart + 1] -
                 psObject->padfY[nVertStart + nVertCount - 1]);
        for (iVert = nVertStart + 1; iVert < nVertStart + nVertCount - 1;
             iVert++) {
=======
        double dfSum = psObject->padfX[nVertStart] *
                       (psObject->padfY[nVertStart + 1] -
                        psObject->padfY[nVertStart + nVertCount - 1]);
        int iVert = nVertStart + 1;
        for (; iVert < nVertStart + nVertCount - 1; iVert++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
            dfSum += psObject->padfX[iVert] *
                     (psObject->padfY[iVert + 1] - psObject->padfY[iVert - 1]);
        }
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======

        dfSum += psObject->padfX[iVert] *
                 (psObject->padfY[nVertStart] - psObject->padfY[iVert - 1]);

        /* --------------------------------------------------------------------
         */
        /*      Reverse if necessary. */
        /* --------------------------------------------------------------------
         */
        if ((dfSum < 0.0 && bInner) || (dfSum > 0.0 && !bInner)) {
<<<<<<< HEAD
            int i;
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        dfSum += psObject->padfX[iVert] *
                 (psObject->padfY[nVertStart] - psObject->padfY[iVert - 1]);

        /* --------------------------------------------------------------------
         */
        /*      Reverse if necessary. */
        /* --------------------------------------------------------------------
         */
        if ((dfSum < 0.0 && bInner) || (dfSum > 0.0 && !bInner)) {
            int i;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        /* If a ring has a non-constant Z value, then consider it as an outer */
        /* ring. */
        /* NOTE: this is a rough approximation. If we were smarter, */
        /* we would check that all points of the ring are coplanar, and compare
         */
        /* that to other rings in the same (oblique) plane. */
        int bDoIsInnerRingTest = TRUE;
        if (!bSameZ) {
            int bPartSameZ = TRUE;
            for (int iVert = nVertStart + 1; iVert < nVertStart + nVertCount;
                 ++iVert) {
                if (psObject->padfZ[iVert] != psObject->padfZ[nVertStart]) {
                    bPartSameZ = FALSE;
                    break;
                }
            }
            if (!bPartSameZ)
                bDoIsInnerRingTest = FALSE;
        }

        int bInner = FALSE;
        if (bDoIsInnerRingTest) {
            for (int iTolerance = 0; iTolerance < 2; iTolerance++) {
                /* In a first attempt, use a relaxed criterion to decide if a
                 * point */
                /* is inside another ring. If all points of the current ring are
                 * in the */
                /* "grey" zone w.r.t that criterion, which seems really
                 * unlikely, */
                /* then use the strict criterion for another pass. */
                const double dfRelativeTolerance = (iTolerance == 0) ? 1e-9 : 0;
                for (int iVert = nVertStart;
                     iVert + 1 < nVertStart + nVertCount; ++iVert) {
                    /* Use point in the middle of segment to avoid testing
                     * common points of rings.
                     */
                    const double dfTestX =
                        (psObject->padfX[iVert] + psObject->padfX[iVert + 1]) /
                        2;
                    const double dfTestY =
                        (psObject->padfY[iVert] + psObject->padfY[iVert + 1]) /
                        2;
                    const double dfTestZ =
                        !bSameZ ? psObject->padfZ[nVertStart] : 0;

                    bInner = SHPRewindIsInnerRing(psObject, iOpRing, dfTestX,
                                                  dfTestY, dfRelativeTolerance,
                                                  bSameZ, dfTestZ);
                    if (bInner >= 0)
                        break;
                }
                if (bInner >= 0)
                    break;
            }
            if (bInner < 0) {
                /* Completely degenerate case. Do not bother touching order. */
                continue;
            }
        }
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main

        dfSum += psObject->padfX[iVert] *
                 (psObject->padfY[nVertStart] - psObject->padfY[iVert - 1]);

        /* --------------------------------------------------------------------
         */
        /*      Reverse if necessary. */
        /* --------------------------------------------------------------------
         */
        if ((dfSum < 0.0 && bInner) || (dfSum > 0.0 && !bInner)) {
<<<<<<< HEAD
            int i;
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        dfSum += psObject->padfX[iVert] *
                 (psObject->padfY[nVertStart] - psObject->padfY[iVert - 1]);

        /* --------------------------------------------------------------------
         */
        /*      Reverse if necessary. */
        /* --------------------------------------------------------------------
         */
        if ((dfSum < 0.0 && bInner) || (dfSum > 0.0 && !bInner)) {
            int i;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        /* If a ring has a non-constant Z value, then consider it as an outer */
        /* ring. */
        /* NOTE: this is a rough approximation. If we were smarter, */
        /* we would check that all points of the ring are coplanar, and compare
         */
        /* that to other rings in the same (oblique) plane. */
        int bDoIsInnerRingTest = TRUE;
        if (!bSameZ) {
            int bPartSameZ = TRUE;
            for (int iVert = nVertStart + 1; iVert < nVertStart + nVertCount;
                 ++iVert) {
                if (psObject->padfZ[iVert] != psObject->padfZ[nVertStart]) {
                    bPartSameZ = FALSE;
                    break;
                }
            }
            if (!bPartSameZ)
                bDoIsInnerRingTest = FALSE;
        }

        int bInner = FALSE;
        if (bDoIsInnerRingTest) {
            for (int iTolerance = 0; iTolerance < 2; iTolerance++) {
                /* In a first attempt, use a relaxed criterion to decide if a
                 * point */
                /* is inside another ring. If all points of the current ring are
                 * in the */
                /* "grey" zone w.r.t that criterion, which seems really
                 * unlikely, */
                /* then use the strict criterion for another pass. */
                const double dfRelativeTolerance = (iTolerance == 0) ? 1e-9 : 0;
                for (int iVert = nVertStart;
                     iVert + 1 < nVertStart + nVertCount; ++iVert) {
                    /* Use point in the middle of segment to avoid testing
                     * common points of rings.
                     */
                    const double dfTestX =
                        (psObject->padfX[iVert] + psObject->padfX[iVert + 1]) /
                        2;
                    const double dfTestY =
                        (psObject->padfY[iVert] + psObject->padfY[iVert + 1]) /
                        2;
                    const double dfTestZ =
                        !bSameZ ? psObject->padfZ[nVertStart] : 0;

                    bInner = SHPRewindIsInnerRing(psObject, iOpRing, dfTestX,
                                                  dfTestY, dfRelativeTolerance,
                                                  bSameZ, dfTestZ);
                    if (bInner >= 0)
                        break;
                }
                if (bInner >= 0)
                    break;
            }
            if (bInner < 0) {
                /* Completely degenerate case. Do not bother touching order. */
                continue;
            }
        }
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main

        dfSum += psObject->padfX[iVert] *
                 (psObject->padfY[nVertStart] - psObject->padfY[iVert - 1]);

        /* --------------------------------------------------------------------
         */
        /*      Reverse if necessary. */
        /* --------------------------------------------------------------------
         */
        if ((dfSum < 0.0 && bInner) || (dfSum > 0.0 && !bInner)) {
<<<<<<< HEAD
            int i;
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        dfSum += psObject->padfX[iVert] *
                 (psObject->padfY[nVertStart] - psObject->padfY[iVert - 1]);

        /* --------------------------------------------------------------------
         */
        /*      Reverse if necessary. */
        /* --------------------------------------------------------------------
         */
        if ((dfSum < 0.0 && bInner) || (dfSum > 0.0 && !bInner)) {
            int i;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        /* If a ring has a non-constant Z value, then consider it as an outer */
        /* ring. */
        /* NOTE: this is a rough approximation. If we were smarter, */
        /* we would check that all points of the ring are coplanar, and compare
         */
        /* that to other rings in the same (oblique) plane. */
        int bDoIsInnerRingTest = TRUE;
        if (!bSameZ) {
            int bPartSameZ = TRUE;
            for (int iVert = nVertStart + 1; iVert < nVertStart + nVertCount;
                 ++iVert) {
                if (psObject->padfZ[iVert] != psObject->padfZ[nVertStart]) {
                    bPartSameZ = FALSE;
                    break;
                }
            }
            if (!bPartSameZ)
                bDoIsInnerRingTest = FALSE;
        }

        int bInner = FALSE;
        if (bDoIsInnerRingTest) {
            for (int iTolerance = 0; iTolerance < 2; iTolerance++) {
                /* In a first attempt, use a relaxed criterion to decide if a
                 * point */
                /* is inside another ring. If all points of the current ring are
                 * in the */
                /* "grey" zone w.r.t that criterion, which seems really
                 * unlikely, */
                /* then use the strict criterion for another pass. */
                const double dfRelativeTolerance = (iTolerance == 0) ? 1e-9 : 0;
                for (int iVert = nVertStart;
                     iVert + 1 < nVertStart + nVertCount; ++iVert) {
                    /* Use point in the middle of segment to avoid testing
                     * common points of rings.
                     */
                    const double dfTestX =
                        (psObject->padfX[iVert] + psObject->padfX[iVert + 1]) /
                        2;
                    const double dfTestY =
                        (psObject->padfY[iVert] + psObject->padfY[iVert + 1]) /
                        2;
                    const double dfTestZ =
                        !bSameZ ? psObject->padfZ[nVertStart] : 0;

                    bInner = SHPRewindIsInnerRing(psObject, iOpRing, dfTestX,
                                                  dfTestY, dfRelativeTolerance,
                                                  bSameZ, dfTestZ);
                    if (bInner >= 0)
                        break;
                }
                if (bInner >= 0)
                    break;
            }
            if (bInner < 0) {
                /* Completely degenerate case. Do not bother touching order. */
                continue;
            }
        }
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

>>>>>>> osgeo-main
        /* --------------------------------------------------------------------
         */
        /*      Determine the current order of this ring so we will know if */
        /*      it has to be reversed. */
        /* --------------------------------------------------------------------
         */

        double dfSum = psObject->padfX[nVertStart] *
                       (psObject->padfY[nVertStart + 1] -
                        psObject->padfY[nVertStart + nVertCount - 1]);
        int iVert = nVertStart + 1;
        for (; iVert < nVertStart + nVertCount - 1; iVert++) {
            dfSum += psObject->padfX[iVert] *
                     (psObject->padfY[iVert + 1] - psObject->padfY[iVert - 1]);
        }

        dfSum += psObject->padfX[iVert] *
                 (psObject->padfY[nVertStart] - psObject->padfY[iVert - 1]);

        /* --------------------------------------------------------------------
         */
        /*      Reverse if necessary. */
        /* --------------------------------------------------------------------
         */
        if ((dfSum < 0.0 && bInner) || (dfSum > 0.0 && !bInner)) {
            bAltered++;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
            for (int i = 0; i < nVertCount / 2; i++) {
                /* Swap X */
                double dfSaved = psObject->padfX[nVertStart + i];
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (int i = 0; i < nVertCount / 2; i++) {
                /* Swap X */
                double dfSaved = psObject->padfX[nVertStart + i];
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            for (i = 0; i < nVertCount / 2; i++) {
                double dfSaved;

                /* Swap X */
                dfSaved = psObject->padfX[nVertStart + i];
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            bAltered++;
            for (int i = 0; i < nVertCount / 2; i++) {
                /* Swap X */
                double dfSaved = psObject->padfX[nVertStart + i];
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
            for (int i = 0; i < nVertCount / 2; i++) {
                /* Swap X */
                double dfSaved = psObject->padfX[nVertStart + i];
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                psObject->padfX[nVertStart + i] =
                    psObject->padfX[nVertStart + nVertCount - i - 1];
                psObject->padfX[nVertStart + nVertCount - i - 1] = dfSaved;

                /* Swap Y */
                dfSaved = psObject->padfY[nVertStart + i];
                psObject->padfY[nVertStart + i] =
                    psObject->padfY[nVertStart + nVertCount - i - 1];
                psObject->padfY[nVertStart + nVertCount - i - 1] = dfSaved;

                /* Swap Z */
                if (psObject->padfZ) {
                    dfSaved = psObject->padfZ[nVertStart + i];
                    psObject->padfZ[nVertStart + i] =
                        psObject->padfZ[nVertStart + nVertCount - i - 1];
                    psObject->padfZ[nVertStart + nVertCount - i - 1] = dfSaved;
                }

                /* Swap M */
                if (psObject->padfM) {
                    dfSaved = psObject->padfM[nVertStart + i];
                    psObject->padfM[nVertStart + i] =
                        psObject->padfM[nVertStart + nVertCount - i - 1];
                    psObject->padfM[nVertStart + nVertCount - i - 1] = dfSaved;
                }
            }
        }
    }

    return bAltered;
}
