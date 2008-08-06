#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "G3d_intern.h"

/*----------------------------------------------------------------------------*/

typedef struct
{

    struct Option *type;
    struct Option *precision;
    struct Option *compression;
    struct Option *dimension;
    struct Option *cache;

} G3d_paramType;

/*----------------------------------------------------------------------------*/

static G3d_paramType *param;


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
 * value described in G3D Defaults.
 * This function has to be used in conjunction with
 * G3d_getStandard3dInputParams() (cf.{g3d:G3d.getStandard3dInputParams}).
 *
 *  \return void
 */

void G3d_setStandard3dInputParams()
{
    param = G3d_malloc(sizeof(G3d_paramType));

    param->type = G_define_option();
    param->type->key = "type";
    param->type->type = TYPE_STRING;
    param->type->required = NO;
    param->type->multiple = NO;
    param->type->answer = "default";
    param->type->options = "default,double,float";
    param->type->description = _("Data type used in the output file");

    param->precision = G_define_option();
    param->precision->key = "precision";
    param->precision->type = TYPE_STRING;
    param->precision->required = NO;
    param->precision->multiple = NO;
    param->precision->answer = "default";
    param->precision->description =
	_("Precision used in the output file (default, max, or 0 to 52)");

    param->compression = G_define_option();
    param->compression->key = "compression";
    param->compression->type = TYPE_STRING;
    param->compression->required = NO;
    param->compression->multiple = NO;
    param->compression->answer = "default";
    param->compression->options = "default,rle,none";
    param->compression->description =
	_("The compression method used in the output file");

    param->dimension = G_define_option();
    param->dimension->key = "tiledimension";
    param->dimension->type = TYPE_STRING;
    param->dimension->required = NO;
    param->dimension->multiple = NO;
    param->dimension->key_desc = "XxYxZ";
    param->dimension->answer = "default";
    param->dimension->description =
	_("The dimensions of the tiles used in the output file");
}

/*----------------------------------------------------------------------------*/

int G3d_getStandard3dParams(int *useTypeDefault, int *type,
			    int *useLzwDefault, int *doLzw,
			    int *useRleDefault, int *doRle,
			    int *usePrecisionDefault, int *precision,
			    int *useDimensionDefault, int *tileX, int *tileY,
			    int *tileZ)
{
    int doCompress;

    *useTypeDefault = *useLzwDefault = *useRleDefault = 0;
    *usePrecisionDefault = *useDimensionDefault = 0;

    G3d_initDefaults();

    if (strcmp(param->type->answer, "double") == 0)
	*type = DCELL_TYPE;
    else if (strcmp(param->type->answer, "float") == 0)
	*type = FCELL_TYPE;
    else {
	*type = G3d_getFileType();
	*useTypeDefault = 1;
    }

    G3d_getCompressionMode(&doCompress, doLzw, doRle, precision);

    if (strcmp(param->precision->answer, "default") != 0) {
	if (strcmp(param->precision->answer, "max") == 0)
	    *precision = -1;
	else if ((sscanf(param->precision->answer, "%d", precision) != 1) ||
		 (*precision < 0)) {
	    G3d_error(_("G3d_getStandard3dParams: precision value invalid"));
	    return 0;
	}
    }
    else
	*usePrecisionDefault = 1;


    if (strcmp(param->compression->answer, "default") != 0) {
	if (strcmp(param->compression->answer, "rle") == 0) {
	    *doRle = G3D_USE_RLE;
	    *doLzw = G3D_NO_LZW;
	}
	else if (strcmp(param->compression->answer, "lzw") == 0) {
	    *doRle = G3D_NO_RLE;
	    *doLzw = G3D_USE_LZW;
	}
	else if (strcmp(param->compression->answer, "rle+lzw") == 0) {
	    *doRle = G3D_USE_RLE;
	    *doLzw = G3D_USE_LZW;
	}
	else {
	    *doRle = G3D_NO_RLE;
	    *doLzw = G3D_NO_LZW;
	}
    }
    else
	*useLzwDefault = *useRleDefault = 1;

    G3d_getTileDimension(tileX, tileY, tileZ);
    if (strcmp(param->dimension->answer, "default") != 0) {
	if (sscanf(param->dimension->answer, "%dx%dx%d",
		   tileX, tileY, tileZ) != 3) {
	    G3d_error(_("G3d_getStandard3dParams: tile dimension value invalid"));
	    return 0;
	}
    }
    else
	*useDimensionDefault = 1;

    G3d_free(param);

    return 1;
}

/*----------------------------------------------------------------------------*/

static struct Option *windowParam = NULL;

void G3d_setWindowParams(void)
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

char *G3d_getWindowParams(void)
{
    if (windowParam == NULL)
	return NULL;
    if (windowParam->answer == NULL)
	return NULL;
    if (strcmp(windowParam->answer, G3D_WINDOW_ELEMENT) == 0)
	return G_store(G3D_WINDOW_ELEMENT);
    return G_store(windowParam->answer);
}
