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

static void _init_zstruct(z_stream * z)
{
    /* The types are defined in zlib.h, we set to NULL so zlib uses
     * its default functions.
     */
    z->zalloc = (alloc_func) 0;
    z->zfree = (free_func) 0;
    z->opaque = (voidpf) 0;
}


int
G_zlib_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz)
{
    int err, nbytes, buf_sz;
    unsigned char *buf;
    z_stream c_stream;

    /* Catch errors early */
    if (src == NULL || dst == NULL)
	return -1;

    /* Don't do anything if either of these are true */
    if (src_sz <= 0 || dst_sz <= 0)
	return 0;

    /* Output buffer has to be 1% + 12 bytes bigger for single pass deflate */
    /* buf_sz = (int)((double)dst_sz * 1.01 + (double)12); */
    buf_sz = compressBound(src_sz);
    if (NULL == (buf = (unsigned char *)
		 G_calloc(buf_sz, sizeof(unsigned char))))
	return -1;

    /* Set-up for default zlib memory handling */
    _init_zstruct(&c_stream);

    /* Set-up the stream */
    c_stream.avail_in = src_sz;
    c_stream.next_in = (unsigned char *) src;
    c_stream.avail_out = buf_sz;
    c_stream.next_out = buf;

    /* Initialize */
    /* Valid zlib compression levels -1 - 9 */
    /* zlib default: Z_DEFAULT_COMPRESSION = -1, equivalent to 6 
     * as used here, 1 gives the best compromise between speed and compression */
    err = deflateInit(&c_stream, G__.compression_level);

    /* If there was an error initializing, return -1 */
    if (err != Z_OK) {
	G_free(buf);
	return -1;
    }

    /* Do single pass compression */
    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
	switch (err) {
	case Z_OK:		/* Destination too small */
	    G_free(buf);
	    deflateEnd(&c_stream);
	    return -2;
	    break;
	default:		/* Give other error */
	    G_free(buf);
	    deflateEnd(&c_stream);
	    return -1;
	    break;
	}
    }

    /* avail_out is updated to bytes remaining in buf, so bytes of compressed
     * data is the original size minus that
     */
    nbytes = buf_sz - c_stream.avail_out;
    if (nbytes >= src_sz) {
	/* compression not possible */
	G_free(buf);
	deflateEnd(&c_stream);
	return -2;
    }
    /* Copy the data from buf to dst */
    for (err = 0; err < nbytes; err++)
	dst[err] = buf[err];

    G_free(buf);
    deflateEnd(&c_stream);

    return nbytes;
}				/* G_zlib_compress() */


int
G_zlib_expand(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz)
{
    int err, nbytes;
    z_stream c_stream;

    /* Catch error condition */
    if (src == NULL || dst == NULL)
	return -2;

    /* Don't do anything if either of these are true */
    if (src_sz <= 0 || dst_sz <= 0)
	return 0;

    /* Set-up default zlib memory handling */
    _init_zstruct(&c_stream);

    /* Set-up I/O streams */
    c_stream.avail_in = src_sz;
    c_stream.next_in = (unsigned char *)src;
    c_stream.avail_out = dst_sz;
    c_stream.next_out = dst;

    /* Call zlib initilization function */
    err = inflateInit(&c_stream);

    /* If not Z_OK return error -1 */
    if (err != Z_OK)
	return -1;

    /* Do single pass inflate */
    err = inflate(&c_stream, Z_FINISH);

    /* Number of bytes inflated to output stream is
     * original bytes available minus what avail_out now says
     */
    nbytes = dst_sz - c_stream.avail_out;

    /* Z_STREAM_END means all input was consumed, 
     * Z_OK means only some was processed (not enough room in dst)
     */
    if (!(err == Z_STREAM_END || err == Z_OK)) {
	if (!(err == Z_BUF_ERROR && nbytes == dst_sz)) {
	    inflateEnd(&c_stream);
	    return -1;
	}
	/* Else, there was extra input, but requested output size was
	 * decompressed successfully.
	 */
    }

    inflateEnd(&c_stream);

    return nbytes;
}				/* G_zlib_expand() */


#endif /* HAVE_ZLIB_H */


/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
