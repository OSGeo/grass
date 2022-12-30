<<<<<<< HEAD
<<<<<<< HEAD
/****************************************************************************
=======

 /****************************************************************************
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
/****************************************************************************
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
 *
 * MODULE:    r.in.pdal
 *
 * AUTHOR(S): Vaclav Petras
 *            Based on r.in.xyz and r.in.lidar by Markus Metz,
 *            Hamish Bowman, Volker Wichmann
 *            Maris Nartiss code refactoring
 *
 * PURPOSE:   Functions performing value updates on each incoming point
 *            during point binning process
 *
 * COPYRIGHT: (C) 2011-2021 by Vaclav Petras and the GRASS Development Team
 *
 *            This program is free software under the GNU General Public
 *            License (>=v2). Read the file COPYING that comes with
 *            GRASS for details.
 *
 *****************************************************************************/

#ifndef __BIN_UPDATE_H__
#define __BIN_UPDATE_H__

#include <grass/raster.h>

#define SIZE_INCREMENT 16

/* forward declarations for point_binning.h */
struct BinIndex;
struct com_node;

void update_n(void *, int, int, int);
void update_min(void *, int, int, int, RASTER_MAP_TYPE, double);
void update_max(void *, int, int, int, RASTER_MAP_TYPE, double);
void update_sum(void *, void *, int, int, int, RASTER_MAP_TYPE, double);
<<<<<<< HEAD
<<<<<<< HEAD
void update_m2(void *, void *, void *, int, int, int, RASTER_MAP_TYPE, double);
=======
void update_m2(void *, void *, void *, int, int, int, RASTER_MAP_TYPE,
               double);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
void update_m2(void *, void *, void *, int, int, int, RASTER_MAP_TYPE, double);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
void update_moving_mean(void *, int, int, int, RASTER_MAP_TYPE, double, int);

int add_z_node(struct BinIndex *, int, double);
void add_cnt_node(struct BinIndex *, int, int);

void update_bin_z_index(struct BinIndex *, void *, int, int, int, double);
void update_bin_cnt_index(struct BinIndex *, void *, int, int, int, int);
void update_com_node(struct com_node *, int, double, double);
<<<<<<< HEAD
<<<<<<< HEAD
void update_bin_com_index(struct BinIndex *, void *, int, int, int, double,
                          double, double);

int row_array_get_value_row_col(void *, int, int, int, RASTER_MAP_TYPE,
                                double *);
=======
void update_bin_com_index(struct BinIndex *, void *,
                          int, int, int, double, double, double);

int row_array_get_value_row_col(void *, int, int,
                                int, RASTER_MAP_TYPE, double *);

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
void update_bin_com_index(struct BinIndex *, void *, int, int, int, double,
                          double, double);

int row_array_get_value_row_col(void *, int, int, int, RASTER_MAP_TYPE,
                                double *);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

#endif /* __BIN_UPDATE_H__ */
