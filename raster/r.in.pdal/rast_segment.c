<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/****************************************************************************
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/****************************************************************************
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
/****************************************************************************
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
/****************************************************************************
=======
>>>>>>> osgeo-main

 /****************************************************************************
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
/****************************************************************************
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
/****************************************************************************
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
 *
 * MODULE:    r.in.pdal
 *
 * AUTHOR(S): Vaclav Petras
 *
 * PURPOSE:   This file is a wrapper for a subset of Segment functions
 *
 * COPYRIGHT: (C) 2015 by Vaclav Petras and the GRASS Development Team
 *
 *            This program is free software under the GNU General Public
 *            License (>=v2). Read the file COPYING that comes with
 *            GRASS for details.
 *
 *****************************************************************************/

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/segment.h>

#include "rast_segment.h"

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static void rast_segment_load(SEGMENT *segment, int rowio,
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
static void rast_segment_load(SEGMENT *segment, int rowio,
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
static void rast_segment_load(SEGMENT *segment, int rowio,
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
static void rast_segment_load(SEGMENT *segment, int rowio,
=======
>>>>>>> osgeo-main
static void rast_segment_load(SEGMENT * segment, int rowio,
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
static void rast_segment_load(SEGMENT *segment, int rowio,
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
static void rast_segment_load(SEGMENT *segment, int rowio,
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                              RASTER_MAP_TYPE map_type)
{
    void *raster_row = Rast_allocate_input_buf(map_type);
    int row;

    for (row = 0; row < Rast_input_window_rows(); row++) {
        /* TODO: free mem */
        Rast_get_row(rowio, raster_row, row, map_type);
        Segment_put_row(segment, raster_row, row);
    }
}

/* TODO: close function */

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
void rast_segment_open(SEGMENT *segment, const char *name,
                       RASTER_MAP_TYPE *map_type)
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
void rast_segment_open(SEGMENT *segment, const char *name,
                       RASTER_MAP_TYPE *map_type)
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
void rast_segment_open(SEGMENT *segment, const char *name,
                       RASTER_MAP_TYPE *map_type)
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
void rast_segment_open(SEGMENT *segment, const char *name,
                       RASTER_MAP_TYPE *map_type)
=======
>>>>>>> osgeo-main
void rast_segment_open(SEGMENT * segment, const char *name,
                       RASTER_MAP_TYPE * map_type)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
void rast_segment_open(SEGMENT *segment, const char *name,
                       RASTER_MAP_TYPE *map_type)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void rast_segment_open(SEGMENT *segment, const char *name,
                       RASTER_MAP_TYPE *map_type)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    /* TODO: check if not passing the mapset is OK */
    int rowio = Rast_open_old(name, "");

    *map_type = Rast_get_map_type(rowio);
    int segment_rows = 64;

    /* we use long segments because this is how the values a binned */
    int segment_cols = Rast_input_window_cols();
    int segments_in_memory = 4;

    if (Segment_open(segment, G_tempfile(), Rast_input_window_rows(),
                     Rast_input_window_cols(), segment_rows, segment_cols,
                     Rast_cell_size(*map_type), segments_in_memory) != 1)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
        G_fatal_error(
            _("Cannot create temporary file with segments of a raster map"));
    rast_segment_load(segment, rowio, *map_type);
    Rast_close(rowio); /* we won't need the raster again */
}

/* 0 on out of region or NULL, 1 on success */
int rast_segment_get_value_xy(SEGMENT *base_segment,
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        G_fatal_error(_("Cannot create temporary file with segments of a raster map"));
=======
        G_fatal_error(
            _("Cannot create temporary file with segments of a raster map"));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
    rast_segment_load(segment, rowio, *map_type);
    Rast_close(rowio); /* we won't need the raster again */
}

/* 0 on out of region or NULL, 1 on success */
<<<<<<< HEAD
int rast_segment_get_value_xy(SEGMENT * base_segment,
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
int rast_segment_get_value_xy(SEGMENT *base_segment,
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
                              struct Cell_head *input_region,
                              RASTER_MAP_TYPE rtype, double x, double y,
                              double *value)
{
    /* Rast gives double, Segment needs off_t */
    off_t base_row = Rast_northing_to_row(y, input_region);
    off_t base_col = Rast_easting_to_col(x, input_region);

    /* skip points which are outside the base raster
     * (null propagation) */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (base_row < 0 || base_col < 0 || base_row >= input_region->rows ||
        base_col >= input_region->cols)
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (base_row < 0 || base_col < 0 || base_row >= input_region->rows ||
        base_col >= input_region->cols)
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
    if (base_row < 0 || base_col < 0 || base_row >= input_region->rows ||
        base_col >= input_region->cols)
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
    if (base_row < 0 || base_col < 0 || base_row >= input_region->rows ||
        base_col >= input_region->cols)
=======
>>>>>>> osgeo-main
    if (base_row < 0 || base_col < 0 ||
        base_row >= input_region->rows || base_col >= input_region->cols)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    if (base_row < 0 || base_col < 0 || base_row >= input_region->rows ||
        base_col >= input_region->cols)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    if (base_row < 0 || base_col < 0 || base_row >= input_region->rows ||
        base_col >= input_region->cols)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        return 0;
    if (rtype == DCELL_TYPE) {
        DCELL tmp;

        Segment_get(base_segment, &tmp, base_row, base_col);
        if (Rast_is_d_null_value(&tmp))
            return 0;
        *value = (double)tmp;
    }
    else if (rtype == FCELL_TYPE) {
        FCELL tmp;

        Segment_get(base_segment, &tmp, base_row, base_col);
        if (Rast_is_f_null_value(&tmp))
            return 0;
        *value = (double)tmp;
    }
    else {
        CELL tmp;

        Segment_get(base_segment, &tmp, base_row, base_col);
        if (Rast_is_c_null_value(&tmp))
            return 0;
        *value = (double)tmp;
    }
    return 1;
}
