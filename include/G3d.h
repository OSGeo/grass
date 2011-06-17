#ifndef GRASS_G3D_H
#define GRASS_G3D_H

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

typedef int write_fn(int, const void *, void *);
typedef int read_fn(int, void *, void *);

/*---------------------------------------------------------------------------*/

/* grass/src/libes/g3d/cache.c */
void G3d_cache_reset(G3D_cache *);
void G3d_cache_dispose(G3D_cache *);
void *G3d_cache_new(int, int, int, write_fn *, void *, read_fn *, void *);
void G3d_cache_set_removeFun(G3D_cache *, write_fn *, void *);
void G3d_cache_set_loadFun(G3D_cache *, read_fn *, void *);
void *G3d_cache_new_read(int, int, int, read_fn *, void *);
int G3d_cache_lock(G3D_cache *, int);
void G3d_cache_lock_intern(G3D_cache *, int);
int G3d_cache_unlock(G3D_cache *, int);
int G3d_cache_unlock_all(G3D_cache *);
int G3d_cache_lock_all(G3D_cache *);
void G3d_cache_autolock_on(G3D_cache *);
void G3d_cache_autolock_off(G3D_cache *);
void G3d_cache_set_minUnlock(G3D_cache *, int);
int G3d_cache_remove_elt(G3D_cache *, int);
int G3d_cache_flush(G3D_cache *, int);
int G3d_cache_remove_all(G3D_cache *);
int G3d_cache_flush_all(G3D_cache *);
void *G3d_cache_elt_ptr(G3D_cache *, int);
int G3d_cache_load(G3D_cache *, int);
int G3d_cache_get_elt(G3D_cache *, int, void *);
int G3d_cache_put_elt(G3D_cache *, int, const void *);

/* grass/src/libes/g3d/cachehash.c */
void G3d_cache_hash_reset(G3d_cache_hash *);
void G3d_cache_hash_dispose(G3d_cache_hash *);
void *G3d_cache_hash_new(int);
void G3d_cache_hash_remove_name(G3d_cache_hash *, int);
void G3d_cache_hash_load_name(G3d_cache_hash *, int, int);
int G3d_cache_hash_name2index(G3d_cache_hash *, int);

/* grass/src/libes/g3d/changeprecision.c */
void G3d_changePrecision(void *, int, const char *);

/* grass/src/libes/g3d/changetype.c */
void G3d_changeType(void *, const char *);

/* grass/src/libes/g3d/filecompare.c */
void G3d_compareFiles(const char *, const char *, const char *, const char *);

/* grass/src/libes/g3d/filename.c */
void G3d_filename(char *, const char *, const char *, const char *);

/* grass/src/libes/g3d/find_grid3.c */
char *G_find_grid3(const char *, const char *);

/* grass/src/libes/g3d/fpcompress.c */
void G_fpcompress_printBinary(char *, int);
void G_fpcompress_dissectXdrDouble(unsigned char *);
int G_fpcompress_writeXdrNums(int, char *, int, int, char *, int, int, int);
int G_fpcompress_writeXdrFloats(int, char *, int, int, char *, int, int);
int G_fpcompress_writeXdrDouble(int, char *, int, int, char *, int, int);
int G_fpcompress_readXdrNums(int, char *, int, int, int, char *, int);
int G_fpcompress_readXdrFloats(int, char *, int, int, int, char *);
int G_fpcompress_readXdrDoubles(int, char *, int, int, int, char *);

/* grass/src/libes/g3d/g3dalloc.c */
void *G3d_malloc(int);
void *G3d_realloc(void *, int);
void G3d_free(void *);

/* grass/src/libes/g3d/g3dcache.c */
int G3d_initCache(G3D_Map *, int);
int G3d_disposeCache(G3D_Map *);
int G3d_flushAllTiles(G3D_Map *);

/* grass/src/libes/g3d/g3dcats.c */
int G3d_writeCats(const char *, struct Categories *);
int G3d_readCats(const char *, const char *, struct Categories *);

/* grass/src/libes/g3d/g3dclose.c */
int G3d_closeCell(G3D_Map *);

/* grass/src/libes/g3d/g3dcolor.c */
int G3d_removeColor(const char *);
int G3d_readColors(const char *, const char *, struct Colors *);
int G3d_writeColors(const char *, const char *, struct Colors *);

/* grass/src/libes/g3d/g3ddefaults.c */
void G3d_setCompressionMode(int, int, int, int);
void G3d_getCompressionMode(int *, int *, int *, int *);
void G3d_setCacheSize(int);
int G3d_getCacheSize(void);
void G3d_setCacheLimit(int);
int G3d_getCacheLimit(void);
void G3d_setFileType(int);
int G3d_getFileType(void);
void G3d_setTileDimension(int, int, int);
void G3d_getTileDimension(int *, int *, int *);
void G3d_setErrorFun(void (*)(const char *));
void G3d_setUnit(const char *);
void G3d_initDefaults(void);

/* grass/src/libes/g3d/g3ddoubleio.c */
int G3d_writeDoubles(int, int, const double *, int);
int G3d_readDoubles(int, int, double *, int);

/* grass/src/libes/g3d/g3derror.c */
void G3d_skipError(const char *);
void G3d_printError(const char *);
void G3d_fatalError(const char *, ...) __attribute__ ((format(printf, 1, 2)))
    __attribute__ ((noreturn));
void G3d_fatalError_noargs(const char *) __attribute__ ((noreturn));
void G3d_error(const char *, ...) __attribute__ ((format(printf, 1, 2)));

/* grass/src/libes/g3d/g3dfpxdr.c */
int G3d_isXdrNullNum(const void *, int);
int G3d_isXdrNullFloat(const float *);
int G3d_isXdrNullDouble(const double *);
void G3d_setXdrNullNum(void *, int);
void G3d_setXdrNullDouble(double *);
void G3d_setXdrNullFloat(float *);
int G3d_initFpXdr(G3D_Map *, int);
int G3d_initCopyToXdr(G3D_Map *, int);
int G3d_copyToXdr(const void *, int);
int G3d_initCopyFromXdr(G3D_Map *, int);
int G3d_copyFromXdr(int, void *);

/* grass/src/libes/g3d/g3dhistory.c */
int G3d_writeHistory(const char *, struct History *);
int G3d_readHistory(const char *, const char *, struct History *);

/* grass/src/libes/g3d/g3dintio.c */
int G3d_writeInts(int, int, const int *, int);
int G3d_readInts(int, int, int *, int);

/* grass/src/libes/g3d/g3dkeys.c */
int G3d_keyGetInt(struct Key_Value *, const char *, int *);
int G3d_keyGetDouble(struct Key_Value *, const char *, double *);
int G3d_keyGetString(struct Key_Value *, const char *, char **);
int G3d_keyGetValue(struct Key_Value *, const char *, char *, char *, int,
		    int, int *);
int G3d_keySetInt(struct Key_Value *, const char *, const int *);
int G3d_keySetDouble(struct Key_Value *, const char *, const double *);
int G3d_keySetString(struct Key_Value *, const char *, char *const *);
int G3d_keySetValue(struct Key_Value *, const char *, const char *,
		    const char *, int, int, const int *);
/* grass/src/libes/g3d/g3dlong.c */
int G3d_longEncode(long *, unsigned char *, int);
void G3d_longDecode(unsigned char *, long *, int, int);

/* grass/src/libes/g3d/g3dmapset.c */
void G3d_makeMapsetMapDirectory(const char *);

/* grass/src/libes/g3d/g3dmask.c */
int G3d_maskClose(void);
int G3d_maskFileExists(void);
int G3d_maskOpenOld(void);
int G3d_maskReopen(int);
int G3d_isMasked(G3D_Map *, int, int, int);
void G3d_maskNum(G3D_Map *, int, int, int, void *, int);
void G3d_maskFloat(G3D_Map *, int, int, int, float *);
void G3d_maskDouble(G3D_Map *, int, int, int, double *);
void G3d_maskTile(G3D_Map *, int, void *, int);
void G3d_maskOn(G3D_Map *);
void G3d_maskOff(G3D_Map *);
int G3d_maskIsOn(G3D_Map *);
int G3d_maskIsOff(G3D_Map *);
const char *G3d_maskFile(void);
int G3d_maskMapExists(void);

/* grass/src/libes/g3d/g3dmisc.c */
int G3d_g3dType2cellType(int);
void G3d_copyFloat2Double(const float *, int, double *, int, int);
void G3d_copyDouble2Float(const double *, int, float *, int, int);
void G3d_copyValues(const void *, int, int, void *, int, int, int);
int G3d_length(int);
int G3d_externLength(int);

/* grass/src/libes/g3d/g3dnull.c */
int G3d_isNullValueNum(const void *, int);
void G3d_setNullValue(void *, int, int);

/* grass/src/libes/g3d/g3dopen2.c */
void *G3d_openNewParam(const char *, int , int, G3D_Region *, int, int, int, int, int, int, int);
/* grass/src/libes/g3d/g3dopen.c */
void *G3d_openCellOldNoHeader(const char *, const char *);
void *G3d_openCellOld(const char *, const char *, G3D_Region *, int, int);
void *G3d_openCellNew(const char *, int, int, G3D_Region *);
void *G3d_openNewOptTileSize(const char *, int , G3D_Region * , int , int );

/* grass/src/libes/g3d/g3dparam.c */
void G3d_setStandard3dInputParams(void);
int G3d_getStandard3dParams(int *, int *, int *, int *, int *, int *, int *,
			    int *, int *, int *, int *, int *);
void G3d_setWindowParams(void);
char *G3d_getWindowParams(void);

/* grass/src/libes/g3d/g3drange.c */
void G3d_range_updateFromTile(G3D_Map *, const void *, int, int, int, int,
			      int, int, int, int);
int G3d_readRange(const char *, const char *, struct FPRange *);
int G3d_range_load(G3D_Map *);
void G3d_range_min_max(G3D_Map *, double *, double *);
int G3d_range_write(G3D_Map *);
int G3d_range_init(G3D_Map *);

/* grass/src/libes/g3d/g3dregion.c */
void G3d_getRegionValue(G3D_Map *, double, double, double, void *, int);
void G3d_adjustRegion(G3D_Region *);
void G3d_regionCopy(G3D_Region *, G3D_Region *);
void G3d_incorporate2dRegion(struct Cell_head *, G3D_Region *);
void G3d_regionFromToCellHead(struct Cell_head *, G3D_Region *);
void G3d_adjustRegionRes(G3D_Region *);
void G3d_extract2dRegion(G3D_Region *, struct Cell_head *);
void G3d_regionToCellHead(G3D_Region *, struct Cell_head *);
int G3d_readRegionMap(const char *, const char *, G3D_Region *);
int G3d_isValidLocation(G3D_Region *, double, double, double);
void G3d_location2coord(G3D_Region *, double, double, double, int *, int *, int *);
void G3d_location2coord2(G3D_Region *, double, double, double, int *, int *, int *);
void G3d_coord2location(G3D_Region *, double, double, double, double *, double *, double *);
/* grass/src/libes/g3d/g3dresample.c */
void G3d_nearestNeighbor(G3D_Map *, int, int, int, void *, int);
void G3d_setResamplingFun(G3D_Map *, void (*)());
void G3d_getResamplingFun(G3D_Map *, void (**)());
void G3d_getNearestNeighborFunPtr(void (**)());

/* grass/src/libes/g3d/g3dvolume.c */
void G3d_getVolumeA(void *, double[2][2][2][3], int, int, int, void *, int);
void G3d_getVolume(void *, double, double, double, double, double, double,
		   double, double, double, double, double, double, int, int,
		   int, void *, int);
void G3d_getAlignedVolume(void *, double, double, double, double, double,
			  double, int, int, int, void *, int);
void G3d_makeAlignedVolumeFile(void *, const char *, double, double, double,
			       double, double, double, int, int, int);
/* grass/src/libes/g3d/g3dwindow.c */
void G3d_getValue(G3D_Map *, int, int, int, void *, int);
float G3d_getFloat(G3D_Map *, int, int, int);
double G3d_getDouble(G3D_Map *, int, int, int);
void G3d_getWindowValue(G3D_Map *, double, double, double, void *, int);


G3D_Region *G3d_windowPtr(void);
void G3d_setWindow(G3D_Region *);
void G3d_setWindowMap(G3D_Map *, G3D_Region *);
void G3d_getWindow(G3D_Region *);

/* grass/src/libes/g3d/g3dwindowio.c */
void G3d_useWindowParams(void);
int G3d_readWindow(G3D_Region *, const char *);

/* int G3d_writeWindow (G3D_Region *, char *); */
/* grass/src/libes/g3d/getblock.c */
void G3d_getBlockNocache(G3D_Map *, int, int, int, int, int, int, void *,
			 int);
void G3d_getBlock(G3D_Map *, int, int, int, int, int, int, void *, int);

/* grass/src/libes/g3d/header.c */
int G3d_readHeader(G3D_Map *, int *, int *, double *, double *, double *,
		   double *, double *, double *, int *, int *, int *,
		   double *, double *, double *, int *, int *, int *, int *,
		   int *, int *, int *, int *, int *, int *, int *, char **);
int G3d_writeHeader(G3D_Map *, int, int, double, double, double, double,
		    double, double, int, int, int, double, double, double,
		    int, int, int, int, int, int, int, int, int, int, int,
		    char *);
int G3d_cacheSizeEncode(int, int);
int G3d__computeCacheSize(G3D_Map *, int);
int G3d_fillHeader(G3D_Map *, int, int, int, int, int, int, int, int, int,
		   int, int, int, int, int, int, int, double, double, double,
		   double, double, double, int, int, int, double, double,
		   double, char *);
/* grass/src/libes/g3d/headerinfo.c */
void G3d_getCoordsMap(G3D_Map *, int *, int *, int *);
void G3d_getCoordsMapWindow(G3D_Map *, int *, int *, int *);
void G3d_getNofTilesMap(G3D_Map *, int *, int *, int *);
void G3d_getRegionMap(G3D_Map *, double *, double *, double *, double *,
		      double *, double *);
void G3d_getWindowMap(G3D_Map *, double *, double *, double *, double *,
		      double *, double *);
void G3d_getTileDimensionsMap(G3D_Map *, int *, int *, int *);
int G3d_tileTypeMap(G3D_Map *);
int G3d_fileTypeMap(G3D_Map *);
int G3d_tilePrecisionMap(G3D_Map *);
int G3d_tileUseCacheMap(G3D_Map *);
void G3d_printHeader(G3D_Map *);
void G3d_getRegionStructMap(G3D_Map *, G3D_Region *);

/* grass/src/libes/g3d/index.c */
int G3d_flushIndex(G3D_Map *);
int G3d_initIndex(G3D_Map *, int);

/* grass/src/libes/g3d/retile.c */
void G3d_retile(void *, const char *, int, int, int);

/* grass/src/libes/g3d/rle.c */
int G_rle_count_only(char *, int, int);
void G_rle_encode(char *, char *, int, int);
void G_rle_decode(char *, char *, int, int, int *, int *);

/* grass/src/libes/g3d/tilealloc.c */
void *G3d_allocTilesType(G3D_Map *, int, int);
void *G3d_allocTiles(G3D_Map *, int);
void G3d_freeTiles(void *);

/* grass/src/libes/g3d/tileio.c */
void *G3d_getTilePtr(G3D_Map *, int);
int G3d_tileLoad(G3D_Map *, int);
int G3d__removeTile(G3D_Map *, int);
float G3d_getFloatRegion(G3D_Map *, int, int, int);
double G3d_getDoubleRegion(G3D_Map *, int, int, int);
void G3d_getValueRegion(G3D_Map *, int, int, int, void *, int);

/* grass/src/libes/g3d/tilemath.c */
void G3d_computeOptimalTileDimension(G3D_Region *, int, int *, int *, int *, int);
void G3d_tileIndex2tile(G3D_Map *, int, int *, int *, int *);
int G3d_tile2tileIndex(G3D_Map *, int, int, int);
void G3d_tileCoordOrigin(G3D_Map *, int, int, int, int *, int *, int *);
void G3d_tileIndexOrigin(G3D_Map *, int, int *, int *, int *);
void G3d_coord2tileCoord(G3D_Map *, int, int, int, int *, int *, int *, int *,
			 int *, int *);
void G3d_coord2tileIndex(G3D_Map *, int, int, int, int *, int *);
int G3d_coordInRange(G3D_Map *, int, int, int);
int G3d_tileIndexInRange(G3D_Map *, int);
int G3d_tileInRange(G3D_Map *, int, int, int);
int G3d_computeClippedTileDimensions(G3D_Map *, int, int *, int *, int *,
				     int *, int *, int *);

/* grass/src/libes/g3d/tilenull.c */
void G3d_setNullTileType(G3D_Map *, void *, int);
void G3d_setNullTile(G3D_Map *, void *);

/* grass/src/libes/g3d/tileread.c */
int G3d_readTile(G3D_Map *, int, void *, int);
int G3d_readTileFloat(G3D_Map *, int, void *);
int G3d_readTileDouble(G3D_Map *, int, void *);
int G3d_lockTile(G3D_Map *, int);
int G3d_unlockTile(G3D_Map *, int);
int G3d_unlockAll(G3D_Map *);
void G3d_autolockOn(G3D_Map *);
void G3d_autolockOff(G3D_Map *);
void G3d_minUnlocked(G3D_Map *, int);
int G3d_beginCycle(G3D_Map *);
int G3d_endCycle(G3D_Map *);

/* grass/src/libes/g3d/tilewrite.c */
int G3d_writeTile(G3D_Map *, int, const void *, int);
int G3d_writeTileFloat(G3D_Map *, int, const void *);
int G3d_writeTileDouble(G3D_Map *, int, const void *);
int G3d_flushTile(G3D_Map *, int);
int G3d_flushTileCube(G3D_Map *, int, int, int, int, int, int);
int G3d_flushTilesInCube(G3D_Map *, int, int, int, int, int, int);
int G3d_putFloat(G3D_Map *, int, int, int, float);
int G3d_putDouble(G3D_Map *, int, int, int, double);
int G3d_putValue(G3D_Map *, int, int, int, const void *, int);

/* grass/src/libes/g3d/writeascii.c */
void G3d_writeAscii(void *, const char *);

#endif /* #ifndef GRASS_G3D_H */
