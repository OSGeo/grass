
/****************************************************************************
 *
 * MODULE:       r3.out.vtk  
 *   	    	
 * AUTHOR(S):    Original author 
 *               Soeren Gebbert soerengebbert at gmx de
 * 		27 Feb 2006 Berlin
 * PURPOSE:      Converts 3D raster maps (RASTER3D) into the VTK-Ascii format  
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
#include "globalDefs.h"
#include "writeVTKData.h"
#include "parameters.h"
#include "errorHandling.h"

/*local prototypes */
static double get_raster_value_as_double(int maptype, void *ptr,
                                         double nullval);
static double get_g3d_raster_value_as_double(void *map, int x, int y, int z,
                                             int type, double nullval);


/* ************************************************************************* */
/* Get the value of the current raster pointer as double ******************* */

/* ************************************************************************* */
double get_raster_value_as_double(int MapType, void *ptr, double nullval)
{
    double val = nullval;

    if (MapType == CELL_TYPE) {
        if (Rast_is_null_value(ptr, MapType)) {
            val = nullval;
        } else {
            val = *(CELL *) ptr;
        }
    }
    if (MapType == FCELL_TYPE) {
        if (Rast_is_null_value(ptr, MapType)) {
            val = nullval;
        } else {
            val = *(FCELL *) ptr;
        }
    }
    if (MapType == DCELL_TYPE) {
        if (Rast_is_null_value(ptr, MapType)) {
            val = nullval;
        } else {
            val = *(DCELL *) ptr;
        }
    }

    return val;
}

/* ************************************************************************* */
/* Get the value of the 3d raster map as double *************************** */

/* ************************************************************************* */
double get_g3d_raster_value_as_double(void *map, int x, int y, int z,
                                      int type, double nullval)
{
    double val = 0;
    float fvalue;
    double dvalue;

    if (type == FCELL_TYPE) {
        Rast3d_get_value(map, x, y, z, &fvalue, type);
        if (Rast3d_is_null_value_num(&fvalue, FCELL_TYPE))
            val = nullval;
        else
            val = (double) fvalue;
    } else {
        Rast3d_get_value(map, x, y, z, &dvalue, type);
        if (Rast3d_is_null_value_num(&dvalue, DCELL_TYPE))
            val = nullval;
        else
            val = dvalue;
    }

    return val;
}

/* ************************************************************************* */
/* This function writes the point coordinates ****************************** */

/* ************************************************************************* */
void write_vtk_points(input_maps * in, FILE * fp, RASTER3D_Region region, int dp,
                      int type, double scale)
{
    int x, y, z, percentage = 0;
    int rows, cols, depths;
    void *rast_top = NULL;
    void *rast_bottom = NULL;
    void *ptr_top = NULL;
    void *ptr_bottom = NULL;
    double topval = 0, bottomval = 0;
    double zcoor, ycoor, xcoor;
    double zcoor1, ycoor1, xcoor1;

    rows = region.rows;
    cols = region.cols;
    depths = region.depths;

    rast_top = Rast_allocate_buf(in->topMapType);
    rast_bottom = Rast_allocate_buf(in->bottomMapType);

    G_debug(3, _("write_vtk_points: Writing point coordinates"));

    for (z = 0; z < depths; z++) {

        for (y = 0; y < rows; y++) {
            G_percent(percentage, (rows * depths - 1), 10);
            percentage++;

            Rast_get_row(in->top, rast_top, y, in->topMapType);

            Rast_get_row(in->bottom, rast_bottom, y, in->bottomMapType);

            for (x = 0, ptr_top = rast_top, ptr_bottom = rast_bottom;
                x < cols;
                x++, ptr_top =
                G_incr_void_ptr(ptr_top, Rast_cell_size(in->topMapType)),
                ptr_bottom =
                G_incr_void_ptr(ptr_bottom,
                                Rast_cell_size(in->bottomMapType))) {

                /*Get the values */
                topval =
                    get_raster_value_as_double(in->topMapType, ptr_top, 0.0);
                bottomval =
                    get_raster_value_as_double(in->bottomMapType, ptr_bottom,
                                               0.0);

                if (type == 1) { /*Structured Grid */
                    /*Calculate the coordinates */
                    xcoor =
                        region.west + (region.ew_res / 2 +
                        region.ew_res * (x));
                    /* Here the raster3d north->south coordinate system is used */
                    ycoor =
                        region.north - (region.ns_res / 2 +
                        region.ns_res * (y));
                    zcoor =
                        (bottomval +
                        z * (topval - bottomval) / (depths - 1)) * scale;

                    xcoor -= x_extent;
                    ycoor -= y_extent;

                    fprintf(fp, "%.*f ", dp, xcoor);
                    fprintf(fp, "%.*f ", dp, ycoor);
                    fprintf(fp, "%.*f\n", dp, zcoor);
                } else { /*Unstructured Grid */
                    /*Write for every cell the coordinates for a hexahedron -> 8 points */
                    /*VTK Hexaeder */
                    /* bottom
                     * 3 --- 2
                     * |     |
                     * 0 --- 1

                     * top
                     * 7 --- 6
                     * |     |
                     * 4 --- 5

                     */
                    xcoor = region.west + (region.ew_res * (x)); /*0, 3, 4, 7 */
                    /* Here the raster3d north->south coordinate system is used */
                    ycoor = region.north - (region.ns_res * (y)); /*2, 3, 6, 7 */
                    zcoor = (bottomval + z * (topval - bottomval) / (depths)) * scale; /*0, 1, 2, 3 */

                    xcoor1 = region.west + (region.ew_res + region.ew_res * (x)); /*1, 2, 5, 6 */
                    /* Here the raster3d north->south coordinate system is used */
                    ycoor1 = region.north - (region.ns_res + region.ns_res * (y)); /*0, 1, 4, 5 */
                    zcoor1 = (bottomval + z * (topval - bottomval) / (depths) + (topval - bottomval) / (depths)) * scale; /*4, 5, ,6 ,7 */

                    xcoor -= x_extent;
                    ycoor -= y_extent;

                    xcoor1 -= x_extent;
                    ycoor1 -= y_extent;


                    /*0 */
                    fprintf(fp, "%.*f ", dp, xcoor);
                    fprintf(fp, "%.*f ", dp, ycoor1);
                    fprintf(fp, "%.*f\n", dp, zcoor);
                    /*1 */
                    fprintf(fp, "%.*f ", dp, xcoor1);
                    fprintf(fp, "%.*f ", dp, ycoor1);
                    fprintf(fp, "%.*f\n", dp, zcoor);
                    /*2 */
                    fprintf(fp, "%.*f ", dp, xcoor1);
                    fprintf(fp, "%.*f ", dp, ycoor);
                    fprintf(fp, "%.*f\n", dp, zcoor);
                    /*3 */
                    fprintf(fp, "%.*f ", dp, xcoor);
                    fprintf(fp, "%.*f ", dp, ycoor);
                    fprintf(fp, "%.*f\n", dp, zcoor);

                    /*4 */
                    fprintf(fp, "%.*f ", dp, xcoor);
                    fprintf(fp, "%.*f ", dp, ycoor1);
                    fprintf(fp, "%.*f\n", dp, zcoor1);
                    /*5 */
                    fprintf(fp, "%.*f ", dp, xcoor1);
                    fprintf(fp, "%.*f ", dp, ycoor1);
                    fprintf(fp, "%.*f\n", dp, zcoor1);
                    /*6 */
                    fprintf(fp, "%.*f ", dp, xcoor1);
                    fprintf(fp, "%.*f ", dp, ycoor);
                    fprintf(fp, "%.*f\n", dp, zcoor1);
                    /*7 */
                    fprintf(fp, "%.*f ", dp, xcoor);
                    fprintf(fp, "%.*f ", dp, ycoor);
                    fprintf(fp, "%.*f\n", dp, zcoor1);
                }
            }
        }
    }

    if (type == 1)
        fprintf(fp, "POINT_DATA %i\n", region.cols * region.rows * region.depths); /*We have pointdata */

    return;
}

/* ************************************************************************* */
/* This function writes the cell for the unstructured grid ***************** */

/* ************************************************************************* */
void write_vtk_unstructured_grid_cells(FILE * fp, RASTER3D_Region region)
{
    int x, y, z, percentage;
    int rows, cols, depths, count;

    rows = region.rows;
    cols = region.cols;
    depths = region.depths;

    G_debug(3, _("write_vtk_unstructured_grid_cells: Writing the cells"));

    fprintf(fp, "CELLS %i %i\n", region.cols * region.rows * region.depths,
            region.cols * region.rows * region.depths * 9);

    count = 0;
    percentage = 0;

    /*The point - cell links */
    for (z = 0; z < depths; z++) {
        for (y = 0; y < rows; y++) {
            G_percent(percentage, (rows * depths - 1), 10);
            percentage++;

            for (x = 0; x < cols; x++) {
                /*Voxel */
                fprintf(fp, "%i %i %i %i %i %i %i %i %i\n", 8,
                        count * 8, count * 8 + 1, count * 8 + 3,
                        count * 8 + 2, count * 8 + 4, count * 8 + 5,
                        count * 8 + 7, count * 8 + 6);

                /*Hexaeder 
                 * fprintf(fp, "%i %i %i %i %i %i %i %i %i\n", 8,
                 * count * 8, count * 8 + 1, count * 8 + 2, count * 8 + 3,
                 * count * 8 + 4, count * 8 + 5, count * 8 + 6,
                 * count * 8 + 7);
                 */
                count++;
            }
        }
    }
    percentage = 0;

    fprintf(fp, "CELL_TYPES %i\n", region.cols * region.rows * region.depths);
    /*the cell types */
    for (z = 0; z < depths; z++) {
        for (y = 0; y < rows; y++) {
            G_percent(percentage, (rows * depths - 1), 10);
            percentage++;

            for (x = 0; x < cols; x++) {
                /*Voxel */
                fprintf(fp, "11\n");
                /*Hexaeder 
                 * fprintf(fp, "12\n");
                 */
            }
        }
    }

    fprintf(fp, "CELL_DATA %i\n", region.cols * region.rows * region.depths); /*We have celldata  */

    return;
}


/* ************************************************************************* */
/* Write the VTK Cell or point data **************************************** */

/* ************************************************************************* */
void write_vtk_data(FILE * fp, void *map, RASTER3D_Region region, char *varname,
                    int dp)
{
    double value;
    double nullvalue;
    int x, y, z, percentage;
    int rows, cols, depths, typeIntern;

    rows = region.rows;
    cols = region.cols;
    depths = region.depths;

    /*the nullvalue */
    if (!sscanf(param.null_val->answer, "%lf", &nullvalue)) {
        G_warning("Null value is not valid, using 0 instead.");
        nullvalue = 0;
    }

    G_debug(3,
            _("write_vtk_data: Writing Celldata %s with rows %i cols %i depths %i to vtk-ascii file"),
            varname, rows, cols, depths);

    fprintf(fp, "SCALARS %s float 1\n", varname);
    fprintf(fp, "LOOKUP_TABLE default\n");

    typeIntern = Rast3d_tile_type_map(map);

    percentage = 0;

    for (z = 0; z < depths; z++) {
        /* In case of structured grid data, the point/cell coordinates
           are computed based on the default north->south raster3d coordinate system.
           We need to compute south -> north ordering for image data.
         */
        if (!param.structgrid->answer) {
            for (y = rows - 1; y >= 0; y--) {
                G_percent(percentage, (rows * depths - 1), 10);
                percentage++;

                for (x = 0; x < cols; x++) {
                    value =
                        get_g3d_raster_value_as_double(map, x, y, z,
                                                       typeIntern, nullvalue);
                    fprintf(fp, "%.*f ", dp, value);
                }
                fprintf(fp, "\n");
            }
        } else {
            for (y = 0; y < rows; y++) {
                G_percent(percentage, (rows * depths - 1), 10);
                percentage++;

                for (x = 0; x < cols; x++) {
                    value =
                        get_g3d_raster_value_as_double(map, x, y, z,
                                                       typeIntern, nullvalue);
                    fprintf(fp, "%.*f ", dp, value);
                }
                fprintf(fp, "\n");
            }
        }
    }
}


/* ************************************************************************* */
/* Write the VTK RGB Voxel Data ******************************************** */

/* ************************************************************************* */
void write_vtk_rgb_data(void *map_r, void *map_g, void *map_b,
                        FILE * fp, const char *varname,
                        RASTER3D_Region region, int dp)
{
    double value = 0;
    int x, y, z, percentage, k;
    int rows, cols, depths;
    int typeIntern[3];
    void *maprgb = NULL;

    G_debug(3, "write_vtk_rgb_data: Writing RGB data");

    rows = region.rows;
    cols = region.cols;
    depths = region.depths;

    typeIntern[0] = Rast3d_tile_type_map(map_r);
    typeIntern[1] = Rast3d_tile_type_map(map_g);
    typeIntern[2] = Rast3d_tile_type_map(map_b);

    percentage = 0;

    /********************** WRITE RGB VOXEL DATA; CELL OR POINT ****************/
    fprintf(fp, "COLOR_SCALARS %s 3\n", varname);

    for (z = 0; z < depths; z++) {
        for (y = 0; y < rows; y++) {
            G_percent(percentage, (rows * depths - 1), 10);
            percentage++;

            for (x = 0; x < cols; x++) {
                for (k = 0; k < 3; k++) {

                    if (k == 0)
                        maprgb = map_r;
                    if (k == 1)
                        maprgb = map_g;
                    if (k == 2)
                        maprgb = map_b;

                    /* In case of structured grid data, the point/cell coordinates
                       are computed based on the default north->south raster3d coordinate system.
                       We need to compute south -> north ordering for image data.
                     */
                    if (!param.structgrid->answer)
                        value =
                        get_g3d_raster_value_as_double(maprgb, x, rows - y - 1, z,
                                                       typeIntern[k],
                                                       0.0);
                    else
                        value =
                        get_g3d_raster_value_as_double(maprgb, x, y, z,
                                                       typeIntern[k],
                                                       0.0);
                    /*Test of value range, the data should be 1 byte gray values */
                    if (value > 255 || value < 0) {
                        G_warning(_("Wrong 3D raster map values! Values should in between 0 and 255!"));
                        fprintf(fp, "0 ");
                    } else {

                        fprintf(fp, "%.*f ", dp, (value / 255));
                    }
                }
                fprintf(fp, "\n");
            }
        }
    }
    return;
}


/* ************************************************************************* */
/* Write the VTK vector Data *********************************************** */

/* ************************************************************************* */
void write_vtk_vector_data(void *map_x, void *map_y, void *map_z,
                           FILE * fp, const char *varname,
                           RASTER3D_Region region, int dp)
{
    double value = 0;
    int x, y, z, percentage, k;
    int rows, cols, depths;
    int typeIntern[3];
    void *mapvect = NULL;

    G_debug(3, "write_vtk_vector_data: Writing vector data");

    rows = region.rows;
    cols = region.cols;
    depths = region.depths;

    typeIntern[0] = Rast3d_tile_type_map(map_x);
    typeIntern[1] = Rast3d_tile_type_map(map_y);
    typeIntern[2] = Rast3d_tile_type_map(map_z);

    percentage = 0;

    /********************** WRITE VECTOR DATA; CELL OR POINT ****************/
    fprintf(fp, "VECTORS %s float\n", varname);

    for (z = 0; z < depths; z++) { /*From the bottom to the top */
        for (y = 0; y < rows; y++) {
            G_percent(percentage, (rows * depths - 1), 10);
            percentage++;

            for (x = 0; x < cols; x++) {
                for (k = 0; k < 3; k++) {

                    if (k == 0)
                        mapvect = map_x;
                    if (k == 1)
                        mapvect = map_y;
                    if (k == 2)
                        mapvect = map_z;

                    /* In case of structured grid data, the point/cell coordinates
                       are computed based on the default north->south raster3d coordinate system.
                       We need to compute south -> north ordering for image data.
                     */
                    if (!param.structgrid->answer)
                        value =
                        get_g3d_raster_value_as_double(mapvect, x, rows - y - 1, z,
                                                       typeIntern[k],
                                                       0.0);
                    else
                        value =
                        get_g3d_raster_value_as_double(mapvect, x, y, z,
                                                       typeIntern[k],
                                                       0.0);
                    fprintf(fp, "%.*f ", dp, value);
                }
                fprintf(fp, "\n");
            }
        }
    }
    return;
}
