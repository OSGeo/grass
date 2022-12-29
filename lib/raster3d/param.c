#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "raster3d_intern.h"

/*----------------------------------------------------------------------------*/

typedef struct
{

    struct Option *type;
    struct Option *precision;
    struct Option *compression;
    struct Option *dimension;
    struct Option *cache;

} Rast3d_paramType;

/*----------------------------------------------------------------------------*/

static Rast3d_paramType *param;


/*!
 * \brief 
 *
 * Initializes a parameter
 * structure for the subset of command line arguments which lets the user
 * overwrite the default properties of the new file.  Applications are
 * encouraged to use this function in order to provide a uniform style.  The
 * command line arguments provided are the <em>type</em> of the cell values, the
 * <em>precision</em>, the properties of the <em>compression</em>, and the dimension
 * of the tiles (<em>tiledimension</em>). Every of these values defaults to the
 * value described in RASTER3D Defaults.
 * This function has to be used in conjunction with
 * Rast3d_getStandard3dInputParams() (cf.{g3d:G3d.getStandard3dInputParams}).
 *
 *  \return void
 */

void Rast3d_set_standard3d_input_params()
{
    param = Rast3d_malloc(sizeof(Rast3d_paramType));

    param->type = G_define_standard_option(G_OPT_R3_TYPE);

    param->precision = G_define_standard_option(G_OPT_R3_PRECISION);

    param->compression = G_define_standard_option(G_OPT_R3_COMPRESSION);

    param->dimension = G_define_standard_option(G_OPT_R3_TILE_DIMENSION);
}

/*----------------------------------------------------------------------------*/

int Rast3d_get_standard3d_params(int *useTypeDefault, int *type,
			    int *useCompressionDefault, int *doCompression,
			    int *usePrecisionDefault, int *precision,
			    int *useDimensionDefault, int *tileX, int *tileY,
			    int *tileZ)
{

    *useTypeDefault = *useCompressionDefault = 0;
    *usePrecisionDefault = *useDimensionDefault = 0;

    Rast3d_init_defaults();

    if (strcmp(param->type->answer, "double") == 0)
	*type = DCELL_TYPE;
    else if (strcmp(param->type->answer, "float") == 0)
	*type = FCELL_TYPE;
    else {
	*type = Rast3d_get_file_type();
	*useTypeDefault = 1;
    }

    Rast3d_get_compression_mode(doCompression, precision);

    if (strcmp(param->precision->answer, "default") != 0) {
	if (strcmp(param->precision->answer, "max") == 0)
	    *precision = -1;
	else if ((sscanf(param->precision->answer, "%d", precision) != 1) ||
		 (*precision < 0)) {
	    Rast3d_error(_("Rast3d_get_standard3d_params: precision value invalid"));
	    return 0;
	}
	}
	else
	*usePrecisionDefault = 1;


	if (strcmp(param->compression->answer, "default") != 0) {
 		if (strcmp(param->compression->answer, "zip") == 0)
			*doCompression = RASTER3D_COMPRESSION;
		else
			*doCompression = RASTER3D_NO_COMPRESSION;
	} else {
		*useCompressionDefault = 1;
	}

    Rast3d_get_tile_dimension(tileX, tileY, tileZ);
    if (strcmp(param->dimension->answer, "default") != 0) {
	if (sscanf(param->dimension->answer, "%dx%dx%d",
		   tileX, tileY, tileZ) != 3) {
	    Rast3d_error(_("Rast3d_get_standard3d_params: tile dimension value invalid"));
	    return 0;
	}
    }
    else
	*useDimensionDefault = 1;

    Rast3d_free(param);

    return 1;
}

/*----------------------------------------------------------------------------*/

static struct Option *windowParam = NULL;

void Rast3d_set_window_params(void)
{
    windowParam = G_define_option();
    windowParam->key = "region3";
    windowParam->type = TYPE_STRING;
    windowParam->required = NO;
    windowParam->multiple = NO;
    windowParam->answer = NULL;
    windowParam->description = _("Window replacing the default");
}

/*----------------------------------------------------------------------------*/

char *Rast3d_get_window_params(void)
{
    if (windowParam == NULL)
	return NULL;
    if (windowParam->answer == NULL)
	return NULL;
    if (strcmp(windowParam->answer, RASTER3D_WINDOW_ELEMENT) == 0)
	return G_store(RASTER3D_WINDOW_ELEMENT);
    return G_store(windowParam->answer);
}
