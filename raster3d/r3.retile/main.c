
/****************************************************************************
 *
 * MODULE:       r3.retile
 *   	    	
 * AUTHOR(S):    Original author 
 *               Soeren Gebbert soerengebbert <at> googlemail <dot> co
 * 
 * PURPOSE:      Retiles an existing RASTER3D map with user defined x, y and z tile size
 *
 * COPYRIGHT:    (C) 2011 by the GRASS Development Team
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
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

/*- Parameters and global variables -----------------------------------------*/
typedef struct {
    struct Option *input, *output, *tiling;
    struct Flag *cache;
} paramType;

paramType param; /*Parameters */

/*- prototypes --------------------------------------------------------------*/
static void fatal_error(void *map, int *fd, int depths, char *errorMsg); /*Simple Error message */
static void set_params(); /*Fill the paramType structure */

/* ************************************************************************* */
/* Error handling ********************************************************** */

/* ************************************************************************* */
void fatal_error(void *map, int *fd, int depths, char *errorMsg)
{
    int i;

    /* Close files and exit */
    if (map != NULL) {
        if (!Rast3d_close(map))
            Rast3d_fatal_error(_("Unable to close the 3D raster map"));
    }

    if (fd != NULL) {
        for (i = 0; i < depths; i++)
            Rast_unopen(fd[i]);
    }

    Rast3d_fatal_error(errorMsg);
    exit(EXIT_FAILURE);

}

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */

/* ************************************************************************* */
void set_params()
{
    param.input = G_define_standard_option(G_OPT_R3_INPUT);

    param.output = G_define_standard_option(G_OPT_R3_OUTPUT);
    param.output->description = _("Name of the retiled 3D raster map");
    
    param.tiling = G_define_standard_option(G_OPT_R3_TILE_DIMENSION);
    
    param.cache = G_define_flag();
    param.cache->key = 'c';
    param.cache->description = "Disable tile caching";
}


/* ************************************************************************* */
/* Main function, open the RASTER3D map and create the raster maps ************** */

/* ************************************************************************* */
int main(int argc, char *argv[])
{
    struct GModule *module;
    RASTER3D_Map *map = NULL;
    int tileX, tileY, tileZ;
    char *mapset;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("tiling"));
    G_add_keyword(_("voxel"));
    module->description = _("Retiles an existing 3D raster map with user defined x, y and z tile size.");

    /* Get parameters from user */
    set_params();

    /* Have GRASS get inputs */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    G_debug(3, "Open 3D raster map <%s>", param.input->answer);

    mapset = G_find_raster3d(param.input->answer, "");
    
    if (mapset == NULL)
        Rast3d_fatal_error(_("3D raster map <%s> not found"),
                       param.input->answer);

    /*Set the defaults */
    Rast3d_init_defaults();
    
    if(!param.cache->answer)
        map = Rast3d_open_cell_old(param.input->answer, mapset, RASTER3D_DEFAULT_WINDOW, 
                          RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);
    else
        map = Rast3d_open_cell_old(param.input->answer, mapset, RASTER3D_DEFAULT_WINDOW, 
                          RASTER3D_TILE_SAME_AS_FILE, RASTER3D_NO_CACHE);

    if (map == NULL)
        Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
                       param.input->answer);

    /* Get the tile dimension */
    Rast3d_get_tile_dimension(&tileX, &tileY, &tileZ);    
    if (strcmp(param.tiling->answer, "default") != 0) {
	if (sscanf(param.tiling->answer, "%dx%dx%d",
		   &tileX, &tileY, &tileZ) != 3) {
	    Rast3d_fatal_error(_("Rast3d_get_standard3d_params: tile dimension value invalid"));
	}
    }
    
    if(!param.cache->answer)
        G_message("Retile map with tile cache enabled");
    else
        G_message("Retile map without tile caching");
    
    Rast3d_retile(map, param.output->answer, tileX, tileY, tileZ);

    /* Close files and exit */
    if (!Rast3d_close(map))
        fatal_error(map, NULL, 0, _("Error closing 3D raster map"));

    map = NULL;

    return (EXIT_SUCCESS);
}
