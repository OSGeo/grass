#ifndef GRASS_RASTER3DDEFS_H
#define GRASS_RASTER3DDEFS_H

/* cache.c */
void Rast3d_cache_reset(RASTER3D_cache *);
void Rast3d_cache_dispose(RASTER3D_cache *);
void *Rast3d_cache_new(int, int, int, write_fn *, void *, read_fn *, void *);
void Rast3d_cache_set_removeFun(RASTER3D_cache *, write_fn *, void *);
void Rast3d_cache_set_loadFun(RASTER3D_cache *, read_fn *, void *);
void *Rast3d_cache_new_read(int, int, int, read_fn *, void *);
int Rast3d_cache_lock(RASTER3D_cache *, int);
void Rast3d_cache_lock_intern(RASTER3D_cache *, int);
int Rast3d_cache_unlock(RASTER3D_cache *, int);
int Rast3d_cache_unlock_all(RASTER3D_cache *);
int Rast3d_cache_lock_all(RASTER3D_cache *);
void Rast3d_cache_autolock_on(RASTER3D_cache *);
void Rast3d_cache_autolock_off(RASTER3D_cache *);
void Rast3d_cache_set_minUnlock(RASTER3D_cache *, int);
int Rast3d_cache_remove_elt(RASTER3D_cache *, int);
int Rast3d_cache_flush(RASTER3D_cache *, int);
int Rast3d_cache_remove_all(RASTER3D_cache *);
int Rast3d_cache_flush_all(RASTER3D_cache *);
void *Rast3d_cache_elt_ptr(RASTER3D_cache *, int);
int Rast3d_cache_load(RASTER3D_cache *, int);
int Rast3d_cache_get_elt(RASTER3D_cache *, int, void *);
int Rast3d_cache_put_elt(RASTER3D_cache *, int, const void *);

/* cachehash.c */
void Rast3d_cache_hash_reset(Rast3d_cache_hash *);
void Rast3d_cache_hash_dispose(Rast3d_cache_hash *);
void *Rast3d_cache_hash_new(int);
void Rast3d_cache_hash_remove_name(Rast3d_cache_hash *, int);
void Rast3d_cache_hash_load_name(Rast3d_cache_hash *, int, int);
int Rast3d_cache_hash_name2index(Rast3d_cache_hash *, int);

/* changeprecision.c */
void Rast3d_changePrecision(void *, int, const char *);

/* changetype.c */
void Rast3d_changeType(void *, const char *);

/* filecompare.c */
void Rast3d_compareFiles(const char *, const char *, const char *, const char *);

/* filename.c */
void Rast3d_filename(char *, const char *, const char *, const char *);

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
void *Rast3d_malloc(int);
void *Rast3d_realloc(void *, int);
void Rast3d_free(void *);

/* g3dcache.c */
int Rast3d_initCache(RASTER3D_Map *, int);
int Rast3d_disposeCache(RASTER3D_Map *);
int Rast3d_flushAllTiles(RASTER3D_Map *);

/* g3dcats.c */
int Rast3d_writeCats(const char *, struct Categories *);
int Rast3d_readCats(const char *, const char *, struct Categories *);

/* g3dclose.c */
int Rast3d_closeCell(RASTER3D_Map *);

/* g3dcolor.c */
int Rast3d_removeColor(const char *);
int Rast3d_readColors(const char *, const char *, struct Colors *);
int Rast3d_writeColors(const char *, const char *, struct Colors *);

/* g3ddefaults.c */
void Rast3d_setCompressionMode(int, int, int, int);
void Rast3d_getCompressionMode(int *, int *, int *, int *);
void Rast3d_setCacheSize(int);
int Rast3d_getCacheSize(void);
void Rast3d_setCacheLimit(int);
int Rast3d_getCacheLimit(void);
void Rast3d_setFileType(int);
int Rast3d_getFileType(void);
void Rast3d_setTileDimension(int, int, int);
void Rast3d_getTileDimension(int *, int *, int *);
void Rast3d_setErrorFun(void (*)(const char *));
void Rast3d_setUnit(const char *);
void Rast3d_initDefaults(void);

/* g3ddoubleio.c */
int Rast3d_writeDoubles(int, int, const double *, int);
int Rast3d_readDoubles(int, int, double *, int);

/* g3derror.c */
void Rast3d_skipError(const char *);
void Rast3d_printError(const char *);
void Rast3d_fatalError(const char *, ...) __attribute__ ((format(printf, 1, 2)))
    __attribute__ ((noreturn));
void Rast3d_fatalError_noargs(const char *) __attribute__ ((noreturn));
void Rast3d_error(const char *, ...) __attribute__ ((format(printf, 1, 2)));

/* g3dfpxdr.c */
int Rast3d_isXdrNullNum(const void *, int);
int Rast3d_isXdrNullFloat(const float *);
int Rast3d_isXdrNullDouble(const double *);
void Rast3d_setXdrNullNum(void *, int);
void Rast3d_setXdrNullDouble(double *);
void Rast3d_setXdrNullFloat(float *);
int Rast3d_initFpXdr(RASTER3D_Map *, int);
int Rast3d_initCopyToXdr(RASTER3D_Map *, int);
int Rast3d_copyToXdr(const void *, int);
int Rast3d_initCopyFromXdr(RASTER3D_Map *, int);
int Rast3d_copyFromXdr(int, void *);

/* g3dhistory.c */
int Rast3d_writeHistory(const char *, struct History *);
int Rast3d_readHistory(const char *, const char *, struct History *);

/* g3dintio.c */
int Rast3d_writeInts(int, int, const int *, int);
int Rast3d_readInts(int, int, int *, int);

/* g3dkeys.c */
int Rast3d_keyGetInt(struct Key_Value *, const char *, int *);
int Rast3d_keyGetDouble(struct Key_Value *, const char *, double *);
int Rast3d_keyGetString(struct Key_Value *, const char *, char **);
int Rast3d_keyGetValue(struct Key_Value *, const char *, char *, char *, int,
		    int, int *);
int Rast3d_keySetInt(struct Key_Value *, const char *, const int *);
int Rast3d_keySetDouble(struct Key_Value *, const char *, const double *);
int Rast3d_keySetString(struct Key_Value *, const char *, char *const *);
int Rast3d_keySetValue(struct Key_Value *, const char *, const char *,
		    const char *, int, int, const int *);
/* g3dlong.c */
int Rast3d_longEncode(long *, unsigned char *, int);
void Rast3d_longDecode(unsigned char *, long *, int, int);

/* g3dmapset.c */
void Rast3d_makeMapsetMapDirectory(const char *);

/* g3dmask.c */
int Rast3d_maskClose(void);
int Rast3d_maskFileExists(void);
int Rast3d_maskOpenOld(void);
int Rast3d_maskReopen(int);
int Rast3d_isMasked(RASTER3D_Map *, int, int, int);
void Rast3d_maskNum(RASTER3D_Map *, int, int, int, void *, int);
void Rast3d_maskFloat(RASTER3D_Map *, int, int, int, float *);
void Rast3d_maskDouble(RASTER3D_Map *, int, int, int, double *);
void Rast3d_maskTile(RASTER3D_Map *, int, void *, int);
void Rast3d_maskOn(RASTER3D_Map *);
void Rast3d_maskOff(RASTER3D_Map *);
int Rast3d_maskIsOn(RASTER3D_Map *);
int Rast3d_maskIsOff(RASTER3D_Map *);
const char *Rast3d_maskFile(void);
int Rast3d_maskMapExists(void);

/* maskfn.c */
int Rast3d_mask_d_select(DCELL *, d_Mask *);
DCELL Rast3d_mask_match_d_interval(DCELL, d_Interval *);
void Rast3d_parse_vallist(char **, d_Mask **);

/* g3dmisc.c */
int Rast3d_g3dType2cellType(int);
void Rast3d_copyFloat2Double(const float *, int, double *, int, int);
void Rast3d_copyDouble2Float(const double *, int, float *, int, int);
void Rast3d_copyValues(const void *, int, int, void *, int, int, int);
int Rast3d_length(int);
int Rast3d_externLength(int);

/* g3dnull.c */
int Rast3d_isNullValueNum(const void *, int);
void Rast3d_setNullValue(void *, int, int);

/* g3dopen2.c */
void *Rast3d_openNewParam(const char *, int , int, RASTER3D_Region *, int, int, int, int, int, int, int);
/* g3dopen.c */
void *Rast3d_openCellOldNoHeader(const char *, const char *);
void *Rast3d_openCellOld(const char *, const char *, RASTER3D_Region *, int, int);
void *Rast3d_openCellNew(const char *, int, int, RASTER3D_Region *);
void *Rast3d_openNewOptTileSize(const char *, int , RASTER3D_Region * , int , int );

/* g3dparam.c */
void Rast3d_setStandard3dInputParams(void);
int Rast3d_getStandard3dParams(int *, int *, int *, int *, int *, int *, int *,
			    int *, int *, int *, int *, int *);
void Rast3d_setWindowParams(void);
char *Rast3d_getWindowParams(void);

/* g3drange.c */
void Rast3d_range_updateFromTile(RASTER3D_Map *, const void *, int, int, int, int,
			      int, int, int, int);
int Rast3d_readRange(const char *, const char *, struct FPRange *);
int Rast3d_range_load(RASTER3D_Map *);
void Rast3d_range_min_max(RASTER3D_Map *, double *, double *);
int Rast3d_range_write(RASTER3D_Map *);
int Rast3d_range_init(RASTER3D_Map *);

/* g3dregion.c */
void Rast3d_getRegionValue(RASTER3D_Map *, double, double, double, void *, int);
void Rast3d_adjustRegion(RASTER3D_Region *);
void Rast3d_regionCopy(RASTER3D_Region *, RASTER3D_Region *);
void Rast3d_incorporate2dRegion(struct Cell_head *, RASTER3D_Region *);
void Rast3d_regionFromToCellHead(struct Cell_head *, RASTER3D_Region *);
void Rast3d_adjustRegionRes(RASTER3D_Region *);
void Rast3d_extract2dRegion(RASTER3D_Region *, struct Cell_head *);
void Rast3d_regionToCellHead(RASTER3D_Region *, struct Cell_head *);
int Rast3d_readRegionMap(const char *, const char *, RASTER3D_Region *);
int Rast3d_isValidLocation(RASTER3D_Region *, double, double, double);
void Rast3d_location2coord(RASTER3D_Region *, double, double, double, int *, int *, int *);
void Rast3d_location2coord2(RASTER3D_Region *, double, double, double, int *, int *, int *);
void Rast3d_coord2location(RASTER3D_Region *, double, double, double, double *, double *, double *);
/* g3dresample.c */
void Rast3d_nearestNeighbor(RASTER3D_Map *, int, int, int, void *, int);
void Rast3d_setResamplingFun(RASTER3D_Map *, void (*)());
void Rast3d_getResamplingFun(RASTER3D_Map *, void (**)());
void Rast3d_getNearestNeighborFunPtr(void (**)());

/* g3dvolume.c */
void Rast3d_getVolumeA(void *, double[2][2][2][3], int, int, int, void *, int);
void Rast3d_getVolume(void *, double, double, double, double, double, double,
		   double, double, double, double, double, double, int, int,
		   int, void *, int);
void Rast3d_getAlignedVolume(void *, double, double, double, double, double,
			  double, int, int, int, void *, int);
void Rast3d_makeAlignedVolumeFile(void *, const char *, double, double, double,
			       double, double, double, int, int, int);
/* g3dwindow.c */
void Rast3d_getValue(RASTER3D_Map *, int, int, int, void *, int);
float Rast3d_getFloat(RASTER3D_Map *, int, int, int);
double Rast3d_getDouble(RASTER3D_Map *, int, int, int);
void Rast3d_getWindowValue(RASTER3D_Map *, double, double, double, void *, int);


RASTER3D_Region *Rast3d_windowPtr(void);
void Rast3d_setWindow(RASTER3D_Region *);
void Rast3d_setWindowMap(RASTER3D_Map *, RASTER3D_Region *);
void Rast3d_getWindow(RASTER3D_Region *);

/* g3dwindowio.c */
void Rast3d_useWindowParams(void);
int Rast3d_readWindow(RASTER3D_Region *, const char *);

/* int Rast3d_writeWindow (RASTER3D_Region *, char *); */
/* getblock.c */
void Rast3d_getBlockNocache(RASTER3D_Map *, int, int, int, int, int, int, void *,
			 int);
void Rast3d_getBlock(RASTER3D_Map *, int, int, int, int, int, int, void *, int);

/* header.c */
int Rast3d_readHeader(RASTER3D_Map *, int *, int *, double *, double *, double *,
		   double *, double *, double *, int *, int *, int *,
		   double *, double *, double *, int *, int *, int *, int *,
		   int *, int *, int *, int *, int *, int *, int *, char **);
int Rast3d_writeHeader(RASTER3D_Map *, int, int, double, double, double, double,
		    double, double, int, int, int, double, double, double,
		    int, int, int, int, int, int, int, int, int, int, int,
		    char *);
int Rast3d_cacheSizeEncode(int, int);
int Rast3d__computeCacheSize(RASTER3D_Map *, int);
int Rast3d_fillHeader(RASTER3D_Map *, int, int, int, int, int, int, int, int, int,
		   int, int, int, int, int, int, int, double, double, double,
		   double, double, double, int, int, int, double, double,
		   double, char *);
/* headerinfo.c */
void Rast3d_getCoordsMap(RASTER3D_Map *, int *, int *, int *);
void Rast3d_getCoordsMapWindow(RASTER3D_Map *, int *, int *, int *);
void Rast3d_getNofTilesMap(RASTER3D_Map *, int *, int *, int *);
void Rast3d_getRegionMap(RASTER3D_Map *, double *, double *, double *, double *,
		      double *, double *);
void Rast3d_getWindowMap(RASTER3D_Map *, double *, double *, double *, double *,
		      double *, double *);
void Rast3d_getTileDimensionsMap(RASTER3D_Map *, int *, int *, int *);
int Rast3d_tileTypeMap(RASTER3D_Map *);
int Rast3d_fileTypeMap(RASTER3D_Map *);
int Rast3d_tilePrecisionMap(RASTER3D_Map *);
int Rast3d_tileUseCacheMap(RASTER3D_Map *);
void Rast3d_printHeader(RASTER3D_Map *);
void Rast3d_getRegionStructMap(RASTER3D_Map *, RASTER3D_Region *);

/* index.c */
int Rast3d_flushIndex(RASTER3D_Map *);
int Rast3d_initIndex(RASTER3D_Map *, int);

/* retile.c */
void Rast3d_retile(void *, const char *, int, int, int);

/* rle.c */
int G_rle_count_only(char *, int, int);
void G_rle_encode(char *, char *, int, int);
void G_rle_decode(char *, char *, int, int, int *, int *);

/* tilealloc.c */
void *Rast3d_allocTilesType(RASTER3D_Map *, int, int);
void *Rast3d_allocTiles(RASTER3D_Map *, int);
void Rast3d_freeTiles(void *);

/* tileio.c */
void *Rast3d_getTilePtr(RASTER3D_Map *, int);
int Rast3d_tileLoad(RASTER3D_Map *, int);
int Rast3d__removeTile(RASTER3D_Map *, int);
float Rast3d_getFloatRegion(RASTER3D_Map *, int, int, int);
double Rast3d_getDoubleRegion(RASTER3D_Map *, int, int, int);
void Rast3d_getValueRegion(RASTER3D_Map *, int, int, int, void *, int);

/* tilemath.c */
void Rast3d_computeOptimalTileDimension(RASTER3D_Region *, int, int *, int *, int *, int);
void Rast3d_tileIndex2tile(RASTER3D_Map *, int, int *, int *, int *);
int Rast3d_tile2tileIndex(RASTER3D_Map *, int, int, int);
void Rast3d_tileCoordOrigin(RASTER3D_Map *, int, int, int, int *, int *, int *);
void Rast3d_tileIndexOrigin(RASTER3D_Map *, int, int *, int *, int *);
void Rast3d_coord2tileCoord(RASTER3D_Map *, int, int, int, int *, int *, int *, int *,
			 int *, int *);
void Rast3d_coord2tileIndex(RASTER3D_Map *, int, int, int, int *, int *);
int Rast3d_coordInRange(RASTER3D_Map *, int, int, int);
int Rast3d_tileIndexInRange(RASTER3D_Map *, int);
int Rast3d_tileInRange(RASTER3D_Map *, int, int, int);
int Rast3d_computeClippedTileDimensions(RASTER3D_Map *, int, int *, int *, int *,
				     int *, int *, int *);

/* tilenull.c */
void Rast3d_setNullTileType(RASTER3D_Map *, void *, int);
void Rast3d_setNullTile(RASTER3D_Map *, void *);

/* tileread.c */
int Rast3d_readTile(RASTER3D_Map *, int, void *, int);
int Rast3d_readTileFloat(RASTER3D_Map *, int, void *);
int Rast3d_readTileDouble(RASTER3D_Map *, int, void *);
int Rast3d_lockTile(RASTER3D_Map *, int);
int Rast3d_unlockTile(RASTER3D_Map *, int);
int Rast3d_unlockAll(RASTER3D_Map *);
void Rast3d_autolockOn(RASTER3D_Map *);
void Rast3d_autolockOff(RASTER3D_Map *);
void Rast3d_minUnlocked(RASTER3D_Map *, int);
int Rast3d_beginCycle(RASTER3D_Map *);
int Rast3d_endCycle(RASTER3D_Map *);

/* tilewrite.c */
int Rast3d_writeTile(RASTER3D_Map *, int, const void *, int);
int Rast3d_writeTileFloat(RASTER3D_Map *, int, const void *);
int Rast3d_writeTileDouble(RASTER3D_Map *, int, const void *);
int Rast3d_flushTile(RASTER3D_Map *, int);
int Rast3d_flushTileCube(RASTER3D_Map *, int, int, int, int, int, int);
int Rast3d_flushTilesInCube(RASTER3D_Map *, int, int, int, int, int, int);
int Rast3d_putFloat(RASTER3D_Map *, int, int, int, float);
int Rast3d_putDouble(RASTER3D_Map *, int, int, int, double);
int Rast3d_putValue(RASTER3D_Map *, int, int, int, const void *, int);

/* writeascii.c */
void Rast3d_writeAscii(void *, const char *);

#endif /* RASTER3DDEFS */
