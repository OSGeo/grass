#include <rpc/types.h>
#include <rpc/xdr.h>

/*---------------------------------------------------------------------------*/

#include <grass/G3d.h>
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

#ifndef GRASS_G3D_H 
typedef struct {

  char *fileName;
  char *tempName;
  char *mapset;

  /* operation performed on map */
     int operation; /* G3D_WRITE_DATA or G3D_READ_DATA */

  /* region */
     G3D_Region region;

  /* window for map */
     G3D_Region window;

  /* resmapling function used for map. default is nearest neighbor */
     void (*resampleFun) ();

  /* units */
     char *unit;

  /* dimension of a single tile in "cells" */
     int tileX, tileY, tileZ;

  /* # of tiles in x, y, and z direction */
     int nx, ny, nz;

  /* data file specific information */

     /* file descriptor */
        int data_fd; /* file descriptor */

     /* type in which data is stored on file */
        int type; /* DCELL_TYPE or FCELL_TYPE */

     /* data concering the compression */
        int precision; /* G3D_MAX_PRECISION or, 0 .. 23 for float, 
		                                0 .. 52 for double */
        int compression; /* G3D_NO_COMPRESSION or G3D_USE_COMPRESSION */
        int useLzw; /* G3D_USE_LZW or G3D_NO_LZW */
        int useRle; /* G3D_USE_RLE or G3D_NO_RLE */
        int useXdr; /* G3D_USE_XDR or G3D_NO_XDR */
     
     /* pointer to first tile in file */
        int offset;

     /* pointer to the first index entry in file */
        long indexOffset;
  
     /* sizeof (long) of the system on which the file is/was written */
        int indexLongNbytes;

     /* max # bytes used in the representation of indices; this is equal to */
     /* # bytes used in the representation of "indexOffset" */
        int indexNbytesUsed;

     /* pointer to the last entry in the file */
        int fileEndPtr;
      
     /* indicates if index is stored in file; used for G3D_READ_DATA only */
        int hasIndex; /* G3D_HAS_INDEX or G3D_NO_INDEX */

  /* information concerning internal storage of data */

     /* index specific information */
        /* index[i] == the offset of tile "i" in the data file */
           long *index;

        /* tileLength[i] == # bytes used to store tile "i" */
           int *tileLength;

     /* tile specific information */

        /* type in which data is stored in memory */
           int typeIntern; /* DCELL_TYPE or FCELL_TYPE */

     /* in non-cache mode the "data" array is used to store one tile */
        char *data;
     
     /* index of tile currently stored in "data"; -1 if none */
        int currentIndex;

     /* cache related variables */

        int useCache;   /* 1 if cache is used */
        void *cache;    /* pointer to cache structure */
        int cacheFD;    /* file discriptor of cache file -- write mode only */
        char *cacheFileName; /* filename of cache file -- write mode only */
        long cachePosLast; /* position of last entry in cache file -- write */
                           /* mode only*/

  /* range info */
     struct FPRange range;

  /* some constants stored for efficiency */
        
     /* number of bytes required to store a single value of "type" */
        int numLengthExtern;
  
     /* number of bytes required to store a single value of "typeIntern" */
        int numLengthIntern;

     /* see header.c for details */
        int clipX, clipY, clipZ;
        int tileXY, tileSize;     
        int nxy, nTiles;

  /* mask related information */

     int useMask; /* 1 if mask is used; 0 otherwise */

} G3D_Map;
#endif
/*---------------------------------------------------------------------------*/

/* global arrays */

extern char *tmpCompress; /* compression support array */
extern int tmpCompressLength; /* in bytes */
extern char *xdr;/* xdr support array */
extern int xdrLength; /* in bytes */

/*---------------------------------------------------------------------------*/

/* global variables */

extern int g3d_do_compression; /* G3D_NO_COMPRESSION or G3D_COMPRESSION */
extern int g3d_do_lzw_compression; /* G3D_USE_LZW or G3D_NO_LZW */
extern int g3d_do_rle_compression; /* G3D_USE_RLE or G3D_NO_RLE */
extern int g3d_precision; /* G3D_ALLOW_PRECISION or G3D_NO_PRECISION */
extern int g3d_cache_default; /* in number of tiles; 0 ==> no cache */
extern int g3d_cache_max; /* in bytes */
extern int g3d_file_type; /* FCELL_TYPE or DCELL_TYPE */
extern int g3d_tile_dimension[3]; 
extern void (*g3d_error_fun)(const char *); 
extern char *g3d_unit_default;

extern G3D_Region g3d_window;

/*---------------------------------------------------------------------------*/

extern void G3d_fatalError (const char * /* msg */ , ...);
extern void G3d_fatalError_noargs (const char * /* msg */);

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




