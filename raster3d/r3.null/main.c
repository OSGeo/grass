
/***************************************************************************
* MODULE:       r3.null
*
* AUTHOR(S):    Roman Waupotitsch, Michael Shapiro, Helena Mitasova,
*               Bill Brown, Lubos Mitas, Jaro Hofierka
*
* PURPOSE:      Explicitly create the 3D NULL-value bitmap file.
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>


#define MAX(a,b) (a > b ? a : b)


typedef struct
{
    struct Option *map, *setNull, *null;
} paramType;

static paramType params;


/* function prototypes */
static void setParams(void);
static void getParams(char **name, d_Mask ** maskRules, int *changeNull,
		      double *newNullVal);
static void modifyNull(char *name, d_Mask * maskRules, int changeNull,
		       double newNullVal);

static void setParams(void)
{
    params.map = G_define_option();
    params.map->key = "map";
    params.map->type = TYPE_STRING;
    params.map->required = YES;
    params.map->multiple = NO;
    params.map->gisprompt = "old,grid3,3d-raster";
    params.map->description =
	_("3d raster map for which to modify null values");

    params.setNull = G_define_option();
    params.setNull->key = "setnull";
    params.setNull->key_desc = "val[-val]";
    params.setNull->type = TYPE_STRING;
    params.setNull->required = NO;
    params.setNull->multiple = YES;
    params.setNull->description = _("List of cell values to be set to NULL");

    params.null = G_define_option();
    params.null->key = "null";
    params.null->type = TYPE_DOUBLE;
    params.null->required = NO;
    params.null->multiple = NO;
    params.null->description = _("The value to replace the null value by");
}

/*--------------------------------------------------------------------------*/

static void
getParams(char **name, d_Mask ** maskRules, int *changeNull,
	  double *newNullVal)
{
    *name = params.map->answer;
    Rast3d_parse_vallist(params.setNull->answers, maskRules);

    *changeNull = (params.null->answer != NULL);
    if (*changeNull)
	if (sscanf(params.null->answer, "%lf", newNullVal) != 1)
	    Rast3d_fatal_error(_("Illegal value for null"));
}

/*-------------------------------------------------------------------------*/

static void
modifyNull(char *name, d_Mask * maskRules, int changeNull, double newNullVal)
{
    void *map, *mapOut;
    RASTER3D_Region region;
    int tileX, tileY, tileZ, x, y, z;
    double value;
    int doCompress, doLzw, doRle, precision;
    int cacheSize;

    cacheSize = Rast3d_cache_size_encode(RASTER3D_USE_CACHE_XY, 1);

    if (NULL == G_find_raster3d(name, ""))
	Rast3d_fatal_error(_("3D raster map <%s> not found"), name);

    fprintf(stderr, "name %s Mapset %s \n", name, G_mapset());
    map = Rast3d_open_cell_old(name, G_mapset(), RASTER3D_DEFAULT_WINDOW,
			  DCELL_TYPE, cacheSize);

    if (map == NULL)
	Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"), name);

    Rast3d_get_region_struct_map(map, &region);
    Rast3d_get_tile_dimensions_map(map, &tileX, &tileY, &tileZ);

    Rast3d_get_compression_mode(&doCompress, &precision);

    mapOut = Rast3d_open_new_param(name, DCELL_TYPE, RASTER3D_USE_CACHE_XY,
			      &region, Rast3d_file_type_map(map),
			      doCompress, Rast3d_tile_precision_map(map), tileX,
			      tileY, tileZ);
    if (mapOut == NULL)
	Rast3d_fatal_error(_("modifyNull: error opening tmp file"));

    Rast3d_min_unlocked(map, RASTER3D_USE_CACHE_X);
    Rast3d_autolock_on(map);
    Rast3d_unlock_all(map);
    Rast3d_min_unlocked(mapOut, RASTER3D_USE_CACHE_X);
    Rast3d_autolock_on(mapOut);
    Rast3d_unlock_all(mapOut);

	for (z = 0; z < region.depths; z++) {
	if ((z % tileZ) == 0) {
	    Rast3d_unlock_all(map);
	    Rast3d_unlock_all(mapOut);
	}
	for (y = 0; y < region.rows; y++)
	    for (x = 0; x < region.cols; x++) {

		value = Rast3d_get_double_region(map, x, y, z);

		if (Rast3d_is_null_value_num(&value, DCELL_TYPE)) {
		    if (changeNull) {
			value = newNullVal;
		    }
		}
		else if (Rast3d_mask_d_select((DCELL *) & value, maskRules)) {
		    Rast3d_set_null_value(&value, 1, DCELL_TYPE);
		}

		Rast3d_put_double(mapOut, x, y, z, value);
	    }
	if ((z % tileZ) == 0) {
	    if (!Rast3d_flush_tiles_in_cube
		(mapOut, 0, 0, MAX(0, z - tileZ), region.rows - 1,
		 region.cols - 1, z))
		Rast3d_fatal_error(_("modifyNull: error flushing tiles in cube"));
	}
    }

    if (!Rast3d_flush_all_tiles(mapOut))
	Rast3d_fatal_error(_("modifyNull: error flushing all tiles"));

    Rast3d_autolock_off(map);
    Rast3d_unlock_all(map);
    Rast3d_autolock_off(mapOut);
    Rast3d_unlock_all(mapOut);

    if (!Rast3d_close(map))
	Rast3d_fatal_error(_("Unable to close 3D raster map <%s>"), name);
    if (!Rast3d_close(mapOut))
	Rast3d_fatal_error(_("modifyNull: Unable to close tmp file"));
}

/*--------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
    char *name;
    d_Mask *maskRules;
    int changeNull;
    double newNullVal;
    struct GModule *module;

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("voxel"));
    module->description =
	_("Explicitly create the 3D NULL-value bitmap file.");

    setParams();
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    getParams(&name, &maskRules, &changeNull, &newNullVal);

    modifyNull(name, maskRules, changeNull, newNullVal);

    exit(EXIT_SUCCESS);
}
