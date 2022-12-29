#include <grass/config.h>
#include <grass/gis.h>

/* compressors:
 * 0: no compression
 * 1: RLE, unit is one byte
 * 2: ZLIB's DEFLATE (default)
 * 3: LZ4, fastest but lowest compression ratio
 * 4: BZIP2: slowest but highest compression ratio
 * 5: ZSTD: faster than ZLIB, higher compression than ZLIB
 */

/* adding a new compressor:
 * add the corresponding functions G_*compress() and G_*_expand()
 * if needed, add checks to configure.in and include/config.in
 * modify compress.h (this file)
 * nothing to change in compress.c
 */

/* upper bounds of the size of the compressed buffer */
int G_no_compress_bound(int);
int G_rle_compress_bound(int);
int G_zlib_compress_bound(int);
int G_lz4_compress_bound(int);
int G_bz2_compress_bound(int);
int G_zstd_compress_bound(int);

typedef int compress_fn(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz);
typedef int expand_fn(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz);
typedef int bound_fn(int src_sz);

struct compressor_list
{
    int available;
    compress_fn *compress;
    expand_fn *expand;
    bound_fn *bound;
    char *name;
};

/* DO NOT CHANGE the order
 * 0: None
 * 1: RLE
 * 2: ZLIB
 * 3: LZ4
 * 4: BZIP2
 * 5: ZSTD
 */
 
static int n_compressors = 6; 

struct compressor_list compressor[] = {
    {1, G_no_compress, G_no_expand, G_no_compress_bound, "NONE"},
    {1, G_rle_compress, G_rle_expand, G_rle_compress_bound, "RLE"},
    {1, G_zlib_compress, G_zlib_expand, G_zlib_compress_bound, "ZLIB"},
    {1, G_lz4_compress, G_lz4_expand, G_lz4_compress_bound, "LZ4"},
#ifdef HAVE_BZLIB_H
    {1, G_bz2_compress, G_bz2_expand, G_bz2_compress_bound, "BZIP2"},
#else
    {0, G_bz2_compress, G_bz2_expand, G_bz2_compress_bound, "BZIP2"},
#endif
#ifdef HAVE_ZSTD_H
    {1, G_zstd_compress, G_zstd_expand, G_zstd_compress_bound, "ZSTD"},
#else
    {0, G_zstd_compress, G_zstd_expand, G_zstd_compress_bound, "ZSTD"},
#endif
    {0, NULL, NULL, NULL, NULL}
};

