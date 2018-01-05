/*
 ****************************************************************************
 *                     -- GRASS Development Team --
 *
 * MODULE:      GRASS gis library
 * FILENAME:    cmprzstd.c
 * AUTHOR(S):   Eric G. Miller <egm2@jps.net>
 *              Markus Metz
 * PURPOSE:     To provide an interface to ZSTD for compressing and 
 *              decompressing data using ZSTD.  It's primary use is in
 *              the storage and reading of GRASS floating point rasters.
 *
 * ALGORITHM:   http://www.zstd.net
 * DATE CREATED: Dec 18 2017
 * COPYRIGHT:   (C) 2017 by the GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (version 2 or greater). Read the file COPYING that 
 *              comes with GRASS for details.
 *
 *****************************************************************************/

/********************************************************************
 * int                                                              *
 * G_zstd_compress (src, srz_sz, dst, dst_sz)                       *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function is a wrapper around the Zstd compression function. *
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
 * G_zstd_expand (src, src_sz, dst, dst_sz)                         *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function is a wrapper around the zstd decompression          *
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

#ifdef HAVE_ZSTD_H
#include <zstd.h>
#endif

#include <grass/gis.h>
#include <grass/glocale.h>


int
G_zstd_compress_bound(int src_sz)
{
    /* ZSTD has a fast version if destLen is large enough 
     * to hold a worst case result
     */
#ifndef HAVE_ZSTD_H
    G_fatal_error(_("GRASS needs to be compiled with ZSTD for ZSTD compression"));
    return -1;
#else
    return ZSTD_compressBound(src_sz);
#endif
}

int
G_zstd_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz)
{
    int err, nbytes, buf_sz;
    unsigned char *buf;

#ifndef HAVE_ZSTD_H
    G_fatal_error(_("GRASS needs to be compiled with ZSTD for ZSTD compression"));
    return -1;
#else

    /* Catch errors early */
    if (src == NULL || dst == NULL) {
	if (src == NULL)
	    G_warning(_("No source buffer"));
	
	if (dst == NULL)
	    G_warning(_("No destination buffer"));
	return -1;
    }

    /* Don't do anything if either of these are true */
    if (src_sz <= 0 || dst_sz <= 0) {
	if (src_sz <= 0)
	    G_warning(_("Invalid source buffer size %d"), src_sz);
	if (dst_sz <= 0)
	    G_warning(_("Invalid destination buffer size %d"), dst_sz);
	return 0;
    }

    /* Output buffer has to be larger for single pass compression */
    buf = dst;
    buf_sz = G_zstd_compress_bound(src_sz);
    if (buf_sz > dst_sz) {
	G_warning("G_zstd_compress(): programmer error, destination is too small");
	if (NULL == (buf = (unsigned char *)
		     G_calloc(buf_sz, sizeof(unsigned char))))
	    return -1;
    }
    else
	buf_sz = dst_sz;

    /* Do single pass compression */
    err = ZSTD_compress((char *)buf, buf_sz, (char *)src, src_sz, 3);

    if (err <= 0 || ZSTD_isError(err)) {
	G_warning(_("ZSTD compression error %d: %s"),
	          err, ZSTD_getErrorName(err));
	if (buf != dst)
	    G_free(buf);
	return -1;
    }
    if (err >= src_sz) {
	/* compression not possible */
	if (buf != dst)
	    G_free(buf);
	return -2;
    }
    
    /* bytes of compressed data is return value */
    nbytes = err;

    if (buf != dst) {
	/* Copy the data from buf to dst */
	for (err = 0; err < nbytes; err++)
	    dst[err] = buf[err];

	G_free(buf);
    }

    return nbytes;
#endif
}

int
G_zstd_expand(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz)
{
    int err, nbytes;

#ifndef HAVE_ZSTD_H
    G_fatal_error(_("GRASS needs to be compiled with ZSTD for ZSTD compression"));
    return -1;
#else

    /* Catch error condition */
    if (src == NULL || dst == NULL) {
	if (src == NULL)
	    G_warning(_("No source buffer"));
	
	if (dst == NULL)
	    G_warning(_("No destination buffer"));
	return -2;
    }

    /* Don't do anything if either of these are true */
    if (src_sz <= 0 || dst_sz <= 0) {
	if (src_sz <= 0)
	    G_warning(_("Invalid source buffer size %d"), src_sz);
	if (dst_sz <= 0)
	    G_warning(_("Invalid destination buffer size %d"), dst_sz);
	return 0;
    }

    /* Do single pass decompress */
    err = ZSTD_decompress((char *)dst, dst_sz, (char *)src, src_sz);

    if (err <= 0 || ZSTD_isError(err)) {
	G_warning(_("ZSTD compression error %d: %s"),
	          err, ZSTD_getErrorName(err));
    	return -1;
    }

    /* Number of bytes inflated to output stream is return value */
    nbytes = err;

    if (nbytes != dst_sz) {
	/* TODO: it is not an error if destination is larger than needed */
	G_warning(_("Got uncompressed size %d, expected %d"), (int)nbytes, dst_sz);
	return -1;
    }

    return nbytes;
#endif
}


/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
