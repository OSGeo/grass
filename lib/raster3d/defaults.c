#include <stdlib.h>
#include <stdio.h>
#include <grass/raster3d.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

#define RASTER3D_NO_DEFAULT -10

#define RASTER3D_COMPRESSION_DEFAULT RASTER3D_COMPRESSION
#define RASTER3D_PRECISION_DEFAULT RASTER3D_MAX_PRECISION
#define RASTER3D_CACHE_SIZE_DEFAULT 1000
#define RASTER3D_CACHE_SIZE_MAX_DEFAULT 16777216
#define RASTER3D_FILE_TYPE_DEFAULT DCELL_TYPE
#define RASTER3D_TILE_X_DEFAULT 16
#define RASTER3D_TILE_Y_DEFAULT 16
#define RASTER3D_TILE_Z_DEFAULT 8
#define RASTER3D_ERROR_FUN_DEFAULT Rast3d_skip_error
#define RASTER3D_UNIT_DEFAULT "none"
#define RASTER3D_VERTICAL_UNIT_DEFAULT U_UNKNOWN

/*---------------------------------------------------------------------------*/

/*!
\brief Name of the environmental variable specifying that compression
       should be used.

       Setting the environmental variable to any value will cause that the
       compression will be used (set when calling Rast3d_init_defaults
       for the first time).

       This environmental variable takes precedence before the environmental
       variable specified by RASTER3D_COMPRESSION_ENV_VAR_NO.
*/
#define RASTER3D_COMPRESSION_ENV_VAR_YES "RASTER3D_USE_COMPRESSION"
/*!
\brief Name of the environmental variable specifying that compression
       should not be used.

       \see RASTER3D_COMPRESSION_ENV_VAR_YES
*/
#define RASTER3D_COMPRESSION_ENV_VAR_NO "RASTER3D_NO_COMPRESSION"

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

int g3d_version = RASTER3D_MAP_VERSION;
int g3d_do_compression = RASTER3D_NO_DEFAULT;
int g3d_precision = RASTER3D_NO_DEFAULT;
int g3d_cache_default = RASTER3D_NO_DEFAULT;
int g3d_cache_max = RASTER3D_NO_DEFAULT;
int g3d_file_type = RASTER3D_NO_DEFAULT;
int g3d_tile_dimension[3] =
    { RASTER3D_NO_DEFAULT, RASTER3D_NO_DEFAULT, RASTER3D_NO_DEFAULT };
void (*g3d_error_fun) (const char *) = NULL;
char *g3d_unit_default = NULL;
int g3d_vertical_unit_default = U_UNDEFINED;

/*---------------------------------------------------------------------------*/


/*!
 * \brief set compression mode
 *
 * <em>doCompress</em> should be one of RASTER3D_NO_COMPRESSION and
 * RASTER3D_COMPRESSION. <em>precision</em> should be either
 * RASTER3D_MAX_PRECISION or a positive integer.
 *
 * \note Note that older parameters <em>doLzw</em> and <em>doRle</em>
 *       (RASTER3D_NO_RLE or RASTER3D_USE_RLE) are no longer in the API.
 *
 * Calls Rast3d_fatal_error() if a wrong parameter value is provided.
 *
 * \param doCompress specifies if a compression should be perfomed
 * \param precision a precision of compression
 */

void
Rast3d_set_compression_mode(int doCompress, int precision)
{
    if ((doCompress != RASTER3D_NO_COMPRESSION) && (doCompress != RASTER3D_COMPRESSION))
	Rast3d_fatal_error("Rast3d_set_compression_mode: wrong value for doCompress.");

    g3d_do_compression = doCompress;

    if (doCompress == RASTER3D_NO_COMPRESSION)
	return;

    if (precision < -1)
	Rast3d_fatal_error("Rast3d_set_compression_mode: wrong value for precision.");

    g3d_precision = precision;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief Gets compression mode
 *
 * \param doCompress pointer to the integer where the compression mode will
 *        be stored
 * \param precision pointer to the integer where the precision mode will
 *        be stored
 *
 * \see Rast3d_set_compression_mode, RASTER3D_COMPRESSION_ENV_VAR_YES,
        RASTER3D_COMPRESSION_ENV_VAR_YES
 */

void
Rast3d_get_compression_mode(int *doCompress, int *precision)
{
    if (doCompress != NULL)
	*doCompress = g3d_do_compression;
    if (precision != NULL)
	*precision = g3d_precision;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief set cache size
 *
 *  \param nTiles
 *  \return void
 */

void Rast3d_set_cache_size(int nTiles)
{
    if (nTiles < 0)
	Rast3d_fatal_error("Rast3d_set_cache_size: size out of range.");

    g3d_cache_default = nTiles;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief get cache size
 *
 *  \return int
 */

int Rast3d_get_cache_size()
{
    return g3d_cache_default;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief Set cache limit
 *
 *  \param nBytes
 *  \return void
 */

void Rast3d_set_cache_limit(int nBytes)
{
    if (nBytes <= 0)
	Rast3d_fatal_error("Rast3d_set_cache_limit: size out of range.");

    g3d_cache_max = nBytes;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief Get cache limit
 *
 *  \return int
 */

int Rast3d_get_cache_limit()
{
    return g3d_cache_max;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief set G3d file type
 *
 *  \param type
 *  \return void
 */

void Rast3d_set_file_type(int type)
{
    if ((type != FCELL_TYPE) && (type != DCELL_TYPE))
	Rast3d_fatal_error("Rast3d_setFileTypeDefault: invalid type");

    g3d_file_type = type;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief get G3d file type
 *
 *  \return int
 */

int Rast3d_get_file_type()
{
    return g3d_file_type;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief set Tile Dimension
 *
 *  \param tileX
 *  \param tileY
 *  \param tileZ
 *  \return void
 */

void Rast3d_set_tile_dimension(int tileX, int tileY, int tileZ)
{
    if ((g3d_tile_dimension[0] = tileX) <= 0)
	Rast3d_fatal_error
	    ("Rast3d_set_tile_dimension: value for tile x environment variable out of range");

    if ((g3d_tile_dimension[1] = tileY) <= 0)
	Rast3d_fatal_error
	    ("Rast3d_set_tile_dimension: value for tile y environment variable out of range");

    if ((g3d_tile_dimension[2] = tileZ) <= 0)
	Rast3d_fatal_error
	    ("Rast3d_set_tile_dimension: value for tile z environment variable out of range");
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief get Tile Dimension
 *
 *  \param tileX
 *  \param tileY
 *  \param tileZ
 *  \return void
 */

void Rast3d_get_tile_dimension(int *tileX, int *tileY, int *tileZ)
{
    *tileX = g3d_tile_dimension[0];
    *tileY = g3d_tile_dimension[1];
    *tileZ = g3d_tile_dimension[2];
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief set error function
 *
 *  \param fun
 *  \return void
 */

void Rast3d_set_error_fun(void (*fun) (const char *))
{
    g3d_error_fun = fun;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief Initializes the default values described
 * in RASTER3D Defaults.  Applications have to use this function only if they need to
 * query the default values before the first file (either old or new) has been
 * opened.
 *
 *  \return void
 */

void Rast3d_init_defaults(void)
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
		    Rast3d_fatal_error
			("Rast3d_init_defaults: precision environment variable has invalid value");
		}
		else {
		    if (g3d_precision < -1) {
			Rast3d_fatal_error
			    ("Rast3d_init_defaults: value for cache environment variable out of range");
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
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: cache environment variable has invalid value");
	    }
	    if (g3d_cache_default < 0) {
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: value for cache environment variable out of range");
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
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: cache environment variable has invalid value");
	    }
	    if (g3d_cache_max < 0) {
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: value for cache environment variable out of range");
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
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: tile dimension x environment variable has invalid value");
	    }
	    if (g3d_tile_dimension[0] <= 0) {
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: value for tile x environment variable out of range");
	    }
	}

	value = getenv(RASTER3D_TILE_DIM_Y_ENV_VAR);

	if (value == NULL) {
	    g3d_tile_dimension[1] = RASTER3D_TILE_Y_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", g3d_tile_dimension + 1) != 1) {
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: tile dimension y environment variable has invalid value");
	    }
	    if (g3d_tile_dimension[1] <= 0) {
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: value for tile y environment variable out of range");
	    }
	}

	value = getenv(RASTER3D_TILE_DIM_Z_ENV_VAR);

	if (value == NULL) {
	    g3d_tile_dimension[2] = RASTER3D_TILE_Z_DEFAULT;
	}
	else {
	    if (sscanf(value, "%d", g3d_tile_dimension + 2) != 1) {
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: tile dimension z environment variable has invalid value");
	    }
	    if (g3d_tile_dimension[2] <= 0) {
		Rast3d_fatal_error
		    ("Rast3d_init_defaults: value for tile z environment variable out of range");
	    }
	}
    }

    if (g3d_error_fun == NULL) {
	value = getenv(RASTER3D_FATAL_ERROR_ENV_VAR);

	if (value != NULL) {
	    g3d_error_fun = Rast3d_fatal_error_noargs;
	}
	else {
	    value = getenv(RASTER3D_PRINT_ERROR_ENV_VAR);

	    if (value != NULL) {
		g3d_error_fun = Rast3d_print_error;
	    }
	    else {
		g3d_error_fun = RASTER3D_ERROR_FUN_DEFAULT;
	    }
	}
    }

    if(g3d_unit_default == NULL)
        g3d_unit_default = G_store(RASTER3D_UNIT_DEFAULT);
    if(g3d_vertical_unit_default == U_UNDEFINED)
        g3d_vertical_unit_default = RASTER3D_VERTICAL_UNIT_DEFAULT;

    windowName = Rast3d_get_window_params();
    if (windowName == NULL) {
	value = getenv(RASTER3D_DEFAULT_WINDOW3D);
	if (value != NULL)
	    if (*value != 0)
		windowName = value;
    }

    if (!Rast3d_read_window(&window, windowName))
	Rast3d_fatal_error("Rast3d_init_defaults: Error reading window");
    Rast3d_set_window(&window);

}
