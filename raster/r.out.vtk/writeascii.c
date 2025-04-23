/****************************************************************************
 *
 * MODULE:       r.out.vtk
 *
 * AUTHOR(S):    Original author
 *               Soeren Gebbert soerengebbert@gmx.de
 *                 08 23 2005 Berlin
 * PURPOSE:      Converts raster maps into the VTK-Ascii format
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
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/config.h>
#include "globaldefs.h"

/*local prototypes */
static double get_raster_value_as_double(int maptype, void *ptr,
                                         double nullval);

/* ************************************************************************* */
/* Get the value of the current raster pointer as double ******************* */
/* ************************************************************************* */
double get_raster_value_as_double(int MapType, void *ptr, double nullval)
{
    double val = nullval;

    if (MapType == CELL_TYPE) {
        if (Rast_is_null_value(ptr, MapType)) {
            val = nullval;
        }
        else {
            val = *(CELL *)ptr;
        }
    }
    if (MapType == FCELL_TYPE) {
        if (Rast_is_null_value(ptr, MapType)) {
            val = nullval;
        }
        else {
            val = *(FCELL *)ptr;
        }
    }
    if (MapType == DCELL_TYPE) {
        if (Rast_is_null_value(ptr, MapType)) {
            val = nullval;
        }
        else {
            val = *(DCELL *)ptr;
        }
    }

    return val;
}

/* ************************************************************************* */
/* Write the default VTK Header, Elevation  is not supported *************** */
/* ************************************************************************* */
void write_vtk_normal_header(FILE *fp, struct Cell_head region,
                             double elevation, int type)
{
    G_debug(3, _("write_vtk_normal_header: Writing VTK-Header"));

    /*Simple vtk ASCII header */
    fprintf(fp, "# vtk DataFile Version 3.0\n");
    /* The header line describes the data. */
    fprintf(fp, "GRASS GIS %i Export\n", GRASS_VERSION_MAJOR);
    fprintf(fp, "ASCII\n");
    fprintf(fp, "DATASET STRUCTURED_POINTS\n"); /*We are using the structured
                                                   point dataset. */

    if (type)
        fprintf(fp, "DIMENSIONS %i %i %i\n", region.cols, region.rows, 1);
    else
        fprintf(fp, "DIMENSIONS %i %i %i\n", region.cols + 1, region.rows + 1,
                1);

    fprintf(fp, "SPACING %lf %lf %lf\n", region.ew_res, region.ns_res, 0.0);

    if (type)
        fprintf(fp, "ORIGIN %lf %lf %lf\n",
                (region.west + region.ew_res / 2) - x_extent,
                (region.south + region.ns_res / 2) - y_extent, elevation);
    else
        fprintf(fp, "ORIGIN %lf %lf %lf\n", region.west - x_extent,
                region.south - y_extent, elevation);
}

/* ************************************************************************* */
/* Write the Elevation VTK Header, Elevation is supported ****************** */
/* ************************************************************************* */
void write_vtk_structured_elevation_header(FILE *fp, struct Cell_head region)
{
    G_debug(3, _("write_vtk_structured_elevation_header: Writing VTK-Header"));

    /*Simple vtk ASCII header */
    fprintf(fp, "# vtk DataFile Version 3.0\n");
    fprintf(fp, "GRASS GIS %i Export\n", GRASS_VERSION_MAJOR);
    fprintf(fp, "ASCII\n");
    fprintf(fp, "DATASET STRUCTURED_GRID\n"); /*We are using the structured grid
                                                 dataset. */
    fprintf(fp, "DIMENSIONS %i %i %i\n", region.cols, region.rows, 1);
    fprintf(fp, "POINTS %i float\n",
            region.cols * region.rows); /*The Coordinates */
}

/* ************************************************************************* */
/* Write the Rectilinear Elevtaion VTK Header, Elevation is supported ****** */
/* ************************************************************************* */
void write_vtk_polygonal_elevation_header(FILE *fp, struct Cell_head region)
{
    G_debug(3, _("write_vtk_polygonal_elevation_header: Writing VTK-Header"));

    /*Simple vtk ASCII header */
    fprintf(fp, "# vtk DataFile Version 3.0\n");
    fprintf(fp, "GRASS GIS %i Export\n", GRASS_VERSION_MAJOR);
    fprintf(fp, "ASCII\n");
    fprintf(fp, "DATASET POLYDATA\n"); /*We are using polydataset. */
    fprintf(fp, "POINTS %i float\n", region.cols * region.rows);
}

/* ************************************************************************* */
/* Write the CellDataHeader ************************************************ */
/* ************************************************************************* */
void write_vtk_celldata_header(FILE *fp, struct Cell_head region)
{
    G_debug(3, _("write_vtk_celldata_header: Writing VTK-DataHeader"));
    fprintf(fp, "CELL_DATA %i\n", region.cols * region.rows);
}

/* ************************************************************************* */
/* Write the PointDataHeader ************************************************ */
/* ************************************************************************* */
void write_vtk_pointdata_header(FILE *fp, struct Cell_head region)
{
    G_debug(3, _("writeVTKPointHeader: Writing VTK-DataHeader"));
    fprintf(fp, "POINT_DATA %i\n", region.cols * region.rows);
}

/* ************************************************************************* */
/* Write the VTK Structured Coordinates ************************************ */
/* ************************************************************************* */
void write_vtk_structured_coordinates(int fd, FILE *fp, char *varname UNUSED,
                                      struct Cell_head region, int out_type,
                                      char *null_value, double scale, int dp)
{
    int ncols = region.cols;
    int nrows = region.rows;
    int row, col;
    int rowcount = 0, colcount = 0;
    double nspos = 0.0, ewpos = 0.0;
    double nullvalue, value;
    void *ptr, *raster;

    G_debug(3, _("write_vtk_structured_coordinates: Writing Coordinates"));

    /*the nullvalue */
    if (!sscanf(null_value, "%lf", &nullvalue)) {
        G_warning("Null value is not valid, using 0 instead.");
        nullvalue = 0;
    }

    raster = Rast_allocate_buf(out_type);

    rowcount = 0;
    for (row = nrows - 1; row >= 0; row--) {
        colcount = 0;
        G_percent((row - nrows) * (-1), nrows, 2);

        Rast_get_row(fd, raster, row, out_type);

        nspos = region.ns_res / 2 + region.south + rowcount * region.ns_res;
        nspos -= y_extent;

        for (col = 0, ptr = raster; col < ncols;
             col++, ptr = G_incr_void_ptr(ptr, Rast_cell_size(out_type))) {
            ewpos = region.ew_res / 2 + region.west + colcount * region.ew_res;
            ewpos -= x_extent;

            value = get_raster_value_as_double(out_type, ptr, nullvalue);
            fprintf(fp, "%.*f %.*f %.*f\n", dp, ewpos, dp, nspos, dp,
                    value * scale);

            colcount++;
        }
        rowcount++;
    }
    G_free(raster);
    return;
}

/* ************************************************************************* */
/* Write Polygonal Coordinates ********************************************* */
/* ************************************************************************* */
void write_vtk_polygonal_coordinates(int fd, FILE *fp, char *varname UNUSED,
                                     struct Cell_head region, int out_type,
                                     char *null_value, double scale,
                                     int polytype, int dp)
{
    int ncols = region.cols;
    int nrows = region.rows;
    int row, col;
    int rowcount = 0, colcount = 0;
    double nspos = 0.0, ewpos = 0.0;
    double value, nullvalue;
    void *ptr, *raster;
    int i, j, count;

    G_debug(3,
            _("write_vtk_polygonal_coordinates: Writing VTK Polygonal data"));

    /*the nullvalue */
    if (!sscanf(null_value, "%lf", &nullvalue)) {
        G_warning("Null value is not valid, using 0 instead.");
        nullvalue = 0;
    }

    /*First we are writing the coordinate points, the elevation cell is only
     * used for the z coordinate */
    raster = Rast_allocate_buf(out_type);

    rowcount = 0;
    for (row = nrows - 1; row >= 0; row--) {
        colcount = 0;
        G_percent((row - nrows) * (-1), nrows, 10);

        Rast_get_row(fd, raster, row, out_type);

        nspos = region.ns_res / 2 + region.south + rowcount * region.ns_res;
        nspos -= y_extent;

        for (col = 0, ptr = raster; col < ncols;
             col++, ptr = G_incr_void_ptr(ptr, Rast_cell_size(out_type))) {
            ewpos = region.ew_res / 2 + region.west + colcount * region.ew_res;
            ewpos -= x_extent;

            value = get_raster_value_as_double(out_type, ptr, nullvalue);
            fprintf(fp, "%.*f %.*f %.*f\n", dp, ewpos, dp, nspos, dp,
                    value * scale);

            colcount++;
        }
        rowcount++;
    }

    /*Now we need to write the Connectivity between the points */

    if (polytype == QUADS) { /*The default */
        /*If Datafiltering should be supported, we use Polygons to represent the
         * grid */
        fprintf(fp, "POLYGONS %i %i\n", (region.rows - 1) * (region.cols - 1),
                5 * (region.rows - 1) * (region.cols - 1));

        /*We create a grid of quads, the corners of the quads are the datapoints
         */
        for (i = 0; i < region.rows - 1; i++) {
            for (j = 0; j < region.cols - 1; j++) {
                fprintf(fp, "4 %i %i %i %i \n", i * region.cols + j,
                        i * region.cols + j + 1, (i + 1) * region.cols + j + 1,
                        (i + 1) * region.cols + j);
            }
        }
    }

    if (polytype == TRIANGLE_STRIPS) {
        /*TriangleStrips, take a look at https://vtk.org/ for the definition of
         * triangle_strips in vtk */
        /*If we use triangelestrips, the number of points per strip is equal to
         * the double number of cols we have */
        fprintf(fp, "TRIANGLE_STRIPS %i %i\n", region.rows - 1,
                (region.rows - 1) + (region.rows - 1) * (2 * region.cols));

        /*For every Row-1 make a strip */
        for (i = 0; i < region.rows - 1; i++) {

            /*Number of Points */
            fprintf(fp, "%i ", region.cols * 2);

            /*Write the points */
            for (j = 0; j < region.cols; j++) {
                fprintf(fp, "%i %i ", i * region.cols + j,
                        (i + 1) * region.cols + j);
            }
            fprintf(fp, "\n");
        }
    }

    if (polytype == VERTICES) {
        /*If the Data should be Triangulated by VTK, we use vertices to
         * represent the grid */
        fprintf(fp, "VERTICES %i %i\n", region.rows,
                region.rows + (region.rows) * (region.cols));

        count = 0;
        for (i = 0; i < region.rows; i++) {
            fprintf(fp, "%i ", (region.cols));
            for (j = 0; j < region.cols; j++) {
                fprintf(fp, "%i ", count);
                count++;
            }
            fprintf(fp, "\n");
        }
    }
    G_free(raster);
    return;
}

/* ************************************************************************* */
/* Write the VTK Data ****************************************************** */
/* ************************************************************************* */
void write_vtk_data(int fd, FILE *fp, char *varname, struct Cell_head region,
                    int out_type, char *null_value, int dp)
{
    int ncols = region.cols;
    int nrows = region.rows;
    int row, col;
    double value, nullvalue;
    void *ptr, *raster;

    G_debug(3, _("write_vtk_data: Writing VTK-Data"));

    /*the nullvalue */
    if (!sscanf(null_value, "%lf", &nullvalue)) {
        G_warning("Null value is not valid, using 0 instead.");
        nullvalue = 0;
    }

    fprintf(fp, "SCALARS %s float 1\n", varname);
    fprintf(fp, "LOOKUP_TABLE default\n");

    raster = Rast_allocate_buf(out_type);

    for (row = nrows - 1; row >= 0; row--) {
        G_percent((row - nrows) * (-1), nrows, 10);

        Rast_get_row(fd, raster, row, out_type);

        for (col = 0, ptr = raster; col < ncols;
             col++, ptr = G_incr_void_ptr(ptr, Rast_cell_size(out_type))) {

            value = get_raster_value_as_double(out_type, ptr, nullvalue);
            fprintf(fp, "%.*f ", dp, value);
        }
        fprintf(fp, "\n");
    }
    G_free(raster);
    return;
}

/* ************************************************************************* */
/* Write the VTK RGB Image Data ******************************************** */
/* ************************************************************************* */
void write_vtk_rgb_image_data(int redfd, int greenfd, int bluefd, FILE *fp,
                              const char *varname, struct Cell_head region,
                              int out_type, int dp)
{
    int ncols = region.cols;
    int nrows = region.rows;
    int row, col;
    void *redptr, *redraster;
    void *greenptr, *greenraster;
    void *blueptr, *blueraster;
    double r = 0.0, g = 0.0, b = 0.0;

    G_debug(3, _("write_vtk_rgb_image_data: Writing VTK-ImageData"));

    fprintf(fp, "COLOR_SCALARS %s 3\n", varname);

    redraster = Rast_allocate_buf(out_type);
    greenraster = Rast_allocate_buf(out_type);
    blueraster = Rast_allocate_buf(out_type);

    for (row = nrows - 1; row >= 0; row--) {
        G_percent((row - nrows) * (-1), nrows, 10);

        Rast_get_row(redfd, redraster, row, out_type);
        Rast_get_row(greenfd, greenraster, row, out_type);
        Rast_get_row(bluefd, blueraster, row, out_type);

        for (col = 0, redptr = redraster, greenptr = greenraster,
            blueptr = blueraster;
             col < ncols;
             col++, redptr = G_incr_void_ptr(redptr, Rast_cell_size(out_type)),
            greenptr = G_incr_void_ptr(greenptr, Rast_cell_size(out_type)),
            blueptr = G_incr_void_ptr(blueptr, Rast_cell_size(out_type))) {

            r = get_raster_value_as_double(out_type, redptr, 0.0);
            g = get_raster_value_as_double(out_type, greenptr, 0.0);
            b = get_raster_value_as_double(out_type, blueptr, 0.0);

            /*Test of valuerange, the data should be 1 byte gray values */
            if (r > 255 || g > 255 || b > 255 || r < 0 || g < 0 || b < 0) {
                G_warning(_(
                    "Wrong map values! Values should in between 0 and 255!\n"));
                fprintf(fp, "0 0 0 \n");
            }
            else {
                fprintf(fp, "%.*f %.*f %.*f \n", dp, r / 255, dp, g / 255, dp,
                        b / 255);
            }
        }
    }
    G_free(redraster);
    G_free(greenraster);
    G_free(blueraster);
    return;
}

/* ************************************************************************* */
/* Write the VTK Vector Data *********************************************** */
/* ************************************************************************* */
void write_vtk_vector_data(int xfd, int yfd, int zfd, FILE *fp,
                           const char *varname, struct Cell_head region,
                           int out_type, int dp)
{
    int ncols = region.cols;
    int nrows = region.rows;
    int row, col;
    void *xptr, *xraster;
    void *yptr, *yraster;
    void *zptr, *zraster;
    double x = 0.0, y = 0.0, z = 0.0;

    G_debug(3, _("write_vtk_vector_data: Writing VTK-vector data"));

    fprintf(fp, "VECTORS %s float\n", varname);

    xraster = Rast_allocate_buf(out_type);
    yraster = Rast_allocate_buf(out_type);
    zraster = Rast_allocate_buf(out_type);

    for (row = nrows - 1; row >= 0; row--) {
        G_percent((row - nrows) * (-1), nrows, 10);

        Rast_get_row(xfd, xraster, row, out_type);
        Rast_get_row(yfd, yraster, row, out_type);
        Rast_get_row(zfd, zraster, row, out_type);

        for (col = 0, xptr = xraster, yptr = yraster, zptr = zraster;
             col < ncols;
             col++, xptr = G_incr_void_ptr(xptr, Rast_cell_size(out_type)),
            yptr = G_incr_void_ptr(yptr, Rast_cell_size(out_type)),
            zptr = G_incr_void_ptr(zptr, Rast_cell_size(out_type))) {

            x = get_raster_value_as_double(out_type, xptr, 0.0);
            y = get_raster_value_as_double(out_type, yptr, 0.0);
            z = get_raster_value_as_double(out_type, zptr, 0.0);

            fprintf(fp, "%.*f %.*f %.*f \n", dp, x, dp, y, dp, z);
        }
    }
    G_free(xraster);
    G_free(yraster);
    G_free(zraster);
    return;
}
