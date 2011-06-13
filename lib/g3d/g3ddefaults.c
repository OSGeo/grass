#include <stdlib.h>
#include <stdio.h>
#include <grass/G3d.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

#define G3D_NO_DEFAULT -10

#define G3D_COMPRESSION_DEFAULT G3D_COMPRESSION
#define G3D_USE_LZW_DEFAULT G3D_NO_LZW
#define G3D_USE_RLE_DEFAULT G3D_USE_RLE
#define G3D_PRECISION_DEFAULT G3D_MAX_PRECISION
#define G3D_CACHE_SIZE_DEFAULT 262144
#define G3D_CACHE_SIZE_MAX_DEFAULT 16777216
#define G3D_FILE_TYPE_DEFAULT DCELL_TYPE
#define G3D_TILE_X_DEFAULT 16
#define G3D_TILE_Y_DEFAULT 16
#define G3D_TILE_Z_DEFAULT 8
#define G3D_ERROR_FUN_DEFAULT G3d_skipError
#define G3D_UNIT_DEFAULT "none"

/*---------------------------------------------------------------------------*/

#define G3D_COMPRESSION_ENV_VAR_YES "G3D_USE_COMPRESSION"
#define G3D_COMPRESSION_ENV_VAR_NO "G3D_NO_COMPRESSION"

#define G3D_LZW_ENV_VAR_YES "G3D_USE_LZW"
#define G3D_LZW_ENV_VAR_NO "G3D_NO_LZW"

#define G3D_RLE_ENV_VAR_YES "G3D_USE_RLE"
#define G3D_RLE_ENV_VAR_NO "G3D_NO_RLE"

#define G3D_PRECISION_ENV_VAR "G3D_PRECISION"
#define G3D_PRECISION_ENV_VAR_MAX "G3D_MAX_PRECISION"

#define G3D_CACHE_SIZE_ENV_VAR "G3D_DEFAULT_CACHE_SIZE"
#define G3D_CACHE_SIZE_MAX_ENV_VAR "G3D_MAX_CACHE_SIZE"

#define G3D_FILE_FLOAT_ENV_VAR "G3D_WRITE_FLOAT"
#define G3D_FILE_DOUBLE_ENV_VAR "G3D_WRITE_DOUBLE"

#define G3D_TILE_DIM_X_ENV_VAR "G3D_TILE_DIMENSION_X"
#define G3D_TILE_DIM_Y_ENV_VAR "G3D_TILE_DIMENSION_Y"
#define G3D_TILE_DIM_Z_ENV_VAR "G3D_TILE_DIMENSION_Z"

#define G3D_FATAL_ERROR_ENV_VAR "G3D_USE_FATAL_ERROR"
#define G3D_PRINT_ERROR_ENV_VAR "G3D_USE_PRINT_ERROR"

#define G3D_DEFAULT_WINDOW3D "G3D_DEFAULT_WINDOW3D"

/*---------------------------------------------------------------------------*/

int g3d_do_compression = G3D_NO_DEFAULT;
int g3d_do_lzw_compression = G3D_NO_DEFAULT;
int g3d_do_rle_compression = G3D_NO_DEFAULT;
int g3d_precision = G3D_NO_DEFAULT;
int g3d_cache_default = G3D_NO_DEFAULT;
int g3d_cache_max = G3D_NO_DEFAULT;
int g3d_file_type = G3D_NO_DEFAULT;
int g3d_tile_dimension[3] =
    { G3D_NO_DEFAULT, G3D_NO_DEFAULT, G3D_NO_DEFAULT };
void (*g3d_error_fun) (const char *) = NULL;
char *g3d_unit_default = NULL;

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * <em>doCompress</em> should be one of G3D_NO_COMPRESSION and
 * G3D_COMPRESSION, <em>doRle</em> should be either G3D_NO_RLE or
 * G3D_USE_RLE, and <em>precision</em> should be either G3D_MAX_PRECISION or
 * a positive integer.
 *
 *  \param doCompress
 *  \param doLzw
 *  \param doRle
 *  \param precision
 *  \return void
 */

void
G3d_setCompressionMode(int doCompress, int doLzw, int doRle, int precision)
{
    if ((doCompress != G3D_NO_COMPRESSION) && (doCompress != G3D_COMPRESSION))
	G3d_fatalError("G3d_setCompressionMode: wrong value for doCompress.");

    g3d_do_compression = doCompress;

    if (doCompress == G3D_NO_COMPRESSION)
	return;

    if ((doLzw != G3D_NO_LZW) && (doLzw != G3D_USE_LZW))
	G3d_fatalError("G3d_setCompressionMode: wrong value for doLzw.");

    if ((doRle != G3D_NO_RLE) && (doRle != G3D_USE_RLE))
	G3d_fatalError("G3d_setCompressionMode: wrong value for doRle.");

    if (precision < -1)
	G3d_fatalError("G3d_setCompressionMode: wrong value for precision.");

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
G3d_getCompressionMode(int *doCompress, int *doLzw, int *doRle,
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

void G3d_setCacheSize(int nTiles)
{
    if (nTiles < 0)
	G3d_fatalError("G3d_setCacheSize: size out of range.");

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

int G3d_getCacheSize()
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

void G3d_setCacheLimit(int nBytes)
{
    if (nBytes <= 0)
	G3d_fatalError("G3d_setCacheLimit: size out of range.");

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

int G3d_getCacheLimit()
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

void G3d_setFileType(int type)
{
    if ((type != FCELL_TYPE) && (type != DCELL_TYPE))
	G3d_fatalError("G3d_setFileTypeDefault: invalid type");

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

int G3d_getFileType()
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

void G3d_setTileDimension(int tileX, int tileY, int tileZ)
{
    if ((g3d_tile_dimension[0] = tileX) <= 0)
	G3d_fatalError
	    ("G3d_setTileDimension: value for tile x environment variable out of range");

    if ((g3d_tile_dimension[1] = tileY) <= 0)
	G3d_fatalError
	    ("G3d_setTileDimension: value for tile y environment variable out of range");

    if ((g3d_tile_dimension[2] = tileZ) <= 0)
	G3d_fatalError
	    ("G3d_setTileDimension: value for tile z environment variable out of range");
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

void G3d_getTileDimension(int *tileX, int *tileY, int *tileZ)
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

void G3d_setErrorFun(void (*fun) (const char *))
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

void G3d_setUnit(const char *unit)
{
    G3d_free(g3d_unit_default);
    g3d_unit_default = G_store(unit);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Initializes the default values described
 * in G3D Defaults.  Applications have to use this function only if they need to
 * query the default values before the first file (either old or new) has been
 * opened.
 *
 *  \return void
 */

void G3d_initDefaults(void)
{
    static int firstTime = 1;
    const char *value, *windowName;
    G3D_Region window;

    if (!firstTime)
	return;
    firstTime = 0;

    if (g3d_do_compression == G3D_NO_DEFAULT) {
	if (NULL != getenv(G3D_COMPRESSION_ENV_VAR_YES)) {
	    g3d_do_compression = G3D_COMPRESSION;
	}
	else {
	    if (NULL != getenv(G3D_COMPRESSION_ENV_VAR_NO)) {
		g3d_do_compression = G3D_NO_COMPRESSION;
	    }
	    else {
		g3d_do_compression = G3D_COMPRESSION_DEFAULT;
	    }
	}
    }

    if (g3d_do_lzw_compression == G3D_NO_DEFAULT) {
	if (NULL != getenv(G3D_LZW_ENV_VAR_YES)) {
	    g3d_do_lzw_compression = G3D_USE_LZW;
	}
	else {
	    if (NULL != getenv(G3D_LZW_ENV_VAR_NO)) {
		g3d_do_lzw_compression = G3D_NO_LZW;
	    }
	    else {
		g3d_do_lzw_compression = G3D_USE_LZW_DEFAULT;
	    }
	}
    }

    if (g3d_do_rle_compression == G3D_NO_DEFAULT) {
	if (NULL != getenv(G3D_RLE_ENV_VAR_YES)) {
	    g3d_do_rle_compression = G3D_USE_RLE;
	}
	else {
	    if (NULL != getenv(G3D_RLE_ENV_VAR_NO)) {
		g3d_do_rle_compression = G3D_NO_RLE;
	    }
	    else {
		g3d_do_rle_compression = G3D_USE_RLE_DEFAULT;
	    }
	}
    }

    if (g3d_precision == G3D_NO_DEFAULT) {
	if (NULL != getenv(G3D_PRECISION_ENV_VAR_MAX)) {
	    g3d_precision = G3D_MAX_PRECISION;
	}
	else {
	    value = getenv(G3D_PRECISION_ENV_VAR);
	    if (value == NULL) {
		g3d_precision = G3D_PRECISION_DEFAULT;
	    }
	    else {
		if (sscanf(value, "%d", &g3d_precision) != 1) {
		    G3d_fatalError
			("G3d_initDefaults: precision environment variable has invalid value");
		}
		else {
		    if (g3d_precision < -1) {
			G3d_fatalError
			    ("G3d_initDefaults: value for cache environment variable out of range");
		    }
		}
	    }
	}
    }

    if (g3d_file_type == G3D_NO_DEFAULT) {
	if (NULL != getenv(G3D_FILE_FLOAT_ENV_VAR)) {
	    g3d_file_type = FCELL_TYPE;
	}
	else {
	    if (NULL != getenv(G3D_FILE_DOUBLE_ENV_VAR)) {
		g3d_file_type = DCELL_TYPE;
	    }
	    else {
		g3d_file_type = G3D_FILE_TYPE_DEFAULT;
	    }
	}
    }

    if (g3d_cache_default == G3D_NO_DEFAULT) {
	value = getenv(G3D_CACHE_SIZE_ENV_VAR);

	if (value == NULL) {
	    g3d_cache_default = G3D_CACHE_SIZE_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", &g3d_cache_default) != 1) {
		G3d_fatalError
		    ("G3d_initDefaults: cache environment variable has invalid value");
	    }
	    if (g3d_cache_default < 0) {
		G3d_fatalError
		    ("G3d_initDefaults: value for cache environment variable out of range");
	    }
	}
    }

    if (g3d_cache_max == G3D_NO_DEFAULT) {
	value = getenv(G3D_CACHE_SIZE_MAX_ENV_VAR);

	if (value == NULL) {
	    g3d_cache_max = G3D_CACHE_SIZE_MAX_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", &g3d_cache_max) != 1) {
		G3d_fatalError
		    ("G3d_initDefaults: cache environment variable has invalid value");
	    }
	    if (g3d_cache_max < 0) {
		G3d_fatalError
		    ("G3d_initDefaults: value for cache environment variable out of range");
	    }
	}
    }

    if (g3d_tile_dimension[0] == G3D_NO_DEFAULT) {
	value = getenv(G3D_TILE_DIM_X_ENV_VAR);

	if (value == NULL) {
	    g3d_tile_dimension[0] = G3D_TILE_X_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", g3d_tile_dimension) != 1) {
		G3d_fatalError
		    ("G3d_initDefaults: tile dimension x environment variable has invalid value");
	    }
	    if (g3d_tile_dimension[0] <= 0) {
		G3d_fatalError
		    ("G3d_initDefaults: value for tile x environment variable out of range");
	    }
	}

	value = getenv(G3D_TILE_DIM_Y_ENV_VAR);

	if (value == NULL) {
	    g3d_tile_dimension[1] = G3D_TILE_Y_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", g3d_tile_dimension + 1) != 1) {
		G3d_fatalError
		    ("G3d_initDefaults: tile dimension y environment variable has invalid value");
	    }
	    if (g3d_tile_dimension[1] <= 0) {
		G3d_fatalError
		    ("G3d_initDefaults: value for tile y environment variable out of range");
	    }
	}

	value = getenv(G3D_TILE_DIM_Z_ENV_VAR);

	if (value == NULL) {
	    g3d_tile_dimension[2] = G3D_TILE_Z_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", g3d_tile_dimension + 2) != 1) {
		G3d_fatalError
		    ("G3d_initDefaults: tile dimension z environment variable has invalid value");
	    }
	    if (g3d_tile_dimension[2] <= 0) {
		G3d_fatalError
		    ("G3d_initDefaults: value for tile z environment variable out of range");
	    }
	}
    }

    if (g3d_error_fun == NULL) {
	value = getenv(G3D_FATAL_ERROR_ENV_VAR);

	if (value != NULL) {
	    g3d_error_fun = G3d_fatalError_noargs;
	}
	else {
	    value = getenv(G3D_PRINT_ERROR_ENV_VAR);

	    if (value != NULL) {
		g3d_error_fun = G3d_printError;
	    }
	    else {
		g3d_error_fun = G3D_ERROR_FUN_DEFAULT;
	    }
	}
    }

    if (g3d_unit_default == NULL)
	g3d_unit_default = G_store(G3D_UNIT_DEFAULT);

    windowName = G3d_getWindowParams();
    if (windowName == NULL) {
	value = getenv(G3D_DEFAULT_WINDOW3D);
	if (value != NULL)
	    if (*value != 0)
		windowName = value;
    }

    if (!G3d_readWindow(&window, windowName))
	G3d_fatalError("G3d_initDefaults: Error reading window");
    G3d_setWindow(&window);

}
