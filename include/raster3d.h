#ifndef GRASS_RASTER3D_H
#define GRASS_RASTER3D_H

#include <grass/gis.h>
#include <grass/raster.h>

#define G3D_TILE_SAME_AS_FILE 2

#define G3D_NO_COMPRESSION 0
#define G3D_COMPRESSION 1

#define G3D_USE_LZW 1
#define G3D_NO_LZW 0

#define G3D_USE_RLE 1
#define G3D_NO_RLE 0

#define G3D_MAX_PRECISION -1

#define G3D_NO_CACHE 0
#define G3D_USE_CACHE_DEFAULT -1
#define G3D_USE_CACHE_X -2
#define G3D_USE_CACHE_Y -3
#define G3D_USE_CACHE_Z -4
#define G3D_USE_CACHE_XY -5
#define G3D_USE_CACHE_XZ -6
#define G3D_USE_CACHE_YZ -7
#define G3D_USE_CACHE_XYZ -8

#define G3D_DEFAULT_WINDOW NULL

#define G3D_DIRECTORY      "grid3"
#define G3D_CELL_ELEMENT   "cell"
#define G3D_CATS_ELEMENT   "cats"
#define G3D_RANGE_ELEMENT  "range"
#define G3D_HEADER_ELEMENT "cellhd"
#define G3D_HISTORY_ELEMENT "hist"
#define G3D_COLOR_ELEMENT  "color"
#define G3D_COLOR2_DIRECTORY  "colr2"
#define G3D_MASK_MAP       "G3D_MASK"
#define G3D_WINDOW_ELEMENT   "WIND3"
#define G3D_DEFAULT_WINDOW_ELEMENT   "DEFAULT_WIND3"
#define G3D_WINDOW_DATABASE "windows3d"
#define G3D_PERMANENT_MAPSET "PERMANENT"

/*---------------------------------------------------------------------------*/

typedef struct
{

    double north, south;
    double east, west;
    double top, bottom;

    /* dimension of data in "cells"; rows == #x; cols == #y; depths == #z */
    int rows, cols, depths;

    double ns_res, ew_res, tb_res;

    int proj;			/* Projection (see gis.h) */
    int zone;			/* Projection zone (see gis.h) */

} G3D_Region;

/*---------------------------------------------------------------------------*/

struct G3D_Map;

typedef void resample_fn(struct G3D_Map *, int, int, int, void *, int);

/*---------------------------------------------------------------------------*/

typedef struct G3D_Map
{

    char *fileName;
    char *tempName;
    char *mapset;

    /* operation performed on map */
    int operation;		/* G3D_WRITE_DATA or G3D_READ_DATA */

    /* region */
    G3D_Region region;

    /* window for map */
    G3D_Region window;

    /* resmapling function used for map. default is nearest neighbor */
    resample_fn *resampleFun;

    /* units */
    char *unit;

    /* dimension of a single tile in "cells" */
    int tileX, tileY, tileZ;

    /* # of tiles in x, y, and z direction */
    int nx, ny, nz;

    /* data file specific information */

    /* file descriptor */
    int data_fd;		/* file descriptor */

    /* type in which data is stored on file */
    int type;			/* DCELL_TYPE or FCELL_TYPE */

    /* data concering the compression */
    int precision;		/* G3D_MAX_PRECISION or, 0 .. 23 for float, 
				   0 .. 52 for double */
    int compression;		/* G3D_NO_COMPRESSION or G3D_USE_COMPRESSION */
    int useLzw;			/* G3D_USE_LZW or G3D_NO_LZW */
    int useRle;			/* G3D_USE_RLE or G3D_NO_RLE */
    int useXdr;			/* G3D_USE_XDR or G3D_NO_XDR */

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
    int hasIndex;		/* G3D_HAS_INDEX or G3D_NO_INDEX */

    /* information concerning internal storage of data */

    /* index specific information */
    /* index[i] == the offset of tile "i" in the data file */
    long *index;

    /* tileLength[i] == # bytes used to store tile "i" */
    int *tileLength;

    /* tile specific information */

    /* type in which data is stored in memory */
    int typeIntern;		/* DCELL_TYPE or FCELL_TYPE */

    /* in non-cache mode the "data" array is used to store one tile */
    char *data;

    /* index of tile currently stored in "data"; -1 if none */
    int currentIndex;

    /* cache related variables */

    int useCache;		/* 1 if cache is used */
    void *cache;		/* pointer to cache structure */
    int cacheFD;		/* file discriptor of cache file -- write mode only */
    char *cacheFileName;	/* filename of cache file -- write mode only */
    long cachePosLast;		/* position of last entry in cache file -- write */
    /* mode only */

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

    int useMask;		/* 1 if mask is used; 0 otherwise */

} G3D_Map;

/*---------------------------------------------------------------------------*/

typedef struct
{

    char *elts;			/* ptr to array of elts */
    int nofElts;		/* size of "elts" */
    int eltSize;		/* size of elt in "elts" */

    int *names;			/* name[i] is the name of elts[i] */

    char *locks;		/* lock[i] == 1 iff elts[i] is locked
				   lock[i] == 0 iff elts[i] is unlocked but active
				   lock[i] == 2 iff elts[i] doesn't contain valid data */
    int autoLock;		/* 1 if auto locking is turned on */
    int nofUnlocked;		/* nof tiles which are unlocked */
    int minUnlocked;		/* min nof elts which have to remain unlocked. min = 1 */

    int *next, *prev;		/* prev/next pointers for fifo */
    int first, last;		/* index (into next) of first and last elt in fifo */
    /* first == -1 iff fifo is empty */

    int (*eltRemoveFun) ();	/* callback activated if the contents of an 
				   elt needs to be removed */
    void *eltRemoveFunData;	/* pointer to user data passed along with 
				   eltRemoveFun */
    int (*eltLoadFun) ();	/* callback activated to load contents of an elt */
    void *eltLoadFunData;	/* pointer to user data passed along with 
				   eltLoadFun */

    void *hash;			/* ptr to hashTable used to relate external names to
				   internal indices (elts) */

} G3D_cache;

/*---------------------------------------------------------------------------*/

typedef struct
{

    int nofNames;
    int *index;
    char *active;
    int lastName;
    int lastIndex;
    int lastIndexActive;

} G3d_cache_hash;

/*---------------------------------------------------------------------------*/

/*Structures */
typedef struct _d_interval
{
    double low, high;
    int inf;
    struct _d_interval *next;
} d_Interval;

typedef struct _d_mask
{
    d_Interval *list;
} d_Mask;

/*---------------------------------------------------------------------------*/

typedef int write_fn(int, const void *, void *);
typedef int read_fn(int, void *, void *);

/*---------------------------------------------------------------------------*/

/*============================== Prototypes ================================*/

#include <grass/raster3ddefs.h>

#endif /* #ifndef GRASS_RASTER3D_H */
