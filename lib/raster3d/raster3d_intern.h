#include <rpc/types.h>
#include <rpc/xdr.h>

/*---------------------------------------------------------------------------*/

#include <grass/raster3d.h>
#include <grass/gis.h>

/*---------------------------------------------------------------------------*/

#define G3D_LONG_LENGTH sizeof (long)

#define G3D_XDR_INT_LENGTH 4
#define G3D_XDR_DOUBLE_LENGTH 8
#define G3D_XDR_FLOAT_LENGTH 4

#define G3D_IS_CORRECT_TYPE(t) (((t) == FCELL_TYPE) || ((t) == DCELL_TYPE))

#define G3D_WRITE_DATA 1
#define G3D_READ_DATA 0

#define G3D_VALID_OPERATION(o) \
                           (((o) == G3D_WRITE_DATA) || ((o) == G3D_READ_DATA))

#define G3D_MIN(a,b) ((a) <= (b) ? (a) : (b))
#define G3D_MAX(a,b) ((a) >= (b) ? (a) : (b))

#define G3D_HAS_INDEX 1
#define G3D_NO_INDEX 0

#define G3D_USE_XDR 1
#define G3D_NO_XDR 0

#define G3D_VALID_XDR_OPTION(o) (((o) == G3D_USE_XDR) || ((o) == G3D_NO_XDR))

/*---------------------------------------------------------------------------*/

/* global arrays */

extern void *tmpCompress;	/* compression support array */
extern int tmpCompressLength;	/* in bytes */
extern void *xdr;		/* xdr support array */
extern int xdrLength;		/* in bytes */

/*---------------------------------------------------------------------------*/

/* global variables */

extern int g3d_do_compression;	/* G3D_NO_COMPRESSION or G3D_COMPRESSION */
extern int g3d_do_lzw_compression;	/* G3D_USE_LZW or G3D_NO_LZW */
extern int g3d_do_rle_compression;	/* G3D_USE_RLE or G3D_NO_RLE */
extern int g3d_precision;	/* G3D_ALLOW_PRECISION or G3D_NO_PRECISION */
extern int g3d_cache_default;	/* in number of tiles; 0 ==> no cache */
extern int g3d_cache_max;	/* in bytes */
extern int g3d_file_type;	/* FCELL_TYPE or DCELL_TYPE */
extern int g3d_tile_dimension[3];
extern void (*g3d_error_fun) (const char *);
extern char *g3d_unit_default;

extern G3D_Region g3d_window;

/*---------------------------------------------------------------------------*/

extern void G3d_fatalError(const char * /* msg */ , ...);
extern void G3d_fatalError_noargs(const char * /* msg */ );

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

#define G3D_REGION_NORTH "North"
#define G3D_REGION_SOUTH "South"
#define G3D_REGION_EAST "East"
#define G3D_REGION_WEST "West"
#define G3D_REGION_TOP "Top"
#define G3D_REGION_BOTTOM "Bottom"
#define G3D_REGION_ROWS "nofRows"
#define G3D_REGION_COLS "nofCols"
#define G3D_REGION_DEPTHS "nofDepths"
#define G3D_REGION_PROJ "Proj"
#define G3D_REGION_ZONE "Zone"
#define G3D_REGION_EWRES "e-w resol"
#define G3D_REGION_NSRES "n-s resol"
#define G3D_REGION_TBRES "t-b resol"
