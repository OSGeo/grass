
/****************************************************************************
*
* MODULE:       r3.cross.rast 
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert at gmx de
* 		23 Feb 2006 Berlin
* PURPOSE:      Creates a cross section 2D map from one G3D raster map based on a 2D elevation map  
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
#include <grass/G3d.h>
#include <grass/glocale.h>


/*- param and global variables -----------------------------------------*/
typedef struct
{
    struct Option *input, *output, *elevation;
    struct Flag *mask;
} paramType;

paramType param;		/*param */
int globalElevMapType;


/*- prototypes --------------------------------------------------------------*/
void fatal_error(void *map, int elevfd, int outfd, char *errorMsg);	/*Simple Error message */
void set_params();		/*Fill the paramType structure */
void rast3d_cross_section(void *map, G3D_Region region, int elevfd, int outfd);	/*Write the raster */
void close_output_map(int fd);	/*close the map */



/* ************************************************************************* */
/* Error handling ********************************************************** */
/* ************************************************************************* */
void fatal_error(void *map, int elevfd, int outfd, char *errorMsg)
{

    /* Close files and exit */

    if (map != NULL) {
	if (!G3d_closeCell(map))
	    G3d_fatalError(_("Could not close G3D map"));
    }

    /*unopen the output map */
    if (outfd != -1)
	G_unopen_cell(outfd);

    if (elevfd != -1)
	close_output_map(elevfd);

    G3d_fatalError(errorMsg);
    exit(EXIT_FAILURE);

}


/* ************************************************************************* */
/* Close the raster map ********************************************* */
/* ************************************************************************* */
void close_output_map(int fd)
{
    if (G_close_cell(fd) < 0)
	G_fatal_error(_("Unable to close output map"));
}


/* ************************************************************************* */
/* Set up the arguments we are expecting *********************************** */
/* ************************************************************************* */
void set_params()
{
    param.input = G_define_option();
    param.input->key = "input";
    param.input->type = TYPE_STRING;
    param.input->required = YES;
    param.input->gisprompt = "old,grid3,3d-raster";
    param.input->description = _("Input 3D raster map for cross section.");

    param.elevation = G_define_option();
    param.elevation->key = "elevation";
    param.elevation->type = TYPE_STRING;
    param.elevation->required = YES;
    param.elevation->description =
	_("2D elevation map used to create the cross section map");
    param.elevation->gisprompt = "old,cell,raster";

    param.output = G_define_option();
    param.output->key = "output";
    param.output->type = TYPE_STRING;
    param.output->required = YES;
    param.output->description = _("Resulting cross section 2D raster map");
    param.output->gisprompt = "new,cell,raster";

    param.mask = G_define_flag();
    param.mask->key = 'm';
    param.mask->description = _("Use g3d mask (if exists) with input map");
}



/* ************************************************************************* */
/* Calulates the resulting raster map ************************************** */
/* ************************************************************************* */
void rast3d_cross_section(void *map, G3D_Region region, int elevfd, int outfd)
{
    double d1 = 0, f1 = 0;
    int x, y, z, check = 0;
    int rows, cols, depths, typeIntern;
    FCELL *fcell = NULL;
    DCELL *dcell = NULL;
    void *elevrast;
    void *ptr;
    int intvalue;
    float fvalue;
    double dvalue;
    int isnull;
    double elevation = 0;
    double top, tbres, bottom;

    /*shoter names ;) */
    rows = region.rows;
    cols = region.cols;
    depths = region.depths;
    top = region.top;
    bottom = region.bottom;

    /*Calculate the top-bottom resolution */
    tbres = (top - bottom) / depths;

    /*Typ of the G3D Tile */
    typeIntern = G3d_tileTypeMap(map);

    /*Allocate mem for the output maps row */
    if (typeIntern == FCELL_TYPE)
	fcell = G_allocate_f_raster_buf();
    else if (typeIntern == DCELL_TYPE)
	dcell = G_allocate_d_raster_buf();

    /*Mem for the input map row */
    elevrast = G_allocate_raster_buf(globalElevMapType);

    for (y = 0; y < rows; y++) {
	G_percent(y, rows - 1, 10);

	/*Read the input map row */
	if (!G_get_raster_row(elevfd, elevrast, y, globalElevMapType))
	    fatal_error(map, elevfd, outfd,
			_("Unable to get elevation raster row"));

	for (x = 0, ptr = elevrast; x < cols; x++, ptr =
	     G_incr_void_ptr(ptr, G_raster_size(globalElevMapType))) {

	    /*we guess the elevation input map has no null values */
	    isnull = 0;

	    if (G_is_null_value(ptr, globalElevMapType)) {
		isnull = 1;	/*input map has nulls */
	    }

	    /*Read the elevation value */
	    if (globalElevMapType == CELL_TYPE) {
		intvalue = *(CELL *) ptr;
		elevation = intvalue;
	    }
	    else if (globalElevMapType == FCELL_TYPE) {
		fvalue = *(FCELL *) ptr;
		elevation = fvalue;
	    }
	    else if (globalElevMapType == DCELL_TYPE) {
		dvalue = *(DCELL *) ptr;
		elevation = dvalue;
	    }

	    /*Check if the elevation is located in the 3d raster map */
	    if (isnull == 0) {
		/*we guess, we have no hit */
		isnull = 1;
		for (z = 0; z < depths; z++) {	/*From the bottom to the top */
		    if (elevation >= z * tbres + bottom && elevation <= (z + 1) * tbres + bottom) {	/*if at the border, choose the value from the top */
			/*Read the value and put it in the output map row */
			if (typeIntern == FCELL_TYPE) {
			    G3d_getValue(map, x, y, z, &f1, typeIntern);
			    if (G3d_isNullValueNum(&f1, FCELL_TYPE))
				G_set_null_value(&fcell[x], 1, FCELL_TYPE);
			    else
				fcell[x] = (FCELL) f1;
			}
			else {
			    G3d_getValue(map, x, y, z, &d1, typeIntern);
			    if (G3d_isNullValueNum(&d1, DCELL_TYPE))
				G_set_null_value(&dcell[x], 1, DCELL_TYPE);
			    else
				dcell[x] = (DCELL) d1;

			}
			/*no NULL value should be set */
			isnull = 0;
		    }
		}
	    }

	    /*Set the NULL values */
	    if (isnull == 1) {
		if (typeIntern == FCELL_TYPE)
		    G_set_null_value(&fcell[x], 1, FCELL_TYPE);
		else if (typeIntern == DCELL_TYPE)
		    G_set_null_value(&dcell[x], 1, DCELL_TYPE);
	    }
	}

	/*Write the data to the output map */
	if (typeIntern == FCELL_TYPE) {
	    check = G_put_f_raster_row(outfd, fcell);
	    if (check != 1)
		fatal_error(map, elevfd, outfd,
			    _("Could not write raster row"));
	}

	if (typeIntern == DCELL_TYPE) {
	    check = G_put_d_raster_row(outfd, dcell);
	    if (check != 1)
		fatal_error(map, elevfd, outfd,
			    _("Could not write raster row"));
	}
    }
    G_debug(3, "\nDone\n");

    /*Free the mem */
    if (elevrast)
	G_free(elevrast);
    if (dcell)
	G_free(dcell);
    if (fcell)
	G_free(fcell);
}


/* ************************************************************************* */
/* Main function, open the G3D map and create the cross section map ******** */
/* ************************************************************************* */
int main(int argc, char *argv[])
{
    G3D_Region region;
    struct Cell_head window2d;
    struct GModule *module;
    void *map = NULL;		/*The 3D Rastermap */
    int changemask = 0;
    int elevfd = -1, outfd = -1;	/*file descriptors */
    int output_type, cols, rows;
    char *mapset = NULL;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster3d, voxel");
    module->description =
	_("Creates cross section 2D raster map from 3d raster map based on 2D elevation map");

    /* Get parameters from user */
    set_params();

    /* Have GRASS get inputs */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_debug(3, "Open 3D raster map %s", param.input->answer);

    if (NULL == G_find_grid3(param.input->answer, ""))
	G3d_fatalError(_("3d raster map <%s> not found"),
		       param.input->answer);

    /* Figure out the region from the map */
    G3d_initDefaults();
    G3d_getWindow(&region);

    /*Check if the g3d-region is equal to the 2d rows and cols */
    rows = G_window_rows();
    cols = G_window_cols();

    /*If not equal, set the 2D windows correct */
    if (rows != region.rows || cols != region.cols) {
	G_message
	    (_("The 2d and 3d region settings are different. I will use the g3d settings to adjust the 2d region."));
	G_get_set_window(&window2d);
	window2d.ns_res = region.ns_res;
	window2d.ew_res = region.ew_res;
	window2d.rows = region.rows;
	window2d.cols = region.cols;
	G_set_window(&window2d);
    }


    /*******************/
    /*Open the 3d raster map */

    /*******************/
    map = G3d_openCellOld(param.input->answer,
			  G_find_grid3(param.input->answer, ""),
			  &region, G3D_TILE_SAME_AS_FILE,
			  G3D_USE_CACHE_DEFAULT);

    if (map == NULL)
	G3d_fatalError(_("Error opening 3d raster map <%s>"),
		       param.input->answer);

    /*Get the output type */
    output_type = G3d_fileTypeMap(map);

    if (output_type == FCELL_TYPE || output_type == DCELL_TYPE) {

	/********************************/
	/*Open the elevation raster map */

	/********************************/
	mapset = G_find_cell2(param.elevation->answer, "");

	if (mapset == NULL) {
	    fatal_error(map, -1, -1, _("Elevation map not found"));
	}

	elevfd = G_open_cell_old(param.elevation->answer, mapset);
	if (elevfd <= 0)
	    fatal_error(map, -1, -1, _("Unable to open elevation map"));

	globalElevMapType = G_get_raster_map_type(elevfd);

	/**********************/
	/*Open the Outputmap */

	/**********************/

	/*Filename check for output map */
	if (G_legal_filename(param.output->answer) < 0)
	    fatal_error(map, elevfd, -1, _("Illegal output file name"));

	if (G_find_cell2(param.output->answer, ""))
	    G_message(_("Output map already exists. Will be overwritten!"));

	if (output_type == FCELL_TYPE) {
	    outfd = G_open_raster_new(param.output->answer, FCELL_TYPE);
	    if (outfd < 0)
		fatal_error(map, elevfd, -1,
			    _("Unable to create raster map"));
	}
	else if (output_type == DCELL_TYPE) {
	    outfd = G_open_raster_new(param.output->answer, DCELL_TYPE);
	    if (outfd < 0)
		fatal_error(map, elevfd, -1,
			    _("Unable to create raster map"));
	}

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

	/************************/
	/*Create the Rastermaps */

	/************************/
	rast3d_cross_section(map, region, elevfd, outfd);

	/*We set the Mask off, if it was off before */
	if (param.mask->answer) {
	    if (G3d_maskFileExists())
		if (G3d_maskIsOn(map) && changemask)
		    G3d_maskOff(map);
	}

	if (G_close_cell(outfd) < 0)
	    fatal_error(map, elevfd, -1, _("Unable to close output map"));
	if (G_close_cell(elevfd) < 0)
	    fatal_error(map, -1, -1, _("Unable to close elevation map"));

    }
    else {
	fatal_error(map, -1, -1,
		    _("Wrong G3D Datatype! Cannot create raster map."));
    }

    /* Close files and exit */
    if (!G3d_closeCell(map))
	G3d_fatalError(_("Could not close G3D map <%s>"),
		       param.input->answer);

    return (EXIT_SUCCESS);
}
