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


#include "grid_decimation.h"

#include <stdlib.h>


/* max size: rows * cols < max of size_t (using 1D array) */
void grid_decimation_create(struct GridDecimation *grid_decimation,
                            size_t rows, size_t cols)
{
    grid_decimation->grid_points =
        G_calloc(rows * cols, sizeof(struct DecimationPoint *));
    grid_decimation->grid_sizes = G_calloc(rows * cols, sizeof(size_t));
    grid_decimation->rows = rows;
    grid_decimation->cols = cols;
    grid_decimation->if_add_point = NULL;
    grid_decimation->on_add_point = NULL;
    grid_decimation->if_context = NULL;
    grid_decimation->on_context = NULL;
}


void grid_decimation_destroy(struct GridDecimation *grid_decimation)
{
    /* TODO: we could also offer mode without dealloc (faster) */
    int row, col;
    size_t point, npoints;
    for (row = 0; row < grid_decimation->rows; row++) {
        for (col = 0; col < grid_decimation->cols; col++) {
            /* TODO: make index function */
            size_t index = row * grid_decimation->cols + col;
            if ((npoints = grid_decimation->grid_sizes[index])) {
                /* delete points in list */
                for (point = 0; point < npoints; point++)
                    G_free(grid_decimation->grid_points[index][point]);
                /* delete list */
                G_free(grid_decimation->grid_points[index]);
            }
        }
    }
    G_free(grid_decimation->grid_points);
    G_free(grid_decimation->grid_sizes);
}


/* TODO: use Cell_head as storage? */
void grid_decimation_create_from_region(struct GridDecimation
                                        *grid_decimation,
                                        struct Cell_head *region)
{
    grid_decimation_create(grid_decimation, region->rows, region->cols);
    grid_decimation_set_region(grid_decimation, region->west, region->east,
                               region->south, region->north, region->ew_res,
                               region->ns_res);
}


/* TODO: change order of ns_res and ew_res to match xy */
void grid_decimation_set_region(struct GridDecimation *grid_decimation,
                                double minx, double maxx, double miny,
                                double maxy, double ew_res, double ns_res)
{
    grid_decimation->minx = minx;
    grid_decimation->maxx = maxx;
    grid_decimation->miny = miny;
    grid_decimation->maxy = maxy;
    grid_decimation->ns_res = ns_res;
    grid_decimation->ew_res = ew_res;
}


void grid_decimation_create_list_with_point(struct GridDecimation
                                            *grid_decimation, size_t index,
                                            struct DecimationPoint *point,
                                            size_t npoints)
{
    struct DecimationPoint **point_list =
        G_malloc(1 * sizeof(struct DecimationPoint *));
    point_list[0] = point;
    grid_decimation->grid_points[index] = point_list;
    grid_decimation->grid_sizes[index] = 1;
}


void grid_decimation_add_point_to_list(struct GridDecimation *grid_decimation,
                                       size_t index,
                                       struct DecimationPoint *point,
                                       size_t npoints)
{
    /* TODO: this might be too much reallocation */
    /* TODO: line_ptns struct could be reused, it is not meant for this but it would work */
    struct DecimationPoint **point_list =
        G_realloc(grid_decimation->grid_points[index],
                  (npoints + 1) * sizeof(struct DecimationPoint *));

    point_list[npoints] = point;
    grid_decimation->grid_points[index] = point_list;
    grid_decimation->grid_sizes[index] = npoints + 1;
}


static size_t grid_decimation_xy_to_index(struct GridDecimation
                                          *grid_decimation, double x,
                                          double y)
{
    /* TODO: test x, y */
    int row = (y - grid_decimation->miny) / grid_decimation->ns_res;
    int col = (x - grid_decimation->minx) / grid_decimation->ew_res;

    if (row < 0 || row > grid_decimation->rows || col < 0 ||
        col > grid_decimation->cols) {
        G_fatal_error
            ("Row (%d) or column (%d) outside of range (0 - %d, 0 - %d)", row,
             col, grid_decimation->rows, grid_decimation->cols);
    }
    size_t index = row * grid_decimation->cols + col;

    /* TODO: are the tests really needed, especially the second one? */
    if (row * grid_decimation->cols + col >
        grid_decimation->rows * grid_decimation->cols) {
        G_fatal_error("Index (%d) out of range (max: %d)",
                      row * grid_decimation->cols + col,
                      grid_decimation->rows * grid_decimation->cols);
    }
    return index;
}


void grid_decimation_try_add_point(struct GridDecimation *grid_decimation,
                                   int cat, double x, double y, double z, void *point_data)
{
    size_t index = grid_decimation_xy_to_index(grid_decimation, x, y);
    int npoints = grid_decimation->grid_sizes[index];

    /* TODO: when max is 1, we don't have to store the point at all */
    if (grid_decimation->max_points && grid_decimation->max_points == npoints)
        return;

    struct DecimationPoint *point = G_malloc(sizeof(struct DecimationPoint));

    point->cat = cat;
    point->x = x;
    point->y = y;
    point->z = z;

    if (!npoints) {
        grid_decimation_create_list_with_point(grid_decimation, index, point,
                                               npoints);
        if (grid_decimation->on_add_point)
            grid_decimation->on_add_point(point, point_data, grid_decimation->on_context);
    }
    else {
        if (grid_decimation->if_add_point
            (point, point_data, grid_decimation->grid_points[index], npoints,
             grid_decimation->if_context)) {
            grid_decimation_add_point_to_list(grid_decimation, index, point,
                                              npoints);
            if (grid_decimation->on_add_point)
                grid_decimation->on_add_point(point, point_data,
                                              grid_decimation->on_context);
        }
        else {
            G_free(point);
        }
    }
}
