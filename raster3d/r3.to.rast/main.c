
/****************************************************************************
 *
 * MODULE:       r3.to.rast 
 *   	    	
 * AUTHOR(S):    Original author 
 *               Soeren Gebbert soerengebbert@gmx.de
 * 		08 01 2005 Berlin
 * PURPOSE:      Converts 3D raster maps to 2D raster maps  
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

/*- Parameters and global variables -----------------------------------------*/
typedef struct {
    struct Option *input, *output;
    struct Flag *mask;
    struct Flag *res; /*If set, use the same resolution as the input map */
} paramType;

paramType param; /*Parameters */

/*- prototypes --------------------------------------------------------------*/
void fatal_error(void *map, int *fd, int depths, char *errorMsg); /*Simple Error message */
void set_params(); /*Fill the paramType structure */
void g3d_to_raster(void *map, RASTER3D_Region region, int *fd); /*Write the raster */
int open_output_map(const char *name, int res_type); /*opens the outputmap */
void close_output_map(int fd); /*close the map */



/* ************************************************************************* */
/* Error handling ********************************************************** */

/* ************************************************************************* */
void fatal_error(void *map, int *fd, int depths, char *errorMsg)
{
    int i;

    /* Close files and exit */
    if (map != NULL) {
        if (!Rast3d_close(map))
            Rast3d_fatal_error(_("Unable to close 3D raster map"));
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
    param.input = G_define_option();
    param.input->key = "input";
    param.input->type = TYPE_STRING;
    param.input->required = YES;
    param.input->gisprompt = "old,grid3,3d-raster";
    param.input->description =
        _("3D raster map(s) to be converted to 2D raster slices");

    param.output = G_define_option();
    param.output->key = "output";
    param.output->type = TYPE_STRING;
    param.output->required = YES;
    param.output->description = _("Basename for resultant raster slice maps");
    param.output->gisprompt = "new,cell,raster";

    param.mask = G_define_flag();
    param.mask->key = 'm';
    param.mask->description = _("Use 3D raster mask (if exists) with input map");

    param.res = G_define_flag();
    param.res->key = 'r';
    param.res->description =
        _("Use the same resolution as the input 3D raster map for the 2D output "
          "maps, independent of the current region settings");
}

/* ************************************************************************* */
/* Write the slices to seperate raster maps ******************************** */

/* ************************************************************************* */
void g3d_to_raster(void *map, RASTER3D_Region region, int *fd)
{
    DCELL d1 = 0;
    FCELL f1 = 0;
    int x, y, z;
    int rows, cols, depths, typeIntern, pos = 0;
    FCELL *fcell = NULL;
    DCELL *dcell = NULL;

    rows = region.rows;
    cols = region.cols;
    depths = region.depths;


    G_debug(2, "g3d_to_raster: Writing %i raster maps with %i rows %i cols.",
            depths, rows, cols);

    typeIntern = Rast3d_tile_type_map(map);

    if (typeIntern == FCELL_TYPE)
        fcell = Rast_allocate_f_buf();
    else if (typeIntern == DCELL_TYPE)
        dcell = Rast_allocate_d_buf();

    pos = 0;
    /*Every Rastermap */
    for (z = 0; z < depths; z++) { /*From the bottom to the top */
        G_debug(2, "Writing raster map %d of %d", z + 1, depths);
        for (y = 0; y < rows; y++) {
            G_percent(y, rows - 1, 10);

            for (x = 0; x < cols; x++) {
                if (typeIntern == FCELL_TYPE) {
                    Rast3d_get_value(map, x, y, z, &f1, typeIntern);
                    if (Rast3d_is_null_value_num(&f1, FCELL_TYPE))
                        Rast_set_null_value(&fcell[x], 1, FCELL_TYPE);
                    else
                        fcell[x] = f1;
                } else {
                    Rast3d_get_value(map, x, y, z, &d1, typeIntern);
                    if (Rast3d_is_null_value_num(&d1, DCELL_TYPE))
                        Rast_set_null_value(&dcell[x], 1, DCELL_TYPE);
                    else
                        dcell[x] = d1;
                }
            }
            if (typeIntern == FCELL_TYPE)
                Rast_put_f_row(fd[pos], fcell);

            if (typeIntern == DCELL_TYPE)
                Rast_put_d_row(fd[pos], dcell);
        }
        G_debug(2, "Finished writing map %d.", z + 1);
        pos++;
    }


    if (dcell)
        G_free(dcell);
    if (fcell)
        G_free(fcell);

}

/* ************************************************************************* */
/* Open the raster output map ********************************************** */

/* ************************************************************************* */
int open_output_map(const char *name, int res_type)
{
    return Rast_open_new(name, res_type);
}

/* ************************************************************************* */
/* Close the raster output map ********************************************* */

/* ************************************************************************* */
void close_output_map(int fd)
{
    Rast_close(fd);
}

/* ************************************************************************* */
/* Main function, open the RASTER3D map and create the raster maps ************** */

/* ************************************************************************* */
int main(int argc, char *argv[])
{
    RASTER3D_Region region, inputmap_bounds;
    struct Cell_head region2d;
    struct GModule *module;
    struct History history;
    void *map = NULL; /*The 3D Rastermap */
    int i = 0, changemask = 0;
    int *fd = NULL, output_type, cols, rows;
    char *RasterFileName;
    int overwrite = 0;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("conversion"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("voxel"));
    module->description = _("Converts 3D raster maps to 2D raster maps");

    /* Get parameters from user */
    set_params();

    /* Have GRASS get inputs */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    G_debug(3, "Open 3D raster map <%s>", param.input->answer);

    if (NULL == G_find_raster3d(param.input->answer, ""))
        Rast3d_fatal_error(_("3D raster map <%s> not found"),
                       param.input->answer);

    /*Set the defaults */
    Rast3d_init_defaults();

    /*Set the resolution of the output maps */
    if (param.res->answer) {

        /*Open the map with current region */
        map = Rast3d_open_cell_old(param.input->answer,
                              G_find_raster3d(param.input->answer, ""),
                              RASTER3D_DEFAULT_WINDOW, RASTER3D_TILE_SAME_AS_FILE,
                              RASTER3D_USE_CACHE_DEFAULT);
        if (map == NULL)
            Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
                           param.input->answer);


        /*Get the region of the map */
        Rast3d_get_region_struct_map(map, &region);
        /*set this region as current 3D window for map */
        Rast3d_set_window_map(map, &region);
        /*Set the 2d region appropriate */
        Rast3d_extract2d_region(&region, &region2d);
        /*Make the new 2d region the default */
        Rast_set_window(&region2d);

    } else {
        /* Figure out the region from the map */
        Rast3d_get_window(&region);

        /*Open the 3d raster map */
        map = Rast3d_open_cell_old(param.input->answer,
                              G_find_raster3d(param.input->answer, ""),
                              &region, RASTER3D_TILE_SAME_AS_FILE,
                              RASTER3D_USE_CACHE_DEFAULT);

        if (map == NULL)
            Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
                           param.input->answer);
    }

    /*Check if the g3d-region is equal to the 2D rows and cols */
    rows = Rast_window_rows();
    cols = Rast_window_cols();

    /*If not equal, set the 3D window correct */
    if (rows != region.rows || cols != region.cols) {
        G_message(_("The 2D and 3D region settings are different. "
                    "Using the 2D window settings to adjust the 2D part of the 3D region."));
        G_get_set_window(&region2d);
        region.ns_res = region2d.ns_res;
        region.ew_res = region2d.ew_res;
        region.rows = region2d.rows;
        region.cols = region2d.cols;
        
        Rast3d_adjust_region(&region);
        
        Rast3d_set_window_map(map, &region);
    }

    /* save the input map region for later use (history meta-data) */
    Rast3d_get_region_struct_map(map, &inputmap_bounds);

    /*Get the output type */
    output_type = Rast3d_file_type_map(map);


    /*prepare the filehandler */
    fd = (int *) G_malloc(region.depths * sizeof (int));

    if (fd == NULL)
        fatal_error(map, NULL, 0, _("Out of memory"));

    G_message(_("Creating %i raster maps"), region.depths);

    /*Loop over all output maps! open */
    for (i = 0; i < region.depths; i++) {
        /*Create the outputmaps */
        G_asprintf(&RasterFileName, "%s_%05d", param.output->answer, i + 1);
        G_message(_("Raster map %i Filename: %s"), i + 1, RasterFileName);

        overwrite = G_check_overwrite(argc, argv);
        
        if (G_find_raster2(RasterFileName, "") && !overwrite)
            G_fatal_error(_("Raster map %d Filename: %s already exists. Use the flag --o to overwrite."),
                      i + 1, RasterFileName);

        if (output_type == FCELL_TYPE)
            fd[i] = open_output_map(RasterFileName, FCELL_TYPE);
        else if (output_type == DCELL_TYPE)
            fd[i] = open_output_map(RasterFileName, DCELL_TYPE);

    }

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

    /*Create the Rastermaps */
    g3d_to_raster(map, region, fd);


    /*Loop over all output maps! close */
    for (i = 0; i < region.depths; i++) {
        close_output_map(fd[i]);

        /* write history */
        G_asprintf(&RasterFileName, "%s_%i", param.output->answer, i + 1);
        G_debug(4, "Raster map %d Filename: %s", i + 1, RasterFileName);
        Rast_short_history(RasterFileName, "raster", &history);

        Rast_set_history(&history, HIST_DATSRC_1, "3D Raster map:");
        Rast_set_history(&history, HIST_DATSRC_2, param.input->answer);

        Rast_append_format_history(&history, "Level %d of %d", i + 1, region.depths);
        Rast_append_format_history(&history, "Level z-range: %f to %f",
                                   region.bottom + (i * region.tb_res),
                                   region.bottom + (i + 1 * region.tb_res));

        Rast_append_format_history(&history, "Input map full z-range: %f to %f",
                                   inputmap_bounds.bottom, inputmap_bounds.top);
        Rast_append_format_history(&history, "Input map z-resolution: %f",
                                   inputmap_bounds.tb_res);

        if (!param.res->answer) {
            Rast_append_format_history(&history, "GIS region full z-range: %f to %f",
                                       region.bottom, region.top);
            Rast_append_format_history(&history, "GIS region z-resolution: %f",
                                       region.tb_res);
        }

        Rast_command_history(&history);
        Rast_write_history(RasterFileName, &history);
    }

    /*We set the Mask off, if it was off before */
    if (param.mask->answer) {
        if (Rast3d_mask_file_exists())
            if (Rast3d_mask_is_on(map) && changemask)
                Rast3d_mask_off(map);
    }


    /*Cleaning */
    if (RasterFileName)
        G_free(RasterFileName);

    if (fd)
        G_free(fd);

    /* Close files and exit */
    if (!Rast3d_close(map))
        fatal_error(map, NULL, 0, _("Unable to close 3D raster map"));

    map = NULL;

    return (EXIT_SUCCESS);
}
