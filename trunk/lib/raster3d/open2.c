#include <grass/gis.h>
#include <grass/raster3d.h>

/*----------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Opens new g3d-file with <em>name</em> in the current mapset. Tiles
 * are stored in memory with <em>typeIntern</em> which must be one of FCELL_TYPE,
 * DCELL_TYPE, or RASTER3D_TILE_SAME_AS_FILE. <em>cache</em> specifies the
 * cache-mode used and must be either RASTER3D_NO_CACHE, RASTER3D_USE_CACHE_DEFAULT,
 * RASTER3D_USE_CACHE_X, RASTER3D_USE_CACHE_Y, RASTER3D_USE_CACHE_Z,
 * RASTER3D_USE_CACHE_XY, RASTER3D_USE_CACHE_XZ, RASTER3D_USE_CACHE_YZ,
 * RASTER3D_USE_CACHE_XYZ, the result of <tt>Rast3d_cache_size_encode ()</tt>
 * (cf.{g3d:G3d.cacheSizeEncode}), or any positive integer which
 * specifies the number of tiles buffered in the cache.  <em>region</em> specifies
 * the 3d region.  
 * The map is created using the <em>type</em> which must be of FCELL_TYPE or DCELL_TYPE.
 * The methods for compression can be specified LZW or RLE. The digits of the floating point mantissa
 * can be specified. In case of FCELL_TYPE 0-23 and 0-52 in case of DCELL_TYPE. 
 * The number cells in X, Y and Z direction defines the size of each tile.
 * Returns a pointer to the cell structure ... if successful,
 * NULL ... otherwise.
 *
 *  \param name The name of the map
 *  \param typeIntern The internal storage type for in memory chached tiles
 *  \param cache The type of the caching
 *  \param region The region of the map
 *  \param type The type of the map (FCELL_TYPE, or DCELL_TYPE)
 *  \param doLzw Use the LZW compression algorithm  
 *  \param doRle Use the Run-Length-Encoding algroithm for compression
 *  \param precision The precision used for the mantissa (0 - 52) or RASTER3D_MAX_PRECISION
 *  \param tileX The number of cells in X direction of a tile
 *  \param tileY The number of cells in Y direction of a tile
 *  \param tileZ The number of cells in Z direction of a tile
 *  \return void * 
 */


void *Rast3d_open_new_param(const char *name, int typeIntern, int cache,
		       RASTER3D_Region * region, int type, int compression,
		       int precision, int tileX, int tileY, int tileZ)
{
    void *map;
    int oldCompress, oldPrecision, oldTileX, oldTileY,
	oldTileZ;
    int oldType;

    Rast3d_init_defaults();

    Rast3d_get_compression_mode(&oldCompress, &oldPrecision);
    Rast3d_set_compression_mode(compression, precision);

    Rast3d_get_tile_dimension(&oldTileX, &oldTileY, &oldTileZ);
    Rast3d_set_tile_dimension(tileX, tileY, tileZ);

    oldType = Rast3d_get_file_type();
    Rast3d_set_file_type(type);

    map = Rast3d_open_cell_new(name, typeIntern, cache, region);

    Rast3d_set_compression_mode(oldCompress, oldPrecision);
    Rast3d_set_tile_dimension(oldTileX, oldTileY, oldTileZ);
    Rast3d_set_file_type(oldType);

    return map;
}

/*----------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Opens new g3d-file with <em>name</em> in the current mapset. This method tries to compute 
 * optimal tile size based on the number of rows, cols and depths and the maximum allowed tile size in KB. 
 * Tiles are stored in memory using RASTER3D_TILE_SAME_AS_FILE method. <em>cache</em> specifies the
 * cache-mode used and must be either RASTER3D_NO_CACHE, RASTER3D_USE_CACHE_DEFAULT,
 * RASTER3D_USE_CACHE_X, RASTER3D_USE_CACHE_Y, RASTER3D_USE_CACHE_Z,
 * RASTER3D_USE_CACHE_XY, RASTER3D_USE_CACHE_XZ, RASTER3D_USE_CACHE_YZ,
 * RASTER3D_USE_CACHE_XYZ, the result of <tt>Rast3d_cache_size_encode ()</tt>
 * (cf.{g3d:G3d.cacheSizeEncode}), or any positive integer which
 * specifies the number of tiles buffered in the cache.  <em>region</em> specifies
 * the 3d region.  
 * The map is created using the <em>type</em> which must be of FCELL_TYPE or DCELL_TYPE.
 * Returns a pointer to the cell structure ... if successful,
 * NULL ... otherwise.
 *
 *  \param name The name of the map
 *  \param cache The type of the caching
 *  \param region The region of the map
 *  \param type The type of the map (FCELL_TYPE, or DCELL_TYPE)
 *  \param maxSize The maximum size of a tile in kilo bytes
 *  \return void * 
 */


void *Rast3d_open_new_opt_tile_size(const char *name, int cache, RASTER3D_Region * region, int type, int maxSize)
{
    void *map;
    int oldTileX, oldTileY, oldTileZ, oldType;
    int tileX, tileY, tileZ;

    Rast3d_init_defaults();


    Rast3d_get_tile_dimension(&oldTileX, &oldTileY, &oldTileZ);

    Rast3d_compute_optimal_tile_dimension(region, type, &tileX, &tileY, &tileZ, maxSize);

    G_debug(1, "New tile dimension X %i Y %i Z %i\n", tileX, tileY, tileZ);

    Rast3d_set_tile_dimension(tileX, tileY, tileZ);

    oldType = Rast3d_get_file_type();
    Rast3d_set_file_type(type);

    map = Rast3d_open_cell_new(name, RASTER3D_TILE_SAME_AS_FILE, cache, region);

    Rast3d_set_tile_dimension(oldTileX, oldTileY, oldTileZ);
    Rast3d_set_file_type(oldType);

    return map;
}
