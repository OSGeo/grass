/*
 ****************************************************************************
 *                     -- GRASS Development Team --
 *
 * MODULE:      GRASS gis library
 * FILENAME:    cmprzlib.c
 * AUTHOR(S):   Eric G. Miller <egm2@jps.net>
 *              Markus Metz
 * PURPOSE:     To provide an interface to libz for compressing and 
 *              decompressing data using DEFLATE.  It's primary use is in
 *              the storage and reading of GRASS floating point rasters.
 *              It replaces the patented LZW compression interface.
 *
 * ALGORITHM:   http://www.gzip.org/zlib/feldspar.html
 * DATE CREATED: Dec 17 2015
 * COPYRIGHT:   (C) 2015 by the GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (version 2 or greater). Read the file COPYING that 
 *              comes with GRASS for details.
 *
 *****************************************************************************/

/********************************************************************
 * int                                                              *
 * G_zlib_compress (src, srz_sz, dst, dst_sz)                       *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function is a wrapper around the zlib deflate() function.   *
 * It uses an all or nothing call to deflate().  If you need a      *
 * continuous compression scheme, you'll have to code your own.     *
 * In order to do a single pass compression, the input src must be  *
 * copied to a buffer 1% + 12 bytes larger than the data.  This may *
 * cause performance degradation.                                   *
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
 * G_zlib_expand (src, src_sz, dst, dst_sz)                         *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function is a wrapper around the zlib inflate() function.   *
 * It uses a single pass call to inflate().  If you need a contin-  *
 * uous expansion scheme, you'll have to code your own.             *
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

#ifndef HAVE_ZLIB_H

#error "GRASS requires libz to compile"

#else

#include <zlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "G.h"


int
G_zlib_compress_bound(int src_sz)
{
    /* from zlib.h:
     * "when using compress or compress2, 
     * destLen must be at least the value returned by
     * compressBound(sourceLen)"
     * no explanation for the "must be"
     */
    return compressBound(src_sz);
}

int
G_zlib_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz)
{
    uLong err, nbytes, buf_sz;
    unsigned char *buf;

    /* Catch errors early */
    if (src == NULL || dst == NULL)
	return -1;

    /* Don't do anything if either of these are true */
    if (src_sz <= 0 || dst_sz <= 0)
	return 0;

    /* Output buffer has to be 1% + 12 bytes bigger for single pass deflate */
    /* buf_sz = (int)((double)dst_sz * 1.01 + (double)12); */

    /* Output buffer should be large enough for single pass compression */
    buf = dst;
    buf_sz = G_zlib_compress_bound(src_sz);
    if (buf_sz > dst_sz) {
	G_warning("G_zlib_compress(): programmer error, destination is too small");
	if (NULL == (buf = (unsigned char *)
		     G_calloc(buf_sz, sizeof(unsigned char))))
	    return -1;
    }
    else
	buf_sz = dst_sz;

    /* Valid zlib compression levels -1 - 9 */
    /* zlib default: Z_DEFAULT_COMPRESSION = -1, equivalent to 6 
     * as used here, 1 gives the best compromise between speed and compression */

    /* Do single pass compression */
    nbytes = buf_sz;
    err = compress2((Bytef *)buf, &nbytes, 	 /* destination */
		    (const Bytef *)src, src_sz,  /* source */
		    G__.compression_level); 	 /* level */

    if (err != Z_OK) {
	if (buf != dst)
	    G_free(buf);
	return -1;
    }

    /* updated buf_sz is bytes of compressed data */
    if (nbytes >= src_sz) {
	/* compression not possible */
	if (buf != dst)
	    G_free(buf);
	return -2;
    }

    if (buf != dst) {
	/* Copy the data from buf to dst */
	for (err = 0; err < nbytes; err++)
	    dst[err] = buf[err];

	G_free(buf);
    }

    return nbytes;
}				/* G_zlib_compress() */


int
G_zlib_expand(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz)
{
    int err;
    uLong ss, nbytes;

    /* Catch error condition */
    if (src == NULL || dst == NULL)
	return -2;

    /* Don't do anything if either of these are true */
    if (src_sz <= 0 || dst_sz <= 0)
	return 0;

    ss = src_sz;

    /* Do single pass decompression */
    nbytes = dst_sz;
    err = uncompress((Bytef *)dst, &nbytes,  /* destination */
		     (const Bytef *)src, ss);   /* source */

    /* If not Z_OK return error -1 */
    if (err != Z_OK)
	return -1;

    /* Number of bytes inflated to output stream is
     * updated buffer size
     */

    if (nbytes != dst_sz) {
	return -1;
    }

    return nbytes;
}				/* G_zlib_expand() */


#endif /* HAVE_ZLIB_H */


/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
