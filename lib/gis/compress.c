/*
 ****************************************************************************
 *                     -- GRASS Development Team --
 *
 * MODULE:      GRASS gis library
 * FILENAME:    compress.c
 * AUTHOR(S):   Markus Metz
 * PURPOSE:     To provide an interface for compressing and 
 *              decompressing data using various methods.  Its primary 
 *              use is in the storage and reading of GRASS rasters.
 *
 * DATE CREATED: Dec 17 2015
 * COPYRIGHT:   (C) 2015 by the GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (version 2 or greater). Read the file COPYING that 
 *              comes with GRASS for details.
 *
 *****************************************************************************/

/********************************************************************
 * Compression methods:                                             *
 * 1 : RLE (generic Run-Length Encoding of single bytes)            *
 * 2 : ZLIB's DEFLATE (good speed and compression)                  *
 * 3 : LZ4 (fastest, low compression)                               *
 * 4 : BZIP2 (slowest, high compression)                            *
 *                                                                  *
 * int                                                              *
 * G_read_compressed (fd, rbytes, dst, nbytes, compression_type)    *
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
 * G_write_compressed (fd, src, nbytes, compression_type)           *
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
 * G_write_uncompressed (fd, src, nbytes)                           *
 *     int fd, nbytes;                                              *
 *     unsigned char *src;                                          *
 * ---------------------------------------------------------------- *
 * Works similar to G_write_compressed() except no attempt at       *
 * compression is made.  This is quicker, but may result in larger  *
 * files.                                                           *
 * Returns the number of bytes written, or -1 for an error. It will *
 * return an error if it fails to write nbytes. Otherwise, the      *
 * return value will always be nbytes + 1 (for compression flag).   *
 *                                                                  *
 ********************************************************************
 */

#include <grass/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "compress.h"

#define G_COMPRESSED_NO (unsigned char)'0'
#define G_COMPRESSED_YES (unsigned char)'1'

/* get compressor number
 * return -1 on error
 * return number >= 0 for known processor */
int G_compressor_number(char *name)
{
    int i;
    
    if (!name)
	return -1;

    for (i = 0; compressor[i].name ; i++) {
	if (G_strcasecmp(name, compressor[i].name) == 0)
	    return i;
    }

    return -1;
}

/* get compressor name
 * return NULL on error
 * return string (name) of known processor */
char *G_compressor_name(int number)
{
    if (number < 0 || number >= n_compressors)
	return NULL;

    return compressor[number].name;
}

/* check compressor number
 * return -1 on error
 * return 0 known but not available
 * return 1 known and available */
int G_check_compressor(int number)
{
    if (number < 0 || number >= n_compressors) {
	G_warning(_("Request for unsupported compressor"));
	return -1;
    }

    return compressor[number].available;
}

int
G_no_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz)
{
    /* Catch errors early */
    if (src == NULL || dst == NULL)
	return -1;

    /* Don't do anything if src is empty */
    if (src_sz <= 0)
	return 0;

    /* dst too small */
    if (dst_sz < src_sz)
	return -2;

    /* Copy the data from src to dst */
    memcpy(dst, src, src_sz);

    return src_sz;
}

int
G_no_expand(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz)
{
    /* Catch errors early */
    if (src == NULL || dst == NULL)
	return -1;

    /* Don't do anything if src is empty */
    if (src_sz <= 0)
	return 0;

    /* dst too small */
    if (dst_sz < src_sz)
	return -2;

    /* Copy the data from src to dst */
    memcpy(dst, src, src_sz);

    return src_sz;
}

/* G_*_compress() returns
 * > 0: number of bytes in dst
 * 0: nothing done
 * -1: error
 * -2: dst too small
 */
int
G_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz, int number)
{
    if (number < 0 || number >= n_compressors) {
	G_fatal_error(_("Request for unsupported compressor"));
	return -1;
    }

    return compressor[number].compress(src, src_sz, dst, dst_sz);
}

/* G_*_expand() returns
 * > 0: number of bytes in dst
 * -1: error
 */
int
G_expand(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz, int number)
{
    if (number < 0 || number >= n_compressors) {
	G_fatal_error(_("Request for unsupported compressor"));
	return -1;
    }

    return compressor[number].expand(src, src_sz, dst, dst_sz);
}

int G_read_compressed(int fd, int rbytes, unsigned char *dst, int nbytes,
                      int compressor)
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
    if (b[0] == G_COMPRESSED_NO) {
	/* Then just copy it to dst */
	for (err = 0; err < nread - 1 && err < nbytes; err++)
	    dst[err] = b[err + 1];

	G_free(b);
	return (nread - 1);
    }
    else if (b[0] != G_COMPRESSED_YES) {
	/* We're not at the start of a row */
	G_free(b);
	return -1;
    }
    /* Okay it's a compressed row */

    /* Just call G_expand() with the buffer we read,
     * Account for first byte being a flag
     */
    err = G_expand(b + 1, bsize - 1, dst, nbytes, compressor);

    /* We're done with b */
    G_free(b);

    /* Return whatever G_expand() returned */
    return err;

}				/* G_read_compressed() */

int G_write_compressed(int fd, unsigned char *src, int nbytes,
                       int compressor)
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

    /* Now just call G_compress() */
    err = G_compress(src, nbytes, dst, dst_sz, compressor);

    /* If compression succeeded write compressed row,
     * otherwise write uncompressed row. Compression will fail
     * if dst is too small (i.e. compressed data is larger)
     */
    if (err > 0 && err <= dst_sz) {
	dst_sz = err;
	/* Write the compression flag */
	compressed = G_COMPRESSED_YES;
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
	compressed = G_COMPRESSED_NO;
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
}				/* G_write_compressed() */

int G_write_uncompressed(int fd, const unsigned char *src, int nbytes)
{
    int err, nwritten;
    unsigned char compressed;

    /* Catch errors */
    if (src == NULL || nbytes < 0)
	return -1;

    /* Write the compression flag */
    compressed = G_COMPRESSED_NO;
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

}				/* G_write_uncompressed() */


/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
