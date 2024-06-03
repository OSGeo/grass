/*
 * r.in.pdal point binning logic
 *
 * Copyright 2011-2015, 2020 by Markus Metz, and The GRASS Development Team
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (move code to separate functions)
 *  Maris Nartiss (refactoring for r.in.pdal)
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef __POINT_BINNING_H__
#define __POINT_BINNING_H__

#include <grass/gis.h>
#include <grass/raster.h>

/* Point binning methods: */
<<<<<<< HEAD
#define METHOD_NONE       0
#define METHOD_N          1
#define METHOD_MIN        2
#define METHOD_MAX        3
#define METHOD_RANGE      4
#define METHOD_SUM        5
#define METHOD_MEAN       6
#define METHOD_STDDEV     7
#define METHOD_VARIANCE   8
#define METHOD_COEFF_VAR  9
=======
#define METHOD_NONE        0
#define METHOD_N           1
#define METHOD_MIN         2
#define METHOD_MAX         3
#define METHOD_RANGE       4
#define METHOD_SUM         5
#define METHOD_MEAN        6
#define METHOD_STDDEV      7
#define METHOD_VARIANCE    8
#define METHOD_COEFF_VAR   9
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
#define METHOD_MEDIAN     10
#define METHOD_MODE       11
#define METHOD_PERCENTILE 12
#define METHOD_SKEWNESS   13
#define METHOD_TRIMMEAN   14
#define METHOD_SIDNMAX    15
#define METHOD_SIDNMIN    16
#define METHOD_EV1        17
#define METHOD_EV2        18
#define METHOD_EV3        19

<<<<<<< HEAD
struct z_node {
=======

struct z_node
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
    int next;
    double z;
};

<<<<<<< HEAD
struct cnt_node {
=======
struct cnt_node
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
    int next;
    CELL value;
    int count;
};

<<<<<<< HEAD
struct com_node {
=======
struct com_node
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
    int n;
    double *meanx;
    double *meany;
    double *comoment;
};

<<<<<<< HEAD
struct BinIndex {
=======
struct BinIndex
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
    int num_nodes;
    int max_nodes;
    void *nodes;
};

<<<<<<< HEAD
struct PointBinning {
=======
struct PointBinning
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
    int method;

    int bin_n;
    int bin_min;
    int bin_max;
    int bin_sum;
    int bin_m2;
    int bin_z_index;
    int bin_cnt_index;
    int bin_eigenvalues;
    int bin_coordinates;

    void *n_array;
    void *min_array;
    void *max_array;
    void *sum_array;
    void *c_array;
    void *mean_array;
    void *m2_array;
    void *index_array;
    void *x_array;
    void *y_array;

    int pth;
    double trim;
};

void *get_cell_ptr(void *, int, int, int, RASTER_MAP_TYPE);
int blank_array(void *, int, int, RASTER_MAP_TYPE, int);

void point_binning_set(struct PointBinning *, char *, char *, char *);
void point_binning_allocate(struct PointBinning *, int, int, RASTER_MAP_TYPE);
void point_binning_free(struct PointBinning *, struct BinIndex *);

<<<<<<< HEAD
void write_values(struct PointBinning *, struct BinIndex *, void *, int, int,
                  RASTER_MAP_TYPE);
void update_value(struct PointBinning *, struct BinIndex *, int, int, int,
                  RASTER_MAP_TYPE, double, double, double);
=======

void write_values(struct PointBinning *,
                  struct BinIndex *, void *, int, int, RASTER_MAP_TYPE);
void update_value(struct PointBinning *,
                  struct BinIndex *, int, int,
                  int, RASTER_MAP_TYPE, double, double, double);

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

#endif /* __POINT_BINNING_H__ */
