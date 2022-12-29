
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
    struct Option *type;
    struct Option *coeff_a;
    struct Option *coeff_b;
    struct Flag *mask;
    struct Flag *res; /*If set, use the same resolution as the input map */
} paramType;

paramType param; /*Parameters */

/*- prototypes --------------------------------------------------------------*/
void fatal_error(void *map, int *fd, int depths, char *errorMsg); /*Simple Error message */
void set_params(); /*Fill the paramType structure */
void g3d_to_raster(void *map, RASTER3D_Region region, int *fd,
                   int output_type, int use_coeffs, double coeff_a,
                   double coeff_b); /*Write the raster */
int open_output_map(const char *name, int res_type); /*opens the outputmap */
void close_output_map(int fd); /*close the map */

/* get the output type */
static int raster_type_option_string_enum(const char *type)
{
    /* this function could go to the library but the exact behavior needs
     * to be figured out */
    if (strcmp("CELL", type) == 0)
        return CELL_TYPE;
    else if (strcmp("DCELL", type) == 0)
        return DCELL_TYPE;
    else
        return FCELL_TYPE;
}


/* ************************************************************************* */
/* Error handling ********************************************************** */

/* ************************************************************************* */
/* TODO: use G_add_error_handler (or future Rast3d_add_error_handler) instead */
/* this one would require implementation of varargs here or though
 * non-existent va_list version of the library function */
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

    Rast3d_fatal_error("%s", errorMsg);
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

    param.type = G_define_standard_option(G_OPT_R_TYPE);
    param.type->required = NO;

    param.coeff_a = G_define_option();
    param.coeff_a->key = "multiply";
    param.coeff_a->type = TYPE_DOUBLE;
    param.coeff_a->required = NO;
    param.coeff_a->label = _("Value to multiply the raster values with");
    param.coeff_a->description = _("Coefficient a in the equation y = ax + b");

    param.coeff_b = G_define_option();
    param.coeff_b->key = "add";
    param.coeff_b->type = TYPE_DOUBLE;
    param.coeff_b->required = NO;
    param.coeff_b->label = _("Value to add to the raster values");
    param.coeff_b->description = _("Coefficient b in the equation y = ax + b");

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
/* Write the slices to separate raster maps ******************************** */

/* ************************************************************************* */
/* coefficients are used only when needed, otherwise the original values
 * is preserved as well as possible */
void g3d_to_raster(void *map, RASTER3D_Region region, int *fd,
                   int output_type, int use_coeffs, double coeff_a,
                   double coeff_b)
{
    CELL c1 = 0;
    FCELL f1 = 0;
    DCELL d1 = 0;
    int x, y, z;
    int rows, cols, depths, typeIntern, pos = 0;
    void *cell = NULL;  /* point to row buffer */
    void *ptr = NULL;  /* pointer to single cell */
    size_t cell_size = 0;

    rows = region.rows;
    cols = region.cols;
    depths = region.depths;


    G_debug(2, "g3d_to_raster: Writing %i raster maps with %i rows %i cols.",
            depths, rows, cols);

    typeIntern = Rast3d_tile_type_map(map);

    /* we test it here undefined just to be sure and then we use else for CELL */
    if (output_type == CELL_TYPE)
        cell = Rast_allocate_c_buf();
    else if (output_type == FCELL_TYPE)
        cell = Rast_allocate_f_buf();
    else if (output_type == DCELL_TYPE)
        cell = Rast_allocate_d_buf();
    else
        Rast3d_fatal_error(_("Unknown raster type <%d>"), output_type);
    cell_size = Rast_cell_size(output_type);

    pos = 0;
    /*Every Rastermap */
    for (z = 0; z < depths; z++) { /*From the bottom to the top */
        G_debug(2, "Writing raster map %d of %d", z + 1, depths);
        G_percent(z, depths - 1, 2);
        for (y = 0; y < rows; y++) {
            ptr = cell;  /* reset at the beginning of a row */
            for (x = 0; x < cols; x++) {
                if (typeIntern == FCELL_TYPE) {
                    Rast3d_get_value(map, x, y, z, &f1, typeIntern);
                    if (Rast3d_is_null_value_num(&f1, FCELL_TYPE)) {
                        Rast_set_null_value(ptr, 1, output_type);
                    }
                    else {
                        if (use_coeffs)
                            f1 = coeff_a * f1 + coeff_b;
                        Rast_set_f_value(ptr, f1, output_type);
                    }
                } else {
                    Rast3d_get_value(map, x, y, z, &d1, typeIntern);
                    if (Rast3d_is_null_value_num(&d1, DCELL_TYPE)) {
                        Rast_set_null_value(ptr, 1, output_type);
                    }
                    else {
                        if (use_coeffs)
                            d1 = coeff_a * d1 + coeff_b;
                        Rast_set_d_value(ptr, d1, output_type);
                    }
                }
                ptr = G_incr_void_ptr(ptr, cell_size);
            }
            Rast_put_row(fd[pos], cell, output_type);
        }
        G_debug(2, "Finished writing map %d.", z + 1);
        pos++;
    }
    G_percent(1, 1, 1);
    G_free(cell);

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
    int use_coeffs = 0;  /* bool */
    double coeff_a = 1;
    double coeff_b = 0;

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

    /* coefficients to modify the map */
    if (param.coeff_a->answer || param.coeff_b->answer)
        use_coeffs = 1;
    if (param.coeff_a->answer)
        coeff_a = atof(param.coeff_a->answer);
    if (param.coeff_b->answer)
        coeff_b = atof(param.coeff_b->answer);

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
    if (param.type->answer) {
        output_type = raster_type_option_string_enum(param.type->answer);
    }
    else {
        output_type = Rast3d_file_type_map(map);
    }

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

        fd[i] = open_output_map(RasterFileName, output_type);

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
    g3d_to_raster(map, region, fd, output_type, use_coeffs, coeff_a, coeff_b);


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
