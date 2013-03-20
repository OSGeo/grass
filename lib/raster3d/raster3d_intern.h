#include <grass/raster3d.h>
#include <grass/gis.h>

/*---------------------------------------------------------------------------*/

#define RASTER3D_LONG_LENGTH sizeof (long)

#define RASTER3D_XDR_INT_LENGTH 4		/* Only kept for backward compatibility */
#define RASTER3D_XDR_DOUBLE_LENGTH 8	/* Only kept for backward compatibility */
#define RASTER3D_XDR_FLOAT_LENGTH 4		/* Only kept for backward compatibility */

#define RASTER3D_IS_CORRECT_TYPE(t) (((t) == FCELL_TYPE) || ((t) == DCELL_TYPE))

#define RASTER3D_WRITE_DATA 1
#define RASTER3D_READ_DATA 0

#define RASTER3D_VALID_OPERATION(o) \
                           (((o) == RASTER3D_WRITE_DATA) || ((o) == RASTER3D_READ_DATA))

#define RASTER3D_MIN(a,b) ((a) <= (b) ? (a) : (b))
#define RASTER3D_MAX(a,b) ((a) >= (b) ? (a) : (b))

#define RASTER3D_HAS_INDEX 1
#define RASTER3D_NO_INDEX 0

#define RASTER3D_USE_XDR 1	/* Only kept for backward compatibility */
#define RASTER3D_NO_XDR 0	/* Only kept for backward compatibility */
/* Only kept for backward compatibility */
#define RASTER3D_VALID_XDR_OPTION(o) (((o) == RASTER3D_USE_XDR) || ((o) == RASTER3D_NO_XDR))

/*---------------------------------------------------------------------------*/

/* global arrays */

extern void *tmpCompress;	/* compression support array */
extern int tmpCompressLength;	/* in bytes */
extern void *xdr;		/* xdr support array */
extern int xdrLength;		/* in bytes */

/*---------------------------------------------------------------------------*/

/* global variables */

extern int g3d_version; /* RASTER3D_MAP_VERSION */
extern int g3d_do_compression;	/* RASTER3D_NO_COMPRESSION or RASTER3D_COMPRESSION */
extern int g3d_precision;	/* RASTER3D_ALLOW_PRECISION or RASTER3D_NO_PRECISION */
extern int g3d_cache_default;	/* in number of tiles; 0 ==> no cache */
extern int g3d_cache_max;	/* in bytes */
extern int g3d_file_type;	/* FCELL_TYPE or DCELL_TYPE */
extern int g3d_tile_dimension[3];
extern void (*g3d_error_fun) (const char *);
extern char *g3d_unit_default;   /* The unit description of the map data */
extern int g3d_vertical_unit_default; /* spatial or temporal units from gis.h, U_METERS; ..., U_YEARS, ... */

extern RASTER3D_Region g3d_window;

/*---------------------------------------------------------------------------*/

extern void Rast3d_fatal_error(const char * /* msg */ , ...);
extern void Rast3d_fatal_error_noargs(const char * /* msg */ );

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

#define RASTER3D_REGION_NORTH "North"
#define RASTER3D_REGION_SOUTH "South"
#define RASTER3D_REGION_EAST "East"
#define RASTER3D_REGION_WEST "West"
#define RASTER3D_REGION_TOP "Top"
#define RASTER3D_REGION_BOTTOM "Bottom"
#define RASTER3D_REGION_ROWS "nofRows"
#define RASTER3D_REGION_COLS "nofCols"
#define RASTER3D_REGION_DEPTHS "nofDepths"
#define RASTER3D_REGION_PROJ "Proj"
#define RASTER3D_REGION_ZONE "Zone"
#define RASTER3D_REGION_EWRES "e-w resol"
#define RASTER3D_REGION_NSRES "n-s resol"
#define RASTER3D_REGION_TBRES "t-b resol"
