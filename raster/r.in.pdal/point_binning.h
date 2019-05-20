/*
 * r.in.lidar projection-related functions
 *
 * Copyright 2011-2015 by Markus Metz, and The GRASS Development Team
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (move code to separate functions)
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef __POINT_BINNING_H__
#define __POINT_BINNING_H__


#include <grass/raster.h>

/* forward declaration */
struct Map_info;

struct node
{
    int next;
    double z;
};

struct BinIndex
{
    int num_nodes;
    int max_nodes;
    struct node *nodes;
};

struct PointBinning
{
    int method;

    int bin_n;
    int bin_min;
    int bin_max;
    int bin_sum;
    int bin_sumsq;
    int bin_index;
    int bin_coordinates;

    void *n_array;
    void *min_array;
    void *max_array;
    void *sum_array;
    void *sumsq_array;
    void *index_array;
    void *x_array;
    void *y_array;

    int pth;
    double trim;
};

int check_rows_cols_fit_to_size_t(int rows, int cols);
void point_binning_memory_test(struct PointBinning *point_binning, int rows,
                               int cols, RASTER_MAP_TYPE rtype);

void point_binning_set(struct PointBinning *point_binning, char *method,
                       char *percentile, char *trim, int coordinates);
void point_binning_allocate(struct PointBinning *point_binning, int rows,
                            int cols, RASTER_MAP_TYPE rtype);

void point_binning_free(struct PointBinning *point_binning,
                        struct BinIndex *bin_index_nodes);

int update_bin_index(struct BinIndex *bin_index, void *index_array,
                     int cols, int row, int col,
                     RASTER_MAP_TYPE map_type, double value);
void write_variance(void *raster_row, void *n_array, void *sum_array,
                    void *sumsq_array, int row, int cols,
                    RASTER_MAP_TYPE rtype, int method);
void write_median(struct BinIndex *bin_index, void *raster_row,
                  void *index_array, int row, int cols,
                  RASTER_MAP_TYPE rtype);
void write_percentile(struct BinIndex *bin_index, void *raster_row,
                      void *index_array, int row, int cols,
                      RASTER_MAP_TYPE rtype, int pth);
void write_skewness(struct BinIndex *bin_index, void *raster_row,
                    void *index_array, int row, int cols,
                    RASTER_MAP_TYPE rtype);
void write_trimmean(struct BinIndex *bin_index, void *raster_row,
                    void *index_array, int row, int cols,
                    RASTER_MAP_TYPE rtype, double trim);

/* forward declarations */
struct Map_info;
struct line_pnts;
struct line_cats;

struct VectorWriter
{
    struct Map_info *info;
    struct line_pnts *points;
    struct line_cats *cats;
#ifdef HAVE_LONG_LONG_INT
    unsigned long long count;
#else
    unsigned long count;
#endif
};

void write_values(struct PointBinning *point_binning,
                  struct BinIndex *bin_index_nodes, void *raster_row, int row,
                  int cols, RASTER_MAP_TYPE rtype,
                  struct VectorWriter *vector_writer);
void update_value(struct PointBinning *point_binning,
                  struct BinIndex *bin_index_nodes, int cols, int arr_row,
                  int arr_col, RASTER_MAP_TYPE rtype, double x, double y,
                  double z);


#endif /* __POINT_BINNING_H__ */
