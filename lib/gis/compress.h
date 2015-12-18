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
 * modify G_get_compressor(), G_compress(), G_expand()
 */

/* cmprrle.c : Run Length Encoding (RLE) */
int
G_rle_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz);
int
G_rle_expand(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz);

/* cmprzlib.c : ZLIB's DEFLATE */
int
G_zlib_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz);
int
G_zlib_expand(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz);

/* cmprlz4.c : LZ4, extremely fast */
int
G_lz4_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz);
int
G_lz4_expand(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz);

/* cmprbzip.c : BZIP2, high compression, faster than ZLIB's DEFLATE with level 9 */
int
G_bz2_compress(unsigned char *src, int src_sz, unsigned char *dst,
		int dst_sz);
int
G_bz2_expand(unsigned char *src, int src_sz, unsigned char *dst,
	      int dst_sz);

/* add more here */
