/*
 * r.in.pdal point binning output value calculation
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
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gmath.h>

#include "point_binning.h"
#include "bin_write.h"

/* Get error corrected sum */
double get_sum(void *sum_array, void *c_array, int row, int cols, int col,
               RASTER_MAP_TYPE rtype)
{
    size_t offset = ((size_t)row * cols + col) * Rast_cell_size(rtype);
    double sum = Rast_get_d_value(((char *)sum_array) + offset, rtype);
    double c = Rast_get_d_value(((char *)c_array) + offset, rtype);

    return sum + c;
}

void write_sum(void *raster_row, void *sum_array, void *c_array, int row,
               int cols, RASTER_MAP_TYPE rtype)
{
    int col;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        double sum = get_sum(sum_array, c_array, row, cols, col, rtype);

        Rast_set_d_value(ptr, sum, rtype);
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_variance(void *raster_row, void *n_array, void *mean_array,
                    void *m2_array, int row, int cols, RASTER_MAP_TYPE rtype,
                    int method)
{
    double variance;
    int col;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        size_t offset = ((size_t)row * cols + col) * Rast_cell_size(rtype);
        size_t n_offset =
            ((size_t)row * cols + col) * Rast_cell_size(CELL_TYPE);
        int n = Rast_get_c_value(((char *)n_array) + n_offset, CELL_TYPE);
        double mean = Rast_get_d_value(((char *)mean_array) + offset, rtype);
        double m2 = Rast_get_d_value(((char *)m2_array) + offset, rtype);

        if (n == 0)
            Rast_set_null_value(ptr, 1, rtype);
        else if (n == 1)
            Rast_set_d_value(ptr, 0.0, rtype);
        else {
            variance = m2 / n;

            if (variance < GRASS_EPSILON)
                variance = 0.0;

            if (method == METHOD_STDDEV)
                variance = sqrt(variance);

            else if (method == METHOD_COEFF_VAR)
                variance = 100 * sqrt(variance) / mean;

            Rast_set_d_value(ptr, variance, rtype);
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_median(struct BinIndex *bin_index, void *raster_row,
                  void *index_array, int row, int cols, RASTER_MAP_TYPE rtype)
{
    int n;
    int j;
    double z;
    int col;
    int node_id, head_id;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        size_t n_offset =
            ((size_t)row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(((char *)index_array) + n_offset,
                               CELL_TYPE)) /* no points in cell */
            Rast_set_null_value(ptr, 1, rtype);
        else { /* one or more points in cell */

            head_id =
                Rast_get_c_value(((char *)index_array) + n_offset, CELL_TYPE);
            node_id = head_id;

            n = 0;

            while (node_id != -1) { /* count number of points in cell */
                n++;
                node_id = ((struct z_node *)bin_index->nodes)[node_id].next;
            }

            if (n == 1) /* only one point, use that */
                Rast_set_d_value(
                    ptr, ((struct z_node *)bin_index->nodes)[head_id].z, rtype);
            else if (n % 2 !=
                     0) { /* odd number of points: median_i = (n + 1) / 2 */
                n = (n + 1) / 2;
                node_id = head_id;
                for (j = 1; j < n; j++) /* get "median element" */
                    node_id = ((struct z_node *)bin_index->nodes)[node_id].next;

                Rast_set_d_value(
                    ptr, ((struct z_node *)bin_index->nodes)[node_id].z, rtype);
            }
            else { /* even number of points: median = (val_below + val_above) /
                      2 */

                z = (n + 1) / 2.0;
                n = floor(z);
                node_id = head_id;
                for (j = 1; j < n; j++) /* get element "below" */
                    node_id = ((struct z_node *)bin_index->nodes)[node_id].next;

                z = (((struct z_node *)bin_index->nodes)[node_id].z +
                     ((struct z_node *)bin_index->nodes)
                         [((struct z_node *)bin_index->nodes)[node_id].next]
                             .z) /
                    2;
                Rast_set_d_value(ptr, z, rtype);
            }
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_mode(struct BinIndex *bin_index, void *raster_row, void *index_array,
                int row, int cols)
{
    int col;
    int node_id;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        size_t n_offset =
            ((size_t)row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(((char *)index_array) + n_offset,
                               CELL_TYPE)) /* no points in cell */
            Rast_set_null_value(ptr, 1, CELL_TYPE);
        else {
            int mode_node = -1;

            node_id =
                Rast_get_c_value(((char *)index_array) + n_offset, CELL_TYPE);

            while (node_id != -1) {
                if (mode_node == -1)
                    mode_node = node_id;
                else if (((struct cnt_node *)bin_index->nodes)[node_id].count >
                         ((struct cnt_node *)bin_index->nodes)[mode_node].count)
                    mode_node = node_id;
                node_id = ((struct cnt_node *)bin_index->nodes)[node_id].next;
            }
            Rast_set_c_value(
                ptr, ((struct cnt_node *)bin_index->nodes)[mode_node].value,
                CELL_TYPE);
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(CELL_TYPE));
    }
}

void write_percentile(struct BinIndex *bin_index, void *raster_row,
                      void *index_array, int row, int cols,
                      RASTER_MAP_TYPE rtype, int pth)
{
    int n;
    int j;
    double z;
    int col;
    int node_id, head_id;
    int r_low, r_up;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        size_t n_offset =
            ((size_t)row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(((char *)index_array) + n_offset,
                               CELL_TYPE)) /* no points in cell */
            Rast_set_null_value(ptr, 1, rtype);
        else {
            head_id =
                Rast_get_c_value(((char *)index_array) + n_offset, CELL_TYPE);
            node_id = head_id;
            n = 0;

            while (node_id != -1) { /* count number of points in cell */
                n++;
                node_id = ((struct z_node *)bin_index->nodes)[node_id].next;
            }

            z = (pth * (n + 1)) / 100.0;
            r_low = floor(z); /* lower rank */
            if (r_low < 1)
                r_low = 1;
            else if (r_low > n)
                r_low = n;

            r_up = ceil(z); /* upper rank */
            if (r_up > n)
                r_up = n;

            node_id = head_id;
            for (j = 1; j < r_low; j++) /* search lower value */
                node_id = ((struct z_node *)bin_index->nodes)[node_id].next;

            z = ((struct z_node *)bin_index->nodes)[node_id]
                    .z; /* save lower value */
            node_id = head_id;
            for (j = 1; j < r_up; j++) /* search upper value */
                node_id = ((struct z_node *)bin_index->nodes)[node_id].next;

            z = (z + ((struct z_node *)bin_index->nodes)[node_id].z) / 2;
            Rast_set_d_value(ptr, z, rtype);
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_skewness(struct BinIndex *bin_index, void *raster_row,
                    void *index_array, int row, int cols, RASTER_MAP_TYPE rtype)
{
    int n;
    double z;
    int col;
    int node_id, head_id;
    double variance, mean, skew, sumdev;
    double sum;
    double sumsq;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        size_t n_offset =
            ((size_t)row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(((char *)index_array) + n_offset,
                               CELL_TYPE)) /* no points in cell */
            Rast_set_null_value(ptr, 1, rtype);
        else {
            head_id =
                Rast_get_c_value(((char *)index_array) + n_offset, CELL_TYPE);
            node_id = head_id;

            n = 0;        /* count */
            sum = 0.0;    /* sum */
            sumsq = 0.0;  /* sum of squares */
            sumdev = 0.0; /* sum of (xi - mean)^3 */
            skew = 0.0;   /* skewness */

            while (node_id != -1) {
                z = ((struct z_node *)bin_index->nodes)[node_id].z;
                n++;
                sum += z;
                sumsq += (z * z);
                node_id = ((struct z_node *)bin_index->nodes)[node_id].next;
            }

            if (n > 1) { /* if n == 1, skew is "0.0" */
                mean = sum / n;
                node_id = head_id;
                while (node_id != -1) {
                    z = ((struct z_node *)bin_index->nodes)[node_id].z;
                    sumdev += pow((z - mean), 3);
                    node_id = ((struct z_node *)bin_index->nodes)[node_id].next;
                }

                variance = (sumsq - sum * sum / n) / n;
                if (variance < GRASS_EPSILON)
                    skew = 0.0;
                else
                    skew = sumdev / ((n - 1) * pow(sqrt(variance), 3));
            }
            Rast_set_d_value(ptr, skew, rtype);
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_trimmean(struct BinIndex *bin_index, void *raster_row,
                    void *index_array, int row, int cols, RASTER_MAP_TYPE rtype,
                    double trim)
{
    int n;
    int j, k;
    int col;
    int node_id, head_id;
    double mean;
    double sum;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        size_t n_offset =
            ((size_t)row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(((char *)index_array) + n_offset,
                               CELL_TYPE)) /* no points in cell */
            Rast_set_null_value(ptr, 1, rtype);
        else {
            head_id =
                Rast_get_c_value(((char *)index_array) + n_offset, CELL_TYPE);

            node_id = head_id;
            n = 0;
            while (node_id != -1) { /* count number of points in cell */
                n++;
                node_id = ((struct z_node *)bin_index->nodes)[node_id].next;
            }

            if (1 == n)
                mean = ((struct z_node *)bin_index->nodes)[head_id].z;
            else {
                k = floor(trim * n +
                          0.5); /* number of ranks to discard on each tail */

                if (k > 0 && (n - 2 * k) > 0) { /* enough elements to discard */
                    node_id = head_id;
                    for (j = 0; j < k; j++) /* move to first rank to consider */
                        node_id =
                            ((struct z_node *)bin_index->nodes)[node_id].next;

                    j = k + 1;
                    k = n - k;
                    n = 0;
                    sum = 0.0;

                    while (j <= k) { /* get values in interval */
                        n++;
                        sum += ((struct z_node *)bin_index->nodes)[node_id].z;
                        node_id =
                            ((struct z_node *)bin_index->nodes)[node_id].next;
                        j++;
                    }
                }
                else {
                    node_id = head_id;
                    n = 0;
                    sum = 0.0;
                    while (node_id != -1) {
                        n++;
                        sum += ((struct z_node *)bin_index->nodes)[node_id].z;
                        node_id =
                            ((struct z_node *)bin_index->nodes)[node_id].next;
                    }
                }
                mean = sum / n;
            }
            Rast_set_d_value(ptr, mean, rtype);
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_sidn(struct BinIndex *bin_index, void *raster_row, void *index_array,
                int row, int cols, int min)
{
    int col;
    int node_id;
    void *ptr = raster_row;
    int count;

    for (col = 0; col < cols; col++) {
        size_t n_offset =
            ((size_t)row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(((char *)index_array) + n_offset,
                               CELL_TYPE)) /* no points in cell */
            Rast_set_c_value(ptr, 0, CELL_TYPE);
        else {

            node_id =
                Rast_get_c_value(((char *)index_array) + n_offset, CELL_TYPE);

            count = ((struct cnt_node *)bin_index->nodes)[node_id].count;
            node_id = ((struct cnt_node *)bin_index->nodes)[node_id].next;
            while (node_id != -1) {
                if (min &&
                    ((struct cnt_node *)bin_index->nodes)[node_id].count <
                        count)
                    count =
                        ((struct cnt_node *)bin_index->nodes)[node_id].count;
                else if (!min &&
                         ((struct cnt_node *)bin_index->nodes)[node_id].count >
                             count)
                    count =
                        ((struct cnt_node *)bin_index->nodes)[node_id].count;
                node_id = ((struct cnt_node *)bin_index->nodes)[node_id].next;
            }
            Rast_set_c_value(ptr, count, CELL_TYPE);
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(CELL_TYPE));
    }
}

void write_ev(struct BinIndex *bin_index, void *raster_row, void *index_array,
              int row, int cols, RASTER_MAP_TYPE rtype, int method)
{
    void *ptr = raster_row;
    double **cov_matrix = G_alloc_matrix(3, 3);
    double *ev = (double *)G_malloc(3 * sizeof(double));

    for (int col = 0; col < cols; col++) {
        size_t n_offset =
            ((size_t)row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(((char *)index_array) + n_offset,
                               CELL_TYPE)) /* no points in cell */
            Rast_set_null_value(ptr, 1, rtype);
        else {
            int node_id;
            struct com_node cn;

            node_id =
                Rast_get_c_value(((char *)index_array) + n_offset, CELL_TYPE);
            cn = ((struct com_node *)bin_index->nodes)[node_id];

            cov_matrix[0][0] = cn.comoment[0] / cn.n;
            cov_matrix[0][1] = cov_matrix[1][0] = cn.comoment[1] / cn.n;
            cov_matrix[0][2] = cov_matrix[2][0] = cn.comoment[2] / cn.n;
            cov_matrix[1][1] = cn.comoment[3] / cn.n;
            cov_matrix[1][2] = cov_matrix[2][1] = cn.comoment[4] / cn.n;
            cov_matrix[2][2] = cn.comoment[5] / cn.n;

            G_math_eigval(cov_matrix, ev, 3);

            switch (method) {
            case METHOD_EV1:
                Rast_set_d_value(ptr, ev[0], rtype);
                break;
            case METHOD_EV2:
                Rast_set_d_value(ptr, ev[1], rtype);
                break;
            case METHOD_EV3:
                Rast_set_d_value(ptr, ev[1], rtype);
                break;
            }
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(CELL_TYPE));
    }

    G_free_matrix(cov_matrix);
    G_free(ev);
}
