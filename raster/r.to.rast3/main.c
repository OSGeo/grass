
/****************************************************************************
*
* MODULE:       r.to.rast3 
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert@gmx.de
* 		08 01 2005 Berlin
* PURPOSE:      Converts 2D raster map slices to one 3D volume map  
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/G3d.h>
#include <grass/glocale.h>
#include <grass/config.h>

/*- params and global variables -----------------------------------------*/
typedef struct
{
    struct Option *input, *output, *tilesize;
    struct Flag *mask;
} paramType;

paramType param;		/*params */
int globalRastMapType;
int globalG3dMapType;


/*- prototypes --------------------------------------------------------------*/
void fatal_error(void *map, int *fd, int depths, char *errorMsg);	/*Simple Error message */
void set_params();		/*Fill the paramType structure */
void raster_to_g3d(void *map, G3D_Region region, int *fd);	/*Write the raster */
int open_input_raster_map(const char *name);	/*opens the outputmap */
void close_input_raster_map(int fd);	/*close the map */



/* ************************************************************************* */
/* Error handling ********************************************************** */
/* ************************************************************************* */
void fatal_error(void *map, int *fd, int depths, char *errorMsg)
{
    int i;

    /* Close files and exit */
    if (map != NULL) {
	/* should unopen map here! but this functionality is not jet implemented */
	if (!G3d_closeCell(map))
	    G3d_fatalError(_("Could not close the map"));
    }

    if (fd != NULL) {
	for (i = 0; i < depths; i++)
	    close_input_raster_map(fd[i]);
    }

    G3d_fatalError(errorMsg);
    exit(EXIT_FAILURE);

}

/* ************************************************************************* */
/* Setg up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params()
{
    param.input = G_define_standard_option(G_OPT_R_INPUTS);
    param.input->description = _("2d raster maps which represent the slices");

    param.output = G_define_standard_option(G_OPT_R3_OUTPUT);

    param.tilesize = G_define_option();
    param.tilesize->description = _("The maximum tile size in kilo bytes. Default is 32KB.");
    param.tilesize->key = "tilesize";
    param.tilesize->answer = "32";
    param.tilesize->type = TYPE_INTEGER;
    param.tilesize->required = NO;
    param.tilesize->multiple = NO;

    param.mask = G_define_flag();
    param.mask->key = 'm';
    param.mask->description = _("Use G3D mask (if exists) with output map");

}



/* ************************************************************************* */
/* Write the raster maps into one G3D map ********************************** */
/* ************************************************************************* */
void raster_to_g3d(void *map, G3D_Region region, int *fd)
{
    int x, y, z;
    int rows, cols, depths;
    void *rast;
    void *ptr;
    FCELL fvalue;
    DCELL dvalue;

    rows = region.rows;
    cols = region.cols;
    depths = region.depths;

    rast = Rast_allocate_buf(globalRastMapType);

    G_debug(3, "raster_to_g3d: Writing %i raster maps with %i rows %i cols.",
	    depths, rows, cols);

    /*Every Rastermap */
    for (z = 0; z < depths; z++) {	/*From the bottom to the top */
	G_debug(4, "Writing g3d slice %i", z + 1);
	for (y = 0; y < rows; y++) {
	    G_percent(y, rows - 1, 10);

	    Rast_get_row(fd[z], rast, y, globalRastMapType);

	    for (x = 0, ptr = rast; x < cols; x++,
		 ptr =
		 G_incr_void_ptr(ptr, Rast_cell_size(globalRastMapType))) {
		if (globalRastMapType == CELL_TYPE) {
		    if (Rast_is_null_value(ptr, globalRastMapType)) {
			G3d_setNullValue(&dvalue, 1, DCELL_TYPE);
		    }
		    else {
			dvalue = *(CELL *) ptr;
		    }
		    if (G3d_putValue
			(map, x, y, z, (char *)&dvalue, DCELL_TYPE) < 0)
			fatal_error(map, fd, depths,
				    "Error writing double data");
		}
		else if (globalRastMapType == FCELL_TYPE) {
		    if (Rast_is_null_value(ptr, globalRastMapType)) {
			G3d_setNullValue(&fvalue, 1, FCELL_TYPE);
		    }
		    else {
			fvalue = *(FCELL *) ptr;
		    }
		    if (G3d_putValue
			(map, x, y, z, (char *)&fvalue, FCELL_TYPE) < 0)
			fatal_error(map, fd, depths,
				    "Error writing float data");

		}
		else if (globalRastMapType == DCELL_TYPE) {
		    if (Rast_is_null_value(ptr, globalRastMapType)) {
			G3d_setNullValue(&dvalue, 1, DCELL_TYPE);
		    }
		    else {
			dvalue = *(DCELL *) ptr;
		    }
		    if (G3d_putValue
			(map, x, y, z, (char *)&dvalue, DCELL_TYPE) < 0)
			fatal_error(map, fd, depths,
				    "Error writing double data");

		}

	    }
	}
	G_debug(2, "\nDone\n");
    }


    if (rast)
	G_free(rast);

}


/* ************************************************************************* */
/* Main function, open the raster maps and create the G3D raster map ******* */
/* ************************************************************************* */
int main(int argc, char *argv[])
{
    G3D_Region region;
    struct Cell_head window2d;
    struct GModule *module;
    void *map = NULL;		/*The 3D Rastermap */
    int i = 0;
    int *fd = NULL;		/*The filehandler array for the 2D inputmaps */
    int cols, rows, opencells;
    char *name;
    int changemask = 0;
    int maptype_tmp, nofile = 0;
    int maxSize;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("volume"));
    G_add_keyword(_("conversion"));
    module->description =
	_("Converts 2D raster map slices to one 3D raster volume map.");

    /* Get parameters from user */
    set_params();

    /* Have GRASS get inputs */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /*Check for output */
    if (param.output->answer == NULL)
	G3d_fatalError(_("No output map"));

    /* Get the tile size */
    maxSize = atoi(param.tilesize->answer);

    /* Figure out the region from the map */
    G3d_initDefaults();
    G3d_getWindow(&region);

    /*Check if the g3d-region is equal to the 2d rows and cols */
    rows = Rast_window_rows();
    cols = Rast_window_cols();

    G_debug(2, "Check the 2d and 3d region settings");

    /*If not equal, set the 2D windows correct */
    if (rows != region.rows || cols != region.cols) {
	G_message(_("The 2d and 3d region settings are different. I will use the g3d settings to adjust the 2d region."));
	G_get_set_window(&window2d);
	window2d.ns_res = region.ns_res;
	window2d.ew_res = region.ew_res;
	window2d.rows = region.rows;
	window2d.cols = region.cols;
	Rast_set_window(&window2d);
    }


    /*prepare the filehandler */
    fd = (int *)G_malloc(region.depths * sizeof(int));

    if (fd == NULL)
	fatal_error(map, NULL, 0, _("Out of memory"));

    name = NULL;

    globalRastMapType = DCELL_TYPE;
    globalG3dMapType = DCELL_TYPE;
    maptype_tmp = DCELL_TYPE;

    opencells = 0;		/*Number of opened maps */
    /*Loop over all output maps! open */
    for (i = 0; i < region.depths; i++) {
	/*Open only existing maps */
	if (nofile == 0 && param.input->answers[i])
	    name = param.input->answers[i];
	else
	    nofile = 1;

	/*if only one map is given, open it depths - times */
	G_verbose_message(_("Open raster map %s - one time for each depth (%d/%d)"),
		  name, i + 1, region.depths);
	fd[i] = open_input_raster_map(name);
	opencells++;

	maptype_tmp = Rast_get_map_type(fd[i]);

	/*maptype */
	if (i == 0)
	    globalRastMapType = maptype_tmp;

	if (maptype_tmp != globalRastMapType) {
	    fatal_error(map, fd, opencells,
			_("Input maps have to be from the same type. CELL, FCELL or DCELL!"));
	}
    }

    G_message(_("Creating 3D raster map"));
    map = NULL;

    /* Set the map type depending from the arster maps type */
    if (globalRastMapType == CELL_TYPE) 
	globalG3dMapType = DCELL_TYPE;
    else 
	globalG3dMapType = FCELL_TYPE;

    map = G3d_openNewOptTileSize(param.output->answer, G3D_USE_CACHE_XY, &region, globalG3dMapType, maxSize);

    if (map == NULL)
	fatal_error(map, fd, opencells, _("Error opening 3d raster map"));

    /*if requested set the Mask on */
    if (param.mask->answer) {
	if (G3d_maskFileExists()) {
	    changemask = 0;
	    if (G3d_maskIsOff(map)) {
		G3d_maskOn(map);
		changemask = 1;
	    }
	}
    }

    /*Create the G3D Rastermap */
    raster_to_g3d(map, region, fd);

    /*We set the Mask off, if it was off before */
    if (param.mask->answer) {
	if (G3d_maskFileExists())
	    if (G3d_maskIsOn(map) && changemask)
		G3d_maskOff(map);
    }

    /*Loop over all output maps! close */
    for (i = 0; i < region.depths; i++)
	close_input_raster_map(fd[i]);

    if (fd)
	G_free(fd);

    /* Flush all tile */
    if (!G3d_flushAllTiles(map))
	G3d_fatalError("Error flushing tiles with G3d_flushAllTiles");
    /* Close files and exit */
    if (!G3d_closeCell(map))
	G3d_fatalError(_("Error closing 3d raster map"));

    map = NULL;

    G_debug(2, "Done\n");

    return (EXIT_SUCCESS);
}



/* ************************************************************************* */
/* Open the raster input map *********************************************** */
/* ************************************************************************* */
int open_input_raster_map(const char *name)
{
    G_debug(3, "Open Raster file %s", name);

    return Rast_open_old(name, "");
}

/* ************************************************************************* */
/* Close the raster input map ********************************************** */
/* ************************************************************************* */
void close_input_raster_map(int fd)
{
    Rast_close(fd);
}
