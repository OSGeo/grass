/******************************************************************************
 * $Id$
 *
 * Project:  Shapelib
 * Purpose:  Implementation of .dbf access API documented in dbf_api.html.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 * Copyright (c) 2012-2019, Even Rouault <even dot rouault at spatialys.com>
=======
 * Copyright (c) 2012-2013, Even Rouault <even dot rouault at mines-paris dot
 *org>
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
 * Copyright (c) 2012-2013, Even Rouault <even dot rouault at mines-paris dot
 *org>
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
 * Copyright (c) 2012-2019, Even Rouault <even dot rouault at spatialys.com>
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
 * Copyright (c) 2012-2013, Even Rouault <even dot rouault at mines-paris dot
 *org>
=======
 * Copyright (c) 2012-2019, Even Rouault <even dot rouault at spatialys.com>
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
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

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifdef USE_CPL
#include "cpl_string.h"
#else

#if defined(WIN32) || defined(_WIN32)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
#define STRCASECMP(a, b) (stricmp(a, b))
#else
#include <strings.h>
#define STRCASECMP(a, b) (strcasecmp(a, b))
#endif

#if defined(_MSC_VER)
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#elif defined(WIN32) || defined(_WIN32)
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
#ifndef snprintf
#define snprintf _snprintf
#endif
=======
#define STRCASECMP(a, b) (stricmp(a, b))
#else
#include <strings.h>
#define STRCASECMP(a, b) (strcasecmp(a, b))
#endif

#if defined(_MSC_VER)
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#elif defined(WIN32) || defined(_WIN32)
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif

#define CPLsprintf  sprintf
#define CPLsnprintf snprintf
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
#endif

#define CPLsprintf  sprintf
#define CPLsnprintf snprintf
<<<<<<< HEAD
=======
#ifndef snprintf
#define snprintf _snprintf
#endif
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
#ifndef snprintf
#define snprintf _snprintf
#endif
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
#endif

SHP_CVSID("$Id$")

#ifndef FALSE
#define FALSE 0
#define TRUE  1
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#endif

/* File header size */
#define XBASE_FILEHDR_SZ         32

#define HEADER_RECORD_TERMINATOR 0x0D

/* See http://www.manmrk.net/tutorials/database/xbase/dbf.html */
#define END_OF_FILE_CHARACTER    0x1A

#ifdef USE_CPL
CPL_INLINE static void CPL_IGNORE_RET_VAL_INT(CPL_UNUSED int unused)
{
}
#else
#define CPL_IGNORE_RET_VAL_INT(x) x
#endif

#ifdef __cplusplus
#define STATIC_CAST(type, x)      static_cast<type>(x)
#define REINTERPRET_CAST(type, x) reinterpret_cast<type>(x)
#define CONST_CAST(type, x)       const_cast<type>(x)
#define SHPLIB_NULLPTR            nullptr
#else
#define STATIC_CAST(type, x)      ((type)(x))
#define REINTERPRET_CAST(type, x) ((type)(x))
#define CONST_CAST(type, x)       ((type)(x))
#define SHPLIB_NULLPTR            NULL
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
#endif

/* File header size */
#define XBASE_FILEHDR_SZ         32

#define HEADER_RECORD_TERMINATOR 0x0D

/* See http://www.manmrk.net/tutorials/database/xbase/dbf.html */
#define END_OF_FILE_CHARACTER    0x1A

#ifdef USE_CPL
CPL_INLINE static void CPL_IGNORE_RET_VAL_INT(CPL_UNUSED int unused)
{
}
#else
#define CPL_IGNORE_RET_VAL_INT(x) x
#endif

#ifdef __cplusplus
#define STATIC_CAST(type, x)      static_cast<type>(x)
#define REINTERPRET_CAST(type, x) reinterpret_cast<type>(x)
#define CONST_CAST(type, x)       const_cast<type>(x)
#define SHPLIB_NULLPTR            nullptr
#else
#define STATIC_CAST(type, x)      ((type)(x))
#define REINTERPRET_CAST(type, x) ((type)(x))
#define CONST_CAST(type, x)       ((type)(x))
#define SHPLIB_NULLPTR            NULL
<<<<<<< HEAD
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
#endif

/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

static void *SfRealloc(void *pMem, int nNewSize)
{
    if (pMem == SHPLIB_NULLPTR)
        return malloc(nNewSize);
    else
        return realloc(pMem, nNewSize);
=======
=======

>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
static void *SfRealloc(void *pMem, int nNewSize)
{
    if (pMem == SHPLIB_NULLPTR)
        return malloc(nNewSize);
    else
<<<<<<< HEAD
        return ((void *)realloc(pMem, nNewSize));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
static void *SfRealloc(void *pMem, int nNewSize)
{
    if (pMem == NULL)
        return ((void *)malloc(nNewSize));
    else
        return ((void *)realloc(pMem, nNewSize));
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        return realloc(pMem, nNewSize);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======

static void *SfRealloc(void *pMem, int nNewSize)
{
    if (pMem == SHPLIB_NULLPTR)
        return malloc(nNewSize);
    else
        return realloc(pMem, nNewSize);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                           DBFWriteHeader()                           */
/*                                                                      */
/*      This is called to write out the file header, and field          */
/*      descriptions before writing any actual data records.  This      */
/*      also computes all the DBFDataSet field offset/size/decimals     */
/*      and so forth values.                                            */
/************************************************************************/

static void DBFWriteHeader(DBFHandle psDBF)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    unsigned char abyHeader[XBASE_FILEHDR_SZ] = {0};
=======
    unsigned char abyHeader[XBASE_FLDHDR_SZ];
    int i;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    unsigned char abyHeader[XBASE_FLDHDR_SZ];
    int i;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    unsigned char abyHeader[XBASE_FILEHDR_SZ] = {0};
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    unsigned char abyHeader[XBASE_FLDHDR_SZ];
    int i;
=======
    unsigned char abyHeader[XBASE_FILEHDR_SZ] = {0};
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    if (!psDBF->bNoHeader)
        return;

    psDBF->bNoHeader = FALSE;

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*    Initialize the file header information.                */
    /* -------------------------------------------------------------------- */
    abyHeader[0] = 0x03; /* memo field? - just copying     */

    /* write out update date */
    abyHeader[1] = STATIC_CAST(unsigned char, psDBF->nUpdateYearSince1900);
    abyHeader[2] = STATIC_CAST(unsigned char, psDBF->nUpdateMonth);
    abyHeader[3] = STATIC_CAST(unsigned char, psDBF->nUpdateDay);

    /* record count preset at zero */

    abyHeader[8] = STATIC_CAST(unsigned char, psDBF->nHeaderLength % 256);
    abyHeader[9] = STATIC_CAST(unsigned char, psDBF->nHeaderLength / 256);

    abyHeader[10] = STATIC_CAST(unsigned char, psDBF->nRecordLength % 256);
    abyHeader[11] = STATIC_CAST(unsigned char, psDBF->nRecordLength / 256);

    abyHeader[29] = STATIC_CAST(unsigned char, psDBF->iLanguageDriver);

    /* -------------------------------------------------------------------- */
    /*      Write the initial 32 byte file header, and all the field        */
    /*      descriptions.                                             */
    /* -------------------------------------------------------------------- */
    psDBF->sHooks.FSeek(psDBF->fp, 0, 0);
    psDBF->sHooks.FWrite(abyHeader, XBASE_FILEHDR_SZ, 1, psDBF->fp);
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /*      Initialize the file header information.                         */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < XBASE_FLDHDR_SZ; i++)
        abyHeader[i] = 0;

    abyHeader[0] = 0x03; /* memo field? - just copying   */

    /* write out update date */
    abyHeader[1] = (unsigned char)psDBF->nUpdateYearSince1900;
    abyHeader[2] = (unsigned char)psDBF->nUpdateMonth;
    abyHeader[3] = (unsigned char)psDBF->nUpdateDay;

    /* record count preset at zero */

    abyHeader[8] = (unsigned char)(psDBF->nHeaderLength % 256);
    abyHeader[9] = (unsigned char)(psDBF->nHeaderLength / 256);

    abyHeader[10] = (unsigned char)(psDBF->nRecordLength % 256);
    abyHeader[11] = (unsigned char)(psDBF->nRecordLength / 256);

    abyHeader[29] = (unsigned char)(psDBF->iLanguageDriver);

    /* -------------------------------------------------------------------- */
    /*      Write the initial 32 byte file header, and all the field        */
    /*      descriptions.                                                   */
    /* -------------------------------------------------------------------- */
    psDBF->sHooks.FSeek(psDBF->fp, 0, 0);
    psDBF->sHooks.FWrite(abyHeader, XBASE_FLDHDR_SZ, 1, psDBF->fp);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    /*    Initialize the file header information.                */
    /* -------------------------------------------------------------------- */
    abyHeader[0] = 0x03; /* memo field? - just copying     */

    /* write out update date */
    abyHeader[1] = STATIC_CAST(unsigned char, psDBF->nUpdateYearSince1900);
    abyHeader[2] = STATIC_CAST(unsigned char, psDBF->nUpdateMonth);
    abyHeader[3] = STATIC_CAST(unsigned char, psDBF->nUpdateDay);

    /* record count preset at zero */

    abyHeader[8] = STATIC_CAST(unsigned char, psDBF->nHeaderLength % 256);
    abyHeader[9] = STATIC_CAST(unsigned char, psDBF->nHeaderLength / 256);

    abyHeader[10] = STATIC_CAST(unsigned char, psDBF->nRecordLength % 256);
    abyHeader[11] = STATIC_CAST(unsigned char, psDBF->nRecordLength / 256);

    abyHeader[29] = STATIC_CAST(unsigned char, psDBF->iLanguageDriver);

    /* -------------------------------------------------------------------- */
    /*      Write the initial 32 byte file header, and all the field        */
    /*      descriptions.                                             */
    /* -------------------------------------------------------------------- */
    psDBF->sHooks.FSeek(psDBF->fp, 0, 0);
    psDBF->sHooks.FWrite(abyHeader, XBASE_FILEHDR_SZ, 1, psDBF->fp);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    psDBF->sHooks.FWrite(psDBF->pszHeader, XBASE_FLDHDR_SZ, psDBF->nFields,
                         psDBF->fp);

    /* -------------------------------------------------------------------- */
    /*      Write out the newline character if there is room for it.        */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
    if (psDBF->nHeaderLength >
        XBASE_FLDHDR_SZ * psDBF->nFields + XBASE_FLDHDR_SZ) {
        char cNewline = HEADER_RECORD_TERMINATOR;
        psDBF->sHooks.FWrite(&cNewline, 1, 1, psDBF->fp);
    }

    /* -------------------------------------------------------------------- */
    /*      If the file is new, add a EOF character.                        */
    /* -------------------------------------------------------------------- */
    if (psDBF->nRecords == 0 && psDBF->bWriteEndOfFileChar) {
        char ch = END_OF_FILE_CHARACTER;

        psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
=======
=======
    /*      Initialize the file header information.                         */
=======
    /*    Initialize the file header information.                */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    abyHeader[0] = 0x03; /* memo field? - just copying     */

    /* write out update date */
    abyHeader[1] = STATIC_CAST(unsigned char, psDBF->nUpdateYearSince1900);
    abyHeader[2] = STATIC_CAST(unsigned char, psDBF->nUpdateMonth);
    abyHeader[3] = STATIC_CAST(unsigned char, psDBF->nUpdateDay);

    /* record count preset at zero */

    abyHeader[8] = STATIC_CAST(unsigned char, psDBF->nHeaderLength % 256);
    abyHeader[9] = STATIC_CAST(unsigned char, psDBF->nHeaderLength / 256);

    abyHeader[10] = STATIC_CAST(unsigned char, psDBF->nRecordLength % 256);
    abyHeader[11] = STATIC_CAST(unsigned char, psDBF->nRecordLength / 256);

    abyHeader[29] = STATIC_CAST(unsigned char, psDBF->iLanguageDriver);

    /* -------------------------------------------------------------------- */
    /*      Write the initial 32 byte file header, and all the field        */
    /*      descriptions.                                             */
    /* -------------------------------------------------------------------- */
    psDBF->sHooks.FSeek(psDBF->fp, 0, 0);
    psDBF->sHooks.FWrite(abyHeader, XBASE_FILEHDR_SZ, 1, psDBF->fp);
    psDBF->sHooks.FWrite(psDBF->pszHeader, XBASE_FLDHDR_SZ, psDBF->nFields,
                         psDBF->fp);

    /* -------------------------------------------------------------------- */
    /*      Write out the newline character if there is room for it.        */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psDBF->nHeaderLength > 32 * psDBF->nFields + 32) {
        char cNewline;

        cNewline = 0x0d;
=======
    if (psDBF->nHeaderLength >
        XBASE_FLDHDR_SZ * psDBF->nFields + XBASE_FLDHDR_SZ) {
        char cNewline = HEADER_RECORD_TERMINATOR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        psDBF->sHooks.FWrite(&cNewline, 1, 1, psDBF->fp);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    if (psDBF->nHeaderLength >
        XBASE_FLDHDR_SZ * psDBF->nFields + XBASE_FLDHDR_SZ) {
        char cNewline = HEADER_RECORD_TERMINATOR;
        psDBF->sHooks.FWrite(&cNewline, 1, 1, psDBF->fp);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    /* -------------------------------------------------------------------- */
    /*      If the file is new, add a EOF character.                        */
    /* -------------------------------------------------------------------- */
    if (psDBF->nRecords == 0 && psDBF->bWriteEndOfFileChar) {
        char ch = END_OF_FILE_CHARACTER;

        psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
<<<<<<< HEAD
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }
}

/************************************************************************/
/*                           DBFFlushRecord()                           */
/*                                                                      */
/*      Write out the current record if there is one.                   */
/************************************************************************/

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static bool DBFFlushRecord(DBFHandle psDBF)
{
    if (psDBF->bCurrentRecordModified && psDBF->nCurrentRecord > -1) {
        psDBF->bCurrentRecordModified = FALSE;

        const SAOffset nRecordOffset =
            psDBF->nRecordLength *
                STATIC_CAST(SAOffset, psDBF->nCurrentRecord) +
            psDBF->nHeaderLength;

        /* --------------------------------------------------------------------
         */
        /*      Guard FSeek with check for whether we're already at position; */
        /*      no-op FSeeks defeat network filesystems' write buffering. */
        /* --------------------------------------------------------------------
         */
        if (psDBF->bRequireNextWriteSeek ||
            psDBF->sHooks.FTell(psDBF->fp) != nRecordOffset) {
            if (psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0) != 0) {
                char szMessage[128];
                snprintf(
                    szMessage, sizeof(szMessage),
                    "Failure seeking to position before writing DBF record %d.",
                    psDBF->nCurrentRecord);
                psDBF->sHooks.Error(szMessage);
                return false;
            }
        }

        if (psDBF->sHooks.FWrite(psDBF->pszCurrentRecord, psDBF->nRecordLength,
                                 1, psDBF->fp) != 1) {
            char szMessage[128];
            snprintf(szMessage, sizeof(szMessage),
                     "Failure writing DBF record %d.", psDBF->nCurrentRecord);
            psDBF->sHooks.Error(szMessage);
            return false;
        }

        /* --------------------------------------------------------------------
         */
        /*      If next op is also a write, allow possible skipping of FSeek. */
        /* --------------------------------------------------------------------
         */
        psDBF->bRequireNextWriteSeek = FALSE;

        if (psDBF->nCurrentRecord == psDBF->nRecords - 1) {
            if (psDBF->bWriteEndOfFileChar) {
                char ch = END_OF_FILE_CHARACTER;
                psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
            }
=======
static int DBFFlushRecord(DBFHandle psDBF)
=======
static bool DBFFlushRecord(DBFHandle psDBF)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
{
    if (psDBF->bCurrentRecordModified && psDBF->nCurrentRecord > -1) {
        psDBF->bCurrentRecordModified = FALSE;

        const SAOffset nRecordOffset =
            psDBF->nRecordLength *
                STATIC_CAST(SAOffset, psDBF->nCurrentRecord) +
            psDBF->nHeaderLength;

        /* --------------------------------------------------------------------
         */
        /*      Guard FSeek with check for whether we're already at position; */
        /*      no-op FSeeks defeat network filesystems' write buffering. */
        /* --------------------------------------------------------------------
         */
        if (psDBF->bRequireNextWriteSeek ||
            psDBF->sHooks.FTell(psDBF->fp) != nRecordOffset) {
            if (psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0) != 0) {
                char szMessage[128];
                snprintf(
                    szMessage, sizeof(szMessage),
                    "Failure seeking to position before writing DBF record %d.",
                    psDBF->nCurrentRecord);
                psDBF->sHooks.Error(szMessage);
                return false;
            }
        }

        if (psDBF->sHooks.FWrite(psDBF->pszCurrentRecord, psDBF->nRecordLength,
                                 1, psDBF->fp) != 1) {
            char szMessage[128];
<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
static int DBFFlushRecord(DBFHandle psDBF)
{
    SAOffset nRecordOffset;

    if (psDBF->bCurrentRecordModified && psDBF->nCurrentRecord > -1) {
        psDBF->bCurrentRecordModified = FALSE;

        nRecordOffset = psDBF->nRecordLength * (SAOffset)psDBF->nCurrentRecord +
                        psDBF->nHeaderLength;

        if (psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0) != 0 ||
            psDBF->sHooks.FWrite(psDBF->pszCurrentRecord, psDBF->nRecordLength,
                                 1, psDBF->fp) != 1) {
            char szMessage[128];
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

            snprintf(szMessage, sizeof(szMessage),
                     "Failure writing DBF record %d.", psDBF->nCurrentRecord);
            psDBF->sHooks.Error(szMessage);
            return FALSE;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
static bool DBFFlushRecord(DBFHandle psDBF)
{
    if (psDBF->bCurrentRecordModified && psDBF->nCurrentRecord > -1) {
        psDBF->bCurrentRecordModified = FALSE;

        const SAOffset nRecordOffset =
            psDBF->nRecordLength *
                STATIC_CAST(SAOffset, psDBF->nCurrentRecord) +
            psDBF->nHeaderLength;

        /* --------------------------------------------------------------------
         */
        /*      Guard FSeek with check for whether we're already at position; */
        /*      no-op FSeeks defeat network filesystems' write buffering. */
        /* --------------------------------------------------------------------
         */
        if (psDBF->bRequireNextWriteSeek ||
            psDBF->sHooks.FTell(psDBF->fp) != nRecordOffset) {
            if (psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0) != 0) {
                char szMessage[128];
                snprintf(
                    szMessage, sizeof(szMessage),
                    "Failure seeking to position before writing DBF record %d.",
                    psDBF->nCurrentRecord);
                psDBF->sHooks.Error(szMessage);
                return false;
            }
        }

        if (psDBF->sHooks.FWrite(psDBF->pszCurrentRecord, psDBF->nRecordLength,
                                 1, psDBF->fp) != 1) {
            char szMessage[128];
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            snprintf(szMessage, sizeof(szMessage),
                     "Failure writing DBF record %d.", psDBF->nCurrentRecord);
            psDBF->sHooks.Error(szMessage);
            return false;
        }

        /* --------------------------------------------------------------------
         */
        /*      If next op is also a write, allow possible skipping of FSeek. */
        /* --------------------------------------------------------------------
         */
        psDBF->bRequireNextWriteSeek = FALSE;

        if (psDBF->nCurrentRecord == psDBF->nRecords - 1) {
            if (psDBF->bWriteEndOfFileChar) {
                char ch = END_OF_FILE_CHARACTER;
                psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
            }
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }
    }

    return true;
}

/************************************************************************/
/*                           DBFLoadRecord()                            */
/************************************************************************/

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static bool DBFLoadRecord(DBFHandle psDBF, int iRecord)
=======
static int DBFLoadRecord(DBFHandle psDBF, int iRecord)
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
{
    if (psDBF->nCurrentRecord != iRecord) {
        if (!DBFFlushRecord(psDBF))
            return false;

        const SAOffset nRecordOffset =
            psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
            psDBF->nHeaderLength;

        if (psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, SEEK_SET) != 0) {
            char szMessage[128];
            snprintf(szMessage, sizeof(szMessage),
                     "fseek(%ld) failed on DBF file.",
                     STATIC_CAST(long, nRecordOffset));
            psDBF->sHooks.Error(szMessage);
            return false;
=======
static int DBFLoadRecord(DBFHandle psDBF, int iRecord)
{
    if (psDBF->nCurrentRecord != iRecord) {
=======
static int DBFLoadRecord(DBFHandle psDBF, int iRecord)
{
    if (psDBF->nCurrentRecord != iRecord) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        SAOffset nRecordOffset;

=======
static bool DBFLoadRecord(DBFHandle psDBF, int iRecord)
{
    if (psDBF->nCurrentRecord != iRecord) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        if (!DBFFlushRecord(psDBF))
            return false;

        const SAOffset nRecordOffset =
            psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
            psDBF->nHeaderLength;

        if (psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, SEEK_SET) != 0) {
            char szMessage[128];
            snprintf(szMessage, sizeof(szMessage),
                     "fseek(%ld) failed on DBF file.",
                     STATIC_CAST(long, nRecordOffset));
            psDBF->sHooks.Error(szMessage);
<<<<<<< HEAD
<<<<<<< HEAD
            return FALSE;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
static bool DBFLoadRecord(DBFHandle psDBF, int iRecord)
{
    if (psDBF->nCurrentRecord != iRecord) {
        if (!DBFFlushRecord(psDBF))
            return false;

        const SAOffset nRecordOffset =
            psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
            psDBF->nHeaderLength;

        if (psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, SEEK_SET) != 0) {
            char szMessage[128];
            snprintf(szMessage, sizeof(szMessage),
                     "fseek(%ld) failed on DBF file.",
                     STATIC_CAST(long, nRecordOffset));
            psDBF->sHooks.Error(szMessage);
            return false;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }

        if (psDBF->sHooks.FRead(psDBF->pszCurrentRecord, psDBF->nRecordLength,
                                1, psDBF->fp) != 1) {
            char szMessage[128];
<<<<<<< HEAD
<<<<<<< HEAD
            snprintf(szMessage, sizeof(szMessage),
                     "fread(%d) failed on DBF file.", psDBF->nRecordLength);
            psDBF->sHooks.Error(szMessage);
            return false;
        }

        psDBF->nCurrentRecord = iRecord;
        /* --------------------------------------------------------------------
         */
        /*      Require a seek for next write in case of mixed R/W operations.
         */
        /* --------------------------------------------------------------------
         */
        psDBF->bRequireNextWriteSeek = TRUE;
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

            snprintf(szMessage, sizeof(szMessage),
                     "fread(%d) failed on DBF file.\n", psDBF->nRecordLength);
            psDBF->sHooks.Error(szMessage);
            return FALSE;
        }

        psDBF->nCurrentRecord = iRecord;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            return FALSE;
=======
            return false;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        }

        if (psDBF->sHooks.FRead(psDBF->pszCurrentRecord, psDBF->nRecordLength,
                                1, psDBF->fp) != 1) {
            char szMessage[128];
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            snprintf(szMessage, sizeof(szMessage),
                     "fread(%d) failed on DBF file.", psDBF->nRecordLength);
            psDBF->sHooks.Error(szMessage);
            return false;
        }

        psDBF->nCurrentRecord = iRecord;
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        /* --------------------------------------------------------------------
         */
        /*      Require a seek for next write in case of mixed R/W operations.
         */
        /* --------------------------------------------------------------------
         */
        psDBF->bRequireNextWriteSeek = TRUE;
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    return true;
}

/************************************************************************/
/*                          DBFUpdateHeader()                           */
/************************************************************************/

void SHPAPI_CALL DBFUpdateHeader(DBFHandle psDBF)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psDBF->bNoHeader)
        DBFWriteHeader(psDBF);

=======
    unsigned char abyFileHeader[32];

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    if (psDBF->bNoHeader)
        DBFWriteHeader(psDBF);

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    unsigned char abyFileHeader[32];

    if (psDBF->bNoHeader)
        DBFWriteHeader(psDBF);

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    if (psDBF->bNoHeader)
        DBFWriteHeader(psDBF);

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (!DBFFlushRecord(psDBF))
        return;

    psDBF->sHooks.FSeek(psDBF->fp, 0, 0);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

    unsigned char abyFileHeader[XBASE_FILEHDR_SZ] = {0};
    psDBF->sHooks.FRead(abyFileHeader, 1, sizeof(abyFileHeader), psDBF->fp);

    abyFileHeader[1] = STATIC_CAST(unsigned char, psDBF->nUpdateYearSince1900);
    abyFileHeader[2] = STATIC_CAST(unsigned char, psDBF->nUpdateMonth);
    abyFileHeader[3] = STATIC_CAST(unsigned char, psDBF->nUpdateDay);
    abyFileHeader[4] = STATIC_CAST(unsigned char, psDBF->nRecords & 0xFF);
    abyFileHeader[5] =
        STATIC_CAST(unsigned char, (psDBF->nRecords >> 8) & 0xFF);
    abyFileHeader[6] =
        STATIC_CAST(unsigned char, (psDBF->nRecords >> 16) & 0xFF);
    abyFileHeader[7] =
        STATIC_CAST(unsigned char, (psDBF->nRecords >> 24) & 0xFF);

    psDBF->sHooks.FSeek(psDBF->fp, 0, 0);
    psDBF->sHooks.FWrite(abyFileHeader, sizeof(abyFileHeader), 1, psDBF->fp);

=======
    psDBF->sHooks.FRead(abyFileHeader, 32, 1, psDBF->fp);
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    unsigned char abyFileHeader[XBASE_FILEHDR_SZ] = {0};
    psDBF->sHooks.FRead(abyFileHeader, 1, sizeof(abyFileHeader), psDBF->fp);

    abyFileHeader[1] = STATIC_CAST(unsigned char, psDBF->nUpdateYearSince1900);
    abyFileHeader[2] = STATIC_CAST(unsigned char, psDBF->nUpdateMonth);
    abyFileHeader[3] = STATIC_CAST(unsigned char, psDBF->nUpdateDay);
    abyFileHeader[4] = STATIC_CAST(unsigned char, psDBF->nRecords & 0xFF);
    abyFileHeader[5] =
        STATIC_CAST(unsigned char, (psDBF->nRecords >> 8) & 0xFF);
    abyFileHeader[6] =
        STATIC_CAST(unsigned char, (psDBF->nRecords >> 16) & 0xFF);
    abyFileHeader[7] =
        STATIC_CAST(unsigned char, (psDBF->nRecords >> 24) & 0xFF);

    psDBF->sHooks.FSeek(psDBF->fp, 0, 0);
    psDBF->sHooks.FWrite(abyFileHeader, sizeof(abyFileHeader), 1, psDBF->fp);

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    psDBF->sHooks.FRead(abyFileHeader, 32, 1, psDBF->fp);

    abyFileHeader[1] = (unsigned char)psDBF->nUpdateYearSince1900;
    abyFileHeader[2] = (unsigned char)psDBF->nUpdateMonth;
    abyFileHeader[3] = (unsigned char)psDBF->nUpdateDay;
    abyFileHeader[4] = (unsigned char)(psDBF->nRecords % 256);
    abyFileHeader[5] = (unsigned char)((psDBF->nRecords / 256) % 256);
    abyFileHeader[6] = (unsigned char)((psDBF->nRecords / (256 * 256)) % 256);
    abyFileHeader[7] =
        (unsigned char)((psDBF->nRecords / (256 * 256 * 256)) % 256);

    psDBF->sHooks.FSeek(psDBF->fp, 0, 0);
    psDBF->sHooks.FWrite(abyFileHeader, 32, 1, psDBF->fp);

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======

    unsigned char abyFileHeader[XBASE_FILEHDR_SZ] = {0};
    psDBF->sHooks.FRead(abyFileHeader, 1, sizeof(abyFileHeader), psDBF->fp);

    abyFileHeader[1] = STATIC_CAST(unsigned char, psDBF->nUpdateYearSince1900);
    abyFileHeader[2] = STATIC_CAST(unsigned char, psDBF->nUpdateMonth);
    abyFileHeader[3] = STATIC_CAST(unsigned char, psDBF->nUpdateDay);
    abyFileHeader[4] = STATIC_CAST(unsigned char, psDBF->nRecords & 0xFF);
    abyFileHeader[5] =
        STATIC_CAST(unsigned char, (psDBF->nRecords >> 8) & 0xFF);
    abyFileHeader[6] =
        STATIC_CAST(unsigned char, (psDBF->nRecords >> 16) & 0xFF);
    abyFileHeader[7] =
        STATIC_CAST(unsigned char, (psDBF->nRecords >> 24) & 0xFF);

    psDBF->sHooks.FSeek(psDBF->fp, 0, 0);
    psDBF->sHooks.FWrite(abyFileHeader, sizeof(abyFileHeader), 1, psDBF->fp);

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    psDBF->sHooks.FFlush(psDBF->fp);
}

/************************************************************************/
/*                       DBFSetLastModifiedDate()                       */
/************************************************************************/

void SHPAPI_CALL DBFSetLastModifiedDate(DBFHandle psDBF, int nYYSince1900,
                                        int nMM, int nDD)
{
    psDBF->nUpdateYearSince1900 = nYYSince1900;
    psDBF->nUpdateMonth = nMM;
    psDBF->nUpdateDay = nDD;
}

/************************************************************************/
/*                              DBFOpen()                               */
/*                                                                      */
/*      Open a .dbf file.                                               */
/************************************************************************/

DBFHandle SHPAPI_CALL DBFOpen(const char *pszFilename, const char *pszAccess)
<<<<<<< HEAD
<<<<<<< HEAD
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
{
    SAHooks sHooks;

    SASetupDefaultHooks(&sHooks);

    return DBFOpenLL(pszFilename, pszAccess, &sHooks);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
}

/************************************************************************/
/*                      DBFGetLenWithoutExtension()                     */
/************************************************************************/

static int DBFGetLenWithoutExtension(const char *pszBasename)
{
    const int nLen = STATIC_CAST(int, strlen(pszBasename));
    for (int i = nLen - 1;
         i > 0 && pszBasename[i] != '/' && pszBasename[i] != '\\'; i--) {
        if (pszBasename[i] == '.') {
            return i;
        }
    }
    return nLen;
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                      DBFGetLenWithoutExtension()                     */
/************************************************************************/

static int DBFGetLenWithoutExtension(const char *pszBasename)
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
}

/************************************************************************/
/*                              DBFOpen()                               */
/*                                                                      */
/*      Open a .dbf file.                                               */
/************************************************************************/

DBFHandle SHPAPI_CALL DBFOpenLL(const char *pszFilename, const char *pszAccess,
                                SAHooks *psHooks)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /* -------------------------------------------------------------------- */
    /*      We only allow the access strings "rb" and "r+".                  */
    /* -------------------------------------------------------------------- */
    if (strcmp(pszAccess, "r") != 0 && strcmp(pszAccess, "r+") != 0 &&
        strcmp(pszAccess, "rb") != 0 && strcmp(pszAccess, "rb+") != 0 &&
        strcmp(pszAccess, "r+b") != 0)
        return SHPLIB_NULLPTR;

=======
    DBFHandle psDBF;
    SAFile pfCPG;
    unsigned char *pabyBuf;
    int nFields, nHeadLen, iField, i;
    char *pszBasename, *pszFullname;
    int nBufSize = 500;
    size_t nFullnameLen;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    /*      We only allow the access strings "rb" and "r+".                  */
    /* -------------------------------------------------------------------- */
    if (strcmp(pszAccess, "r") != 0 && strcmp(pszAccess, "r+") != 0 &&
        strcmp(pszAccess, "rb") != 0 && strcmp(pszAccess, "rb+") != 0 &&
        strcmp(pszAccess, "r+b") != 0)
        return SHPLIB_NULLPTR;

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    DBFHandle psDBF;
    SAFile pfCPG;
    unsigned char *pabyBuf;
    int nFields, nHeadLen, iField, i;
    char *pszBasename, *pszFullname;
    int nBufSize = 500;
    size_t nFullnameLen;

    /* -------------------------------------------------------------------- */
    /*      We only allow the access strings "rb" and "r+".                  */
    /* -------------------------------------------------------------------- */
    if (strcmp(pszAccess, "r") != 0 && strcmp(pszAccess, "r+") != 0 &&
        strcmp(pszAccess, "rb") != 0 && strcmp(pszAccess, "rb+") != 0 &&
        strcmp(pszAccess, "r+b") != 0)
        return (NULL);

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    /* -------------------------------------------------------------------- */
    /*      We only allow the access strings "rb" and "r+".                  */
    /* -------------------------------------------------------------------- */
    if (strcmp(pszAccess, "r") != 0 && strcmp(pszAccess, "r+") != 0 &&
        strcmp(pszAccess, "rb") != 0 && strcmp(pszAccess, "rb+") != 0 &&
        strcmp(pszAccess, "r+b") != 0)
        return SHPLIB_NULLPTR;

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (strcmp(pszAccess, "r") == 0)
        pszAccess = "rb";

    if (strcmp(pszAccess, "r+") == 0)
        pszAccess = "rb+";

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*    Compute the base (layer) name.  If there is any extension    */
    /*    on the passed in filename we will strip it off.            */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = DBFGetLenWithoutExtension(pszFilename);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszFilename, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".dbf", 5);

    DBFHandle psDBF = STATIC_CAST(DBFHandle, calloc(1, sizeof(DBFInfo)));
    psDBF->fp = psHooks->FOpen(pszFullname, pszAccess);
    memcpy(&(psDBF->sHooks), psHooks, sizeof(SAHooks));

    if (psDBF->fp == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".DBF", 5);
        psDBF->fp = psDBF->sHooks.FOpen(pszFullname, pszAccess);
    }

    memcpy(pszFullname + nLenWithoutExtension, ".cpg", 5);
    SAFile pfCPG = psHooks->FOpen(pszFullname, "r");
    if (pfCPG == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".CPG", 5);
        pfCPG = psHooks->FOpen(pszFullname, "r");
    }

    free(pszFullname);

    if (psDBF->fp == SHPLIB_NULLPTR) {
        free(psDBF);
        if (pfCPG)
            psHooks->FClose(pfCPG);
        return SHPLIB_NULLPTR;
=======
    /*      Compute the base (layer) name.  If there is any extension       */
    /*      on the passed in filename we will strip it off.                 */
=======
    /*    Compute the base (layer) name.  If there is any extension    */
    /*    on the passed in filename we will strip it off.            */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = DBFGetLenWithoutExtension(pszFilename);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszFilename, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".dbf", 5);

<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /*      Compute the base (layer) name.  If there is any extension       */
    /*      on the passed in filename we will strip it off.                 */
    /* -------------------------------------------------------------------- */
    pszBasename = (char *)malloc(strlen(pszFilename) + 5);
    strcpy(pszBasename, pszFilename);
    for (i = (int)strlen(pszBasename) - 1;
         i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' &&
         pszBasename[i] != '\\';
         i--) {
    }

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if (pszBasename[i] == '.')
        pszBasename[i] = '\0';

    nFullnameLen = strlen(pszBasename) + 5;
    pszFullname = (char *)malloc(nFullnameLen);
    snprintf(pszFullname, nFullnameLen, "%s.dbf", pszBasename);

    psDBF = (DBFHandle)calloc(1, sizeof(DBFInfo));
=======
    DBFHandle psDBF = STATIC_CAST(DBFHandle, calloc(1, sizeof(DBFInfo)));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    psDBF->fp = psHooks->FOpen(pszFullname, pszAccess);
    memcpy(&(psDBF->sHooks), psHooks, sizeof(SAHooks));

    if (psDBF->fp == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".DBF", 5);
        psDBF->fp = psDBF->sHooks.FOpen(pszFullname, pszAccess);
    }

    memcpy(pszFullname + nLenWithoutExtension, ".cpg", 5);
    SAFile pfCPG = psHooks->FOpen(pszFullname, "r");
    if (pfCPG == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".CPG", 5);
        pfCPG = psHooks->FOpen(pszFullname, "r");
    }

    free(pszFullname);

    if (psDBF->fp == SHPLIB_NULLPTR) {
        free(psDBF);
        if (pfCPG)
            psHooks->FClose(pfCPG);
<<<<<<< HEAD
        return (NULL);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    /*    Compute the base (layer) name.  If there is any extension    */
    /*    on the passed in filename we will strip it off.            */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = DBFGetLenWithoutExtension(pszFilename);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszFilename, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".dbf", 5);

    DBFHandle psDBF = STATIC_CAST(DBFHandle, calloc(1, sizeof(DBFInfo)));
    psDBF->fp = psHooks->FOpen(pszFullname, pszAccess);
    memcpy(&(psDBF->sHooks), psHooks, sizeof(SAHooks));

    if (psDBF->fp == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".DBF", 5);
        psDBF->fp = psDBF->sHooks.FOpen(pszFullname, pszAccess);
    }

    memcpy(pszFullname + nLenWithoutExtension, ".cpg", 5);
    SAFile pfCPG = psHooks->FOpen(pszFullname, "r");
    if (pfCPG == SHPLIB_NULLPTR) {
        memcpy(pszFullname + nLenWithoutExtension, ".CPG", 5);
        pfCPG = psHooks->FOpen(pszFullname, "r");
    }

    free(pszFullname);

    if (psDBF->fp == SHPLIB_NULLPTR) {
        free(psDBF);
        if (pfCPG)
            psHooks->FClose(pfCPG);
        return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    psDBF->bNoHeader = FALSE;
    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;

    /* -------------------------------------------------------------------- */
    /*  Read Table Header info                                              */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    const int nBufSize = 500;
    unsigned char *pabyBuf = STATIC_CAST(unsigned char *, malloc(nBufSize));
    if (psDBF->sHooks.FRead(pabyBuf, XBASE_FILEHDR_SZ, 1, psDBF->fp) != 1) {
=======
    pabyBuf = (unsigned char *)malloc(nBufSize);
    if (psDBF->sHooks.FRead(pabyBuf, 32, 1, psDBF->fp) != 1) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    pabyBuf = (unsigned char *)malloc(nBufSize);
    if (psDBF->sHooks.FRead(pabyBuf, 32, 1, psDBF->fp) != 1) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    pabyBuf = (unsigned char *)malloc(nBufSize);
    if (psDBF->sHooks.FRead(pabyBuf, 32, 1, psDBF->fp) != 1) {
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    const int nBufSize = 500;
    unsigned char *pabyBuf = STATIC_CAST(unsigned char *, malloc(nBufSize));
    if (psDBF->sHooks.FRead(pabyBuf, XBASE_FILEHDR_SZ, 1, psDBF->fp) != 1) {
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        psDBF->sHooks.FClose(psDBF->fp);
        if (pfCPG)
            psDBF->sHooks.FClose(pfCPG);
        free(pabyBuf);
        free(psDBF);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
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
    }

    DBFSetLastModifiedDate(psDBF, pabyBuf[1], pabyBuf[2], pabyBuf[3]);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    psDBF->nRecords = pabyBuf[4] | (pabyBuf[5] << 8) | (pabyBuf[6] << 16) |
                      ((pabyBuf[7] & 0x7f) << 24);

    const int nHeadLen = pabyBuf[8] | (pabyBuf[9] << 8);
    psDBF->nHeaderLength = nHeadLen;
    psDBF->nRecordLength = pabyBuf[10] | (pabyBuf[11] << 8);
    psDBF->iLanguageDriver = pabyBuf[29];

    if (psDBF->nRecordLength == 0 || nHeadLen < XBASE_FILEHDR_SZ) {
=======
    psDBF->nRecords = pabyBuf[4] + pabyBuf[5] * 256 + pabyBuf[6] * 256 * 256 +
                      (pabyBuf[7] & 0x7f) * 256 * 256 * 256;
=======
    psDBF->nRecords = pabyBuf[4] | (pabyBuf[5] << 8) | (pabyBuf[6] << 16) |
                      ((pabyBuf[7] & 0x7f) << 24);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    const int nHeadLen = pabyBuf[8] | (pabyBuf[9] << 8);
    psDBF->nHeaderLength = nHeadLen;
    psDBF->nRecordLength = pabyBuf[10] | (pabyBuf[11] << 8);
    psDBF->iLanguageDriver = pabyBuf[29];

<<<<<<< HEAD
    if (psDBF->nRecordLength == 0 || nHeadLen < 32) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    psDBF->nRecords = pabyBuf[4] + pabyBuf[5] * 256 + pabyBuf[6] * 256 * 256 +
                      (pabyBuf[7] & 0x7f) * 256 * 256 * 256;

    psDBF->nHeaderLength = nHeadLen = pabyBuf[8] + pabyBuf[9] * 256;
    psDBF->nRecordLength = pabyBuf[10] + pabyBuf[11] * 256;
    psDBF->iLanguageDriver = pabyBuf[29];

    if (psDBF->nRecordLength == 0 || nHeadLen < 32) {
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psDBF->nRecordLength == 0 || nHeadLen < XBASE_FILEHDR_SZ) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    psDBF->nRecords = pabyBuf[4] | (pabyBuf[5] << 8) | (pabyBuf[6] << 16) |
                      ((pabyBuf[7] & 0x7f) << 24);

    const int nHeadLen = pabyBuf[8] | (pabyBuf[9] << 8);
    psDBF->nHeaderLength = nHeadLen;
    psDBF->nRecordLength = pabyBuf[10] | (pabyBuf[11] << 8);
    psDBF->iLanguageDriver = pabyBuf[29];

    if (psDBF->nRecordLength == 0 || nHeadLen < XBASE_FILEHDR_SZ) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        psDBF->sHooks.FClose(psDBF->fp);
        if (pfCPG)
            psDBF->sHooks.FClose(pfCPG);
        free(pabyBuf);
        free(psDBF);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return SHPLIB_NULLPTR;
=======
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
    }

    const int nFields = (nHeadLen - XBASE_FILEHDR_SZ) / XBASE_FLDHDR_SZ;
    psDBF->nFields = nFields;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /* coverity[tainted_data] */
    psDBF->pszCurrentRecord = STATIC_CAST(char *, malloc(psDBF->nRecordLength));
=======
    psDBF->pszCurrentRecord = (char *)malloc(psDBF->nRecordLength);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    psDBF->pszCurrentRecord = (char *)malloc(psDBF->nRecordLength);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    /* coverity[tainted_data] */
    psDBF->pszCurrentRecord = STATIC_CAST(char *, malloc(psDBF->nRecordLength));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    psDBF->pszCurrentRecord = (char *)malloc(psDBF->nRecordLength);
=======
    /* coverity[tainted_data] */
    psDBF->pszCurrentRecord = STATIC_CAST(char *, malloc(psDBF->nRecordLength));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    /* -------------------------------------------------------------------- */
    /*  Figure out the code page from the LDID and CPG                      */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    psDBF->pszCodePage = SHPLIB_NULLPTR;
=======

    psDBF->pszCodePage = NULL;
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (pfCPG) {
        memset(pabyBuf, 0, nBufSize);
        psDBF->sHooks.FRead(pabyBuf, 1, nBufSize - 1, pfCPG);
        const size_t n = strcspn(REINTERPRET_CAST(char *, pabyBuf), "\n\r");
        if (n > 0) {
            pabyBuf[n] = '\0';
            psDBF->pszCodePage = STATIC_CAST(char *, malloc(n + 1));
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    psDBF->pszCodePage = NULL;
=======
    psDBF->pszCodePage = SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    if (pfCPG) {
        memset(pabyBuf, 0, nBufSize);
        psDBF->sHooks.FRead(pabyBuf, 1, nBufSize - 1, pfCPG);
        const size_t n = strcspn(REINTERPRET_CAST(char *, pabyBuf), "\n\r");
        if (n > 0) {
            pabyBuf[n] = '\0';
<<<<<<< HEAD
            psDBF->pszCodePage = (char *)malloc(n + 1);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            psDBF->pszCodePage = STATIC_CAST(char *, malloc(n + 1));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    psDBF->pszCodePage = SHPLIB_NULLPTR;
    if (pfCPG) {
        memset(pabyBuf, 0, nBufSize);
        psDBF->sHooks.FRead(pabyBuf, 1, nBufSize - 1, pfCPG);
        const size_t n = strcspn(REINTERPRET_CAST(char *, pabyBuf), "\n\r");
        if (n > 0) {
            pabyBuf[n] = '\0';
            psDBF->pszCodePage = STATIC_CAST(char *, malloc(n + 1));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            memcpy(psDBF->pszCodePage, pabyBuf, n + 1);
        }
        psDBF->sHooks.FClose(pfCPG);
    }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    if (psDBF->pszCodePage == SHPLIB_NULLPTR && pabyBuf[29] != 0) {
        snprintf(REINTERPRET_CAST(char *, pabyBuf), nBufSize, "LDID/%d",
                 psDBF->iLanguageDriver);
        psDBF->pszCodePage = STATIC_CAST(
            char *, malloc(strlen(REINTERPRET_CAST(char *, pabyBuf)) + 1));
        strcpy(psDBF->pszCodePage, REINTERPRET_CAST(char *, pabyBuf));
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psDBF->pszCodePage == NULL && pabyBuf[29] != 0) {
        snprintf((char *)pabyBuf, nBufSize, "LDID/%d", psDBF->iLanguageDriver);
        psDBF->pszCodePage = (char *)malloc(strlen((char *)pabyBuf) + 1);
        strcpy(psDBF->pszCodePage, (char *)pabyBuf);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    if (psDBF->pszCodePage == SHPLIB_NULLPTR && pabyBuf[29] != 0) {
        snprintf(REINTERPRET_CAST(char *, pabyBuf), nBufSize, "LDID/%d",
                 psDBF->iLanguageDriver);
        psDBF->pszCodePage = STATIC_CAST(
            char *, malloc(strlen(REINTERPRET_CAST(char *, pabyBuf)) + 1));
        strcpy(psDBF->pszCodePage, REINTERPRET_CAST(char *, pabyBuf));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    /* -------------------------------------------------------------------- */
    /*  Read in Field Definitions                                           */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    pabyBuf = STATIC_CAST(unsigned char *, SfRealloc(pabyBuf, nHeadLen));
    psDBF->pszHeader = REINTERPRET_CAST(char *, pabyBuf);
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    psDBF->sHooks.FSeek(psDBF->fp, XBASE_FILEHDR_SZ, 0);
    if (psDBF->sHooks.FRead(pabyBuf, nHeadLen - XBASE_FILEHDR_SZ, 1,
                            psDBF->fp) != 1) {
        psDBF->sHooks.FClose(psDBF->fp);
        free(pabyBuf);
        free(psDBF->pszCurrentRecord);
        free(psDBF->pszCodePage);
        free(psDBF);
        return SHPLIB_NULLPTR;
    }

    psDBF->panFieldOffset = STATIC_CAST(int *, malloc(sizeof(int) * nFields));
    psDBF->panFieldSize = STATIC_CAST(int *, malloc(sizeof(int) * nFields));
    psDBF->panFieldDecimals = STATIC_CAST(int *, malloc(sizeof(int) * nFields));
    psDBF->pachFieldType = STATIC_CAST(char *, malloc(sizeof(char) * nFields));

    for (int iField = 0; iField < nFields; iField++) {
        unsigned char *pabyFInfo = pabyBuf + iField * XBASE_FLDHDR_SZ;
        if (pabyFInfo[0] == HEADER_RECORD_TERMINATOR) {
            psDBF->nFields = iField;
            break;
        }

        if (pabyFInfo[11] == 'N' || pabyFInfo[11] == 'F') {
            psDBF->panFieldSize[iField] = pabyFInfo[16];
            psDBF->panFieldDecimals[iField] = pabyFInfo[17];
        }
        else {
            psDBF->panFieldSize[iField] = pabyFInfo[16];
            psDBF->panFieldDecimals[iField] = 0;

            /*
            ** The following seemed to be used sometimes to handle files with
            long
            ** string fields, but in other cases (such as bug 1202) the decimals
            field
            ** just seems to indicate some sort of preferred formatting, not
            very
            ** wide fields.  So I have disabled this code.  FrankW.
                    psDBF->panFieldSize[iField] = pabyFInfo[16] +
            pabyFInfo[17]*256; psDBF->panFieldDecimals[iField] = 0;
            */
        }

        psDBF->pachFieldType[iField] = STATIC_CAST(char, pabyFInfo[11]);
=======

    pabyBuf = (unsigned char *)SfRealloc(pabyBuf, nHeadLen);
    psDBF->pszHeader = (char *)pabyBuf;

=======

    pabyBuf = (unsigned char *)SfRealloc(pabyBuf, nHeadLen);
    psDBF->pszHeader = (char *)pabyBuf;

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    psDBF->sHooks.FSeek(psDBF->fp, 32, 0);
    if (psDBF->sHooks.FRead(pabyBuf, nHeadLen - 32, 1, psDBF->fp) != 1) {
=======
    pabyBuf = STATIC_CAST(unsigned char *, SfRealloc(pabyBuf, nHeadLen));
    psDBF->pszHeader = REINTERPRET_CAST(char *, pabyBuf);

    psDBF->sHooks.FSeek(psDBF->fp, XBASE_FILEHDR_SZ, 0);
    if (psDBF->sHooks.FRead(pabyBuf, nHeadLen - XBASE_FILEHDR_SZ, 1,
                            psDBF->fp) != 1) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        psDBF->sHooks.FClose(psDBF->fp);
        free(pabyBuf);
        free(psDBF->pszCurrentRecord);
        free(psDBF->pszCodePage);
        free(psDBF);
        return SHPLIB_NULLPTR;
    }

    psDBF->panFieldOffset = STATIC_CAST(int *, malloc(sizeof(int) * nFields));
    psDBF->panFieldSize = STATIC_CAST(int *, malloc(sizeof(int) * nFields));
    psDBF->panFieldDecimals = STATIC_CAST(int *, malloc(sizeof(int) * nFields));
    psDBF->pachFieldType = STATIC_CAST(char *, malloc(sizeof(char) * nFields));

    for (int iField = 0; iField < nFields; iField++) {
        unsigned char *pabyFInfo = pabyBuf + iField * XBASE_FLDHDR_SZ;
        if (pabyFInfo[0] == HEADER_RECORD_TERMINATOR) {
            psDBF->nFields = iField;
            break;
        }

        if (pabyFInfo[11] == 'N' || pabyFInfo[11] == 'F') {
            psDBF->panFieldSize[iField] = pabyFInfo[16];
            psDBF->panFieldDecimals[iField] = pabyFInfo[17];
        }
        else {
            psDBF->panFieldSize[iField] = pabyFInfo[16];
            psDBF->panFieldDecimals[iField] = 0;

            /*
            ** The following seemed to be used sometimes to handle files with
            long
            ** string fields, but in other cases (such as bug 1202) the decimals
            field
            ** just seems to indicate some sort of preferred formatting, not
            very
            ** wide fields.  So I have disabled this code.  FrankW.
                    psDBF->panFieldSize[iField] = pabyFInfo[16] +
            pabyFInfo[17]*256; psDBF->panFieldDecimals[iField] = 0;
            */
        }

<<<<<<< HEAD
        psDBF->pachFieldType[iField] = (char)pabyFInfo[11];
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        psDBF->pachFieldType[iField] = STATIC_CAST(char, pabyFInfo[11]);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    pabyBuf = STATIC_CAST(unsigned char *, SfRealloc(pabyBuf, nHeadLen));
    psDBF->pszHeader = REINTERPRET_CAST(char *, pabyBuf);

    psDBF->sHooks.FSeek(psDBF->fp, XBASE_FILEHDR_SZ, 0);
    if (psDBF->sHooks.FRead(pabyBuf, nHeadLen - XBASE_FILEHDR_SZ, 1,
                            psDBF->fp) != 1) {
        psDBF->sHooks.FClose(psDBF->fp);
        free(pabyBuf);
        free(psDBF->pszCurrentRecord);
        free(psDBF->pszCodePage);
        free(psDBF);
        return SHPLIB_NULLPTR;
    }

    psDBF->panFieldOffset = STATIC_CAST(int *, malloc(sizeof(int) * nFields));
    psDBF->panFieldSize = STATIC_CAST(int *, malloc(sizeof(int) * nFields));
    psDBF->panFieldDecimals = STATIC_CAST(int *, malloc(sizeof(int) * nFields));
    psDBF->pachFieldType = STATIC_CAST(char *, malloc(sizeof(char) * nFields));

    for (int iField = 0; iField < nFields; iField++) {
        unsigned char *pabyFInfo = pabyBuf + iField * XBASE_FLDHDR_SZ;
        if (pabyFInfo[0] == HEADER_RECORD_TERMINATOR) {
            psDBF->nFields = iField;
            break;
        }

        if (pabyFInfo[11] == 'N' || pabyFInfo[11] == 'F') {
            psDBF->panFieldSize[iField] = pabyFInfo[16];
            psDBF->panFieldDecimals[iField] = pabyFInfo[17];
        }
        else {
            psDBF->panFieldSize[iField] = pabyFInfo[16];
            psDBF->panFieldDecimals[iField] = 0;

            /*
            ** The following seemed to be used sometimes to handle files with
            long
            ** string fields, but in other cases (such as bug 1202) the decimals
            field
            ** just seems to indicate some sort of preferred formatting, not
            very
            ** wide fields.  So I have disabled this code.  FrankW.
                    psDBF->panFieldSize[iField] = pabyFInfo[16] +
            pabyFInfo[17]*256; psDBF->panFieldDecimals[iField] = 0;
            */
        }

        psDBF->pachFieldType[iField] = STATIC_CAST(char, pabyFInfo[11]);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        if (iField == 0)
            psDBF->panFieldOffset[iField] = 1;
        else
            psDBF->panFieldOffset[iField] = psDBF->panFieldOffset[iField - 1] +
                                            psDBF->panFieldSize[iField - 1];
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* Check that the total width of fields does not exceed the record width */
    if (psDBF->nFields > 0 && psDBF->panFieldOffset[psDBF->nFields - 1] +
                                      psDBF->panFieldSize[psDBF->nFields - 1] >
                                  psDBF->nRecordLength) {
        DBFClose(psDBF);
        return SHPLIB_NULLPTR;
    }

    DBFSetWriteEndOfFileChar(psDBF, TRUE);

    psDBF->bRequireNextWriteSeek = TRUE;

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    return (psDBF);
}

/************************************************************************/
/*                              DBFClose()                              */
/************************************************************************/

void SHPAPI_CALL DBFClose(DBFHandle psDBF)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psDBF == SHPLIB_NULLPTR)
=======
    if (psDBF == NULL)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psDBF == NULL)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psDBF == SHPLIB_NULLPTR)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (psDBF == NULL)
=======
    if (psDBF == SHPLIB_NULLPTR)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        return;

    /* -------------------------------------------------------------------- */
    /*      Write out header if not already written.                        */
    /* -------------------------------------------------------------------- */
    if (psDBF->bNoHeader)
        DBFWriteHeader(psDBF);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    CPL_IGNORE_RET_VAL_INT(DBFFlushRecord(psDBF));

    /* -------------------------------------------------------------------- */
    /*      Update last access date, and number of records if we have    */
    /*    write access.                                    */
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    DBFFlushRecord(psDBF);

    /* -------------------------------------------------------------------- */
    /*      Update last access date, and number of records if we have       */
    /*      write access.                                                   */
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    CPL_IGNORE_RET_VAL_INT(DBFFlushRecord(psDBF));

    /* -------------------------------------------------------------------- */
    /*      Update last access date, and number of records if we have    */
    /*    write access.                                    */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* -------------------------------------------------------------------- */
    if (psDBF->bUpdated)
        DBFUpdateHeader(psDBF);

    /* -------------------------------------------------------------------- */
    /*      Close, and free resources.                                      */
    /* -------------------------------------------------------------------- */
    psDBF->sHooks.FClose(psDBF->fp);

<<<<<<< HEAD
<<<<<<< HEAD
    if (psDBF->panFieldOffset != SHPLIB_NULLPTR) {
=======
    if (psDBF->panFieldOffset != NULL) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    DBFFlushRecord(psDBF);
=======
    CPL_IGNORE_RET_VAL_INT(DBFFlushRecord(psDBF));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    /* -------------------------------------------------------------------- */
    /*      Update last access date, and number of records if we have    */
    /*    write access.                                    */
    /* -------------------------------------------------------------------- */
    if (psDBF->bUpdated)
        DBFUpdateHeader(psDBF);

    /* -------------------------------------------------------------------- */
    /*      Close, and free resources.                                      */
    /* -------------------------------------------------------------------- */
    psDBF->sHooks.FClose(psDBF->fp);

<<<<<<< HEAD
    if (psDBF->panFieldOffset != NULL) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psDBF->panFieldOffset != SHPLIB_NULLPTR) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (psDBF->panFieldOffset != NULL) {
=======
    if (psDBF->panFieldOffset != SHPLIB_NULLPTR) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        free(psDBF->panFieldOffset);
        free(psDBF->panFieldSize);
        free(psDBF->panFieldDecimals);
        free(psDBF->pachFieldType);
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psDBF->pszWorkField != SHPLIB_NULLPTR)
=======
    if (psDBF->pszWorkField != NULL)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psDBF->pszWorkField != NULL)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psDBF->pszWorkField != SHPLIB_NULLPTR)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (psDBF->pszWorkField != NULL)
=======
    if (psDBF->pszWorkField != SHPLIB_NULLPTR)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        free(psDBF->pszWorkField);

    free(psDBF->pszHeader);
    free(psDBF->pszCurrentRecord);
    free(psDBF->pszCodePage);

    free(psDBF);
}

/************************************************************************/
/*                             DBFCreate()                              */
/*                                                                      */
/* Create a new .dbf file with default code page LDID/87 (0x57)         */
/************************************************************************/

DBFHandle SHPAPI_CALL DBFCreate(const char *pszFilename)
{
    return DBFCreateEx(pszFilename, "LDID/87"); // 0x57
}

/************************************************************************/
/*                            DBFCreateEx()                             */
/*                                                                      */
/*      Create a new .dbf file.                                         */
/************************************************************************/

DBFHandle SHPAPI_CALL DBFCreateEx(const char *pszFilename,
                                  const char *pszCodePage)
{
    SAHooks sHooks;

    SASetupDefaultHooks(&sHooks);

    return DBFCreateLL(pszFilename, pszCodePage, &sHooks);
}

/************************************************************************/
/*                             DBFCreate()                              */
/*                                                                      */
/*      Create a new .dbf file.                                         */
/************************************************************************/

DBFHandle SHPAPI_CALL DBFCreateLL(const char *pszFilename,
                                  const char *pszCodePage, SAHooks *psHooks)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /* -------------------------------------------------------------------- */
    /*    Compute the base (layer) name.  If there is any extension    */
    /*    on the passed in filename we will strip it off.            */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = DBFGetLenWithoutExtension(pszFilename);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszFilename, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".dbf", 5);

    /* -------------------------------------------------------------------- */
    /*      Create the file.                                                */
    /* -------------------------------------------------------------------- */
    SAFile fp = psHooks->FOpen(pszFullname, "wb");
    if (fp == SHPLIB_NULLPTR) {
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    char chZero = '\0';
    psHooks->FWrite(&chZero, 1, 1, fp);
    psHooks->FClose(fp);

    fp = psHooks->FOpen(pszFullname, "rb+");
    if (fp == SHPLIB_NULLPTR) {
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    memcpy(pszFullname + nLenWithoutExtension, ".cpg", 5);
    int ldid = -1;
    if (pszCodePage != SHPLIB_NULLPTR) {
=======
    DBFHandle psDBF;
    SAFile fp;
    char *pszFullname, *pszBasename;
    int i, ldid = -1;
    char chZero = '\0';
    size_t nFullnameLen;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    /*    Compute the base (layer) name.  If there is any extension    */
    /*    on the passed in filename we will strip it off.            */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
    pszBasename = (char *)malloc(strlen(pszFilename) + 5);
    strcpy(pszBasename, pszFilename);
    for (i = (int)strlen(pszBasename) - 1;
         i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' &&
         pszBasename[i] != '\\';
         i--) {
    }

=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    DBFHandle psDBF;
    SAFile fp;
    char *pszFullname, *pszBasename;
    int i, ldid = -1;
    char chZero = '\0';
    size_t nFullnameLen;

    /* -------------------------------------------------------------------- */
    /*      Compute the base (layer) name.  If there is any extension       */
    /*      on the passed in filename we will strip it off.                 */
    /* -------------------------------------------------------------------- */
    pszBasename = (char *)malloc(strlen(pszFilename) + 5);
    strcpy(pszBasename, pszFilename);
    for (i = (int)strlen(pszBasename) - 1;
         i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' &&
         pszBasename[i] != '\\';
         i--) {
    }

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if (pszBasename[i] == '.')
        pszBasename[i] = '\0';

    nFullnameLen = strlen(pszBasename) + 5;
    pszFullname = (char *)malloc(nFullnameLen);
    snprintf(pszFullname, nFullnameLen, "%s.dbf", pszBasename);
=======
    const int nLenWithoutExtension = DBFGetLenWithoutExtension(pszFilename);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszFilename, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".dbf", 5);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    /* -------------------------------------------------------------------- */
    /*      Create the file.                                                */
    /* -------------------------------------------------------------------- */
    SAFile fp = psHooks->FOpen(pszFullname, "wb");
    if (fp == SHPLIB_NULLPTR) {
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    char chZero = '\0';
    psHooks->FWrite(&chZero, 1, 1, fp);
    psHooks->FClose(fp);

    fp = psHooks->FOpen(pszFullname, "rb+");
    if (fp == SHPLIB_NULLPTR) {
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

<<<<<<< HEAD
    snprintf(pszFullname, nFullnameLen, "%s.cpg", pszBasename);
    if (pszCodePage != NULL) {
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    memcpy(pszFullname + nLenWithoutExtension, ".cpg", 5);
    int ldid = -1;
    if (pszCodePage != SHPLIB_NULLPTR) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    /* -------------------------------------------------------------------- */
    /*    Compute the base (layer) name.  If there is any extension    */
    /*    on the passed in filename we will strip it off.            */
    /* -------------------------------------------------------------------- */
    const int nLenWithoutExtension = DBFGetLenWithoutExtension(pszFilename);
    char *pszFullname = STATIC_CAST(char *, malloc(nLenWithoutExtension + 5));
    memcpy(pszFullname, pszFilename, nLenWithoutExtension);
    memcpy(pszFullname + nLenWithoutExtension, ".dbf", 5);

    /* -------------------------------------------------------------------- */
    /*      Create the file.                                                */
    /* -------------------------------------------------------------------- */
    SAFile fp = psHooks->FOpen(pszFullname, "wb");
    if (fp == SHPLIB_NULLPTR) {
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    char chZero = '\0';
    psHooks->FWrite(&chZero, 1, 1, fp);
    psHooks->FClose(fp);

    fp = psHooks->FOpen(pszFullname, "rb+");
    if (fp == SHPLIB_NULLPTR) {
        free(pszFullname);
        return SHPLIB_NULLPTR;
    }

    memcpy(pszFullname + nLenWithoutExtension, ".cpg", 5);
    int ldid = -1;
    if (pszCodePage != SHPLIB_NULLPTR) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        if (strncmp(pszCodePage, "LDID/", 5) == 0) {
            ldid = atoi(pszCodePage + 5);
            if (ldid > 255)
                ldid = -1; // don't use 0 to indicate out of range as LDID/0 is
                           // a valid one
        }
        if (ldid < 0) {
            SAFile fpCPG = psHooks->FOpen(pszFullname, "w");
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            psHooks->FWrite(
                CONST_CAST(void *, STATIC_CAST(const void *, pszCodePage)),
                strlen(pszCodePage), 1, fpCPG);
            psHooks->FClose(fpCPG);
        }
    }
    if (pszCodePage == SHPLIB_NULLPTR || ldid >= 0) {
        psHooks->Remove(pszFullname);
    }

    free(pszFullname);

    /* -------------------------------------------------------------------- */
    /*    Create the info structure.                    */
    /* -------------------------------------------------------------------- */
    DBFHandle psDBF = STATIC_CAST(DBFHandle, calloc(1, sizeof(DBFInfo)));
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

            psHooks->FWrite((char *)pszCodePage, strlen(pszCodePage), 1, fpCPG);
            psHooks->FClose(fpCPG);
        }
    }
    if (pszCodePage == NULL || ldid >= 0) {
        psHooks->Remove(pszFullname);
    }

    free(pszBasename);
    free(pszFullname);

    /* -------------------------------------------------------------------- */
    /*      Create the info structure.                                      */
    /* -------------------------------------------------------------------- */
    psDBF = (DBFHandle)calloc(1, sizeof(DBFInfo));
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
            psHooks->FWrite(
                CONST_CAST(void *, STATIC_CAST(const void *, pszCodePage)),
                strlen(pszCodePage), 1, fpCPG);
            psHooks->FClose(fpCPG);
        }
    }
    if (pszCodePage == SHPLIB_NULLPTR || ldid >= 0) {
        psHooks->Remove(pszFullname);
    }

    free(pszFullname);

    /* -------------------------------------------------------------------- */
    /*    Create the info structure.                    */
    /* -------------------------------------------------------------------- */
    DBFHandle psDBF = STATIC_CAST(DBFHandle, calloc(1, sizeof(DBFInfo)));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

=======

            psHooks->FWrite((char *)pszCodePage, strlen(pszCodePage), 1, fpCPG);
=======
            psHooks->FWrite(
                CONST_CAST(void *, STATIC_CAST(const void *, pszCodePage)),
                strlen(pszCodePage), 1, fpCPG);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
            psHooks->FClose(fpCPG);
        }
    }
    if (pszCodePage == SHPLIB_NULLPTR || ldid >= 0) {
        psHooks->Remove(pszFullname);
    }

    free(pszFullname);

    /* -------------------------------------------------------------------- */
    /*    Create the info structure.                    */
    /* -------------------------------------------------------------------- */
    DBFHandle psDBF = STATIC_CAST(DBFHandle, calloc(1, sizeof(DBFInfo)));

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    memcpy(&(psDBF->sHooks), psHooks, sizeof(SAHooks));
    psDBF->fp = fp;
    psDBF->nRecords = 0;
    psDBF->nFields = 0;
    psDBF->nRecordLength = 1;
    psDBF->nHeaderLength =
        XBASE_FILEHDR_SZ + 1; /* + 1 for HEADER_RECORD_TERMINATOR */

    psDBF->panFieldOffset = SHPLIB_NULLPTR;
    psDBF->panFieldSize = SHPLIB_NULLPTR;
    psDBF->panFieldDecimals = SHPLIB_NULLPTR;
    psDBF->pachFieldType = SHPLIB_NULLPTR;
    psDBF->pszHeader = SHPLIB_NULLPTR;

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;
    psDBF->pszCurrentRecord = SHPLIB_NULLPTR;

    psDBF->bNoHeader = TRUE;

    psDBF->iLanguageDriver = ldid > 0 ? ldid : 0;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    psDBF->pszCodePage = SHPLIB_NULLPTR;
    if (pszCodePage) {
        psDBF->pszCodePage =
            STATIC_CAST(char *, malloc(strlen(pszCodePage) + 1));
=======
    psDBF->pszCodePage = NULL;
    if (pszCodePage) {
        psDBF->pszCodePage = (char *)malloc(strlen(pszCodePage) + 1);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    psDBF->pszCodePage = NULL;
    if (pszCodePage) {
        psDBF->pszCodePage = (char *)malloc(strlen(pszCodePage) + 1);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    psDBF->pszCodePage = SHPLIB_NULLPTR;
    if (pszCodePage) {
        psDBF->pszCodePage =
            STATIC_CAST(char *, malloc(strlen(pszCodePage) + 1));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        strcpy(psDBF->pszCodePage, pszCodePage);
    }
    DBFSetLastModifiedDate(psDBF, 95, 7, 26); /* dummy date */

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    DBFSetWriteEndOfFileChar(psDBF, TRUE);

    psDBF->bRequireNextWriteSeek = TRUE;

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    return (psDBF);
}

/************************************************************************/
/*                            DBFAddField()                             */
/*                                                                      */
/*      Add a field to a newly created .dbf or to an existing one       */
/************************************************************************/

int SHPAPI_CALL DBFAddField(DBFHandle psDBF, const char *pszFieldName,
                            DBFFieldType eType, int nWidth, int nDecimals)
{
    char chNativeType;

    if (eType == FTLogical)
        chNativeType = 'L';
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    else if (eType == FTDate)
        chNativeType = 'D';
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    else if (eType == FTDate)
        chNativeType = 'D';
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    else if (eType == FTDate)
        chNativeType = 'D';
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    else if (eType == FTString)
        chNativeType = 'C';
    else
        chNativeType = 'N';

    return DBFAddNativeFieldType(psDBF, pszFieldName, chNativeType, nWidth,
                                 nDecimals);
}

/************************************************************************/
/*                        DBFGetNullCharacter()                         */
/************************************************************************/

static char DBFGetNullCharacter(char chType)
{
    switch (chType) {
    case 'N':
    case 'F':
        return '*';
    case 'D':
        return '0';
    case 'L':
        return '?';
    default:
        return ' ';
    }
}

/************************************************************************/
/*                            DBFAddField()                             */
/*                                                                      */
/*      Add a field to a newly created .dbf file before any records     */
/*      are written.                                                    */
/************************************************************************/

int SHPAPI_CALL DBFAddNativeFieldType(DBFHandle psDBF, const char *pszFieldName,
                                      char chType, int nWidth, int nDecimals)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    char *pszFInfo;
    int i;
    int nOldRecordLength, nOldHeaderLength;
    char *pszRecord;
    char chFieldFill;
    SAOffset nRecordOffset;

<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* make sure that everything is written in .dbf */
    if (!DBFFlushRecord(psDBF))
        return -1;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psDBF->nHeaderLength + XBASE_FLDHDR_SZ > 65535) {
        char szMessage[128];
        snprintf(szMessage, sizeof(szMessage),
                 "Cannot add field %s. Header length limit reached "
                 "(max 65535 bytes, 2046 fields).",
                 pszFieldName);
        psDBF->sHooks.Error(szMessage);
        return -1;
    }

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* -------------------------------------------------------------------- */
    /*      Do some checking to ensure we can add records to this file.     */
    /* -------------------------------------------------------------------- */
    if (nWidth < 1)
        return -1;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (nWidth > XBASE_FLD_MAX_WIDTH)
        nWidth = XBASE_FLD_MAX_WIDTH;
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if (nWidth > 255)
        nWidth = 255;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (nWidth > XBASE_FLD_MAX_WIDTH)
        nWidth = XBASE_FLD_MAX_WIDTH;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (nWidth > 255)
        nWidth = 255;
=======
    if (nWidth > XBASE_FLD_MAX_WIDTH)
        nWidth = XBASE_FLD_MAX_WIDTH;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    if (psDBF->nRecordLength + nWidth > 65535) {
        char szMessage[128];
        snprintf(szMessage, sizeof(szMessage),
                 "Cannot add field %s. Record length limit reached "
                 "(max 65535 bytes).",
                 pszFieldName);
        psDBF->sHooks.Error(szMessage);
        return -1;
    }
<<<<<<< HEAD
<<<<<<< HEAD
=======

    const int nOldRecordLength = psDBF->nRecordLength;
    const int nOldHeaderLength = psDBF->nHeaderLength;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

<<<<<<< HEAD
<<<<<<< HEAD
    const int nOldRecordLength = psDBF->nRecordLength;
    const int nOldHeaderLength = psDBF->nHeaderLength;

=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======

<<<<<<< HEAD
=======
    const int nOldRecordLength = psDBF->nRecordLength;
    const int nOldHeaderLength = psDBF->nHeaderLength;

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* -------------------------------------------------------------------- */
    /*      SfRealloc all the arrays larger to hold the additional field      */
    /*      information.                                                    */
    /* -------------------------------------------------------------------- */
    psDBF->nFields++;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    psDBF->panFieldOffset = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields));

    psDBF->panFieldSize = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldSize, sizeof(int) * psDBF->nFields));

    psDBF->panFieldDecimals =
        STATIC_CAST(int *, SfRealloc(psDBF->panFieldDecimals,
                                     sizeof(int) * psDBF->nFields));

    psDBF->pachFieldType = STATIC_CAST(
        char *, SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields));
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    psDBF->panFieldOffset =
        (int *)SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields);

    psDBF->panFieldSize =
        (int *)SfRealloc(psDBF->panFieldSize, sizeof(int) * psDBF->nFields);

    psDBF->panFieldDecimals =
        (int *)SfRealloc(psDBF->panFieldDecimals, sizeof(int) * psDBF->nFields);

    psDBF->pachFieldType =
        (char *)SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    psDBF->panFieldOffset = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields));

    psDBF->panFieldSize = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldSize, sizeof(int) * psDBF->nFields));

    psDBF->panFieldDecimals =
        STATIC_CAST(int *, SfRealloc(psDBF->panFieldDecimals,
                                     sizeof(int) * psDBF->nFields));

    psDBF->pachFieldType = STATIC_CAST(
        char *, SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

=======
    psDBF->panFieldOffset =
        (int *)SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields);
=======
    psDBF->panFieldOffset = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    psDBF->panFieldSize = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldSize, sizeof(int) * psDBF->nFields));

    psDBF->panFieldDecimals =
        STATIC_CAST(int *, SfRealloc(psDBF->panFieldDecimals,
                                     sizeof(int) * psDBF->nFields));

    psDBF->pachFieldType = STATIC_CAST(
        char *, SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields));

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    /* -------------------------------------------------------------------- */
    /*      Assign the new field information fields.                        */
    /* -------------------------------------------------------------------- */
    psDBF->panFieldOffset[psDBF->nFields - 1] = psDBF->nRecordLength;
    psDBF->nRecordLength += nWidth;
    psDBF->panFieldSize[psDBF->nFields - 1] = nWidth;
    psDBF->panFieldDecimals[psDBF->nFields - 1] = nDecimals;
    psDBF->pachFieldType[psDBF->nFields - 1] = chType;

    /* -------------------------------------------------------------------- */
    /*      Extend the required header information.                         */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    psDBF->nHeaderLength += XBASE_FLDHDR_SZ;
=======
    psDBF->nHeaderLength += 32;
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    psDBF->bUpdated = FALSE;

    psDBF->pszHeader = STATIC_CAST(
        char *, SfRealloc(psDBF->pszHeader, psDBF->nFields * XBASE_FLDHDR_SZ));

    char *pszFInfo = psDBF->pszHeader + XBASE_FLDHDR_SZ * (psDBF->nFields - 1);

    for (int i = 0; i < XBASE_FLDHDR_SZ; i++)
        pszFInfo[i] = '\0';

    strncpy(pszFInfo, pszFieldName, XBASE_FLDNAME_LEN_WRITE);
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    psDBF->nHeaderLength += 32;
=======
    psDBF->nHeaderLength += XBASE_FLDHDR_SZ;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    psDBF->bUpdated = FALSE;

    psDBF->pszHeader = STATIC_CAST(
        char *, SfRealloc(psDBF->pszHeader, psDBF->nFields * XBASE_FLDHDR_SZ));

    char *pszFInfo = psDBF->pszHeader + XBASE_FLDHDR_SZ * (psDBF->nFields - 1);

    for (int i = 0; i < XBASE_FLDHDR_SZ; i++)
        pszFInfo[i] = '\0';

<<<<<<< HEAD
    if ((int)strlen(pszFieldName) < 10)
        strncpy(pszFInfo, pszFieldName, strlen(pszFieldName));
    else
        strncpy(pszFInfo, pszFieldName, 10);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    strncpy(pszFInfo, pszFieldName, XBASE_FLDNAME_LEN_WRITE);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    psDBF->nHeaderLength += XBASE_FLDHDR_SZ;
    psDBF->bUpdated = FALSE;

    psDBF->pszHeader = STATIC_CAST(
        char *, SfRealloc(psDBF->pszHeader, psDBF->nFields * XBASE_FLDHDR_SZ));

    char *pszFInfo = psDBF->pszHeader + XBASE_FLDHDR_SZ * (psDBF->nFields - 1);

    for (int i = 0; i < XBASE_FLDHDR_SZ; i++)
        pszFInfo[i] = '\0';

    strncpy(pszFInfo, pszFieldName, XBASE_FLDNAME_LEN_WRITE);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    pszFInfo[11] = psDBF->pachFieldType[psDBF->nFields - 1];

    if (chType == 'C') {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        pszFInfo[16] = STATIC_CAST(unsigned char, nWidth % 256);
        pszFInfo[17] = STATIC_CAST(unsigned char, nWidth / 256);
    }
    else {
        pszFInfo[16] = STATIC_CAST(unsigned char, nWidth);
        pszFInfo[17] = STATIC_CAST(unsigned char, nDecimals);
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        pszFInfo[16] = (unsigned char)(nWidth % 256);
        pszFInfo[17] = (unsigned char)(nWidth / 256);
    }
    else {
        pszFInfo[16] = (unsigned char)nWidth;
        pszFInfo[17] = (unsigned char)nDecimals;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        pszFInfo[16] = (unsigned char)(nWidth % 256);
        pszFInfo[17] = (unsigned char)(nWidth / 256);
    }
    else {
        pszFInfo[16] = (unsigned char)nWidth;
        pszFInfo[17] = (unsigned char)nDecimals;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        pszFInfo[16] = STATIC_CAST(unsigned char, nWidth % 256);
        pszFInfo[17] = STATIC_CAST(unsigned char, nWidth / 256);
    }
    else {
        pszFInfo[16] = STATIC_CAST(unsigned char, nWidth);
        pszFInfo[17] = STATIC_CAST(unsigned char, nDecimals);
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    /* -------------------------------------------------------------------- */
    /*      Make the current record buffer appropriately larger.            */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    psDBF->pszCurrentRecord = STATIC_CAST(
        char *, SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength));
=======
    psDBF->pszCurrentRecord =
        (char *)SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    psDBF->pszCurrentRecord =
        (char *)SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    psDBF->pszCurrentRecord = STATIC_CAST(
        char *, SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    psDBF->pszCurrentRecord =
        (char *)SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength);
=======
    psDBF->pszCurrentRecord = STATIC_CAST(
        char *, SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    /* we're done if dealing with new .dbf */
    if (psDBF->bNoHeader)
        return (psDBF->nFields - 1);

    /* -------------------------------------------------------------------- */
    /*      For existing .dbf file, shift records                           */
    /* -------------------------------------------------------------------- */

    /* alloc record */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    char *pszRecord =
        STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nRecordLength));
=======
    pszRecord = (char *)malloc(sizeof(char) * psDBF->nRecordLength);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    pszRecord = (char *)malloc(sizeof(char) * psDBF->nRecordLength);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    pszRecord = (char *)malloc(sizeof(char) * psDBF->nRecordLength);
=======
    char *pszRecord =
        STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nRecordLength));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    const char chFieldFill = DBFGetNullCharacter(chType);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
    char *pszRecord =
        STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nRecordLength));

    const char chFieldFill = DBFGetNullCharacter(chType);

>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    SAOffset nRecordOffset;
    for (int i = psDBF->nRecords - 1; i >= 0; --i) {
        nRecordOffset =
            nOldRecordLength * STATIC_CAST(SAOffset, i) + nOldHeaderLength;
<<<<<<< HEAD

        /* load record */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1, psDBF->fp) !=
            1) {
            free(pszRecord);
            return -1;
        }
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    for (i = psDBF->nRecords - 1; i >= 0; --i) {
        nRecordOffset = nOldRecordLength * (SAOffset)i + nOldHeaderLength;

        /* load record */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1, psDBF->fp);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    for (i = psDBF->nRecords - 1; i >= 0; --i) {
        nRecordOffset = nOldRecordLength * (SAOffset)i + nOldHeaderLength;

        /* load record */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1, psDBF->fp);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
    SAOffset nRecordOffset;
    for (int i = psDBF->nRecords - 1; i >= 0; --i) {
        nRecordOffset =
            nOldRecordLength * STATIC_CAST(SAOffset, i) + nOldHeaderLength;
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

        /* load record */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1, psDBF->fp) !=
            1) {
            free(pszRecord);
            return -1;
        }
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

        /* set new field's value to NULL */
        memset(pszRecord + nOldRecordLength, chFieldFill, nWidth);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        nRecordOffset = psDBF->nRecordLength * STATIC_CAST(SAOffset, i) +
                        psDBF->nHeaderLength;

        /* move record to the new place*/
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        psDBF->sHooks.FWrite(pszRecord, psDBF->nRecordLength, 1, psDBF->fp);
    }

    if (psDBF->bWriteEndOfFileChar) {
        char ch = END_OF_FILE_CHARACTER;

        nRecordOffset =
            psDBF->nRecordLength * STATIC_CAST(SAOffset, psDBF->nRecords) +
            psDBF->nHeaderLength;

        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
=======
        nRecordOffset =
            psDBF->nRecordLength * (SAOffset)i + psDBF->nHeaderLength;
=======
        nRecordOffset = psDBF->nRecordLength * STATIC_CAST(SAOffset, i) +
                        psDBF->nHeaderLength;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

        /* move record to the new place*/
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        psDBF->sHooks.FWrite(pszRecord, psDBF->nRecordLength, 1, psDBF->fp);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        nRecordOffset =
            psDBF->nRecordLength * (SAOffset)i + psDBF->nHeaderLength;

        /* move record to the new place */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        psDBF->sHooks.FWrite(pszRecord, psDBF->nRecordLength, 1, psDBF->fp);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        nRecordOffset = psDBF->nRecordLength * STATIC_CAST(SAOffset, i) +
                        psDBF->nHeaderLength;

        /* move record to the new place*/
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        psDBF->sHooks.FWrite(pszRecord, psDBF->nRecordLength, 1, psDBF->fp);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    if (psDBF->bWriteEndOfFileChar) {
        char ch = END_OF_FILE_CHARACTER;

        nRecordOffset =
            psDBF->nRecordLength * STATIC_CAST(SAOffset, psDBF->nRecords) +
            psDBF->nHeaderLength;

        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
<<<<<<< HEAD
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    /* free record */
    free(pszRecord);

    /* force update of header with new header, record length and new field */
    psDBF->bNoHeader = TRUE;
    DBFUpdateHeader(psDBF);

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;
    psDBF->bUpdated = TRUE;

    return (psDBF->nFields - 1);
}

/************************************************************************/
/*                          DBFReadAttribute()                          */
/*                                                                      */
/*      Read one of the attribute fields of a record.                   */
/************************************************************************/

static void *DBFReadAttribute(DBFHandle psDBF, int hEntity, int iField,
                              char chReqType)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /* -------------------------------------------------------------------- */
    /*      Verify selection.                                               */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity >= psDBF->nRecords)
        return SHPLIB_NULLPTR;

    if (iField < 0 || iField >= psDBF->nFields)
        return SHPLIB_NULLPTR;

    /* -------------------------------------------------------------------- */
    /*    Have we read the record?                    */
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, hEntity))
        return SHPLIB_NULLPTR;

    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);

=======
    unsigned char *pabyRec;
    void *pReturnField = NULL;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    /*      Verify selection.                                               */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity >= psDBF->nRecords)
        return SHPLIB_NULLPTR;

    if (iField < 0 || iField >= psDBF->nFields)
        return SHPLIB_NULLPTR;

=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    unsigned char *pabyRec;
    void *pReturnField = NULL;

    /* -------------------------------------------------------------------- */
    /*      Verify selection.                                               */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity >= psDBF->nRecords)
        return (NULL);

    if (iField < 0 || iField >= psDBF->nFields)
        return (NULL);

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    /* -------------------------------------------------------------------- */
    /*    Have we read the record?                    */
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, hEntity))
        return SHPLIB_NULLPTR;

    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    /* -------------------------------------------------------------------- */
    /*      Verify selection.                                               */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity >= psDBF->nRecords)
        return SHPLIB_NULLPTR;

    if (iField < 0 || iField >= psDBF->nFields)
        return SHPLIB_NULLPTR;

    /* -------------------------------------------------------------------- */
    /*    Have we read the record?                    */
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, hEntity))
        return SHPLIB_NULLPTR;

    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* -------------------------------------------------------------------- */
    /*      Ensure we have room to extract the target field.                */
    /* -------------------------------------------------------------------- */
    if (psDBF->panFieldSize[iField] >= psDBF->nWorkFieldLength) {
        psDBF->nWorkFieldLength = psDBF->panFieldSize[iField] + 100;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if (psDBF->pszWorkField == SHPLIB_NULLPTR)
            psDBF->pszWorkField =
                STATIC_CAST(char *, malloc(psDBF->nWorkFieldLength));
        else
            psDBF->pszWorkField = STATIC_CAST(
                char *, realloc(psDBF->pszWorkField, psDBF->nWorkFieldLength));
    }

    /* -------------------------------------------------------------------- */
    /*    Extract the requested field.                    */
    /* -------------------------------------------------------------------- */
    memcpy(psDBF->pszWorkField,
           REINTERPRET_CAST(const char *, pabyRec) +
               psDBF->panFieldOffset[iField],
=======
        if (psDBF->pszWorkField == NULL)
            psDBF->pszWorkField = (char *)malloc(psDBF->nWorkFieldLength);
        else
=======
        if (psDBF->pszWorkField == SHPLIB_NULLPTR)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
            psDBF->pszWorkField =
                STATIC_CAST(char *, malloc(psDBF->nWorkFieldLength));
        else
            psDBF->pszWorkField = STATIC_CAST(
                char *, realloc(psDBF->pszWorkField, psDBF->nWorkFieldLength));
    }

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        if (psDBF->pszWorkField == NULL)
            psDBF->pszWorkField = (char *)malloc(psDBF->nWorkFieldLength);
        else
            psDBF->pszWorkField =
                (char *)realloc(psDBF->pszWorkField, psDBF->nWorkFieldLength);
    }

    /* -------------------------------------------------------------------- */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    /*      Extract the requested field.                                    */
    /* -------------------------------------------------------------------- */
    memcpy(psDBF->pszWorkField,
           ((const char *)pabyRec) + psDBF->panFieldOffset[iField],
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
        if (psDBF->pszWorkField == SHPLIB_NULLPTR)
            psDBF->pszWorkField =
                STATIC_CAST(char *, malloc(psDBF->nWorkFieldLength));
        else
            psDBF->pszWorkField = STATIC_CAST(
                char *, realloc(psDBF->pszWorkField, psDBF->nWorkFieldLength));
    }

    /* -------------------------------------------------------------------- */
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /*    Extract the requested field.                    */
    /* -------------------------------------------------------------------- */
    memcpy(psDBF->pszWorkField,
           REINTERPRET_CAST(const char *, pabyRec) +
               psDBF->panFieldOffset[iField],
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
           psDBF->panFieldSize[iField]);
    psDBF->pszWorkField[psDBF->panFieldSize[iField]] = '\0';

    void *pReturnField = psDBF->pszWorkField;

    /* -------------------------------------------------------------------- */
    /*      Decode the field.                                               */
    /* -------------------------------------------------------------------- */
    if (chReqType == 'I') {
        psDBF->fieldValue.nIntField = atoi(psDBF->pszWorkField);

        pReturnField = &(psDBF->fieldValue.nIntField);
    }
    else if (chReqType == 'N') {
        psDBF->fieldValue.dfDoubleField =
            psDBF->sHooks.Atof(psDBF->pszWorkField);

        pReturnField = &(psDBF->fieldValue.dfDoubleField);
    }

/* -------------------------------------------------------------------- */
/*      Should we trim white space off the string attribute value?      */
/* -------------------------------------------------------------------- */
#ifdef TRIM_DBF_WHITESPACE
    else {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        char *pchSrc = psDBF->pszWorkField;
        char *pchDst = pchSrc;

=======
        char *pchSrc, *pchDst;

        pchDst = pchSrc = psDBF->pszWorkField;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        char *pchSrc, *pchDst;

        pchDst = pchSrc = psDBF->pszWorkField;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        char *pchSrc, *pchDst;

        pchDst = pchSrc = psDBF->pszWorkField;
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        char *pchSrc = psDBF->pszWorkField;
        char *pchDst = pchSrc;

<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        while (*pchSrc == ' ')
            pchSrc++;

        while (*pchSrc != '\0')
            *(pchDst++) = *(pchSrc++);
        *pchDst = '\0';

        while (pchDst != psDBF->pszWorkField && *(--pchDst) == ' ')
            *pchDst = '\0';
    }
#endif

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    return pReturnField;
=======
    return (pReturnField);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    return (pReturnField);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    return pReturnField;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    return (pReturnField);
=======
    return pReturnField;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                        DBFReadIntAttribute()                         */
/*                                                                      */
/*      Read an integer attribute.                                      */
/************************************************************************/

int SHPAPI_CALL DBFReadIntegerAttribute(DBFHandle psDBF, int iRecord,
                                        int iField)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    int *pnValue =
        STATIC_CAST(int *, DBFReadAttribute(psDBF, iRecord, iField, 'I'));

    if (pnValue == SHPLIB_NULLPTR)
        return 0;
    else
        return *pnValue;
=======
    int *pnValue;
=======
    int *pnValue =
        STATIC_CAST(int *, DBFReadAttribute(psDBF, iRecord, iField, 'I'));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    if (pnValue == SHPLIB_NULLPTR)
        return 0;
    else
<<<<<<< HEAD
        return (*pnValue);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    int *pnValue;

    pnValue = (int *)DBFReadAttribute(psDBF, iRecord, iField, 'I');

    if (pnValue == NULL)
        return 0;
    else
        return (*pnValue);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        return *pnValue;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    int *pnValue =
        STATIC_CAST(int *, DBFReadAttribute(psDBF, iRecord, iField, 'I'));

    if (pnValue == SHPLIB_NULLPTR)
        return 0;
    else
        return *pnValue;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                        DBFReadDoubleAttribute()                      */
/*                                                                      */
/*      Read a double attribute.                                        */
/************************************************************************/

double SHPAPI_CALL DBFReadDoubleAttribute(DBFHandle psDBF, int iRecord,
                                          int iField)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    double *pdValue =
        STATIC_CAST(double *, DBFReadAttribute(psDBF, iRecord, iField, 'N'));

    if (pdValue == SHPLIB_NULLPTR)
        return 0.0;
    else
        return *pdValue;
=======
    double *pdValue;
=======
    double *pdValue =
        STATIC_CAST(double *, DBFReadAttribute(psDBF, iRecord, iField, 'N'));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    if (pdValue == SHPLIB_NULLPTR)
        return 0.0;
    else
<<<<<<< HEAD
        return (*pdValue);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    double *pdValue;

    pdValue = (double *)DBFReadAttribute(psDBF, iRecord, iField, 'N');

    if (pdValue == NULL)
        return 0.0;
    else
        return (*pdValue);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        return *pdValue;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    double *pdValue =
        STATIC_CAST(double *, DBFReadAttribute(psDBF, iRecord, iField, 'N'));

    if (pdValue == SHPLIB_NULLPTR)
        return 0.0;
    else
        return *pdValue;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                        DBFReadStringAttribute()                      */
/*                                                                      */
/*      Read a string attribute.                                        */
/************************************************************************/

const char SHPAPI_CALL1(*)
    DBFReadStringAttribute(DBFHandle psDBF, int iRecord, int iField)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

{
    return STATIC_CAST(const char *,
                       DBFReadAttribute(psDBF, iRecord, iField, 'C'));
=======
{
    return ((const char *)DBFReadAttribute(psDBF, iRecord, iField, 'C'));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return ((const char *)DBFReadAttribute(psDBF, iRecord, iField, 'C'));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return ((const char *)DBFReadAttribute(psDBF, iRecord, iField, 'C'));
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======

{
    return STATIC_CAST(const char *,
                       DBFReadAttribute(psDBF, iRecord, iField, 'C'));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                        DBFReadLogicalAttribute()                     */
/*                                                                      */
/*      Read a logical attribute.                                       */
/************************************************************************/

const char SHPAPI_CALL1(*)
    DBFReadLogicalAttribute(DBFHandle psDBF, int iRecord, int iField)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

{
    return STATIC_CAST(const char *,
                       DBFReadAttribute(psDBF, iRecord, iField, 'L'));
=======
{
    return ((const char *)DBFReadAttribute(psDBF, iRecord, iField, 'L'));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return ((const char *)DBFReadAttribute(psDBF, iRecord, iField, 'L'));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return ((const char *)DBFReadAttribute(psDBF, iRecord, iField, 'L'));
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======

{
    return STATIC_CAST(const char *,
                       DBFReadAttribute(psDBF, iRecord, iField, 'L'));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                         DBFIsValueNULL()                             */
/*                                                                      */
/*      Return TRUE if the passed string is NULL.                       */
/************************************************************************/

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static bool DBFIsValueNULL(char chType, const char *pszValue)
=======
static int DBFIsValueNULL(char chType, const char *pszValue)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
static int DBFIsValueNULL(char chType, const char *pszValue)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
static int DBFIsValueNULL(char chType, const char *pszValue)
=======
static bool DBFIsValueNULL(char chType, const char *pszValue)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
{
    if (pszValue == SHPLIB_NULLPTR)
        return true;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
    if (pszValue == NULL)
        return TRUE;
=======
static bool DBFIsValueNULL(char chType, const char *pszValue)
{
    if (pszValue == SHPLIB_NULLPTR)
        return true;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (pszValue == NULL)
        return TRUE;

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if (pszValue == NULL)
        return TRUE;

=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    switch (chType) {
    case 'N':
    case 'F':
        /*
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        ** We accept all asterisks or all blanks as NULL
        ** though according to the spec I think it should be all
        ** asterisks.
        */
<<<<<<< HEAD
        if (pszValue[0] == '*')
            return true;

        for (int i = 0; pszValue[i] != '\0'; i++) {
            if (pszValue[i] != ' ')
                return false;
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
         ** We accept all asterisks or all blanks as NULL
         ** though according to the spec I think it should be all
         ** asterisks.
         */
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        if (pszValue[0] == '*')
            return true;

        for (int i = 0; pszValue[i] != '\0'; i++) {
            if (pszValue[i] != ' ')
<<<<<<< HEAD
                return FALSE;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
                return false;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        ** We accept all asterisks or all blanks as NULL
        ** though according to the spec I think it should be all
        ** asterisks.
        */
        if (pszValue[0] == '*')
            return true;

        for (int i = 0; pszValue[i] != '\0'; i++) {
            if (pszValue[i] != ' ')
                return false;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }
        return true;

    case 'D':
        /* NULL date fields have value "00000000" */
        return strncmp(pszValue, "00000000", 8) == 0;

    case 'L':
        /* NULL boolean fields have value "?" */
        return pszValue[0] == '?';

    default:
        /* empty string fields are considered NULL */
        return strlen(pszValue) == 0;
    }
}

/************************************************************************/
/*                         DBFIsAttributeNULL()                         */
/*                                                                      */
/*      Return TRUE if value for field is NULL.                         */
/*                                                                      */
/*      Contributed by Jim Matthews.                                    */
/************************************************************************/

int SHPAPI_CALL DBFIsAttributeNULL(DBFHandle psDBF, int iRecord, int iField)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    const char *pszValue = DBFReadStringAttribute(psDBF, iRecord, iField);

    if (pszValue == SHPLIB_NULLPTR)
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    const char *pszValue;

    pszValue = DBFReadStringAttribute(psDBF, iRecord, iField);

    if (pszValue == NULL)
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    const char *pszValue;

    pszValue = DBFReadStringAttribute(psDBF, iRecord, iField);

    if (pszValue == NULL)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    const char *pszValue = DBFReadStringAttribute(psDBF, iRecord, iField);

    if (pszValue == SHPLIB_NULLPTR)
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        return TRUE;

    return DBFIsValueNULL(psDBF->pachFieldType[iField], pszValue);
}

/************************************************************************/
/*                          DBFGetFieldCount()                          */
/*                                                                      */
/*      Return the number of fields in this table.                      */
/************************************************************************/

int SHPAPI_CALL DBFGetFieldCount(DBFHandle psDBF)
<<<<<<< HEAD
<<<<<<< HEAD
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
{
    return (psDBF->nFields);
}

/************************************************************************/
/*                         DBFGetRecordCount()                          */
/*                                                                      */
/*      Return the number of records in this table.                     */
/************************************************************************/

int SHPAPI_CALL DBFGetRecordCount(DBFHandle psDBF)
<<<<<<< HEAD
<<<<<<< HEAD
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
{
    return (psDBF->nRecords);
}

/************************************************************************/
/*                          DBFGetFieldInfo()                           */
/*                                                                      */
/*      Return any requested information about the field.               */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*      pszFieldName must be at least XBASE_FLDNAME_LEN_READ+1 (=12)    */
/*      bytes long.                                                     */
=======

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
/*      pszFieldName must be at least XBASE_FLDNAME_LEN_READ+1 (=12)    */
/*      bytes long.                                                     */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======

=======
/*      pszFieldName must be at least XBASE_FLDNAME_LEN_READ+1 (=12)    */
/*      bytes long.                                                     */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
/************************************************************************/

DBFFieldType SHPAPI_CALL DBFGetFieldInfo(DBFHandle psDBF, int iField,
                                         char *pszFieldName, int *pnWidth,
                                         int *pnDecimals)
<<<<<<< HEAD
<<<<<<< HEAD
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
{
    if (iField < 0 || iField >= psDBF->nFields)
        return (FTInvalid);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (pnWidth != SHPLIB_NULLPTR)
        *pnWidth = psDBF->panFieldSize[iField];

    if (pnDecimals != SHPLIB_NULLPTR)
        *pnDecimals = psDBF->panFieldDecimals[iField];

    if (pszFieldName != SHPLIB_NULLPTR) {
        strncpy(pszFieldName,
                STATIC_CAST(char *, psDBF->pszHeader) +
                    iField * XBASE_FLDHDR_SZ,
                XBASE_FLDNAME_LEN_READ);
        pszFieldName[XBASE_FLDNAME_LEN_READ] = '\0';
        for (int i = XBASE_FLDNAME_LEN_READ - 1;
             i > 0 && pszFieldName[i] == ' '; i--)
=======
    if (pnWidth != NULL)
=======
    if (pnWidth != SHPLIB_NULLPTR)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        *pnWidth = psDBF->panFieldSize[iField];

    if (pnDecimals != SHPLIB_NULLPTR)
        *pnDecimals = psDBF->panFieldDecimals[iField];

<<<<<<< HEAD
    if (pszFieldName != NULL) {
        int i;

        strncpy(pszFieldName, (char *)psDBF->pszHeader + iField * 32, 11);
        pszFieldName[11] = '\0';
        for (i = 10; i > 0 && pszFieldName[i] == ' '; i--)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (pnWidth != NULL)
        *pnWidth = psDBF->panFieldSize[iField];

    if (pnDecimals != NULL)
        *pnDecimals = psDBF->panFieldDecimals[iField];

    if (pszFieldName != NULL) {
        int i;

        strncpy(pszFieldName, (char *)psDBF->pszHeader + iField * 32, 11);
        pszFieldName[11] = '\0';
        for (i = 10; i > 0 && pszFieldName[i] == ' '; i--)
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
    if (pnWidth != SHPLIB_NULLPTR)
        *pnWidth = psDBF->panFieldSize[iField];

    if (pnDecimals != SHPLIB_NULLPTR)
        *pnDecimals = psDBF->panFieldDecimals[iField];

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (pszFieldName != SHPLIB_NULLPTR) {
        strncpy(pszFieldName,
                STATIC_CAST(char *, psDBF->pszHeader) +
                    iField * XBASE_FLDHDR_SZ,
                XBASE_FLDNAME_LEN_READ);
        pszFieldName[XBASE_FLDNAME_LEN_READ] = '\0';
        for (int i = XBASE_FLDNAME_LEN_READ - 1;
             i > 0 && pszFieldName[i] == ' '; i--)
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            pszFieldName[i] = '\0';
    }

    if (psDBF->pachFieldType[iField] == 'L')
        return (FTLogical);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    else if (psDBF->pachFieldType[iField] == 'D')
        return (FTDate);

=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    else if (psDBF->pachFieldType[iField] == 'N' ||
             psDBF->pachFieldType[iField] == 'F') {
        if (psDBF->panFieldDecimals[iField] > 0) {
            /* || psDBF->panFieldSize[iField] >= 10 ) */ /* GDAL bug #809 */
            return (FTDouble);
<<<<<<< HEAD
        }
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    else if (psDBF->pachFieldType[iField] == 'D')
        return (FTDate);

<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    else if (psDBF->pachFieldType[iField] == 'N' ||
             psDBF->pachFieldType[iField] == 'F') {
        if (psDBF->panFieldDecimals[iField] > 0) {
            /* || psDBF->panFieldSize[iField] >= 10 ) */ /* GDAL bug #809 */
            return (FTDouble);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        }
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        }
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        else
            return (FTInteger);
    }
    else {
        return (FTString);
    }
}

/************************************************************************/
/*                         DBFWriteAttribute()                          */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*                                    */
/*    Write an attribute record to the file.                */
/************************************************************************/

static bool DBFWriteAttribute(DBFHandle psDBF, int hEntity, int iField,
                              void *pValue)
{
    /* -------------------------------------------------------------------- */
    /*    Is this a valid record?                        */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return false;

=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
/*                                                                      */
/*      Write an attribute record to the file.                          */

=======
/*                                    */
/*    Write an attribute record to the file.                */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
/************************************************************************/

static bool DBFWriteAttribute(DBFHandle psDBF, int hEntity, int iField,
                              void *pValue)
{
    /* -------------------------------------------------------------------- */
    /*    Is this a valid record?                        */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return false;

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
/*                                    */
/*    Write an attribute record to the file.                */
/************************************************************************/

static bool DBFWriteAttribute(DBFHandle psDBF, int hEntity, int iField,
                              void *pValue)
{
    /* -------------------------------------------------------------------- */
    /*    Is this a valid record?                        */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return false;

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psDBF->bNoHeader)
        DBFWriteHeader(psDBF);

    /* -------------------------------------------------------------------- */
    /*      Is this a brand new record?                                     */
    /* -------------------------------------------------------------------- */
    if (hEntity == psDBF->nRecords) {
        if (!DBFFlushRecord(psDBF))
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            return false;

        psDBF->nRecords++;
        for (int i = 0; i < psDBF->nRecordLength; i++)
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            return FALSE;

        psDBF->nRecords++;
        for (i = 0; i < psDBF->nRecordLength; i++)
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
            return false;

        psDBF->nRecords++;
        for (int i = 0; i < psDBF->nRecordLength; i++)
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            psDBF->pszCurrentRecord[i] = ' ';

        psDBF->nCurrentRecord = hEntity;
    }

    /* -------------------------------------------------------------------- */
    /*      Is this an existing record, but different than the last one     */
    /*      we accessed?                                                    */
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, hEntity))
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        return false;

    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);
=======
        return FALSE;

    pabyRec = (unsigned char *)psDBF->pszCurrentRecord;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        return FALSE;

    pabyRec = (unsigned char *)psDBF->pszCurrentRecord;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        return FALSE;

    pabyRec = (unsigned char *)psDBF->pszCurrentRecord;
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        return false;

    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    psDBF->bCurrentRecordModified = TRUE;
    psDBF->bUpdated = TRUE;

    /* -------------------------------------------------------------------- */
    /*      Translate NULL value to valid DBF file representation.          */
    /*                                                                      */
    /*      Contributed by Jim Matthews.                                    */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (pValue == SHPLIB_NULLPTR) {
        memset(pabyRec + psDBF->panFieldOffset[iField],
               DBFGetNullCharacter(psDBF->pachFieldType[iField]),
               psDBF->panFieldSize[iField]);
        return true;
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (pValue == NULL) {
        memset((char *)(pabyRec + psDBF->panFieldOffset[iField]),
               DBFGetNullCharacter(psDBF->pachFieldType[iField]),
               psDBF->panFieldSize[iField]);
        return TRUE;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    if (pValue == SHPLIB_NULLPTR) {
        memset(pabyRec + psDBF->panFieldOffset[iField],
               DBFGetNullCharacter(psDBF->pachFieldType[iField]),
               psDBF->panFieldSize[iField]);
        return true;
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    /* -------------------------------------------------------------------- */
    /*      Assign all the record fields.                                   */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    bool nRetResult = true;

    switch (psDBF->pachFieldType[iField]) {
    case 'D':
    case 'N':
    case 'F': {
        int nWidth = psDBF->panFieldSize[iField];

        char szSField[XBASE_FLD_MAX_WIDTH + 1];
        if (STATIC_CAST(int, sizeof(szSField)) - 2 < nWidth)
            nWidth = sizeof(szSField) - 2;

        char szFormat[20];
        snprintf(szFormat, sizeof(szFormat), "%%%d.%df", nWidth,
                 psDBF->panFieldDecimals[iField]);
        CPLsnprintf(szSField, sizeof(szSField), szFormat,
                    *STATIC_CAST(double *, pValue));
        szSField[sizeof(szSField) - 1] = '\0';
        if (STATIC_CAST(int, strlen(szSField)) > psDBF->panFieldSize[iField]) {
=======
=======
    bool nRetResult = true;

>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    switch (psDBF->pachFieldType[iField]) {
    case 'D':
    case 'N':
    case 'F': {
        int nWidth = psDBF->panFieldSize[iField];

        char szSField[XBASE_FLD_MAX_WIDTH + 1];
        if (STATIC_CAST(int, sizeof(szSField)) - 2 < nWidth)
            nWidth = sizeof(szSField) - 2;

<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    switch (psDBF->pachFieldType[iField]) {
    case 'D':
    case 'N':
    case 'F': {
        int nWidth = psDBF->panFieldSize[iField];

        if ((int)sizeof(szSField) - 2 < nWidth)
            nWidth = sizeof(szSField) - 2;

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        snprintf(szFormat, sizeof(szFormat), "%%%d.%df", nWidth,
                 psDBF->panFieldDecimals[iField]);
        snprintf(szSField, sizeof(szSField), szFormat, *((double *)pValue));
        if ((int)strlen(szSField) > psDBF->panFieldSize[iField]) {
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    bool nRetResult = true;

    switch (psDBF->pachFieldType[iField]) {
    case 'D':
    case 'N':
    case 'F': {
        int nWidth = psDBF->panFieldSize[iField];

        char szSField[XBASE_FLD_MAX_WIDTH + 1];
        if (STATIC_CAST(int, sizeof(szSField)) - 2 < nWidth)
            nWidth = sizeof(szSField) - 2;

        char szFormat[20];
        snprintf(szFormat, sizeof(szFormat), "%%%d.%df", nWidth,
                 psDBF->panFieldDecimals[iField]);
        CPLsnprintf(szSField, sizeof(szSField), szFormat,
                    *STATIC_CAST(double *, pValue));
        szSField[sizeof(szSField) - 1] = '\0';
        if (STATIC_CAST(int, strlen(szSField)) > psDBF->panFieldSize[iField]) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            szSField[psDBF->panFieldSize[iField]] = '\0';
            nRetResult = false;
        }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        memcpy(
            REINTERPRET_CAST(char *, pabyRec + psDBF->panFieldOffset[iField]),
            szSField, strlen(szSField));
=======
        strncpy((char *)(pabyRec + psDBF->panFieldOffset[iField]), szSField,
                strlen(szSField));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        strncpy((char *)(pabyRec + psDBF->panFieldOffset[iField]), szSField,
                strlen(szSField));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        char szFormat[20];
        snprintf(szFormat, sizeof(szFormat), "%%%d.%df", nWidth,
                 psDBF->panFieldDecimals[iField]);
        CPLsnprintf(szSField, sizeof(szSField), szFormat,
                    *STATIC_CAST(double *, pValue));
        szSField[sizeof(szSField) - 1] = '\0';
        if (STATIC_CAST(int, strlen(szSField)) > psDBF->panFieldSize[iField]) {
            szSField[psDBF->panFieldSize[iField]] = '\0';
            nRetResult = false;
        }
        memcpy(
            REINTERPRET_CAST(char *, pabyRec + psDBF->panFieldOffset[iField]),
            szSField, strlen(szSField));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        strncpy((char *)(pabyRec + psDBF->panFieldOffset[iField]), szSField,
                strlen(szSField));
=======
        memcpy(
            REINTERPRET_CAST(char *, pabyRec + psDBF->panFieldOffset[iField]),
            szSField, strlen(szSField));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        break;
    }

    case 'L':
        if (psDBF->panFieldSize[iField] >= 1 &&
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
            (*STATIC_CAST(char *, pValue) == 'F' ||
             *STATIC_CAST(char *, pValue) == 'T'))
            *(pabyRec + psDBF->panFieldOffset[iField]) =
                *STATIC_CAST(char *, pValue);
<<<<<<< HEAD
=======
            (*(char *)pValue == 'F' || *(char *)pValue == 'T'))
            *(pabyRec + psDBF->panFieldOffset[iField]) = *(char *)pValue;
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        break;

    default: {
        int j;
        if (STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue))) >
            psDBF->panFieldSize[iField]) {
            j = psDBF->panFieldSize[iField];
            nRetResult = false;
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            (*(char *)pValue == 'F' || *(char *)pValue == 'T'))
            *(pabyRec + psDBF->panFieldOffset[iField]) = *(char *)pValue;
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        break;

    default: {
        int j;
        if (STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue))) >
            psDBF->panFieldSize[iField]) {
            j = psDBF->panFieldSize[iField];
<<<<<<< HEAD
            nRetResult = FALSE;
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            nRetResult = false;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
            (*STATIC_CAST(char *, pValue) == 'F' ||
             *STATIC_CAST(char *, pValue) == 'T'))
            *(pabyRec + psDBF->panFieldOffset[iField]) =
                *STATIC_CAST(char *, pValue);
        break;

    default: {
        int j;
        if (STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue))) >
            psDBF->panFieldSize[iField]) {
            j = psDBF->panFieldSize[iField];
            nRetResult = false;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }
        else {
            memset(pabyRec + psDBF->panFieldOffset[iField], ' ',
                   psDBF->panFieldSize[iField]);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            j = STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue)));
=======
            j = (int)strlen((char *)pValue);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }

        strncpy(
            REINTERPRET_CAST(char *, pabyRec + psDBF->panFieldOffset[iField]),
            STATIC_CAST(const char *, pValue), j);
        break;
    }
    }

    return nRetResult;
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            j = (int)strlen((char *)pValue);
=======
            j = STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue)));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        }

        strncpy(
            REINTERPRET_CAST(char *, pabyRec + psDBF->panFieldOffset[iField]),
            STATIC_CAST(const char *, pValue), j);
        break;
    }
    }

<<<<<<< HEAD
    return (nRetResult);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    return nRetResult;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
            j = STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue)));
        }

        strncpy(
            REINTERPRET_CAST(char *, pabyRec + psDBF->panFieldOffset[iField]),
            STATIC_CAST(const char *, pValue), j);
        break;
    }
    }

    return nRetResult;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                     DBFWriteAttributeDirectly()                      */
/*                                                                      */
/*      Write an attribute record to the file, but without any          */
/*      reformatting based on type.  The provided buffer is written     */
/*      as is to the field position in the record.                      */
/************************************************************************/

int SHPAPI_CALL DBFWriteAttributeDirectly(DBFHandle psDBF, int hEntity,
                                          int iField, void *pValue)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /* -------------------------------------------------------------------- */
    /*    Is this a valid record?                        */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return (FALSE);

=======
    int i, j;
    unsigned char *pabyRec;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    /*    Is this a valid record?                        */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return (FALSE);

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    int i, j;
    unsigned char *pabyRec;

    /* -------------------------------------------------------------------- */
    /*      Is this a valid record?                                         */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return (FALSE);

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    /* -------------------------------------------------------------------- */
    /*    Is this a valid record?                        */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return (FALSE);

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psDBF->bNoHeader)
        DBFWriteHeader(psDBF);

    /* -------------------------------------------------------------------- */
    /*      Is this a brand new record?                                     */
    /* -------------------------------------------------------------------- */
    if (hEntity == psDBF->nRecords) {
        if (!DBFFlushRecord(psDBF))
            return FALSE;

        psDBF->nRecords++;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psDBF->nRecordLength; i++)
=======
        for (i = 0; i < psDBF->nRecordLength; i++)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = 0; i < psDBF->nRecordLength; i++)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        for (int i = 0; i < psDBF->nRecordLength; i++)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        for (i = 0; i < psDBF->nRecordLength; i++)
=======
        for (int i = 0; i < psDBF->nRecordLength; i++)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            psDBF->pszCurrentRecord[i] = ' ';

        psDBF->nCurrentRecord = hEntity;
    }

    /* -------------------------------------------------------------------- */
    /*      Is this an existing record, but different than the last one     */
    /*      we accessed?                                                    */
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, hEntity))
        return FALSE;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);
=======
    pabyRec = (unsigned char *)psDBF->pszCurrentRecord;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    pabyRec = (unsigned char *)psDBF->pszCurrentRecord;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    pabyRec = (unsigned char *)psDBF->pszCurrentRecord;
=======
    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    /* -------------------------------------------------------------------- */
    /*      Assign all the record fields.                                   */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    int j;
    if (STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue))) >
        psDBF->panFieldSize[iField])
=======
    if ((int)strlen((char *)pValue) > psDBF->panFieldSize[iField])
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if ((int)strlen((char *)pValue) > psDBF->panFieldSize[iField])
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    if ((int)strlen((char *)pValue) > psDBF->panFieldSize[iField])
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    int j;
    if (STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue))) >
        psDBF->panFieldSize[iField])
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        j = psDBF->panFieldSize[iField];
    else {
        memset(pabyRec + psDBF->panFieldOffset[iField], ' ',
               psDBF->panFieldSize[iField]);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        j = STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue)));
    }

    strncpy(REINTERPRET_CAST(char *, pabyRec + psDBF->panFieldOffset[iField]),
            STATIC_CAST(const char *, pValue), j);
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        j = (int)strlen((char *)pValue);
    }

    strncpy((char *)(pabyRec + psDBF->panFieldOffset[iField]), (char *)pValue,
            j);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        j = (int)strlen((char *)pValue);
    }

    strncpy((char *)(pabyRec + psDBF->panFieldOffset[iField]), (char *)pValue,
            j);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        j = STATIC_CAST(int, strlen(STATIC_CAST(char *, pValue)));
    }

    strncpy(REINTERPRET_CAST(char *, pabyRec + psDBF->panFieldOffset[iField]),
            STATIC_CAST(const char *, pValue), j);
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    psDBF->bCurrentRecordModified = TRUE;
    psDBF->bUpdated = TRUE;

    return (TRUE);
}

/************************************************************************/
/*                      DBFWriteDoubleAttribute()                       */
/*                                                                      */
/*      Write a double attribute.                                       */
/************************************************************************/

int SHPAPI_CALL DBFWriteDoubleAttribute(DBFHandle psDBF, int iRecord,
                                        int iField, double dValue)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    return (DBFWriteAttribute(psDBF, iRecord, iField,
                              STATIC_CAST(void *, &dValue)));
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)&dValue));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)&dValue));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField,
                              STATIC_CAST(void *, &dValue)));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)&dValue));
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField,
                              STATIC_CAST(void *, &dValue)));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                      DBFWriteIntegerAttribute()                      */
/*                                                                      */
/*      Write a integer attribute.                                      */
/************************************************************************/

int SHPAPI_CALL DBFWriteIntegerAttribute(DBFHandle psDBF, int iRecord,
                                         int iField, int nValue)
{
    double dValue = nValue;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    return (DBFWriteAttribute(psDBF, iRecord, iField,
                              STATIC_CAST(void *, &dValue)));
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)&dValue));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)&dValue));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField,
                              STATIC_CAST(void *, &dValue)));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)&dValue));
=======
    return (DBFWriteAttribute(psDBF, iRecord, iField,
                              STATIC_CAST(void *, &dValue)));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                      DBFWriteStringAttribute()                       */
/*                                                                      */
/*      Write a string attribute.                                       */
/************************************************************************/

int SHPAPI_CALL DBFWriteStringAttribute(DBFHandle psDBF, int iRecord,
                                        int iField, const char *pszValue)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

{
    return (
        DBFWriteAttribute(psDBF, iRecord, iField,
                          STATIC_CAST(void *, CONST_CAST(char *, pszValue))));
=======
{
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)pszValue));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)pszValue));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)pszValue));
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======

{
    return (
        DBFWriteAttribute(psDBF, iRecord, iField,
                          STATIC_CAST(void *, CONST_CAST(char *, pszValue))));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                      DBFWriteNULLAttribute()                         */
/*                                                                      */
/*      Write a string attribute.                                       */
/************************************************************************/

int SHPAPI_CALL DBFWriteNULLAttribute(DBFHandle psDBF, int iRecord, int iField)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

{
    return (DBFWriteAttribute(psDBF, iRecord, iField, SHPLIB_NULLPTR));
=======
{
    return (DBFWriteAttribute(psDBF, iRecord, iField, NULL));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return (DBFWriteAttribute(psDBF, iRecord, iField, NULL));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return (DBFWriteAttribute(psDBF, iRecord, iField, NULL));
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======

{
    return (DBFWriteAttribute(psDBF, iRecord, iField, SHPLIB_NULLPTR));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                      DBFWriteLogicalAttribute()                      */
/*                                                                      */
/*      Write a logical attribute.                                      */
/************************************************************************/

int SHPAPI_CALL DBFWriteLogicalAttribute(DBFHandle psDBF, int iRecord,
                                         int iField, const char lValue)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

{
    return (
        DBFWriteAttribute(psDBF, iRecord, iField,
                          STATIC_CAST(void *, CONST_CAST(char *, &lValue))));
=======
{
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)(&lValue)));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)(&lValue)));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
{
    return (DBFWriteAttribute(psDBF, iRecord, iField, (void *)(&lValue)));
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======

{
    return (
        DBFWriteAttribute(psDBF, iRecord, iField,
                          STATIC_CAST(void *, CONST_CAST(char *, &lValue))));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                         DBFWriteTuple()                              */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/*                                    */
/*    Write an attribute record to the file.                */
=======
/*                                                                      */
/*      Write an attribute record to the file.                          */

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
/*                                                                      */
/*      Write an attribute record to the file.                          */

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
/*                                    */
/*    Write an attribute record to the file.                */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
/*                                                                      */
/*      Write an attribute record to the file.                          */

=======
/*                                    */
/*    Write an attribute record to the file.                */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
/************************************************************************/

int SHPAPI_CALL DBFWriteTuple(DBFHandle psDBF, int hEntity, void *pRawTuple)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /* -------------------------------------------------------------------- */
    /*    Is this a valid record?                        */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return (FALSE);

=======
    int i;
    unsigned char *pabyRec;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    /*    Is this a valid record?                        */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return (FALSE);

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    int i;
    unsigned char *pabyRec;

    /* -------------------------------------------------------------------- */
    /*      Is this a valid record?                                         */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return (FALSE);

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    /* -------------------------------------------------------------------- */
    /*    Is this a valid record?                        */
    /* -------------------------------------------------------------------- */
    if (hEntity < 0 || hEntity > psDBF->nRecords)
        return (FALSE);

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (psDBF->bNoHeader)
        DBFWriteHeader(psDBF);

    /* -------------------------------------------------------------------- */
    /*      Is this a brand new record?                                     */
    /* -------------------------------------------------------------------- */
    if (hEntity == psDBF->nRecords) {
        if (!DBFFlushRecord(psDBF))
            return FALSE;

        psDBF->nRecords++;
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = 0; i < psDBF->nRecordLength; i++)
=======
        for (i = 0; i < psDBF->nRecordLength; i++)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = 0; i < psDBF->nRecordLength; i++)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        for (int i = 0; i < psDBF->nRecordLength; i++)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        for (i = 0; i < psDBF->nRecordLength; i++)
=======
        for (int i = 0; i < psDBF->nRecordLength; i++)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            psDBF->pszCurrentRecord[i] = ' ';

        psDBF->nCurrentRecord = hEntity;
    }

    /* -------------------------------------------------------------------- */
    /*      Is this an existing record, but different than the last one     */
    /*      we accessed?                                                    */
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, hEntity))
        return FALSE;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);
=======
    pabyRec = (unsigned char *)psDBF->pszCurrentRecord;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    pabyRec = (unsigned char *)psDBF->pszCurrentRecord;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    pabyRec = (unsigned char *)psDBF->pszCurrentRecord;
=======
    unsigned char *pabyRec =
        REINTERPRET_CAST(unsigned char *, psDBF->pszCurrentRecord);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    memcpy(pabyRec, pRawTuple, psDBF->nRecordLength);

    psDBF->bCurrentRecordModified = TRUE;
    psDBF->bUpdated = TRUE;

    return (TRUE);
}

/************************************************************************/
/*                            DBFReadTuple()                            */
/*                                                                      */
/*      Read a complete record.  Note that the result is only valid     */
/*      till the next record read for any reason.                       */
/************************************************************************/

const char SHPAPI_CALL1(*) DBFReadTuple(DBFHandle psDBF, int hEntity)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

{
    if (hEntity < 0 || hEntity >= psDBF->nRecords)
        return SHPLIB_NULLPTR;

    if (!DBFLoadRecord(psDBF, hEntity))
        return SHPLIB_NULLPTR;

    return STATIC_CAST(const char *, psDBF->pszCurrentRecord);
=======
=======

>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
{
    if (hEntity < 0 || hEntity >= psDBF->nRecords)
        return SHPLIB_NULLPTR;

    if (!DBFLoadRecord(psDBF, hEntity))
        return SHPLIB_NULLPTR;

<<<<<<< HEAD
    return (const char *)psDBF->pszCurrentRecord;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
{
    if (hEntity < 0 || hEntity >= psDBF->nRecords)
        return (NULL);

    if (!DBFLoadRecord(psDBF, hEntity))
        return NULL;

    return (const char *)psDBF->pszCurrentRecord;
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    return STATIC_CAST(const char *, psDBF->pszCurrentRecord);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======

{
    if (hEntity < 0 || hEntity >= psDBF->nRecords)
        return SHPLIB_NULLPTR;

    if (!DBFLoadRecord(psDBF, hEntity))
        return SHPLIB_NULLPTR;

    return STATIC_CAST(const char *, psDBF->pszCurrentRecord);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
/*                          DBFCloneEmpty()                              */
/*                                                                      */
/*      Read one of the attribute fields of a record.                   */
/************************************************************************/

DBFHandle SHPAPI_CALL DBFCloneEmpty(DBFHandle psDBF, const char *pszFilename)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    DBFHandle newDBF = DBFCreateEx(pszFilename, psDBF->pszCodePage);
    if (newDBF == SHPLIB_NULLPTR)
        return SHPLIB_NULLPTR;

    newDBF->nFields = psDBF->nFields;
    newDBF->nRecordLength = psDBF->nRecordLength;
    newDBF->nHeaderLength = psDBF->nHeaderLength;

    if (psDBF->pszHeader) {
        newDBF->pszHeader =
            STATIC_CAST(char *, malloc(XBASE_FLDHDR_SZ * psDBF->nFields));
        memcpy(newDBF->pszHeader, psDBF->pszHeader,
               XBASE_FLDHDR_SZ * psDBF->nFields);
    }

    newDBF->panFieldOffset =
        STATIC_CAST(int *, malloc(sizeof(int) * psDBF->nFields));
    memcpy(newDBF->panFieldOffset, psDBF->panFieldOffset,
           sizeof(int) * psDBF->nFields);
    newDBF->panFieldSize =
        STATIC_CAST(int *, malloc(sizeof(int) * psDBF->nFields));
    memcpy(newDBF->panFieldSize, psDBF->panFieldSize,
           sizeof(int) * psDBF->nFields);
    newDBF->panFieldDecimals =
        STATIC_CAST(int *, malloc(sizeof(int) * psDBF->nFields));
    memcpy(newDBF->panFieldDecimals, psDBF->panFieldDecimals,
           sizeof(int) * psDBF->nFields);
    newDBF->pachFieldType =
        STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nFields));
    memcpy(newDBF->pachFieldType, psDBF->pachFieldType,
           sizeof(char) * psDBF->nFields);

    newDBF->bNoHeader = TRUE;
    newDBF->bUpdated = TRUE;
    newDBF->bWriteEndOfFileChar = psDBF->bWriteEndOfFileChar;

    DBFWriteHeader(newDBF);
    DBFClose(newDBF);

    newDBF = DBFOpen(pszFilename, "rb+");
    newDBF->bWriteEndOfFileChar = psDBF->bWriteEndOfFileChar;

=======
    DBFHandle newDBF;

    newDBF = DBFCreateEx(pszFilename, psDBF->pszCodePage);
    if (newDBF == NULL)
        return (NULL);
=======
    DBFHandle newDBF = DBFCreateEx(pszFilename, psDBF->pszCodePage);
    if (newDBF == SHPLIB_NULLPTR)
        return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    newDBF->nFields = psDBF->nFields;
    newDBF->nRecordLength = psDBF->nRecordLength;
    newDBF->nHeaderLength = psDBF->nHeaderLength;

    if (psDBF->pszHeader) {
        newDBF->pszHeader =
            STATIC_CAST(char *, malloc(XBASE_FLDHDR_SZ * psDBF->nFields));
        memcpy(newDBF->pszHeader, psDBF->pszHeader,
               XBASE_FLDHDR_SZ * psDBF->nFields);
    }

    newDBF->panFieldOffset =
        STATIC_CAST(int *, malloc(sizeof(int) * psDBF->nFields));
    memcpy(newDBF->panFieldOffset, psDBF->panFieldOffset,
           sizeof(int) * psDBF->nFields);
    newDBF->panFieldSize =
        STATIC_CAST(int *, malloc(sizeof(int) * psDBF->nFields));
    memcpy(newDBF->panFieldSize, psDBF->panFieldSize,
           sizeof(int) * psDBF->nFields);
    newDBF->panFieldDecimals =
        STATIC_CAST(int *, malloc(sizeof(int) * psDBF->nFields));
    memcpy(newDBF->panFieldDecimals, psDBF->panFieldDecimals,
           sizeof(int) * psDBF->nFields);
    newDBF->pachFieldType =
        STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nFields));
    memcpy(newDBF->pachFieldType, psDBF->pachFieldType,
           sizeof(char) * psDBF->nFields);

    newDBF->bNoHeader = TRUE;
    newDBF->bUpdated = TRUE;
    newDBF->bWriteEndOfFileChar = psDBF->bWriteEndOfFileChar;

    DBFWriteHeader(newDBF);
    DBFClose(newDBF);

    newDBF = DBFOpen(pszFilename, "rb+");
    newDBF->bWriteEndOfFileChar = psDBF->bWriteEndOfFileChar;

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    DBFHandle newDBF;

    newDBF = DBFCreateEx(pszFilename, psDBF->pszCodePage);
    if (newDBF == NULL)
        return (NULL);

    newDBF->nFields = psDBF->nFields;
    newDBF->nRecordLength = psDBF->nRecordLength;
    newDBF->nHeaderLength = psDBF->nHeaderLength;

    if (psDBF->pszHeader) {
        newDBF->pszHeader = (char *)malloc(XBASE_FLDHDR_SZ * psDBF->nFields);
        memcpy(newDBF->pszHeader, psDBF->pszHeader,
               XBASE_FLDHDR_SZ * psDBF->nFields);
    }

    newDBF->panFieldOffset = (int *)malloc(sizeof(int) * psDBF->nFields);
    memcpy(newDBF->panFieldOffset, psDBF->panFieldOffset,
           sizeof(int) * psDBF->nFields);
    newDBF->panFieldSize = (int *)malloc(sizeof(int) * psDBF->nFields);
    memcpy(newDBF->panFieldSize, psDBF->panFieldSize,
           sizeof(int) * psDBF->nFields);
    newDBF->panFieldDecimals = (int *)malloc(sizeof(int) * psDBF->nFields);
    memcpy(newDBF->panFieldDecimals, psDBF->panFieldDecimals,
           sizeof(int) * psDBF->nFields);
    newDBF->pachFieldType = (char *)malloc(sizeof(char) * psDBF->nFields);
    memcpy(newDBF->pachFieldType, psDBF->pachFieldType,
           sizeof(char) * psDBF->nFields);

    newDBF->bNoHeader = TRUE;
    newDBF->bUpdated = TRUE;

    DBFWriteHeader(newDBF);
    DBFClose(newDBF);

    newDBF = DBFOpen(pszFilename, "rb+");

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
    DBFHandle newDBF = DBFCreateEx(pszFilename, psDBF->pszCodePage);
    if (newDBF == SHPLIB_NULLPTR)
        return SHPLIB_NULLPTR;

    newDBF->nFields = psDBF->nFields;
    newDBF->nRecordLength = psDBF->nRecordLength;
    newDBF->nHeaderLength = psDBF->nHeaderLength;

    if (psDBF->pszHeader) {
        newDBF->pszHeader =
            STATIC_CAST(char *, malloc(XBASE_FLDHDR_SZ * psDBF->nFields));
        memcpy(newDBF->pszHeader, psDBF->pszHeader,
               XBASE_FLDHDR_SZ * psDBF->nFields);
    }

    newDBF->panFieldOffset =
        STATIC_CAST(int *, malloc(sizeof(int) * psDBF->nFields));
    memcpy(newDBF->panFieldOffset, psDBF->panFieldOffset,
           sizeof(int) * psDBF->nFields);
    newDBF->panFieldSize =
        STATIC_CAST(int *, malloc(sizeof(int) * psDBF->nFields));
    memcpy(newDBF->panFieldSize, psDBF->panFieldSize,
           sizeof(int) * psDBF->nFields);
    newDBF->panFieldDecimals =
        STATIC_CAST(int *, malloc(sizeof(int) * psDBF->nFields));
    memcpy(newDBF->panFieldDecimals, psDBF->panFieldDecimals,
           sizeof(int) * psDBF->nFields);
    newDBF->pachFieldType =
        STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nFields));
    memcpy(newDBF->pachFieldType, psDBF->pachFieldType,
           sizeof(char) * psDBF->nFields);

    newDBF->bNoHeader = TRUE;
    newDBF->bUpdated = TRUE;
    newDBF->bWriteEndOfFileChar = psDBF->bWriteEndOfFileChar;

    DBFWriteHeader(newDBF);
    DBFClose(newDBF);

    newDBF = DBFOpen(pszFilename, "rb+");
    newDBF->bWriteEndOfFileChar = psDBF->bWriteEndOfFileChar;

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    return (newDBF);
}

/************************************************************************/
/*                       DBFGetNativeFieldType()                        */
/*                                                                      */
/*      Return the DBase field type for the specified field.            */
/*                                                                      */
/*      Value can be one of: 'C' (String), 'D' (Date), 'F' (Float),     */
/*                           'N' (Numeric, with or without decimal),    */
/*                           'L' (Logical),                             */
/*                           'M' (Memo: 10 digits .DBT block ptr)       */
/************************************************************************/

char SHPAPI_CALL DBFGetNativeFieldType(DBFHandle psDBF, int iField)
<<<<<<< HEAD
<<<<<<< HEAD
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
{
    if (iField >= 0 && iField < psDBF->nFields)
        return psDBF->pachFieldType[iField];

    return ' ';
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
<<<<<<< HEAD
/*                            str_to_upper()                            */

/************************************************************************/

static void str_to_upper(char *string)
{
    int len;
    int i = -1;

    len = (int)strlen(string);

    while (++i < len)
        if (isalpha(string[i]) && islower(string[i]))
            string[i] = (char)toupper((int)string[i]);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
}

/************************************************************************/
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
/*                          DBFGetFieldIndex()                          */
/*                                                                      */
/*      Get the index number for a field in a .dbf file.                */
/*                                                                      */
/*      Contributed by Jim Matthews.                                    */
/************************************************************************/

int SHPAPI_CALL DBFGetFieldIndex(DBFHandle psDBF, const char *pszFieldName)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    char name[XBASE_FLDNAME_LEN_READ + 1];

    for (int i = 0; i < DBFGetFieldCount(psDBF); i++) {
        DBFGetFieldInfo(psDBF, i, name, SHPLIB_NULLPTR, SHPLIB_NULLPTR);
        if (!STRCASECMP(pszFieldName, name))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    char name[12], name1[12], name2[12];
    int i;

=======
    char name[12], name1[12], name2[12];
    int i;

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    strncpy(name1, pszFieldName, 11);
    name1[11] = '\0';
    str_to_upper(name1);

    for (i = 0; i < DBFGetFieldCount(psDBF); i++) {
        DBFGetFieldInfo(psDBF, i, name, NULL, NULL);
        strncpy(name2, name, 11);
        str_to_upper(name2);

        if (!strncmp(name1, name2, 10))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
    char name[XBASE_FLDNAME_LEN_READ + 1];

    for (int i = 0; i < DBFGetFieldCount(psDBF); i++) {
        DBFGetFieldInfo(psDBF, i, name, SHPLIB_NULLPTR, SHPLIB_NULLPTR);
        if (!STRCASECMP(pszFieldName, name))
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            return (i);
    }
    return (-1);
}

/************************************************************************/
/*                         DBFIsRecordDeleted()                         */
/*                                                                      */
/*      Returns TRUE if the indicated record is deleted, otherwise      */
/*      it returns FALSE.                                               */
/************************************************************************/

int SHPAPI_CALL DBFIsRecordDeleted(DBFHandle psDBF, int iShape)
{
    /* -------------------------------------------------------------------- */
    /*      Verify selection.                                               */
    /* -------------------------------------------------------------------- */
    if (iShape < 0 || iShape >= psDBF->nRecords)
        return TRUE;

    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    /*    Have we read the record?                    */
=======
    /*      Have we read the record?                                        */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    /*      Have we read the record?                                        */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    /*    Have we read the record?                    */
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    /*      Have we read the record?                                        */
=======
    /*    Have we read the record?                    */
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, iShape))
        return FALSE;

    /* -------------------------------------------------------------------- */
    /*      '*' means deleted.                                              */
    /* -------------------------------------------------------------------- */
    return psDBF->pszCurrentRecord[0] == '*';
}

/************************************************************************/
/*                        DBFMarkRecordDeleted()                        */
/************************************************************************/

int SHPAPI_CALL DBFMarkRecordDeleted(DBFHandle psDBF, int iShape,
                                     int bIsDeleted)
{
<<<<<<< HEAD
<<<<<<< HEAD
=======
    /* -------------------------------------------------------------------- */
    /*      Verify selection.                                               */
    /* -------------------------------------------------------------------- */
    if (iShape < 0 || iShape >= psDBF->nRecords)
        return FALSE;

    /* -------------------------------------------------------------------- */
    /*      Is this an existing record, but different than the last one     */
    /*      we accessed?                                                    */
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, iShape))
        return FALSE;

    /* -------------------------------------------------------------------- */
    /*      Assign value, marking record as dirty if it changes.            */
    /* -------------------------------------------------------------------- */
    char chNewFlag;
<<<<<<< HEAD

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    /* -------------------------------------------------------------------- */
    /*      Verify selection.                                               */
    /* -------------------------------------------------------------------- */
    if (iShape < 0 || iShape >= psDBF->nRecords)
        return FALSE;

    /* -------------------------------------------------------------------- */
    /*      Is this an existing record, but different than the last one     */
    /*      we accessed?                                                    */
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, iShape))
        return FALSE;

    /* -------------------------------------------------------------------- */
    /*      Assign value, marking record as dirty if it changes.            */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
    char chNewFlag;
<<<<<<< HEAD
=======

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    /* -------------------------------------------------------------------- */
    /*      Verify selection.                                               */
    /* -------------------------------------------------------------------- */
    if (iShape < 0 || iShape >= psDBF->nRecords)
        return FALSE;

    /* -------------------------------------------------------------------- */
    /*      Is this an existing record, but different than the last one     */
    /*      we accessed?                                                    */
    /* -------------------------------------------------------------------- */
    if (!DBFLoadRecord(psDBF, iShape))
        return FALSE;

    /* -------------------------------------------------------------------- */
    /*      Assign value, marking record as dirty if it changes.            */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    char chNewFlag;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (bIsDeleted)
        chNewFlag = '*';
    else
        chNewFlag = ' ';

    if (psDBF->pszCurrentRecord[0] != chNewFlag) {
        psDBF->bCurrentRecordModified = TRUE;
        psDBF->bUpdated = TRUE;
        psDBF->pszCurrentRecord[0] = chNewFlag;
    }

    return TRUE;
}

/************************************************************************/
/*                            DBFGetCodePage                            */
/************************************************************************/

const char SHPAPI_CALL1(*) DBFGetCodePage(DBFHandle psDBF)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (psDBF == SHPLIB_NULLPTR)
        return SHPLIB_NULLPTR;
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if (psDBF == NULL)
        return NULL;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (psDBF == SHPLIB_NULLPTR)
        return SHPLIB_NULLPTR;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (psDBF == NULL)
        return NULL;
=======
    if (psDBF == SHPLIB_NULLPTR)
        return SHPLIB_NULLPTR;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    return psDBF->pszCodePage;
}

/************************************************************************/
/*                          DBFDeleteField()                            */
/*                                                                      */
/*      Remove a field from a .dbf file                                 */
/************************************************************************/

int SHPAPI_CALL DBFDeleteField(DBFHandle psDBF, int iField)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    int nOldRecordLength, nOldHeaderLength;
    int nDeletedFieldOffset, nDeletedFieldSize;
    SAOffset nRecordOffset;
    char *pszRecord;
    int i, iRecord;

<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (iField < 0 || iField >= psDBF->nFields)
        return FALSE;

    /* make sure that everything is written in .dbf */
    if (!DBFFlushRecord(psDBF))
        return FALSE;

    /* get information about field to be deleted */
    int nOldRecordLength = psDBF->nRecordLength;
    int nOldHeaderLength = psDBF->nHeaderLength;
    int nDeletedFieldOffset = psDBF->panFieldOffset[iField];
    int nDeletedFieldSize = psDBF->panFieldSize[iField];

    /* update fields info */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    for (int i = iField + 1; i < psDBF->nFields; i++) {
=======
    for (i = iField + 1; i < psDBF->nFields; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    for (i = iField + 1; i < psDBF->nFields; i++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    for (int i = iField + 1; i < psDBF->nFields; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    for (i = iField + 1; i < psDBF->nFields; i++) {
=======
    for (int i = iField + 1; i < psDBF->nFields; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        psDBF->panFieldOffset[i - 1] =
            psDBF->panFieldOffset[i] - nDeletedFieldSize;
        psDBF->panFieldSize[i - 1] = psDBF->panFieldSize[i];
        psDBF->panFieldDecimals[i - 1] = psDBF->panFieldDecimals[i];
        psDBF->pachFieldType[i - 1] = psDBF->pachFieldType[i];
    }

    /* resize fields arrays */
    psDBF->nFields--;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    psDBF->panFieldOffset = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields));

    psDBF->panFieldSize = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldSize, sizeof(int) * psDBF->nFields));

    psDBF->panFieldDecimals =
        STATIC_CAST(int *, SfRealloc(psDBF->panFieldDecimals,
                                     sizeof(int) * psDBF->nFields));

    psDBF->pachFieldType = STATIC_CAST(
        char *, SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields));
=======
    psDBF->panFieldOffset =
        (int *)SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields);
=======
    psDBF->panFieldOffset = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    psDBF->panFieldSize = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldSize, sizeof(int) * psDBF->nFields));

    psDBF->panFieldDecimals =
        STATIC_CAST(int *, SfRealloc(psDBF->panFieldDecimals,
                                     sizeof(int) * psDBF->nFields));

<<<<<<< HEAD
    psDBF->pachFieldType =
        (char *)SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    psDBF->panFieldOffset =
        (int *)SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields);

    psDBF->panFieldSize =
        (int *)SfRealloc(psDBF->panFieldSize, sizeof(int) * psDBF->nFields);

    psDBF->panFieldDecimals =
        (int *)SfRealloc(psDBF->panFieldDecimals, sizeof(int) * psDBF->nFields);

    psDBF->pachFieldType =
        (char *)SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    psDBF->pachFieldType = STATIC_CAST(
        char *, SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    psDBF->panFieldOffset = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields));

    psDBF->panFieldSize = STATIC_CAST(
        int *, SfRealloc(psDBF->panFieldSize, sizeof(int) * psDBF->nFields));

    psDBF->panFieldDecimals =
        STATIC_CAST(int *, SfRealloc(psDBF->panFieldDecimals,
                                     sizeof(int) * psDBF->nFields));

    psDBF->pachFieldType = STATIC_CAST(
        char *, SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    /* update header information */
    psDBF->nHeaderLength -= XBASE_FLDHDR_SZ;
    psDBF->nRecordLength -= nDeletedFieldSize;

    /* overwrite field information in header */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    memmove(psDBF->pszHeader + iField * XBASE_FLDHDR_SZ,
            psDBF->pszHeader + (iField + 1) * XBASE_FLDHDR_SZ,
            sizeof(char) * (psDBF->nFields - iField) * XBASE_FLDHDR_SZ);

    psDBF->pszHeader = STATIC_CAST(
        char *, SfRealloc(psDBF->pszHeader, psDBF->nFields * XBASE_FLDHDR_SZ));

    /* update size of current record appropriately */
    psDBF->pszCurrentRecord = STATIC_CAST(
        char *, SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength));
=======
    memmove(psDBF->pszHeader + iField * 32,
            psDBF->pszHeader + (iField + 1) * 32,
            sizeof(char) * (psDBF->nFields - iField) * 32);
=======
    memmove(psDBF->pszHeader + iField * XBASE_FLDHDR_SZ,
            psDBF->pszHeader + (iField + 1) * XBASE_FLDHDR_SZ,
            sizeof(char) * (psDBF->nFields - iField) * XBASE_FLDHDR_SZ);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    psDBF->pszHeader = STATIC_CAST(
        char *, SfRealloc(psDBF->pszHeader, psDBF->nFields * XBASE_FLDHDR_SZ));

    /* update size of current record appropriately */
<<<<<<< HEAD
    psDBF->pszCurrentRecord =
        (char *)SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    memmove(psDBF->pszHeader + iField * 32,
            psDBF->pszHeader + (iField + 1) * 32,
            sizeof(char) * (psDBF->nFields - iField) * 32);

    psDBF->pszHeader = (char *)SfRealloc(psDBF->pszHeader, psDBF->nFields * 32);

    /* update size of current record appropriately */
    psDBF->pszCurrentRecord =
        (char *)SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength);
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    psDBF->pszCurrentRecord = STATIC_CAST(
        char *, SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    memmove(psDBF->pszHeader + iField * XBASE_FLDHDR_SZ,
            psDBF->pszHeader + (iField + 1) * XBASE_FLDHDR_SZ,
            sizeof(char) * (psDBF->nFields - iField) * XBASE_FLDHDR_SZ);

    psDBF->pszHeader = STATIC_CAST(
        char *, SfRealloc(psDBF->pszHeader, psDBF->nFields * XBASE_FLDHDR_SZ));

    /* update size of current record appropriately */
    psDBF->pszCurrentRecord = STATIC_CAST(
        char *, SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    /* we're done if we're dealing with not yet created .dbf */
    if (psDBF->bNoHeader && psDBF->nRecords == 0)
        return TRUE;

    /* force update of header with new header and record length */
    psDBF->bNoHeader = TRUE;
    DBFUpdateHeader(psDBF);

    /* alloc record */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    char *pszRecord =
        STATIC_CAST(char *, malloc(sizeof(char) * nOldRecordLength));

    /* shift records to their new positions */
    for (int iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
        SAOffset nRecordOffset =
            nOldRecordLength * STATIC_CAST(SAOffset, iRecord) +
            nOldHeaderLength;

        /* load record */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1, psDBF->fp) !=
            1) {
            free(pszRecord);
            return FALSE;
        }

        nRecordOffset = psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                        psDBF->nHeaderLength;
=======
    pszRecord = (char *)malloc(sizeof(char) * nOldRecordLength);
=======
    char *pszRecord =
        STATIC_CAST(char *, malloc(sizeof(char) * nOldRecordLength));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    /* shift records to their new positions */
    for (int iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
        SAOffset nRecordOffset =
            nOldRecordLength * STATIC_CAST(SAOffset, iRecord) +
            nOldHeaderLength;

        /* load record */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    pszRecord = (char *)malloc(sizeof(char) * nOldRecordLength);

    /* shift records to their new positions */
    for (iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
        nRecordOffset = nOldRecordLength * (SAOffset)iRecord + nOldHeaderLength;

        /* load record */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1, psDBF->fp);

        nRecordOffset =
            psDBF->nRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
    char *pszRecord =
        STATIC_CAST(char *, malloc(sizeof(char) * nOldRecordLength));

    /* shift records to their new positions */
    for (int iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
        SAOffset nRecordOffset =
            nOldRecordLength * STATIC_CAST(SAOffset, iRecord) +
            nOldHeaderLength;

        /* load record */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1, psDBF->fp) !=
            1) {
            free(pszRecord);
            return FALSE;
        }

        nRecordOffset = psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                        psDBF->nHeaderLength;
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

        /* move record in two steps */
        psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
        psDBF->sHooks.FWrite(pszRecord, nDeletedFieldOffset, 1, psDBF->fp);
        psDBF->sHooks.FWrite(
            pszRecord + nDeletedFieldOffset + nDeletedFieldSize,
            nOldRecordLength - nDeletedFieldOffset - nDeletedFieldSize, 1,
            psDBF->fp);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    }

    if (psDBF->bWriteEndOfFileChar) {
        char ch = END_OF_FILE_CHARACTER;
        SAOffset nEOFOffset =
            psDBF->nRecordLength * STATIC_CAST(SAOffset, psDBF->nRecords) +
            psDBF->nHeaderLength;

        psDBF->sHooks.FSeek(psDBF->fp, nEOFOffset, 0);
        psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    if (psDBF->bWriteEndOfFileChar) {
        char ch = END_OF_FILE_CHARACTER;
        SAOffset nEOFOffset =
            psDBF->nRecordLength * STATIC_CAST(SAOffset, psDBF->nRecords) +
            psDBF->nHeaderLength;

        psDBF->sHooks.FSeek(psDBF->fp, nEOFOffset, 0);
        psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
<<<<<<< HEAD
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    /* TODO: truncate file */

    /* free record */
    free(pszRecord);

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;
    psDBF->bUpdated = TRUE;

    return TRUE;
}

/************************************************************************/
/*                          DBFReorderFields()                          */
/*                                                                      */
/*      Reorder the fields of a .dbf file                               */
/*                                                                      */
/* panMap must be exactly psDBF->nFields long and be a permutation      */
/* of [0, psDBF->nFields-1]. This assumption will not be asserted in the*/
/* code of DBFReorderFields.                                            */
/************************************************************************/

int SHPAPI_CALL DBFReorderFields(DBFHandle psDBF, int *panMap)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    SAOffset nRecordOffset;
    int i, iRecord;
    int *panFieldOffsetNew;
    int *panFieldSizeNew;
    int *panFieldDecimalsNew;
    char *pachFieldTypeNew;
    char *pszHeaderNew;
    char *pszRecord;
    char *pszRecordNew;

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
    if (psDBF->nFields == 0)
        return TRUE;

    /* make sure that everything is written in .dbf */
    if (!DBFFlushRecord(psDBF))
        return FALSE;

    /* a simple malloc() would be enough, but calloc() helps clang static
     * analyzer */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    int *panFieldOffsetNew =
        STATIC_CAST(int *, calloc(sizeof(int), psDBF->nFields));
    int *panFieldSizeNew =
        STATIC_CAST(int *, calloc(sizeof(int), psDBF->nFields));
    int *panFieldDecimalsNew =
        STATIC_CAST(int *, calloc(sizeof(int), psDBF->nFields));
    char *pachFieldTypeNew =
        STATIC_CAST(char *, calloc(sizeof(char), psDBF->nFields));
    char *pszHeaderNew = STATIC_CAST(
        char *, malloc(sizeof(char) * XBASE_FLDHDR_SZ * psDBF->nFields));
<<<<<<< HEAD

    /* shuffle fields definitions */
    for (int i = 0; i < psDBF->nFields; i++) {
        panFieldSizeNew[i] = psDBF->panFieldSize[panMap[i]];
        panFieldDecimalsNew[i] = psDBF->panFieldDecimals[panMap[i]];
        pachFieldTypeNew[i] = psDBF->pachFieldType[panMap[i]];
        memcpy(pszHeaderNew + i * XBASE_FLDHDR_SZ,
               psDBF->pszHeader + panMap[i] * XBASE_FLDHDR_SZ, XBASE_FLDHDR_SZ);
    }
    panFieldOffsetNew[0] = 1;
    for (int i = 1; i < psDBF->nFields; i++) {
=======
    panFieldOffsetNew = (int *)calloc(sizeof(int), psDBF->nFields);
    panFieldSizeNew = (int *)malloc(sizeof(int) * psDBF->nFields);
    panFieldDecimalsNew = (int *)malloc(sizeof(int) * psDBF->nFields);
    pachFieldTypeNew = (char *)malloc(sizeof(char) * psDBF->nFields);
    pszHeaderNew = (char *)malloc(sizeof(char) * 32 * psDBF->nFields);
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    /* shuffle fields definitions */
    for (int i = 0; i < psDBF->nFields; i++) {
        panFieldSizeNew[i] = psDBF->panFieldSize[panMap[i]];
        panFieldDecimalsNew[i] = psDBF->panFieldDecimals[panMap[i]];
        pachFieldTypeNew[i] = psDBF->pachFieldType[panMap[i]];
        memcpy(pszHeaderNew + i * XBASE_FLDHDR_SZ,
               psDBF->pszHeader + panMap[i] * XBASE_FLDHDR_SZ, XBASE_FLDHDR_SZ);
    }
    panFieldOffsetNew[0] = 1;
<<<<<<< HEAD
    for (i = 1; i < psDBF->nFields; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    panFieldOffsetNew = (int *)calloc(sizeof(int), psDBF->nFields);
    panFieldSizeNew = (int *)malloc(sizeof(int) * psDBF->nFields);
    panFieldDecimalsNew = (int *)malloc(sizeof(int) * psDBF->nFields);
    pachFieldTypeNew = (char *)malloc(sizeof(char) * psDBF->nFields);
    pszHeaderNew = (char *)malloc(sizeof(char) * 32 * psDBF->nFields);

    /* shuffle fields definitions */
    for (i = 0; i < psDBF->nFields; i++) {
        panFieldSizeNew[i] = psDBF->panFieldSize[panMap[i]];
        panFieldDecimalsNew[i] = psDBF->panFieldDecimals[panMap[i]];
        pachFieldTypeNew[i] = psDBF->pachFieldType[panMap[i]];
        memcpy(pszHeaderNew + i * 32, psDBF->pszHeader + panMap[i] * 32, 32);
    }
    panFieldOffsetNew[0] = 1;
    for (i = 1; i < psDBF->nFields; i++) {
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    for (int i = 1; i < psDBF->nFields; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    int *panFieldOffsetNew =
        STATIC_CAST(int *, calloc(sizeof(int), psDBF->nFields));
    int *panFieldSizeNew =
        STATIC_CAST(int *, calloc(sizeof(int), psDBF->nFields));
    int *panFieldDecimalsNew =
        STATIC_CAST(int *, calloc(sizeof(int), psDBF->nFields));
    char *pachFieldTypeNew =
        STATIC_CAST(char *, calloc(sizeof(char), psDBF->nFields));
    char *pszHeaderNew = STATIC_CAST(
        char *, malloc(sizeof(char) * XBASE_FLDHDR_SZ * psDBF->nFields));

    /* shuffle fields definitions */
    for (int i = 0; i < psDBF->nFields; i++) {
        panFieldSizeNew[i] = psDBF->panFieldSize[panMap[i]];
        panFieldDecimalsNew[i] = psDBF->panFieldDecimals[panMap[i]];
        pachFieldTypeNew[i] = psDBF->pachFieldType[panMap[i]];
        memcpy(pszHeaderNew + i * XBASE_FLDHDR_SZ,
               psDBF->pszHeader + panMap[i] * XBASE_FLDHDR_SZ, XBASE_FLDHDR_SZ);
    }
    panFieldOffsetNew[0] = 1;
    for (int i = 1; i < psDBF->nFields; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        panFieldOffsetNew[i] =
            panFieldOffsetNew[i - 1] + panFieldSizeNew[i - 1];
    }

    free(psDBF->pszHeader);
    psDBF->pszHeader = pszHeaderNew;

    bool errorAbort = false;

    /* we're done if we're dealing with not yet created .dbf */
    if (!(psDBF->bNoHeader && psDBF->nRecords == 0)) {
        /* force update of header with new header and record length */
        psDBF->bNoHeader = TRUE;
        DBFUpdateHeader(psDBF);

        /* alloc record */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        char *pszRecord =
            STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nRecordLength));
        char *pszRecordNew =
            STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nRecordLength));
<<<<<<< HEAD

        /* shuffle fields in records */
        for (int iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
            const SAOffset nRecordOffset =
                psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            if (psDBF->sHooks.FRead(pszRecord, psDBF->nRecordLength, 1,
                                    psDBF->fp) != 1) {
                errorAbort = true;
                break;
            }

            pszRecordNew[0] = pszRecord[0];

            for (int i = 0; i < psDBF->nFields; i++) {
=======
        pszRecord = (char *)malloc(sizeof(char) * psDBF->nRecordLength);
        pszRecordNew = (char *)malloc(sizeof(char) * psDBF->nRecordLength);
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

        /* shuffle fields in records */
        for (int iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
            const SAOffset nRecordOffset =
                psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            if (psDBF->sHooks.FRead(pszRecord, psDBF->nRecordLength, 1,
                                    psDBF->fp) != 1) {
                errorAbort = true;
                break;
            }

            pszRecordNew[0] = pszRecord[0];

<<<<<<< HEAD
            for (i = 0; i < psDBF->nFields; i++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        pszRecord = (char *)malloc(sizeof(char) * psDBF->nRecordLength);
        pszRecordNew = (char *)malloc(sizeof(char) * psDBF->nRecordLength);

        /* shuffle fields in records */
        for (iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
            nRecordOffset =
                psDBF->nRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            psDBF->sHooks.FRead(pszRecord, psDBF->nRecordLength, 1, psDBF->fp);

            pszRecordNew[0] = pszRecord[0];

            for (i = 0; i < psDBF->nFields; i++) {
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (int i = 0; i < psDBF->nFields; i++) {
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        char *pszRecord =
            STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nRecordLength));
        char *pszRecordNew =
            STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nRecordLength));

        /* shuffle fields in records */
        for (int iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
            const SAOffset nRecordOffset =
                psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            if (psDBF->sHooks.FRead(pszRecord, psDBF->nRecordLength, 1,
                                    psDBF->fp) != 1) {
                errorAbort = true;
                break;
            }

            pszRecordNew[0] = pszRecord[0];

            for (int i = 0; i < psDBF->nFields; i++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
                memcpy(pszRecordNew + panFieldOffsetNew[i],
                       pszRecord + psDBF->panFieldOffset[panMap[i]],
                       psDBF->panFieldSize[panMap[i]]);
            }

            /* write record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            psDBF->sHooks.FWrite(pszRecordNew, psDBF->nRecordLength, 1,
                                 psDBF->fp);
        }

        /* free record */
        free(pszRecord);
        free(pszRecordNew);
    }

    if (errorAbort) {
        free(panFieldOffsetNew);
        free(panFieldSizeNew);
        free(panFieldDecimalsNew);
        free(pachFieldTypeNew);
        psDBF->nCurrentRecord = -1;
        psDBF->bCurrentRecordModified = FALSE;
        psDBF->bUpdated = FALSE;
        return FALSE;
    }

    free(psDBF->panFieldOffset);
    free(psDBF->panFieldSize);
    free(psDBF->panFieldDecimals);
    free(psDBF->pachFieldType);

    psDBF->panFieldOffset = panFieldOffsetNew;
    psDBF->panFieldSize = panFieldSizeNew;
    psDBF->panFieldDecimals = panFieldDecimalsNew;
    psDBF->pachFieldType = pachFieldTypeNew;

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;
    psDBF->bUpdated = TRUE;

    return TRUE;
}

/************************************************************************/
/*                          DBFAlterFieldDefn()                         */
/*                                                                      */
/*      Alter a field definition in a .dbf file                         */
/************************************************************************/

int SHPAPI_CALL DBFAlterFieldDefn(DBFHandle psDBF, int iField,
                                  const char *pszFieldName, char chType,
                                  int nWidth, int nDecimals)
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    int i;
    int iRecord;
    int nOffset;
    int nOldWidth;
    int nOldRecordLength;
    SAOffset nRecordOffset;
    char *pszFInfo;
    char chOldType;
    int bIsNULL;
    char chFieldFill;

<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    if (iField < 0 || iField >= psDBF->nFields)
        return FALSE;

    /* make sure that everything is written in .dbf */
    if (!DBFFlushRecord(psDBF))
        return FALSE;

    const char chFieldFill = DBFGetNullCharacter(chType);

    const char chOldType = psDBF->pachFieldType[iField];
    const int nOffset = psDBF->panFieldOffset[iField];
    const int nOldWidth = psDBF->panFieldSize[iField];
    const int nOldRecordLength = psDBF->nRecordLength;

    /* -------------------------------------------------------------------- */
    /*      Do some checking to ensure we can add records to this file.     */
    /* -------------------------------------------------------------------- */
    if (nWidth < 1)
        return -1;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (nWidth > XBASE_FLD_MAX_WIDTH)
        nWidth = XBASE_FLD_MAX_WIDTH;
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if (nWidth > 255)
        nWidth = 255;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (nWidth > XBASE_FLD_MAX_WIDTH)
        nWidth = XBASE_FLD_MAX_WIDTH;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (nWidth > 255)
        nWidth = 255;
=======
    if (nWidth > XBASE_FLD_MAX_WIDTH)
        nWidth = XBASE_FLD_MAX_WIDTH;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    /* -------------------------------------------------------------------- */
    /*      Assign the new field information fields.                        */
    /* -------------------------------------------------------------------- */
    psDBF->panFieldSize[iField] = nWidth;
    psDBF->panFieldDecimals[iField] = nDecimals;
    psDBF->pachFieldType[iField] = chType;

    /* -------------------------------------------------------------------- */
    /*      Update the header information.                                  */
    /* -------------------------------------------------------------------- */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    char *pszFInfo = psDBF->pszHeader + XBASE_FLDHDR_SZ * iField;

    for (int i = 0; i < XBASE_FLDHDR_SZ; i++)
        pszFInfo[i] = '\0';

    strncpy(pszFInfo, pszFieldName, XBASE_FLDNAME_LEN_WRITE);
=======
    pszFInfo = psDBF->pszHeader + 32 * iField;
=======
    char *pszFInfo = psDBF->pszHeader + XBASE_FLDHDR_SZ * iField;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    for (int i = 0; i < XBASE_FLDHDR_SZ; i++)
        pszFInfo[i] = '\0';

<<<<<<< HEAD
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    pszFInfo = psDBF->pszHeader + 32 * iField;

    for (i = 0; i < 32; i++)
        pszFInfo[i] = '\0';

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    if ((int)strlen(pszFieldName) < 10)
        strncpy(pszFInfo, pszFieldName, strlen(pszFieldName));
    else
        strncpy(pszFInfo, pszFieldName, 10);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    strncpy(pszFInfo, pszFieldName, XBASE_FLDNAME_LEN_WRITE);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
    char *pszFInfo = psDBF->pszHeader + XBASE_FLDHDR_SZ * iField;

    for (int i = 0; i < XBASE_FLDHDR_SZ; i++)
        pszFInfo[i] = '\0';

    strncpy(pszFInfo, pszFieldName, XBASE_FLDNAME_LEN_WRITE);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    pszFInfo[11] = psDBF->pachFieldType[iField];

    if (chType == 'C') {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        pszFInfo[16] = STATIC_CAST(unsigned char, nWidth % 256);
        pszFInfo[17] = STATIC_CAST(unsigned char, nWidth / 256);
    }
    else {
        pszFInfo[16] = STATIC_CAST(unsigned char, nWidth);
        pszFInfo[17] = STATIC_CAST(unsigned char, nDecimals);
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        pszFInfo[16] = (unsigned char)(nWidth % 256);
        pszFInfo[17] = (unsigned char)(nWidth / 256);
    }
    else {
        pszFInfo[16] = (unsigned char)nWidth;
        pszFInfo[17] = (unsigned char)nDecimals;
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        pszFInfo[16] = (unsigned char)(nWidth % 256);
        pszFInfo[17] = (unsigned char)(nWidth / 256);
    }
    else {
        pszFInfo[16] = (unsigned char)nWidth;
        pszFInfo[17] = (unsigned char)nDecimals;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        pszFInfo[16] = STATIC_CAST(unsigned char, nWidth % 256);
        pszFInfo[17] = STATIC_CAST(unsigned char, nWidth / 256);
    }
    else {
        pszFInfo[16] = STATIC_CAST(unsigned char, nWidth);
        pszFInfo[17] = STATIC_CAST(unsigned char, nDecimals);
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    /* -------------------------------------------------------------------- */
    /*      Update offsets                                                  */
    /* -------------------------------------------------------------------- */
    if (nWidth != nOldWidth) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        for (int i = iField + 1; i < psDBF->nFields; i++)
            psDBF->panFieldOffset[i] += nWidth - nOldWidth;
        psDBF->nRecordLength += nWidth - nOldWidth;

        psDBF->pszCurrentRecord = STATIC_CAST(
            char *, SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength));
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        for (i = iField + 1; i < psDBF->nFields; i++)
            psDBF->panFieldOffset[i] += nWidth - nOldWidth;
        psDBF->nRecordLength += nWidth - nOldWidth;

        psDBF->pszCurrentRecord =
            (char *)SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        for (i = iField + 1; i < psDBF->nFields; i++)
            psDBF->panFieldOffset[i] += nWidth - nOldWidth;
        psDBF->nRecordLength += nWidth - nOldWidth;

        psDBF->pszCurrentRecord =
            (char *)SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
        for (int i = iField + 1; i < psDBF->nFields; i++)
            psDBF->panFieldOffset[i] += nWidth - nOldWidth;
        psDBF->nRecordLength += nWidth - nOldWidth;

        psDBF->pszCurrentRecord = STATIC_CAST(
            char *, SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }

    /* we're done if we're dealing with not yet created .dbf */
    if (psDBF->bNoHeader && psDBF->nRecords == 0)
        return TRUE;

    /* force update of header with new header and record length */
    psDBF->bNoHeader = TRUE;
    DBFUpdateHeader(psDBF);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    bool errorAbort = false;
=======
    if (nWidth < nOldWidth || (nWidth == nOldWidth && chType != chOldType)) {
        char *pszRecord = (char *)malloc(sizeof(char) * nOldRecordLength);
        char *pszOldField = (char *)malloc(sizeof(char) * (nOldWidth + 1));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (nWidth < nOldWidth || (nWidth == nOldWidth && chType != chOldType)) {
        char *pszRecord = (char *)malloc(sizeof(char) * nOldRecordLength);
        char *pszOldField = (char *)malloc(sizeof(char) * (nOldWidth + 1));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    bool errorAbort = false;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    if (nWidth < nOldWidth || (nWidth == nOldWidth && chType != chOldType)) {
        char *pszRecord = (char *)malloc(sizeof(char) * nOldRecordLength);
        char *pszOldField = (char *)malloc(sizeof(char) * (nOldWidth + 1));
=======
    bool errorAbort = false;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

    if (nWidth < nOldWidth || (nWidth == nOldWidth && chType != chOldType)) {
        char *pszRecord =
            STATIC_CAST(char *, malloc(sizeof(char) * nOldRecordLength));
        char *pszOldField =
            STATIC_CAST(char *, malloc(sizeof(char) * (nOldWidth + 1)));

        /* cppcheck-suppress uninitdata */
        pszOldField[nOldWidth] = 0;

        /* move records to their new positions */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        for (int iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
            SAOffset nRecordOffset =
                nOldRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;
<<<<<<< HEAD

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1,
                                    psDBF->fp) != 1) {
                errorAbort = true;
                break;
            }

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            const bool bIsNULL = DBFIsValueNULL(chOldType, pszOldField);

            if (nWidth != nOldWidth) {
                if ((chOldType == 'N' || chOldType == 'F' ||
                     chOldType == 'D') &&
=======
        for (iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
            nRecordOffset =
                nOldRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1,
                                    psDBF->fp) != 1) {
                errorAbort = true;
                break;
            }

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            const bool bIsNULL = DBFIsValueNULL(chOldType, pszOldField);

            if (nWidth != nOldWidth) {
<<<<<<< HEAD
                if ((chOldType == 'N' || chOldType == 'F') &&
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        for (iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
            nRecordOffset =
                nOldRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1, psDBF->fp);

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            bIsNULL = DBFIsValueNULL(chOldType, pszOldField);

            if (nWidth != nOldWidth) {
                if ((chOldType == 'N' || chOldType == 'F') &&
<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                if ((chOldType == 'N' || chOldType == 'F' ||
                     chOldType == 'D') &&
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        for (int iRecord = 0; iRecord < psDBF->nRecords; iRecord++) {
            SAOffset nRecordOffset =
                nOldRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1,
                                    psDBF->fp) != 1) {
                errorAbort = true;
                break;
            }

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            const bool bIsNULL = DBFIsValueNULL(chOldType, pszOldField);

            if (nWidth != nOldWidth) {
                if ((chOldType == 'N' || chOldType == 'F' ||
                     chOldType == 'D') &&
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
                    pszOldField[0] == ' ') {
                    /* Strip leading spaces when truncating a numeric field */
                    memmove(pszRecord + nOffset,
                            pszRecord + nOffset + nOldWidth - nWidth, nWidth);
                }
                if (nOffset + nOldWidth < nOldRecordLength) {
                    memmove(pszRecord + nOffset + nWidth,
                            pszRecord + nOffset + nOldWidth,
                            nOldRecordLength - (nOffset + nOldWidth));
                }
            }

            /* Convert null value to the appropriate value of the new type */
            if (bIsNULL) {
                memset(pszRecord + nOffset, chFieldFill, nWidth);
            }

            nRecordOffset =
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;
=======
                psDBF->nRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
                psDBF->nRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
                psDBF->nRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;
=======
                psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

            /* write record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            psDBF->sHooks.FWrite(pszRecord, psDBF->nRecordLength, 1, psDBF->fp);
        }

        if (!errorAbort && psDBF->bWriteEndOfFileChar) {
            char ch = END_OF_FILE_CHARACTER;

            SAOffset nRecordOffset =
                psDBF->nRecordLength * STATIC_CAST(SAOffset, psDBF->nRecords) +
                psDBF->nHeaderLength;

            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
        }
        /* TODO: truncate file */

        free(pszRecord);
        free(pszOldField);
    }
    else if (nWidth > nOldWidth) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        char *pszRecord =
            STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nRecordLength));
        char *pszOldField =
            STATIC_CAST(char *, malloc(sizeof(char) * (nOldWidth + 1)));
<<<<<<< HEAD
=======
        char *pszRecord = (char *)malloc(sizeof(char) * psDBF->nRecordLength);
        char *pszOldField = (char *)malloc(sizeof(char) * (nOldWidth + 1));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        char *pszRecord = (char *)malloc(sizeof(char) * psDBF->nRecordLength);
        char *pszOldField = (char *)malloc(sizeof(char) * (nOldWidth + 1));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
        char *pszRecord = (char *)malloc(sizeof(char) * psDBF->nRecordLength);
        char *pszOldField = (char *)malloc(sizeof(char) * (nOldWidth + 1));
=======
        char *pszRecord =
            STATIC_CAST(char *, malloc(sizeof(char) * psDBF->nRecordLength));
        char *pszOldField =
            STATIC_CAST(char *, malloc(sizeof(char) * (nOldWidth + 1)));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

        /* cppcheck-suppress uninitdata */
        pszOldField[nOldWidth] = 0;

        /* move records to their new positions */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        for (int iRecord = psDBF->nRecords - 1; iRecord >= 0; iRecord--) {
            SAOffset nRecordOffset =
                nOldRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;
<<<<<<< HEAD

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1,
                                    psDBF->fp) != 1) {
                errorAbort = true;
                break;
            }

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            const bool bIsNULL = DBFIsValueNULL(chOldType, pszOldField);
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        for (iRecord = psDBF->nRecords - 1; iRecord >= 0; iRecord--) {
            nRecordOffset =
                nOldRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1, psDBF->fp);

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            bIsNULL = DBFIsValueNULL(chOldType, pszOldField);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        for (int iRecord = psDBF->nRecords - 1; iRecord >= 0; iRecord--) {
            SAOffset nRecordOffset =
                nOldRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1,
                                    psDBF->fp) != 1) {
                errorAbort = true;
                break;
            }

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            const bool bIsNULL = DBFIsValueNULL(chOldType, pszOldField);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

=======
        for (iRecord = psDBF->nRecords - 1; iRecord >= 0; iRecord--) {
            nRecordOffset =
                nOldRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

            /* load record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            if (psDBF->sHooks.FRead(pszRecord, nOldRecordLength, 1,
                                    psDBF->fp) != 1) {
                errorAbort = true;
                break;
            }

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            const bool bIsNULL = DBFIsValueNULL(chOldType, pszOldField);

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            if (nOffset + nOldWidth < nOldRecordLength) {
                memmove(pszRecord + nOffset + nWidth,
                        pszRecord + nOffset + nOldWidth,
                        nOldRecordLength - (nOffset + nOldWidth));
            }

            /* Convert null value to the appropriate value of the new type */
            if (bIsNULL) {
                memset(pszRecord + nOffset, chFieldFill, nWidth);
            }
            else {
                if ((chOldType == 'N' || chOldType == 'F')) {
                    /* Add leading spaces when expanding a numeric field */
                    memmove(pszRecord + nOffset + nWidth - nOldWidth,
                            pszRecord + nOffset, nOldWidth);
                    memset(pszRecord + nOffset, ' ', nWidth - nOldWidth);
                }
                else {
                    /* Add trailing spaces */
                    memset(pszRecord + nOffset + nOldWidth, ' ',
                           nWidth - nOldWidth);
                }
            }

            nRecordOffset =
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;
=======
                psDBF->nRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
                psDBF->nRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
                psDBF->nRecordLength * (SAOffset)iRecord + psDBF->nHeaderLength;
=======
                psDBF->nRecordLength * STATIC_CAST(SAOffset, iRecord) +
                psDBF->nHeaderLength;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

            /* write record */
            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            psDBF->sHooks.FWrite(pszRecord, psDBF->nRecordLength, 1, psDBF->fp);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        }

        if (!errorAbort && psDBF->bWriteEndOfFileChar) {
            char ch = END_OF_FILE_CHARACTER;

            SAOffset nRecordOffset =
                psDBF->nRecordLength * STATIC_CAST(SAOffset, psDBF->nRecords) +
                psDBF->nHeaderLength;

            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }

        if (!errorAbort && psDBF->bWriteEndOfFileChar) {
            char ch = END_OF_FILE_CHARACTER;

            SAOffset nRecordOffset =
                psDBF->nRecordLength * STATIC_CAST(SAOffset, psDBF->nRecords) +
                psDBF->nHeaderLength;

            psDBF->sHooks.FSeek(psDBF->fp, nRecordOffset, 0);
            psDBF->sHooks.FWrite(&ch, 1, 1, psDBF->fp);
<<<<<<< HEAD
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        }

        free(pszRecord);
        free(pszOldField);
    }

    if (errorAbort) {
        psDBF->nCurrentRecord = -1;
        psDBF->bCurrentRecordModified = TRUE;
        psDBF->bUpdated = FALSE;

        return FALSE;
    }
    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;
    psDBF->bUpdated = TRUE;

    return TRUE;
}

/************************************************************************/
/*                    DBFSetWriteEndOfFileChar()                        */
/************************************************************************/

void SHPAPI_CALL DBFSetWriteEndOfFileChar(DBFHandle psDBF, int bWriteFlag)
{
    psDBF->bWriteEndOfFileChar = bWriteFlag;
}
