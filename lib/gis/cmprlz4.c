/*
 ****************************************************************************
 *                     -- GRASS Development Team --
 *
 * MODULE:      GRASS gis library
 * FILENAME:    cmprlz4.c
 * AUTHOR(S):   Eric G. Miller <egm2@jps.net>
 *              Markus Metz
 * PURPOSE:     To provide an interface to lz4 for compressing and 
 *              decompressing data using LZ$.  It's primary use is in
 *              the storage and reading of GRASS floating point rasters.
 *
 * ALGORITHM:   https://code.google.com/p/lz4/
 * DATE CREATED: Dec 18 2015
 * COPYRIGHT:   (C) 2015 by the GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (version 2 or greater). Read the file COPYING that 
 *              comes with GRASS for details.
 *
 *****************************************************************************/

/********************************************************************
 * int                                                              *
 * G_lz4_compress (src, srz_sz, dst, dst_sz)                        *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function is a wrapper around the LZ4 cimpression function.  *
 * It uses an all or nothing call.                                  *
 * If you need a continuous compression scheme, you'll have to code *
 * your own.                                                        *
 * In order to do a single pass compression, the input src must be  *
 * copied to a buffer larger than the data.  This may cause         *
 * performance degradation.                                         *
 *                                                                  *
 * The function either returns the number of bytes of compressed    *
 * data in dst, or an error code.                                   *
 *                                                                  *
 * Errors include:                                                  *
 *        -1 -- Compression failed.                                 *
 *        -2 -- dst is too small.                                   *
 *                                                                  *
 * ================================================================ *
 * int                                                              *
 * G_lz4_expand (src, src_sz, dst, dst_sz)                          *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function is a wrapper around the lz4 decompression          *
 * function.  It uses a single pass call.  If you need a continuous *
 * expansion scheme, you'll have to code your own.                  *
 *                                                                  *
 * The function returns the number of bytes expanded into 'dst' or  *
 * and error code.                                                  *
 *                                                                  *
 * Errors include:                                                  *
 *        -1 -- Expansion failed.                                   *
 *                                                                  *
 ********************************************************************
 */

#include <grass/config.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "lz4.h"


int
G_lz4_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz)
{
    int err, nbytes, buf_sz;
    unsigned char *buf;

    /* Catch errors early */
    if (src == NULL || dst == NULL)
	return -1;

    /* Don't do anything if either of these are true */
    if (src_sz <= 0 || dst_sz <= 0)
	return 0;

    /* Output buffer has to be larger for single pass compression */
    buf_sz = LZ4_compressBound(src_sz);
    if (NULL == (buf = (unsigned char *)
		 G_calloc(buf_sz, sizeof(unsigned char))))
	return -1;

    /* Do single pass compression */
    err = LZ4_compress_default((char *)src, (char *)buf, src_sz, buf_sz);
    if (err <= 0) {
	G_free(buf);
	return -1;
    }
    if (err >= src_sz) {
	/* compression not possible */
	G_free(buf);
	return -2;
    }
    
    /* bytes of compressed data is return value */
    nbytes = err;

    /* Copy the data from buf to dst */
    for (err = 0; err < nbytes; err++)
	dst[err] = buf[err];

    G_free(buf);

    return nbytes;
}

int
G_lz4_expand(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz)
{
    int err, nbytes;

    /* Catch error condition */
    if (src == NULL || dst == NULL)
	return -2;

    /* Don't do anything if either of these are true */
    if (src_sz <= 0 || dst_sz <= 0)
	return 0;

    /* Do single pass decompress */
    err = LZ4_decompress_safe((char *)src, (char *)dst, src_sz, dst_sz);
    /* err = LZ4_decompress_fast(src, dst, src_sz); */

    /* Number of bytes inflated to output stream is return value */
    nbytes = err;

    if (nbytes != dst_sz) {
	return -1;
    }

    return nbytes;
}


/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
