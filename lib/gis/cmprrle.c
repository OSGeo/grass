/*
 ****************************************************************************
 *                     -- GRASS Development Team --
 *
 * MODULE:      GRASS gis library
 * FILENAME:    cmprrle.c
 * AUTHOR(S):   Markus Metz
 * PURPOSE:     To provide generic RLE for compressing and 
 *              decompressing data.  Its primary use is in
 *              the storage and reading of GRASS rasters.
 *
 * ALGORITHM:   Run Length Encoding
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
 * G_rle_compress (src, srz_sz, dst, dst_sz)                        *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function compresses data with RLE.                          *
 * It uses an all or nothing call.                                  *
 * If you need a continuous compression scheme, you'll have to code *
 * your own.                                                        *
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
 * G_rle_expand (src, src_sz, dst, dst_sz)                          *
 *     int src_sz, dst_sz;                                          *
 *     unsigned char *src, *dst;                                    *
 * ---------------------------------------------------------------- *
 * This function decompresses data compressed with RLE.              *
 * It is equivalent to a single pass call to an external expansion  * 
 * function.                                                        *
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

#include <grass/gis.h>
#include <grass/glocale.h>

/* no fast mode if destination is large enough to hold 
 * worst case compression */ 
int G_rle_compress_bound(int src_sz)
{
    return ((src_sz >> 1) * 3 + (src_sz & 1));
}

int
G_rle_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz)
{
    int i, nbytes;
    unsigned char prev_b; 
    int cnt;

    /* Catch errors early */
    if (src == NULL || dst == NULL)
	return -1;

    /* Don't do anything if src is empty or smaller than 4 bytes */
    if (src_sz <= 3)
	return 0;

    /* modified RLE:
     * unit is 1 byte, only sequences longer than 1 are encoded
     * single occurrences don't have a following count
     * multiple occurrences are twice in dst, followed by the count
     * example:
     * ABBCCC
     * is encoded as
     * ABB2CC3 
     */
 
    prev_b = src[0];
    cnt = 1;
    nbytes = 0;
    for (i = 1; i < src_sz; i++) {
	if (prev_b != src[i] || cnt == 255) {
	    /* write to dst */
	    if (cnt == 1) {
		if (nbytes >= dst_sz)
		    return -2;
		dst[nbytes++] = prev_b;
	    }
	    else {
		/* cnt > 1 */
		if (nbytes >= dst_sz - 2)
		    return -2;
		dst[nbytes++] = prev_b;
		dst[nbytes++] = prev_b;
		dst[nbytes++] = (unsigned char) cnt;
	    }
	    cnt = 0;
	}
	prev_b = src[i];
	cnt++;
    }
    /* write out the last sequence */
    if (cnt == 1) {
	if (nbytes >= dst_sz)
	    return -2;
	dst[nbytes++] = prev_b;
    }
    else {
	if (nbytes >= dst_sz - 2)
	    return -2;
	dst[nbytes++] = prev_b;
	dst[nbytes++] = prev_b;
	dst[nbytes++] = (unsigned char) cnt;
    }

    return nbytes;
}

int
G_rle_expand(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz)
{
    int i, j, nbytes, cnt;
    unsigned char prev_b;

    /* Catch errors early */
    if (src == NULL || dst == NULL)
	return -1;

    /* Don't do anything if src is empty */
    if (src_sz <= 0)
	return 0;

    /* RLE expand */
    prev_b = src[0];
    cnt = 1;
    nbytes = 0;
    i = 1;
    while (i < src_sz) {
	/* single occurrences don't have a following count
	 * multiple occurrences are twice in src, followed by the count */
	if (cnt == 2) {
	    if (i >= src_sz)
		return -1;
	    cnt = src[i];
	    if (nbytes + cnt > dst_sz)
		return -1;
	    for (j = 0; j < cnt; j++) {
		dst[nbytes++] = prev_b;
	    }
	    cnt = 0;
	    i++;
	    if (i >= src_sz)
		return nbytes;
	}
	if (cnt == 1) {
	    if (prev_b != src[i]) {
		if (nbytes + cnt > dst_sz)
		    return -1;
		dst[nbytes++] = prev_b;
		cnt = 0;
	    }
	}
	prev_b = src[i];
	cnt++;
	i++;
    }
    if (nbytes >= dst_sz)
	return -1;
    if (cnt == 1)
	dst[nbytes++] = prev_b;

    return nbytes;
}

/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
