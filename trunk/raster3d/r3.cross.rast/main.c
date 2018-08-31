
/****************************************************************************
 *
 * MODULE:       r3.cross.rast 
 *
 * AUTHOR(S):    Original author 
 *               Soeren Gebbert soerengebbert at gmx de
 *               23 Feb 2006 Berlin
 * PURPOSE:      Creates a cross section 2D map from one RASTER3D raster map based on a 2D elevation map  
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
#include <grass/raster3d.h>
#include <grass/glocale.h>

/*- param and global variables -----------------------------------------*/
typedef struct {
    struct Option *input, *output, *elevation;
    struct Flag *mask;
} paramType;

paramType param; /*param */
int globalElevMapType;


/*- prototypes --------------------------------------------------------------*/
void fatal_error(void *map, int elevfd, int outfd, char *errorMsg); /*Simple Error message */
void set_params(); /*Fill the paramType structure */
void rast3d_cross_section(void *map, RASTER3D_Region region, int elevfd, int outfd); /*Write the raster */
void close_output_map(int fd); /*close the map */



/* ************************************************************************* */
/* Error handling ********************************************************** */
/* ************************************************************************* */
void fatal_error(void *map, int elevfd, int outfd, char *errorMsg)
{

    /* Close files and exit */

    if (map != NULL) {
        if (!Rast3d_close(map))
            Rast3d_fatal_error(_("Unable to close 3D raster map"));
    }

    /*unopen the output map */
    if (outfd != -1)
        Rast_unopen(outfd);

    if (elevfd != -1)
        close_output_map(elevfd);

    Rast3d_fatal_error("%s", errorMsg);
    exit(EXIT_FAILURE);

}


/* ************************************************************************* */
/* Close the raster map ********************************************* */
/* ************************************************************************* */
void close_output_map(int fd)
{
    Rast_close(fd);
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
    param.input->description = _("Input 3D raster map for cross section");

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
    param.mask->description = _("Use 3D raster mask (if exists) with input map");
}



/* ************************************************************************* */
/* Compute the cross section raster map ************************************ */
/* ************************************************************************* */
void rast3d_cross_section(void *map,RASTER3D_Region region, int elevfd, int outfd)
{
    int col, row;
    int rows, cols, depths, typeIntern;
    FCELL *fcell = NULL;
    DCELL *dcell = NULL;
    void *elevrast;
    void *ptr;
    int intvalue;
    float fvalue;
    double dvalue;
    double elevation = 0;
    double north, east;
    struct Cell_head window;
 
    Rast_get_window(&window);
    
    rows = region.rows;
    cols = region.cols;
    depths = region.depths;
    
    /*Typ of the RASTER3D Tile */
    typeIntern = Rast3d_tile_type_map(map);

    /*Allocate mem for the output maps row */
    if (typeIntern == FCELL_TYPE)
        fcell = Rast_allocate_f_buf();
    else if (typeIntern == DCELL_TYPE)
        dcell = Rast_allocate_d_buf();

    /*Mem for the input map row */
    elevrast = Rast_allocate_buf(globalElevMapType);

    for (row = 0; row < rows; row++) {
        G_percent(row, rows - 1, 10);

        /*Read the input map row */
        Rast_get_row(elevfd, elevrast, row, globalElevMapType);

        for (col = 0, ptr = elevrast; col < cols; col++, ptr =
            G_incr_void_ptr(ptr, Rast_cell_size(globalElevMapType))) {

            if (Rast_is_null_value(ptr, globalElevMapType)) {
                if (typeIntern == FCELL_TYPE)
                    Rast_set_null_value(&fcell[col], 1, FCELL_TYPE);
                else if (typeIntern == DCELL_TYPE)
                    Rast_set_null_value(&dcell[col], 1, DCELL_TYPE);
                continue;
            }

            /*Read the elevation value */
            if (globalElevMapType == CELL_TYPE) {
                intvalue = *(CELL *) ptr;
                elevation = intvalue;
            } else if (globalElevMapType == FCELL_TYPE) {
                fvalue = *(FCELL *) ptr;
                elevation = fvalue;
            } else if (globalElevMapType == DCELL_TYPE) {
                dvalue = *(DCELL *) ptr;
                elevation = dvalue;
            }

            /* Compute the coordinates */
            north = Rast_row_to_northing((double)row + 0.5, &window);
            east = Rast_col_to_easting((double)col + 0.5, &window);

            /* Get the voxel value */
            if (typeIntern == FCELL_TYPE)
                Rast3d_get_region_value(map, north, east, elevation, &fcell[col], FCELL_TYPE);

            if (typeIntern == DCELL_TYPE)
                Rast3d_get_region_value(map, north, east, elevation, &dcell[col], DCELL_TYPE);
        }

        /*Write the data to the output map */
        if (typeIntern == FCELL_TYPE)
            Rast_put_f_row(outfd, fcell);

        if (typeIntern == DCELL_TYPE)
            Rast_put_d_row(outfd, dcell);
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
/* Main function, open the RASTER3D map and create the cross section map ******** */

/* ************************************************************************* */
int main(int argc, char *argv[])
{
    RASTER3D_Region region;
    struct Cell_head window2d;
    struct GModule *module;
    void *map = NULL; /*The 3D Rastermap */
    int changemask = 0;
    int elevfd = -1, outfd = -1; /*file descriptors */
    int output_type, cols, rows;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("profile"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("voxel"));
    module->description =
        _("Creates cross section 2D raster map from 3D raster map based on 2D elevation map");

    /* Get parameters from user */
    set_params();

    /* Have GRASS get inputs */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    G_debug(3, "Open 3D raster map %s", param.input->answer);

    if (NULL == G_find_raster3d(param.input->answer, ""))
        Rast3d_fatal_error(_("3D raster map <%s> not found"),
                       param.input->answer);

    /* Figure out the region from the map */
    Rast3d_init_defaults();
    Rast3d_get_window(&region);

    /*Check if the g3d-region is equal to the 2d rows and cols */
    rows = Rast_window_rows();
    cols = Rast_window_cols();

    /*If not equal, set the 2D windows correct */
    if (rows != region.rows || cols != region.cols) {
        G_message
            (_("The 2D and 3D region settings are different. Using the 3D raster map settings to adjust the 2D region."));
        G_get_set_window(&window2d);
        window2d.ns_res = region.ns_res;
        window2d.ew_res = region.ew_res;
        window2d.rows = region.rows;
        window2d.cols = region.cols;
        Rast_set_window(&window2d);
    }


    /*******************/
    /*Open the 3d raster map */

    /*******************/
    map = Rast3d_open_cell_old(param.input->answer,
                          G_find_raster3d(param.input->answer, ""),
                          &region, RASTER3D_TILE_SAME_AS_FILE,
                          RASTER3D_USE_CACHE_DEFAULT);

    if (map == NULL)
        Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
                       param.input->answer);

    /*Get the output type */
    output_type = Rast3d_file_type_map(map);

    if (output_type == FCELL_TYPE || output_type == DCELL_TYPE) {

        /********************************/
        /*Open the elevation raster map */

        /********************************/

        elevfd = Rast_open_old(param.elevation->answer, "");

        globalElevMapType = Rast_get_map_type(elevfd);

        /**********************/
        /*Open the Outputmap */

        /**********************/

        if (G_find_raster2(param.output->answer, ""))
            G_message(_("Output map already exists. Will be overwritten!"));

        if (output_type == FCELL_TYPE)
            outfd = Rast_open_new(param.output->answer, FCELL_TYPE);
        else if (output_type == DCELL_TYPE)
            outfd = Rast_open_new(param.output->answer, DCELL_TYPE);

        /*if requested set the Mask on */
        if (param.mask->answer) {
            if (Rast3d_mask_file_exists()) {
                changemask = 0;
                if (Rast3d_mask_is_off(map)) {
                    Rast3d_mask_on(map);
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
            if (Rast3d_mask_file_exists())
                if (Rast3d_mask_is_on(map) && changemask)
                    Rast3d_mask_off(map);
        }

        Rast_close(outfd);
        Rast_close(elevfd);

    } else {
        fatal_error(map, -1, -1,
                    _("Wrong 3D raster datatype! Cannot create raster map"));
    }

    /* Close files and exit */
    if (!Rast3d_close(map))
        Rast3d_fatal_error(_("Unable to close 3D raster map <%s>"),
                       param.input->answer);

    return (EXIT_SUCCESS);
}
