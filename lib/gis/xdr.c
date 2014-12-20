/*!
 * \file lib/gis/xdr.c
 *
 * \brief GIS Library - XDR related functions.
 *
 * (C) 2012-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Glynn Clements
 */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>

#include "G.h"

static void swap_int(void *dstp, const void *srcp) {
    unsigned char *dst = (unsigned char *) dstp;
    const unsigned char *src = (const unsigned char *) srcp;
    if (G__.little_endian) {
	dst[0] = src[3];
	dst[1] = src[2];
	dst[2] = src[1];
	dst[3] = src[0];
    }
    else
	memcpy(dst, src, 4);
}

static void swap_float(void *dstp, const void *srcp) {
    unsigned char *dst = (unsigned char *) dstp;
    const unsigned char *src = (const unsigned char *) srcp;
    if (G__.little_endian) {
	dst[0] = src[3];
	dst[1] = src[2];
	dst[2] = src[1];
	dst[3] = src[0];
    }
    else
	memcpy(dst, src, 4);
}

static void swap_double(void *dstp, const void *srcp) {
    unsigned char *dst = (unsigned char *) dstp;
    const unsigned char *src = (const unsigned char *) srcp;
    if (G__.little_endian) {
	dst[0] = src[7];
	dst[1] = src[6];
	dst[2] = src[5];
	dst[3] = src[4];
	dst[4] = src[3];
	dst[5] = src[2];
	dst[6] = src[1];
	dst[7] = src[0];
    }
    else
	memcpy(dst, src, 8);
}

void G_xdr_get_int(int *dst, const void *src)
{
    swap_int(dst, src);
}

void G_xdr_put_int(void *dst, const int *src)
{
    swap_int(dst, src);
}

void G_xdr_get_float(float *dst, const void *src)
{
    swap_float(dst, src);
}

void G_xdr_put_float(void *dst, const float *src)
{
    swap_float(dst, src);
}

void G_xdr_get_double(double *dst, const void *src)
{
    swap_double(dst, src);
}

void G_xdr_put_double(void *dst, const double *src)
{
    swap_double(dst, src);
}

