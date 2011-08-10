#include <stdlib.h>
#include <stdio.h>
#include <grass/raster3d.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

#define RASTER3D_NO_DEFAULT -10

#define RASTER3D_COMPRESSION_DEFAULT RASTER3D_COMPRESSION
#define RASTER3D_USE_LZW_DEFAULT RASTER3D_NO_LZW
#define RASTER3D_USE_RLE_DEFAULT RASTER3D_USE_RLE
#define RASTER3D_PRECISION_DEFAULT RASTER3D_MAX_PRECISION
#define RASTER3D_CACHE_SIZE_DEFAULT 1000
#define RASTER3D_CACHE_SIZE_MAX_DEFAULT 16777216
#define RASTER3D_FILE_TYPE_DEFAULT DCELL_TYPE
#define RASTER3D_TILE_X_DEFAULT 16
#define RASTER3D_TILE_Y_DEFAULT 16
#define RASTER3D_TILE_Z_DEFAULT 8
#define RASTER3D_ERROR_FUN_DEFAULT Rast3d_skipError
#define RASTER3D_UNIT_DEFAULT "none"

/*---------------------------------------------------------------------------*/

#define RASTER3D_COMPRESSION_ENV_VAR_YES "RASTER3D_USE_COMPRESSION"
#define RASTER3D_COMPRESSION_ENV_VAR_NO "RASTER3D_NO_COMPRESSION"

#define RASTER3D_LZW_ENV_VAR_YES "RASTER3D_USE_LZW"
#define RASTER3D_LZW_ENV_VAR_NO "RASTER3D_NO_LZW"

#define RASTER3D_RLE_ENV_VAR_YES "RASTER3D_USE_RLE"
#define RASTER3D_RLE_ENV_VAR_NO "RASTER3D_NO_RLE"

#define RASTER3D_PRECISION_ENV_VAR "RASTER3D_PRECISION"
#define RASTER3D_PRECISION_ENV_VAR_MAX "RASTER3D_MAX_PRECISION"

#define RASTER3D_CACHE_SIZE_ENV_VAR "RASTER3D_DEFAULT_CACHE_SIZE"
#define RASTER3D_CACHE_SIZE_MAX_ENV_VAR "RASTER3D_MAX_CACHE_SIZE"

#define RASTER3D_FILE_FLOAT_ENV_VAR "RASTER3D_WRITE_FLOAT"
#define RASTER3D_FILE_DOUBLE_ENV_VAR "RASTER3D_WRITE_DOUBLE"

#define RASTER3D_TILE_DIM_X_ENV_VAR "RASTER3D_TILE_DIMENSION_X"
#define RASTER3D_TILE_DIM_Y_ENV_VAR "RASTER3D_TILE_DIMENSION_Y"
#define RASTER3D_TILE_DIM_Z_ENV_VAR "RASTER3D_TILE_DIMENSION_Z"

#define RASTER3D_FATAL_ERROR_ENV_VAR "RASTER3D_USE_FATAL_ERROR"
#define RASTER3D_PRINT_ERROR_ENV_VAR "RASTER3D_USE_PRINT_ERROR"

#define RASTER3D_DEFAULT_WINDOW3D "RASTER3D_DEFAULT_WINDOW3D"

/*---------------------------------------------------------------------------*/

int g3d_do_compression = RASTER3D_NO_DEFAULT;
int g3d_do_lzw_compression = RASTER3D_NO_DEFAULT;
int g3d_do_rle_compression = RASTER3D_NO_DEFAULT;
int g3d_precision = RASTER3D_NO_DEFAULT;
int g3d_cache_default = RASTER3D_NO_DEFAULT;
int g3d_cache_max = RASTER3D_NO_DEFAULT;
int g3d_file_type = RASTER3D_NO_DEFAULT;
int g3d_tile_dimension[3] =
    { RASTER3D_NO_DEFAULT, RASTER3D_NO_DEFAULT, RASTER3D_NO_DEFAULT };
void (*g3d_error_fun) (const char *) = NULL;
char *g3d_unit_default = NULL;

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * <em>doCompress</em> should be one of RASTER3D_NO_COMPRESSION and
 * RASTER3D_COMPRESSION, <em>doRle</em> should be either RASTER3D_NO_RLE or
 * RASTER3D_USE_RLE, and <em>precision</em> should be either RASTER3D_MAX_PRECISION or
 * a positive integer.
 *
 *  \param doCompress
 *  \param doLzw
 *  \param doRle
 *  \param precision
 *  \return void
 */

void
Rast3d_setCompressionMode(int doCompress, int doLzw, int doRle, int precision)
{
    if ((doCompress != RASTER3D_NO_COMPRESSION) && (doCompress != RASTER3D_COMPRESSION))
	Rast3d_fatalError("Rast3d_setCompressionMode: wrong value for doCompress.");

    g3d_do_compression = doCompress;

    if (doCompress == RASTER3D_NO_COMPRESSION)
	return;

    if ((doLzw != RASTER3D_NO_LZW) && (doLzw != RASTER3D_USE_LZW))
	Rast3d_fatalError("Rast3d_setCompressionMode: wrong value for doLzw.");

    if ((doRle != RASTER3D_NO_RLE) && (doRle != RASTER3D_USE_RLE))
	Rast3d_fatalError("Rast3d_setCompressionMode: wrong value for doRle.");

    if (precision < -1)
	Rast3d_fatalError("Rast3d_setCompressionMode: wrong value for precision.");

    g3d_do_lzw_compression = doLzw;
    g3d_do_rle_compression = doRle;
    g3d_precision = precision;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * 
 *
 *  \param doCompress
 *  \param doLzw
 *  \param doRle
 *  \param precision
 *  \return void
 */

void
Rast3d_getCompressionMode(int *doCompress, int *doLzw, int *doRle,
		       int *precision)
{
    if (doCompress != NULL)
	*doCompress = g3d_do_compression;
    if (doLzw != NULL)
	*doLzw = g3d_do_lzw_compression;
    if (doRle != NULL)
	*doRle = g3d_do_rle_compression;
    if (precision != NULL)
	*precision = g3d_precision;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  set cache size
 *
 *  \param nTiles
 *  \return void
 */

void Rast3d_setCacheSize(int nTiles)
{
    if (nTiles < 0)
	Rast3d_fatalError("Rast3d_setCacheSize: size out of range.");

    g3d_cache_default = nTiles;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  get cache size
 *
 *  \return int
 */

int Rast3d_getCacheSize()
{
    return g3d_cache_default;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief Set cache limit
 *
 *  set cache limit
 *
 *  \param nBytes
 *  \return void
 */

void Rast3d_setCacheLimit(int nBytes)
{
    if (nBytes <= 0)
	Rast3d_fatalError("Rast3d_setCacheLimit: size out of range.");

    g3d_cache_max = nBytes;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief Get cache limit
 *
 *  get cache limit
 *
 *  \param nBytes
 *  \return int
 */

int Rast3d_getCacheLimit()
{
    return g3d_cache_max;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  set G3d file type
 *
 *  \param type
 *  \return void
 */

void Rast3d_setFileType(int type)
{
    if ((type != FCELL_TYPE) && (type != DCELL_TYPE))
	Rast3d_fatalError("Rast3d_setFileTypeDefault: invalid type");

    g3d_file_type = type;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * get G3d file type
 *
 *  \param type
 *  \return int
 */

int Rast3d_getFileType()
{
    return g3d_file_type;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * set Tile Dimension
 *
 *  \param tileX
 *  \param tileY
 *  \param tileZ
 *  \return void
 */

void Rast3d_setTileDimension(int tileX, int tileY, int tileZ)
{
    if ((g3d_tile_dimension[0] = tileX) <= 0)
	Rast3d_fatalError
	    ("Rast3d_setTileDimension: value for tile x environment variable out of range");

    if ((g3d_tile_dimension[1] = tileY) <= 0)
	Rast3d_fatalError
	    ("Rast3d_setTileDimension: value for tile y environment variable out of range");

    if ((g3d_tile_dimension[2] = tileZ) <= 0)
	Rast3d_fatalError
	    ("Rast3d_setTileDimension: value for tile z environment variable out of range");
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  get Tile Dimension
 *
 *  \param tileX
 *  \param tileY
 *  \param tileZ
 *  \return void
 */

void Rast3d_getTileDimension(int *tileX, int *tileY, int *tileZ)
{
    *tileX = g3d_tile_dimension[0];
    *tileY = g3d_tile_dimension[1];
    *tileZ = g3d_tile_dimension[2];
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  set error function
 *
 *  \param 
 *  \return void
 */

void Rast3d_setErrorFun(void (*fun) (const char *))
{
    g3d_error_fun = fun;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  set G3d unit
 *
 *  \param unit
 *  \return void
 */

void Rast3d_setUnit(const char *unit)
{
    Rast3d_free(g3d_unit_default);
    g3d_unit_default = G_store(unit);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Initializes the default values described
 * in RASTER3D Defaults.  Applications have to use this function only if they need to
 * query the default values before the first file (either old or new) has been
 * opened.
 *
 *  \return void
 */

void Rast3d_initDefaults(void)
{
    static int firstTime = 1;
    const char *value, *windowName;
    RASTER3D_Region window;

    if (!firstTime)
	return;
    firstTime = 0;

    if (g3d_do_compression == RASTER3D_NO_DEFAULT) {
	if (NULL != getenv(RASTER3D_COMPRESSION_ENV_VAR_YES)) {
	    g3d_do_compression = RASTER3D_COMPRESSION;
	}
	else {
	    if (NULL != getenv(RASTER3D_COMPRESSION_ENV_VAR_NO)) {
		g3d_do_compression = RASTER3D_NO_COMPRESSION;
	    }
	    else {
		g3d_do_compression = RASTER3D_COMPRESSION_DEFAULT;
	    }
	}
    }

    if (g3d_do_lzw_compression == RASTER3D_NO_DEFAULT) {
	if (NULL != getenv(RASTER3D_LZW_ENV_VAR_YES)) {
	    g3d_do_lzw_compression = RASTER3D_USE_LZW;
	}
	else {
	    if (NULL != getenv(RASTER3D_LZW_ENV_VAR_NO)) {
		g3d_do_lzw_compression = RASTER3D_NO_LZW;
	    }
	    else {
		g3d_do_lzw_compression = RASTER3D_USE_LZW_DEFAULT;
	    }
	}
    }

    if (g3d_do_rle_compression == RASTER3D_NO_DEFAULT) {
	if (NULL != getenv(RASTER3D_RLE_ENV_VAR_YES)) {
	    g3d_do_rle_compression = RASTER3D_USE_RLE;
	}
	else {
	    if (NULL != getenv(RASTER3D_RLE_ENV_VAR_NO)) {
		g3d_do_rle_compression = RASTER3D_NO_RLE;
	    }
	    else {
		g3d_do_rle_compression = RASTER3D_USE_RLE_DEFAULT;
	    }
	}
    }

    if (g3d_precision == RASTER3D_NO_DEFAULT) {
	if (NULL != getenv(RASTER3D_PRECISION_ENV_VAR_MAX)) {
	    g3d_precision = RASTER3D_MAX_PRECISION;
	}
	else {
	    value = getenv(RASTER3D_PRECISION_ENV_VAR);
	    if (value == NULL) {
		g3d_precision = RASTER3D_PRECISION_DEFAULT;
	    }
	    else {
		if (sscanf(value, "%d", &g3d_precision) != 1) {
		    Rast3d_fatalError
			("Rast3d_initDefaults: precision environment variable has invalid value");
		}
		else {
		    if (g3d_precision < -1) {
			Rast3d_fatalError
			    ("Rast3d_initDefaults: value for cache environment variable out of range");
		    }
		}
	    }
	}
    }

    if (g3d_file_type == RASTER3D_NO_DEFAULT) {
	if (NULL != getenv(RASTER3D_FILE_FLOAT_ENV_VAR)) {
	    g3d_file_type = FCELL_TYPE;
	}
	else {
	    if (NULL != getenv(RASTER3D_FILE_DOUBLE_ENV_VAR)) {
		g3d_file_type = DCELL_TYPE;
	    }
	    else {
		g3d_file_type = RASTER3D_FILE_TYPE_DEFAULT;
	    }
	}
    }

    if (g3d_cache_default == RASTER3D_NO_DEFAULT) {
	value = getenv(RASTER3D_CACHE_SIZE_ENV_VAR);

	if (value == NULL) {
	    g3d_cache_default = RASTER3D_CACHE_SIZE_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", &g3d_cache_default) != 1) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: cache environment variable has invalid value");
	    }
	    if (g3d_cache_default < 0) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: value for cache environment variable out of range");
	    }
	}
    }

    if (g3d_cache_max == RASTER3D_NO_DEFAULT) {
	value = getenv(RASTER3D_CACHE_SIZE_MAX_ENV_VAR);

	if (value == NULL) {
	    g3d_cache_max = RASTER3D_CACHE_SIZE_MAX_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", &g3d_cache_max) != 1) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: cache environment variable has invalid value");
	    }
	    if (g3d_cache_max < 0) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: value for cache environment variable out of range");
	    }
	}
    }

    if (g3d_tile_dimension[0] == RASTER3D_NO_DEFAULT) {
	value = getenv(RASTER3D_TILE_DIM_X_ENV_VAR);

	if (value == NULL) {
	    g3d_tile_dimension[0] = RASTER3D_TILE_X_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", g3d_tile_dimension) != 1) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: tile dimension x environment variable has invalid value");
	    }
	    if (g3d_tile_dimension[0] <= 0) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: value for tile x environment variable out of range");
	    }
	}

	value = getenv(RASTER3D_TILE_DIM_Y_ENV_VAR);

	if (value == NULL) {
	    g3d_tile_dimension[1] = RASTER3D_TILE_Y_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", g3d_tile_dimension + 1) != 1) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: tile dimension y environment variable has invalid value");
	    }
	    if (g3d_tile_dimension[1] <= 0) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: value for tile y environment variable out of range");
	    }
	}

	value = getenv(RASTER3D_TILE_DIM_Z_ENV_VAR);

	if (value == NULL) {
	    g3d_tile_dimension[2] = RASTER3D_TILE_Z_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", g3d_tile_dimension + 2) != 1) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: tile dimension z environment variable has invalid value");
	    }
	    if (g3d_tile_dimension[2] <= 0) {
		Rast3d_fatalError
		    ("Rast3d_initDefaults: value for tile z environment variable out of range");
	    }
	}
    }

    if (g3d_error_fun == NULL) {
	value = getenv(RASTER3D_FATAL_ERROR_ENV_VAR);

	if (value != NULL) {
	    g3d_error_fun = Rast3d_fatalError_noargs;
	}
	else {
	    value = getenv(RASTER3D_PRINT_ERROR_ENV_VAR);

	    if (value != NULL) {
		g3d_error_fun = Rast3d_printError;
	    }
	    else {
		g3d_error_fun = RASTER3D_ERROR_FUN_DEFAULT;
	    }
	}
    }

    if (g3d_unit_default == NULL)
	g3d_unit_default = G_store(RASTER3D_UNIT_DEFAULT);

    windowName = Rast3d_getWindowParams();
    if (windowName == NULL) {
	value = getenv(RASTER3D_DEFAULT_WINDOW3D);
	if (value != NULL)
	    if (*value != 0)
		windowName = value;
    }

    if (!Rast3d_readWindow(&window, windowName))
	Rast3d_fatalError("Rast3d_initDefaults: Error reading window");
    Rast3d_setWindow(&window);

}
