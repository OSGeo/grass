#include <grass/config.h>
#include <grass/gis.h>

/* compressors:
 * 0: no compression
 * 1: RLE, unit is one byte
 * 2: ZLIB's DEFLATE (default)
 * 3: LZ4, fastest but lowest compression ratio
 * 4: BZIP2: slowest but highest compression ratio
 */

/* adding a new compressor:
 * add the corresponding functions G_*compress() and G_*_expand()
 * if needed, add checks to configure.in and include/config.in
 * modify compress.h
 * modify G_compress(), G_expand()
 */

typedef int compress_fn(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz);
typedef int expand_fn(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz);

struct compressor_list
{
    int available;
    compress_fn *compress;
    expand_fn *expand;
    char *name;
};

/* DO NOT CHANGE the order
 * 0: None
 * 1: RLE
 * 2: ZLIB
 * 3: LZ4
 * 4: BZIP2 */
 
static int n_compressors = 5; 

struct compressor_list compressor[] = {
    {1, G_no_compress, G_no_expand, "NONE"},
    {1, G_rle_compress, G_rle_expand, "RLE"},
    {1, G_zlib_compress, G_zlib_expand, "ZLIB"},
    {1, G_lz4_compress, G_lz4_expand, "LZ4"},
#ifdef HAVE_BZLIB_H
    {1, G_bz2_compress, G_bz2_expand, "BZIP2"},
#else
    {0, G_bz2_compress, G_bz2_expand, "BZIP2"},
#endif
    {0, NULL, NULL, NULL}
};

