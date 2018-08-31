/*
 ****************************************************************************
 *                     -- GRASS Development Team --
 *
 * MODULE:      GRASS gis library
 * FILENAME:    cmprbzip.c
 * AUTHOR(S):   Markus Metz
 * PURPOSE:     To provide an interface to libbzip2 for compressing and 
 *              decompressing data.  Its primary use is in
 *              the storage and reading of GRASS rasters.
 *
 * ALGORITHM:   http://www.bzip.org
 * DATE CREATED: Nov 19 2015
 * COPYRIGHT:   (C) 2015 by the GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (version 2 or greater). Read the file COPYING that 
 *              comes with GRASS for details.
 *
 *****************************************************************************/

/********************************************************************
 * int                                                              *
 * G_bz2_compress (src, srz_sz, dst, dst_sz)                        *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function is a wrapper around the bzip2 compression          *
 * function. It uses an all or nothing call.                        *
 * If you need a continuous compression scheme, you'll have to code *
 * your own.                                                        *
 * In order to do a single pass compression, the input src must be  *
 * copied to a buffer 1% + 600 bytes larger than the data.  This    *
 * may cause performance degradation.                               *
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
 * G_bz2_expand (src, src_sz, dst, dst_sz)                          *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function is a wrapper around the bzip2 decompression        *
 * function. It uses a single pass call to inflate().               *
 * If you need a continuous expansion scheme, you'll have to code   *
 * your own.                                                        *
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

#ifdef HAVE_BZLIB_H
#include <bzlib.h>
#endif

#include <grass/gis.h>
#include <grass/glocale.h>


int
G_bz2_compress_bound(int src_sz)
{
    /* from the documentation:
     * To guarantee that the compressed data will fit in its buffer, 
     * allocate an output buffer of size 1% larger than the uncompressed data, 
     * plus six hundred extra bytes.
     * bzip2 does not provide a compressbound fn
     * and apparently does not have a fast version if destLen is
     * large enough to hold a worst case result
     */
    return src_sz;
}

int
G_bz2_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz)
{
    int err;
    int i, buf_sz;
    unsigned int nbytes;
    unsigned char *buf;

#ifndef HAVE_BZLIB_H
    G_fatal_error(_("GRASS needs to be compiled with BZIP2 for BZIP2 compression"));
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

    /* Output buffer has to be 1% + 600 bytes bigger for single pass compression */
    buf = dst;
    buf_sz = G_bz2_compress_bound(src_sz);
    if (buf_sz > dst_sz) {
	G_warning("G_bz2_compress(): programmer error, destination is too small");
	if (NULL == (buf = (unsigned char *)
		     G_calloc(buf_sz, sizeof(unsigned char))))
	    return -1;
    }
    else
	buf_sz = dst_sz;

    /* Do single pass compression */
    nbytes = buf_sz;
    err = BZ2_bzBuffToBuffCompress((char *)buf, &nbytes, /* destination */
                                   (char *)src, src_sz,  /* source */
				   9,			 /* blockSize100k */ 
				   0,                    /* verbosity */
				   100);                 /* workFactor */

    if (err != BZ_OK) {
	G_warning(_("BZIP2 version %s compression error %d"),
	          BZ2_bzlibVersion(), err);
	if (buf != dst)
	    G_free(buf);
	return -1;
    }

    /* updated buf_sz is bytes of compressed data */
    if (nbytes >= (unsigned int)src_sz) {
	/* compression not possible */
	if (buf != dst)
	    G_free(buf);
	return -2;
    }

    if (buf != dst) {
	/* Copy the data from buf to dst */
	for (i = 0; i < nbytes; i++)
	    dst[i] = buf[i];

	G_free(buf);
    }

    return nbytes;
#endif
}				/* G_bz2_compress() */

int
G_bz2_expand(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz)
{
    int err;
    unsigned int nbytes;

#ifndef HAVE_BZLIB_H
    G_fatal_error(_("GRASS needs to be compiled with BZIP2 for BZIP2 compression"));
    return -2;
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


    /* Do single pass decompression */
    nbytes = dst_sz;
    err = BZ2_bzBuffToBuffDecompress((char *)dst, &nbytes,  /* destination */
                                     (char *)src, src_sz,   /* source */
				     0,                     /* small */
				     0);                    /* verbosity */

    if (err != BZ_OK) {
	G_warning(_("BZIP2 version %s decompression error %d"),
	          BZ2_bzlibVersion(), err);
	return -1;
    }

    /* Number of bytes inflated to output stream is
     * updated buffer size
     */

    if (nbytes != dst_sz) {
	/* TODO: it is not an error if destination is larger than needed */
	G_warning(_("Got uncompressed size %d, expected %d"), (int)nbytes, dst_sz);
	return -1;
    }

    return nbytes;
#endif
}


/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
