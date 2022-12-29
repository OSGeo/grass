/****************************************************************************
 *
 * MODULE:       v.decimate
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      Reduce the number of points in a vector map
 * COPYRIGHT:    (C) 2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/


#ifndef GRASS_GRID_DECIMATION_H
#define GRASS_GRID_DECIMATION_H

#include <grass/gis.h>

struct DecimationPoint
{
    int cat;
    double x;
    double y;
    double z;
};


struct GridDecimation
{
    struct DecimationPoint ***grid_points;
    size_t *grid_sizes;
    int rows;
    int cols;
    int max_points;
    double minx;
    double maxx;
    double miny;
    double maxy;
    double ns_res;
    double ew_res;
    int (*if_add_point) (struct DecimationPoint * point, void *point_data,
                         struct DecimationPoint ** point_list, size_t npoints,
                         void *context);
    void (*on_add_point) (struct DecimationPoint * point, void *point_data, void *context);
    void *if_context;
    void *on_context;
};


/* max size: rows * cols < max of size_t (using 1D array) */
void grid_decimation_create(struct GridDecimation *grid_decimation,
                            size_t rows, size_t cols);

void grid_decimation_create_from_region(struct GridDecimation
                                        *grid_decimation,
                                        struct Cell_head *region);

/* TODO: possible use (also?) Cell_head as input or storage */
void grid_decimation_set_region(struct GridDecimation *grid_decimation,
                                double minx, double maxx, double miny,
                                double maxy, double ns_res, double ew_res);

void grid_decimation_create_list_with_point(struct GridDecimation
                                            *grid_decimation, size_t index,
                                            struct DecimationPoint *point,
                                            size_t npoints);

void grid_decimation_add_point_to_list(struct GridDecimation *grid_decimation,
                                       size_t index,
                                       struct DecimationPoint *point,
                                       size_t npoints);

void grid_decimation_try_add_point(struct GridDecimation *grid_decimation,
                                   int cat, double x, double y, double z, void *point_data);

void grid_decimation_destroy(struct GridDecimation *grid_decimation);

#endif /* GRASS_GRID_DECIMATION_H */
