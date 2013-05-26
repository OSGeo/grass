
/***************************************************************************
* MODULE:       r3.mask
*
* AUTHOR(S):    Roman Waupotitsch, Michael Shapiro, Helena Mitasova,
*		Bill Brown, Lubos Mitas, Jaro Hofierka
*
* PURPOSE:      Establishes the current working 3D raster mask.
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

/*--------------------------------------------------------------------------*/

typedef struct
{
    struct Option *map, *maskVals;
} paramType;

static paramType params;

/*--------------------------------------------------------------------------*/

void getParams(char **name, d_Mask ** maskRules)
{
    *name = params.map->answer;
    Rast3d_parse_vallist(params.maskVals->answers, maskRules);
}

/*-------------------------------------------------------------------------*/

#define MAX(a,b) (a > b ? a : b)

static void makeMask(char *name, d_Mask * maskRules)
{
    void *map, *mask;
    RASTER3D_Region region;
    int tileX, tileY, tileZ, x, y, z, cacheSize;
    double value;
    float floatNull;

    cacheSize = Rast3d_cache_size_encode(RASTER3D_USE_CACHE_XY, 1);

    if (NULL == G_find_raster3d(name, ""))
	Rast3d_fatal_error(_("3D raster map <%s> not found"), name);

    map = Rast3d_open_cell_old(name, G_mapset(), RASTER3D_DEFAULT_WINDOW,
			  DCELL_TYPE, cacheSize);

    if (map == NULL)
	Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"), name);

    Rast3d_get_region_struct_map(map, &region);

    Rast3d_get_tile_dimensions_map(map, &tileX, &tileY, &tileZ);

    mask = Rast3d_open_new_param(Rast3d_mask_file(), FCELL_TYPE, cacheSize,
			    &region, FCELL_TYPE, RASTER3D_COMPRESSION, 0,
			    tileX, tileY, tileZ);

    if (mask == NULL)
	Rast3d_fatal_error(_("Unable to open 3D raster mask file"));

    Rast3d_min_unlocked(map, RASTER3D_USE_CACHE_X);
    Rast3d_autolock_on(map);
    Rast3d_unlock_all(map);
    Rast3d_min_unlocked(mask, RASTER3D_USE_CACHE_X);
    Rast3d_autolock_on(mask);
    Rast3d_unlock_all(mask);

    Rast3d_set_null_value(&floatNull, 1, FCELL_TYPE);

    for (z = 0; z < region.depths; z++) {
	if ((z % tileZ) == 0) {
	    Rast3d_unlock_all(map);
	    Rast3d_unlock_all(mask);
	}
    
	for (y = 0; y < region.rows; y++)	/* We count from north to south in the cube coordinate system */
	    for (x = 0; x < region.cols; x++) {
		value = Rast3d_get_double_region(map, x, y, z);
		if (Rast3d_mask_d_select((DCELL *) & value, maskRules))
		    Rast3d_put_float(mask, x, y, z, (float)floatNull);	/* mask-out value */
		else
		    Rast3d_put_float(mask, x, y, z, (float)0.0);	/* not mask-out value */
	    }
	if ((z % tileZ) == 0) {
	    if (!Rast3d_flush_tiles_in_cube
		(mask, 0, 0, MAX(0, z - tileZ), region.rows - 1,
		 region.cols - 1, z))
		Rast3d_fatal_error(_("makeMask: error flushing tiles in cube"));
	}
    }

    if (!Rast3d_flush_all_tiles(mask))
	Rast3d_fatal_error(_("makeMask: error flushing all tiles"));

    Rast3d_autolock_off(map);
    Rast3d_unlock_all(map);
    Rast3d_autolock_off(mask);
    Rast3d_unlock_all(mask);

    if (!Rast3d_close(mask))
	Rast3d_fatal_error(_("Unable to close 3D raster mask file"));
    if (!Rast3d_close(map))
	Rast3d_fatal_error(_("Unable to close raster map <%s>"), name);
}

/*--------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    char *name;
    d_Mask *maskRules;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("mask"));
    G_add_keyword(_("voxel"));
    module->description =
	_("Establishes the current working 3D raster mask.");

    params.map = G_define_option();
    params.map->key = "map";
    params.map->type = TYPE_STRING;
    params.map->required = YES;
    params.map->multiple = NO;
    params.map->gisprompt = "old,grid3,3d-raster";
    params.map->description = _("3D raster map with reference values");

    params.maskVals = G_define_option();
    params.maskVals->key = "maskvalues";
    params.maskVals->key_desc = "val[-val]";
    params.maskVals->type = TYPE_STRING;
    params.maskVals->required = NO;
    params.maskVals->multiple = YES;
    params.maskVals->description = _("List of cell values to be masked out");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (Rast3d_mask_file_exists())
	G_fatal_error(_("Cannot create mask file: RASTER3D_MASK already exists"));

    getParams(&name, &maskRules);

    makeMask(name, maskRules);

    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
