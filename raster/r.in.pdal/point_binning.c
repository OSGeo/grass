/*
 * r.in.pdal point binning logic
 *
 * Copyright 2011-2015, 2020 by Markus Metz, and the GRASS Development Team
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (move code to separate functions)
 *  Maris Nartiss (refactoring for r.in.pdal)
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>

#include "point_binning.h"
#include "bin_update.h"
#include "bin_write.h"

void *get_cell_ptr(void *array, int cols, int row, int col,
                   RASTER_MAP_TYPE map_type)
{
    return G_incr_void_ptr(array, ((row * (size_t)cols) + col) *
                                      Rast_cell_size(map_type));
}

int blank_array(void *array, int nrows, int ncols, RASTER_MAP_TYPE map_type,
                int value)
{
    /* flood fill initialize the array to either 0 or NULL */
    /*  "value" can be either 0 (for 0.0) or -1 (for NULL) */
    int row, col;
    void *ptr;

    ptr = array;

    switch (value) {
    case 0:
        /* fill with 0 */
        /* simpler to use Rast_raster_cpy() or similar ?? */

        for (row = 0; row < nrows; row++) {
            for (col = 0; col < ncols; col++) {
                Rast_set_c_value(ptr, 0, map_type);
                ptr = G_incr_void_ptr(ptr, Rast_cell_size(map_type));
            }
        }
        break;

    case -1:
        /* fill with NULL */
        /* alloc for col+1, do we come up (nrows) short? no. */
        for (row = 0; row < nrows; row++) {
            Rast_set_null_value(ptr, ncols, map_type);
            ptr = G_incr_void_ptr(ptr, ncols * Rast_cell_size(map_type));
        }
        break;

    default:
        return -1;
    }

    return 0;
}

void point_binning_set(struct PointBinning *point_binning, char *method,
                       char *percentile, char *trim)
{
    /* figure out what maps we need in memory */
    /*  n               n
       min              min
       max              max
       range            min max         max - min
       sum              sum c
       mean             sum n           sum/n
       stddev           mean m2 n       sqrt((sumsq - sum*sum/n)/n)
       variance         mean m2 n       (sumsq - sum*sum/n)/n
       coeff_var        mean m2 n       sqrt((sumsq - sum*sum/n)/n) / (sum/n)
       median           n               array index to linked list
       mode             n               array index to linked list
       percentile       n               array index to linked list
       skewness         n               array index to linked list
       trimmean         n               array index to linked list
       sidnmax          n               array index to linked list
       sidnmin          n               array index to linked list
       ev1, ev2, ev3    n               array index to linked list
     */
    point_binning->method = METHOD_NONE;
    point_binning->bin_n = FALSE;
    point_binning->bin_min = FALSE;
    point_binning->bin_max = FALSE;
    point_binning->bin_sum = FALSE;
    point_binning->bin_m2 = FALSE;
    point_binning->bin_z_index = FALSE;
    point_binning->bin_cnt_index = FALSE;
    point_binning->bin_eigenvalues = FALSE;
    point_binning->bin_coordinates = FALSE;

    point_binning->n_array = NULL;
    point_binning->min_array = NULL;
    point_binning->max_array = NULL;
    point_binning->mean_array = NULL;
    point_binning->sum_array = NULL;
    point_binning->c_array = NULL;
    point_binning->m2_array = NULL;
    point_binning->index_array = NULL;
    point_binning->x_array = NULL;
    point_binning->y_array = NULL;

    if (strcmp(method, "n") == 0) {
        point_binning->method = METHOD_N;
        point_binning->bin_n = TRUE;
    }
    if (strcmp(method, "min") == 0) {
        point_binning->method = METHOD_MIN;
        point_binning->bin_min = TRUE;
    }
    if (strcmp(method, "max") == 0) {
        point_binning->method = METHOD_MAX;
        point_binning->bin_max = TRUE;
    }
    if (strcmp(method, "range") == 0) {
        point_binning->method = METHOD_RANGE;
        point_binning->bin_min = TRUE;
        point_binning->bin_max = TRUE;
    }
    if (strcmp(method, "sum") == 0) {
        point_binning->method = METHOD_SUM;
        point_binning->bin_sum = TRUE;
    }
    if (strcmp(method, "mean") == 0) {
        point_binning->method = METHOD_MEAN;
        point_binning->bin_sum = TRUE;
        point_binning->bin_n = TRUE;
    }
    if (strcmp(method, "stddev") == 0) {
        point_binning->method = METHOD_STDDEV;
        point_binning->bin_m2 = TRUE;
    }
    if (strcmp(method, "variance") == 0) {
        point_binning->method = METHOD_VARIANCE;
        point_binning->bin_m2 = TRUE;
    }
    if (strcmp(method, "coeff_var") == 0) {
        point_binning->method = METHOD_COEFF_VAR;
        point_binning->bin_m2 = TRUE;
    }
    if (strcmp(method, "median") == 0) {
        point_binning->method = METHOD_MEDIAN;
        point_binning->bin_z_index = TRUE;
    }
    if (strcmp(method, "mode") == 0) {
        point_binning->method = METHOD_MODE;
        point_binning->bin_cnt_index = TRUE;
    }
    if (strcmp(method, "percentile") == 0) {
        if (percentile != NULL)
            point_binning->pth = atoi(percentile);
        else
            G_fatal_error(_("Unable to calculate percentile without the pth "
                            "option specified!"));
        point_binning->method = METHOD_PERCENTILE;
        point_binning->bin_z_index = TRUE;
    }
    if (strcmp(method, "skewness") == 0) {
        point_binning->method = METHOD_SKEWNESS;
        point_binning->bin_z_index = TRUE;
    }
    if (strcmp(method, "trimmean") == 0) {
        if (trim != NULL)
            point_binning->trim = atof(trim) / 100.0;
        else
            G_fatal_error(_("Unable to calculate trimmed mean without the trim "
                            "option specified!"));
        point_binning->method = METHOD_TRIMMEAN;
        point_binning->bin_z_index = TRUE;
    }
    if (strcmp(method, "sidnmax") == 0) {
        point_binning->method = METHOD_SIDNMAX;
        point_binning->bin_cnt_index = TRUE;
    }
    if (strcmp(method, "sidnmin") == 0) {
        point_binning->method = METHOD_SIDNMIN;
        point_binning->bin_cnt_index = TRUE;
    }
    if (strcmp(method, "ev1") == 0) {
        point_binning->method = METHOD_EV1;
        point_binning->bin_eigenvalues = TRUE;
    }
    if (strcmp(method, "ev2") == 0) {
        point_binning->method = METHOD_EV2;
        point_binning->bin_eigenvalues = TRUE;
    }
    if (strcmp(method, "ev3") == 0) {
        point_binning->method = METHOD_EV3;
        point_binning->bin_eigenvalues = TRUE;
    }
}

void point_binning_allocate(struct PointBinning *point_binning, int rows,
                            int cols, RASTER_MAP_TYPE rtype)
{
    if (point_binning->bin_n) {
        G_debug(2, "allocating n_array");
        point_binning->n_array =
            G_calloc((size_t)rows * (cols + 1), Rast_cell_size(CELL_TYPE));
        blank_array(point_binning->n_array, rows, cols, CELL_TYPE, 0);
    }
    if (point_binning->bin_min) {
        G_debug(2, "allocating min_array");
        point_binning->min_array =
            G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->min_array, rows, cols, rtype,
                    -1); /* fill with NULLs */
    }
    if (point_binning->bin_max) {
        G_debug(2, "allocating max_array");
        point_binning->max_array =
            G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->max_array, rows, cols, rtype,
                    -1); /* fill with NULLs */
    }
    if (point_binning->bin_sum) {
        G_debug(2, "allocating sum_array");
        point_binning->sum_array =
            G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->sum_array, rows, cols, rtype, 0);
        point_binning->c_array =
            G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->c_array, rows, cols, rtype, 0);
    }
    if (point_binning->bin_m2) {
        G_debug(2, "allocating m2_array");
        point_binning->m2_array =
            G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->m2_array, rows, cols, rtype, 0);
        point_binning->mean_array =
            G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->mean_array, rows, cols, rtype, -1);
        point_binning->n_array =
            G_calloc((size_t)rows * (cols + 1), Rast_cell_size(CELL_TYPE));
        blank_array(point_binning->n_array, rows, cols, CELL_TYPE, 0);
    }
    if (point_binning->bin_z_index || point_binning->bin_cnt_index ||
        point_binning->bin_eigenvalues) {
        G_debug(2, "allocating index_array");
        point_binning->index_array =
            G_calloc((size_t)rows * (cols + 1), Rast_cell_size(CELL_TYPE));
        blank_array(point_binning->index_array, rows, cols, CELL_TYPE,
                    -1); /* fill with NULLs */
    }
}

void point_binning_free(struct PointBinning *point_binning,
                        struct BinIndex *bin_index_nodes)
{
    if (point_binning->bin_n)
        G_free(point_binning->n_array);
    if (point_binning->bin_min)
        G_free(point_binning->min_array);
    if (point_binning->bin_max)
        G_free(point_binning->max_array);
    if (point_binning->bin_sum) {
        G_free(point_binning->sum_array);
        G_free(point_binning->c_array);
    }
    if (point_binning->bin_m2) {
        G_free(point_binning->m2_array);
        G_free(point_binning->mean_array);
        G_free(point_binning->n_array);
    }
    if (point_binning->bin_z_index || point_binning->bin_cnt_index ||
        point_binning->bin_eigenvalues) {
        G_free(point_binning->index_array);
        G_free(bin_index_nodes->nodes);
        bin_index_nodes->num_nodes = 0;
        bin_index_nodes->max_nodes = 0;
        bin_index_nodes->nodes = NULL;
    }
}

void write_values(struct PointBinning *point_binning,
                  struct BinIndex *bin_index_nodes, void *raster_row, int row,
                  int cols, RASTER_MAP_TYPE rtype)
{
    void *ptr = NULL;
    int col;

    switch (point_binning->method) {
    case METHOD_N: /* n is a straight copy */
        Rast_raster_cpy(raster_row,
                        ((char *)point_binning->n_array) +
                            ((size_t)row * cols * Rast_cell_size(CELL_TYPE)),
                        cols, CELL_TYPE);
        break;

    case METHOD_MIN:
        Rast_raster_cpy(raster_row,
                        ((char *)point_binning->min_array) +
                            ((size_t)row * cols * Rast_cell_size(rtype)),
                        cols, rtype);
        break;

    case METHOD_MAX:
        Rast_raster_cpy(raster_row,
                        ((char *)point_binning->max_array) +
                            ((size_t)row * cols * Rast_cell_size(rtype)),
                        cols, rtype);
        break;

    case METHOD_SUM:
        write_sum(raster_row, point_binning->sum_array, point_binning->c_array,
                  row, cols, rtype);
        break;

    case METHOD_RANGE: /* (max-min) */
        ptr = raster_row;
        for (col = 0; col < cols; col++) {
            size_t offset = ((size_t)row * cols + col) * Rast_cell_size(rtype);
            double min = Rast_get_d_value(
                ((char *)point_binning->min_array) + offset, rtype);
            double max = Rast_get_d_value(
                ((char *)point_binning->max_array) + offset, rtype);

            Rast_set_d_value(ptr, max - min, rtype);
            ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
        }
        break;

    case METHOD_MEAN: /* (sum / n) */
        ptr = raster_row;
        for (col = 0; col < cols; col++) {
            size_t n_offset =
                ((size_t)row * cols + col) * Rast_cell_size(CELL_TYPE);
            int n = Rast_get_c_value(
                ((char *)point_binning->n_array) + n_offset, CELL_TYPE);
            double sum = get_sum(point_binning->sum_array,
                                 point_binning->c_array, row, cols, col, rtype);

            if (n == 0)
                Rast_set_null_value(ptr, 1, rtype);
            else
                Rast_set_d_value(ptr, (sum / n), rtype);

            ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
        }
        break;

    case METHOD_STDDEV:    /*  sqrt(variance)        */
    case METHOD_VARIANCE:  /*  (sumsq - sum*sum/n)/n */
    case METHOD_COEFF_VAR: /*  100 * stdev / mean    */
        write_variance(raster_row, point_binning->n_array,
                       point_binning->mean_array, point_binning->m2_array, row,
                       cols, rtype, point_binning->method);
        break;

    case METHOD_MEDIAN: /* median, if only one point in cell we will use that */
        write_median(bin_index_nodes, raster_row, point_binning->index_array,
                     row, cols, rtype);
        break;

    case METHOD_MODE:
        write_mode(bin_index_nodes, raster_row, point_binning->index_array, row,
                   cols);
        break;

    case METHOD_PERCENTILE: /* rank = (pth*(n+1))/100; interpolate linearly */
        write_percentile(bin_index_nodes, raster_row,
                         point_binning->index_array, row, cols, rtype,
                         point_binning->pth);
        break;

    case METHOD_SKEWNESS: /* skewness = sum(xi-mean)^3/(N-1)*s^3 */
        write_skewness(bin_index_nodes, raster_row, point_binning->index_array,
                       row, cols, rtype);
        break;

    case METHOD_TRIMMEAN:
        write_trimmean(bin_index_nodes, raster_row, point_binning->index_array,
                       row, cols, rtype, point_binning->trim);
        break;

    case METHOD_SIDNMAX:
        write_sidn(bin_index_nodes, raster_row, point_binning->index_array, row,
                   cols, 0);
        break;

    case METHOD_SIDNMIN:
        write_sidn(bin_index_nodes, raster_row, point_binning->index_array, row,
                   cols, 1);
        break;

    case METHOD_EV1:
    case METHOD_EV2:
    case METHOD_EV3:
        write_ev(bin_index_nodes, raster_row, point_binning->index_array, row,
                 cols, rtype, point_binning->method);
        break;

    default:
        G_debug(2, "No method selected");
    }
}

void update_value(struct PointBinning *point_binning,
                  struct BinIndex *bin_index_nodes, int cols, int arr_row,
                  int arr_col, RASTER_MAP_TYPE rtype, double x, double y,
                  double z)
{
    if (point_binning->bin_n)
        update_n(point_binning->n_array, cols, arr_row, arr_col);
    if (point_binning->bin_min)
        update_min(point_binning->min_array, cols, arr_row, arr_col, rtype, z);
    if (point_binning->bin_max)
        update_max(point_binning->max_array, cols, arr_row, arr_col, rtype, z);
    if (point_binning->bin_sum)
        update_sum(point_binning->sum_array, point_binning->c_array, cols,
                   arr_row, arr_col, rtype, z);
    if (point_binning->bin_m2)
        update_m2(point_binning->n_array, point_binning->mean_array,
                  point_binning->m2_array, cols, arr_row, arr_col, rtype, z);
    if (point_binning->bin_z_index)
        update_bin_z_index(bin_index_nodes, point_binning->index_array, cols,
                           arr_row, arr_col, z);
    if (point_binning->bin_cnt_index)
        update_bin_cnt_index(bin_index_nodes, point_binning->index_array, cols,
                             arr_row, arr_col, z);
    if (point_binning->bin_eigenvalues)
        update_bin_com_index(bin_index_nodes, point_binning->index_array, cols,
                             arr_row, arr_col, x, y, z);
}
