/*
 ****************************************************************************
 *                     -- GRASS Development Team --
 *
 * MODULE:      GRASS gis library
 * FILENAME:    flate.c
 * AUTHOR(S):   Eric G. Miller <egm2@jps.net>
 * PURPOSE:     To provide an interface to libz for compressing and 
 *              decompressing data using DEFLATE.  It's primary use is in
 *              the storage and reading of GRASS floating point rasters.
 *              It replaces the patented LZW compression interface.
 *
 * ALGORITHM:   http://www.gzip.org/zlib/feldspar.html
 * DATE CREATED: Nov 19 2000
 * COPYRIGHT:   (C) 2000 by the GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (version 2 or greater). Read the file COPYING that 
 *              comes with GRASS for details.
 *
 *****************************************************************************/

/********************************************************************
 * int                                                              *
 * G_zlib_read (fd, rbytes, dst, nbytes)                            *
 *     int fd, rbytes, nbytes;                                      *
 *     unsigned char *dst;                                          *
 * ---------------------------------------------------------------- *
 * This is the basic function for reading a compressed chunk of a   *
 * data file.  The file descriptor should be in the proper location *
 * and the 'dst' array should have enough space for the data.       *
 * 'nbytes' is the size of 'dst'.  The 'rbytes' parameter is the    *
 * number of bytes to read (knowable from the offsets index). For   *
 * best results, 'nbytes' should be the exact amount of space       *
 * needed for the expansion.  Too large a value of nbytes may cause *
 * more data to be expanded than is desired.                        *
 * Returns: The number of bytes decompressed into dst, or an error. *
 *                                                                  *
 * Errors include:                                                  *
 *        -1  -- Error Reading or Decompressing data.               *
 *        -2  -- Not enough space in dst.  You must make dst larger *
 *               and then call the function again (remembering to   *
 *               reset the file descriptor to it's proper location. *
 *                                                                  *
 * ================================================================ *
 * int                                                              *
 * G_zlib_write (fd, src, nbytes)                                   *
 *     int fd, nbytes;                                              *
 *     unsigned char *src;                                          *
 * ---------------------------------------------------------------- *
 * This is the basic function for writing and compressing a data    *
 * chunk to a file.  The file descriptor should be in the correct   *
 * location prior to this call. The function will compress 'nbytes' *
 * of 'src' and write it to the file 'fd'.  Returns the number of   *
 * bytes written or an error code:                                  *
 *                                                                  *
 * Errors include:                                                  *
 *        -1 -- Compression Failed.                                 *
 *        -2 -- Unable to write to file.                            *
 *                                                                  *
 * ================================================================ *
 * int                                                              *
 * G_zlib_write_noCompress (fd, src, nbytes)                        *
 *     int fd, nbytes;                                              *
 *     unsigned char *src;                                          *
 * ---------------------------------------------------------------- *
 * Works similar to G_zlib_write() except no attempt at compression *
 * is made.  This is quicker, but may result in larger files.       *
 * Returns the number of bytes written, or -1 for an error. It will *
 * return an error if it fails to write nbytes. Otherwise, the      *
 * return value will always be nbytes + 1 (for compression flag).   *
 *                                                                  *
 * ================================================================ *
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>

#define G_ZLIB_COMPRESSED_NO (unsigned char)'0'
#define G_ZLIB_COMPRESSED_YES (unsigned char)'1'

static void _init_zstruct(z_stream * z)
{
    /* The types are defined in zlib.h, we set to NULL so zlib uses
     * its default functions.
     */
    z->zalloc = (alloc_func) 0;
    z->zfree = (free_func) 0;
    z->opaque = (voidpf) 0;
}

int G_zlib_read(int fd, int rbytes, unsigned char *dst, int nbytes)
{
    int bsize, nread, err;
    unsigned char *b;

    if (dst == NULL || nbytes < 0)
	return -2;

    bsize = rbytes;

    /* Our temporary input buffer for read */
    if (NULL == (b = (unsigned char *)
		 G_calloc(bsize, sizeof(unsigned char))))
	return -1;

    /* Read from the file until we get our bsize or an error */
    nread = 0;
    do {
	err = read(fd, b + nread, bsize - nread);
	if (err >= 0)
	    nread += err;
    } while (err > 0 && nread < bsize);

    /* If the bsize if less than rbytes and we didn't get an error.. */
    if (nread < rbytes && err > 0) {
	G_free(b);
	return -1;
    }

    /* Test if row is compressed */
    if (b[0] == G_ZLIB_COMPRESSED_NO) {
	/* Then just copy it to dst */
	for (err = 0; err < nread - 1 && err < nbytes; err++)
	    dst[err] = b[err + 1];

	G_free(b);
	return (nread - 1);
    }
    else if (b[0] != G_ZLIB_COMPRESSED_YES) {
	/* We're not at the start of a row */
	G_free(b);
	return -1;
    }
    /* Okay it's a compressed row */

    /* Just call G_zlib_expand() with the buffer we read,
     * Account for first byte being a flag
     */
    err = G_zlib_expand(b + 1, bsize - 1, dst, nbytes);

    /* We're done with b */
    G_free(b);

    /* Return whatever G_zlib_expand() returned */
    return err;

}				/* G_zlib_read() */


int G_zlib_write(int fd, const unsigned char *src, int nbytes)
{
    int dst_sz, nwritten, err;
    unsigned char *dst, compressed;

    /* Catch errors */
    if (src == NULL || nbytes < 0)
	return -1;

    dst_sz = nbytes;
    if (NULL == (dst = (unsigned char *)
		 G_calloc(dst_sz, sizeof(unsigned char))))
	return -1;

    /* Now just call G_zlib_compress() */
    err = G_zlib_compress(src, nbytes, dst, dst_sz);

    /* If compression succeeded write compressed row,
     * otherwise write uncompressed row. Compression will fail
     * if dst is too small (i.e. compressed data is larger)
     */
    if (err > 0 && err <= dst_sz) {
	dst_sz = err;
	/* Write the compression flag */
	compressed = G_ZLIB_COMPRESSED_YES;
	if (write(fd, &compressed, 1) != 1) {
	    G_free(dst);
	    return -1;
	}
	nwritten = 0;
	do {
	    err = write(fd, dst + nwritten, dst_sz - nwritten);
	    if (err >= 0)
		nwritten += err;
	} while (err > 0 && nwritten < dst_sz);
	/* Account for extra byte */
	nwritten++;
    }
    else {
	/* Write compression flag */
	compressed = G_ZLIB_COMPRESSED_NO;
	if (write(fd, &compressed, 1) != 1) {
	    G_free(dst);
	    return -1;
	}
	nwritten = 0;
	do {
	    err = write(fd, src + nwritten, nbytes - nwritten);
	    if (err >= 0)
		nwritten += err;
	} while (err > 0 && nwritten < nbytes);
	/* Account for extra byte */
	nwritten++;
    }				/* if (err > 0) */

    /* Done with the dst buffer */
    G_free(dst);

    /* If we didn't write all the data return an error */
    if (err < 0)
	return -2;

    return nwritten;
}				/* G_zlib_write() */


int G_zlib_write_noCompress(int fd, const unsigned char *src, int nbytes)
{
    int err, nwritten;
    unsigned char compressed;

    /* Catch errors */
    if (src == NULL || nbytes < 0)
	return -1;

    /* Write the compression flag */
    compressed = G_ZLIB_COMPRESSED_NO;
    if (write(fd, &compressed, 1) != 1)
	return -1;

    /* Now write the data */
    nwritten = 0;
    do {
	err = write(fd, src + nwritten, nbytes - nwritten);
	if (err > 0)
	    nwritten += err;
    } while (err > 0 && nwritten < nbytes);

    if (err < 0 || nwritten != nbytes)
	return -1;

    /* Account for extra compressed flag */
    nwritten++;

    /* That's all */
    return nwritten;

}				/* G_zlib_write_noCompress() */


int
G_zlib_compress(const unsigned char *src, int src_sz, unsigned char *dst,
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
    buf_sz = (int)((double)dst_sz * 1.01 + (double)12);
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

    /* Initialize using default compression (usually 6) */
    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);

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
    if (nbytes > dst_sz) {	/* Not enough room to copy output */
	G_free(buf);
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
G_zlib_expand(const unsigned char *src, int src_sz, unsigned char *dst,
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
