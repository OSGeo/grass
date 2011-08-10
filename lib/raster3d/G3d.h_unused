#define G3D_FLOAT 0
#define G3D_DOUBLE 1
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
#define G3D_COLOR_ELEMENT  "color"
#define G3D_COLOR2_DIRECTORY  "colr2"
#define G3D_MASK_MAP       "G3D_MASK"
#define G3D_WINDOW_ELEMENT   "WIND3"
#define G3D_DEFAULT_WINDOW_ELEMENT   "DEFAULT_WIND3"
#define G3D_WINDOW_DATABASE "windows3d"
#define G3D_PERMANENT_MAPSET "PERMANENT"

/*---------------------------------------------------------------------------*/

typedef struct {

  double north, south;
  double east, west;
  double top, bottom;

  /* dimension of data in "cells"; rows == #x; cols == #y; depths == #z */
  int rows, cols, depths;

  double ns_res, ew_res, tb_res;

  int proj;  /* Projection (see gis.h) */
  int zone;  /* Projection zone (see gis.h) */

} G3D_Region;

/*---------------------------------------------------------------------------*/

extern void *G3d_openCellOldNoHeader (/* name */);

extern void *G3d_openCellOld (/* name, cache */);
extern void *G3d_openCellOld_NEW (/* name, cache */);
extern void *G3d_openCellNew (/* name, cache, compression, type,
                                 tileX, tileY, tileZ,
                                 xMin, yMin, zMin, xMax, yMax, zMax */);
extern void *G3d_openGrid3File (/* name, x, y, z */);
extern void *G3d_malloc (/* nBytes */);
extern void *G3d_realloc (/* ptr, nBytes */);
extern char *G3d_allocTiles (/* map, nofTiles */);
extern char *G3d_allocTilesType (/* map, nofTiles, type */);
extern char *G3d_getTilePtr (/* map, tileIndex */);

extern float G3d_getFloat (/* map, x, y, z */);
extern double G3d_getDouble (/* map, x, y, z */);
extern float G3d_getFloatRegion (/* map, x, y, z */);
extern double G3d_getDoubleRegion (/* map, x, y, z */);

extern void *G3d_cache_new ();
extern void *G3d_cache_new_read ();
extern char *G3d_cache_elt_ptr (/* c, name */);

extern void *G3d_cache_hash_new (/* nofNames */);

extern void G3d_skipError (/* msg */);
extern void G3d_printError (/* msg */);
/*extern void *G3d_maskFile ();
*/

extern void G3d_setWindow (/* window */);
/*extern void G3d_getWindow ();*/
extern G3D_Region *G3d_windowPtr ();

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
