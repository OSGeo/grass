#ifndef GRASS_RASTER3DDEFS_H
#define GRASS_RASTER3DDEFS_H

/* cache.c */
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

/* cachehash.c */
void G3d_cache_hash_reset(G3d_cache_hash *);
void G3d_cache_hash_dispose(G3d_cache_hash *);
void *G3d_cache_hash_new(int);
void G3d_cache_hash_remove_name(G3d_cache_hash *, int);
void G3d_cache_hash_load_name(G3d_cache_hash *, int, int);
int G3d_cache_hash_name2index(G3d_cache_hash *, int);

/* changeprecision.c */
void G3d_changePrecision(void *, int, const char *);

/* changetype.c */
void G3d_changeType(void *, const char *);

/* filecompare.c */
void G3d_compareFiles(const char *, const char *, const char *, const char *);

/* filename.c */
void G3d_filename(char *, const char *, const char *, const char *);

/* find_grid3.c */
char *G_find_grid3(const char *, const char *);

/* fpcompress.c */
void G_fpcompress_printBinary(char *, int);
void G_fpcompress_dissectXdrDouble(unsigned char *);
int G_fpcompress_writeXdrNums(int, char *, int, int, char *, int, int, int);
int G_fpcompress_writeXdrFloats(int, char *, int, int, char *, int, int);
int G_fpcompress_writeXdrDouble(int, char *, int, int, char *, int, int);
int G_fpcompress_readXdrNums(int, char *, int, int, int, char *, int);
int G_fpcompress_readXdrFloats(int, char *, int, int, int, char *);
int G_fpcompress_readXdrDoubles(int, char *, int, int, int, char *);

/* g3dalloc.c */
void *G3d_malloc(int);
void *G3d_realloc(void *, int);
void G3d_free(void *);

/* g3dcache.c */
int G3d_initCache(G3D_Map *, int);
int G3d_disposeCache(G3D_Map *);
int G3d_flushAllTiles(G3D_Map *);

/* g3dcats.c */
int G3d_writeCats(const char *, struct Categories *);
int G3d_readCats(const char *, const char *, struct Categories *);

/* g3dclose.c */
int G3d_closeCell(G3D_Map *);

/* g3dcolor.c */
int G3d_removeColor(const char *);
int G3d_readColors(const char *, const char *, struct Colors *);
int G3d_writeColors(const char *, const char *, struct Colors *);

/* g3ddefaults.c */
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

/* g3ddoubleio.c */
int G3d_writeDoubles(int, int, const double *, int);
int G3d_readDoubles(int, int, double *, int);

/* g3derror.c */
void G3d_skipError(const char *);
void G3d_printError(const char *);
void G3d_fatalError(const char *, ...) __attribute__ ((format(printf, 1, 2)))
    __attribute__ ((noreturn));
void G3d_fatalError_noargs(const char *) __attribute__ ((noreturn));
void G3d_error(const char *, ...) __attribute__ ((format(printf, 1, 2)));

/* g3dfpxdr.c */
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

/* g3dhistory.c */
int G3d_writeHistory(const char *, struct History *);
int G3d_readHistory(const char *, const char *, struct History *);

/* g3dintio.c */
int G3d_writeInts(int, int, const int *, int);
int G3d_readInts(int, int, int *, int);

/* g3dkeys.c */
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
/* g3dlong.c */
int G3d_longEncode(long *, unsigned char *, int);
void G3d_longDecode(unsigned char *, long *, int, int);

/* g3dmapset.c */
void G3d_makeMapsetMapDirectory(const char *);

/* g3dmask.c */
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

/* maskfn.c */
int G3d_mask_d_select(DCELL *, d_Mask *);
DCELL G3d_mask_match_d_interval(DCELL, d_Interval *);
void G3d_parse_vallist(char **, d_Mask **);

/* g3dmisc.c */
int G3d_g3dType2cellType(int);
void G3d_copyFloat2Double(const float *, int, double *, int, int);
void G3d_copyDouble2Float(const double *, int, float *, int, int);
void G3d_copyValues(const void *, int, int, void *, int, int, int);
int G3d_length(int);
int G3d_externLength(int);

/* g3dnull.c */
int G3d_isNullValueNum(const void *, int);
void G3d_setNullValue(void *, int, int);

/* g3dopen2.c */
void *G3d_openNewParam(const char *, int , int, G3D_Region *, int, int, int, int, int, int, int);
/* g3dopen.c */
void *G3d_openCellOldNoHeader(const char *, const char *);
void *G3d_openCellOld(const char *, const char *, G3D_Region *, int, int);
void *G3d_openCellNew(const char *, int, int, G3D_Region *);
void *G3d_openNewOptTileSize(const char *, int , G3D_Region * , int , int );

/* g3dparam.c */
void G3d_setStandard3dInputParams(void);
int G3d_getStandard3dParams(int *, int *, int *, int *, int *, int *, int *,
			    int *, int *, int *, int *, int *);
void G3d_setWindowParams(void);
char *G3d_getWindowParams(void);

/* g3drange.c */
void G3d_range_updateFromTile(G3D_Map *, const void *, int, int, int, int,
			      int, int, int, int);
int G3d_readRange(const char *, const char *, struct FPRange *);
int G3d_range_load(G3D_Map *);
void G3d_range_min_max(G3D_Map *, double *, double *);
int G3d_range_write(G3D_Map *);
int G3d_range_init(G3D_Map *);

/* g3dregion.c */
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
/* g3dresample.c */
void G3d_nearestNeighbor(G3D_Map *, int, int, int, void *, int);
void G3d_setResamplingFun(G3D_Map *, void (*)());
void G3d_getResamplingFun(G3D_Map *, void (**)());
void G3d_getNearestNeighborFunPtr(void (**)());

/* g3dvolume.c */
void G3d_getVolumeA(void *, double[2][2][2][3], int, int, int, void *, int);
void G3d_getVolume(void *, double, double, double, double, double, double,
		   double, double, double, double, double, double, int, int,
		   int, void *, int);
void G3d_getAlignedVolume(void *, double, double, double, double, double,
			  double, int, int, int, void *, int);
void G3d_makeAlignedVolumeFile(void *, const char *, double, double, double,
			       double, double, double, int, int, int);
/* g3dwindow.c */
void G3d_getValue(G3D_Map *, int, int, int, void *, int);
float G3d_getFloat(G3D_Map *, int, int, int);
double G3d_getDouble(G3D_Map *, int, int, int);
void G3d_getWindowValue(G3D_Map *, double, double, double, void *, int);


G3D_Region *G3d_windowPtr(void);
void G3d_setWindow(G3D_Region *);
void G3d_setWindowMap(G3D_Map *, G3D_Region *);
void G3d_getWindow(G3D_Region *);

/* g3dwindowio.c */
void G3d_useWindowParams(void);
int G3d_readWindow(G3D_Region *, const char *);

/* int G3d_writeWindow (G3D_Region *, char *); */
/* getblock.c */
void G3d_getBlockNocache(G3D_Map *, int, int, int, int, int, int, void *,
			 int);
void G3d_getBlock(G3D_Map *, int, int, int, int, int, int, void *, int);

/* header.c */
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
/* headerinfo.c */
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

/* index.c */
int G3d_flushIndex(G3D_Map *);
int G3d_initIndex(G3D_Map *, int);

/* retile.c */
void G3d_retile(void *, const char *, int, int, int);

/* rle.c */
int G_rle_count_only(char *, int, int);
void G_rle_encode(char *, char *, int, int);
void G_rle_decode(char *, char *, int, int, int *, int *);

/* tilealloc.c */
void *G3d_allocTilesType(G3D_Map *, int, int);
void *G3d_allocTiles(G3D_Map *, int);
void G3d_freeTiles(void *);

/* tileio.c */
void *G3d_getTilePtr(G3D_Map *, int);
int G3d_tileLoad(G3D_Map *, int);
int G3d__removeTile(G3D_Map *, int);
float G3d_getFloatRegion(G3D_Map *, int, int, int);
double G3d_getDoubleRegion(G3D_Map *, int, int, int);
void G3d_getValueRegion(G3D_Map *, int, int, int, void *, int);

/* tilemath.c */
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

/* tilenull.c */
void G3d_setNullTileType(G3D_Map *, void *, int);
void G3d_setNullTile(G3D_Map *, void *);

/* tileread.c */
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

/* tilewrite.c */
int G3d_writeTile(G3D_Map *, int, const void *, int);
int G3d_writeTileFloat(G3D_Map *, int, const void *);
int G3d_writeTileDouble(G3D_Map *, int, const void *);
int G3d_flushTile(G3D_Map *, int);
int G3d_flushTileCube(G3D_Map *, int, int, int, int, int, int);
int G3d_flushTilesInCube(G3D_Map *, int, int, int, int, int, int);
int G3d_putFloat(G3D_Map *, int, int, int, float);
int G3d_putDouble(G3D_Map *, int, int, int, double);
int G3d_putValue(G3D_Map *, int, int, int, const void *, int);

/* writeascii.c */
void G3d_writeAscii(void *, const char *);

#endif /* RASTER3DDEFS */
