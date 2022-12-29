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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/vector.h>

#include "point_binning.h"
#include "local_proto.h"

#define SIZE_INCREMENT 10

static int new_node(struct BinIndex *bin_index)
{
    int n = bin_index->num_nodes++;

    if (bin_index->num_nodes >= bin_index->max_nodes) {
        bin_index->max_nodes += SIZE_INCREMENT;
        bin_index->nodes = G_realloc(bin_index->nodes,
                                     (size_t) bin_index->max_nodes *
                                     sizeof(struct node));
    }

    return n;
}


/* add node to sorted, single linked list 
 * returns id if head has to be saved to index array, otherwise -1 */
static int add_node(struct BinIndex *bin_index, int head, double z)
{
    int node_id, last_id, newnode_id, head_id;

    head_id = head;
    node_id = head_id;
    last_id = head_id;

    while (node_id != -1 && bin_index->nodes[node_id].z < z) {
        last_id = node_id;
        node_id = bin_index->nodes[node_id].next;
    }

    /* end of list, simply append */
    if (node_id == -1) {
        newnode_id = new_node(bin_index);
        bin_index->nodes[newnode_id].next = -1;
        bin_index->nodes[newnode_id].z = z;
        bin_index->nodes[last_id].next = newnode_id;
        return -1;
    }
    else if (node_id == head_id) {      /* pole position, insert as head */
        newnode_id = new_node(bin_index);
        bin_index->nodes[newnode_id].next = head_id;
        head_id = newnode_id;
        bin_index->nodes[newnode_id].z = z;
        return (head_id);
    }
    else {                      /* somewhere in the middle, insert */
        newnode_id = new_node(bin_index);
        bin_index->nodes[newnode_id].z = z;
        bin_index->nodes[newnode_id].next = node_id;
        bin_index->nodes[last_id].next = newnode_id;
        return -1;
    }
}


int update_bin_index(struct BinIndex *bin_index, void *index_array,
                     int cols, int row, int col,
                     RASTER_MAP_TYPE map_type, double value)
{
    int head_id;
    void *ptr = index_array;

    ptr =
        G_incr_void_ptr(ptr,
                        (((size_t) row * cols) + col) * Rast_cell_size(CELL_TYPE));

    /* first node */
    if (Rast_is_null_value(ptr, CELL_TYPE)) {
        head_id = new_node(bin_index);
        bin_index->nodes[head_id].next = -1;
        bin_index->nodes[head_id].z = value;
        /* store index to head */
        Rast_set_c_value(ptr, head_id, CELL_TYPE);
    }
    /* head is already there */
    else {

        /* get index to head */
        head_id = Rast_get_c_value(ptr, CELL_TYPE);
        head_id = add_node(bin_index, head_id, value);
        /* if id valid, store index to head */
        if (head_id != -1)
            Rast_set_c_value(ptr, head_id, CELL_TYPE);
    }
    /* for consistency with functions from support.c */
    return 0;
}

void point_binning_set(struct PointBinning *point_binning, char *method,
                       char *percentile, char *trim, int bin_coordinates)
{

    /* figure out what maps we need in memory */
    /*  n               n
       min              min
       max              max
       range            min max         max - min
       sum              sum
       mean             sum n           sum/n
       stddev           sum sumsq n     sqrt((sumsq - sum*sum/n)/n)
       variance         sum sumsq n     (sumsq - sum*sum/n)/n
       coeff_var        sum sumsq n     sqrt((sumsq - sum*sum/n)/n) / (sum/n)
       median           n               array index to linked list
       percentile       n               array index to linked list
       skewness         n               array index to linked list
       trimmean         n               array index to linked list
     */
    point_binning->method = METHOD_NONE;
    point_binning->bin_n = FALSE;
    point_binning->bin_min = FALSE;
    point_binning->bin_max = FALSE;
    point_binning->bin_sum = FALSE;
    point_binning->bin_sumsq = FALSE;
    point_binning->bin_index = FALSE;
    point_binning->bin_coordinates = FALSE;

    point_binning->n_array = NULL;
    point_binning->min_array = NULL;
    point_binning->max_array = NULL;
    point_binning->sum_array = NULL;
    point_binning->sumsq_array = NULL;
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
        point_binning->bin_sum = TRUE;
        point_binning->bin_sumsq = TRUE;
        point_binning->bin_n = TRUE;
    }
    if (strcmp(method, "variance") == 0) {
        point_binning->method = METHOD_VARIANCE;
        point_binning->bin_sum = TRUE;
        point_binning->bin_sumsq = TRUE;
        point_binning->bin_n = TRUE;
    }
    if (strcmp(method, "coeff_var") == 0) {
        point_binning->method = METHOD_COEFF_VAR;
        point_binning->bin_sum = TRUE;
        point_binning->bin_sumsq = TRUE;
        point_binning->bin_n = TRUE;
    }
    if (strcmp(method, "median") == 0) {
        point_binning->method = METHOD_MEDIAN;
        point_binning->bin_index = TRUE;
    }
    if (strcmp(method, "percentile") == 0) {
        if (percentile != NULL)
            point_binning->pth = atoi(percentile);
        else
            G_fatal_error(_("Unable to calculate percentile without the pth option specified!"));
        point_binning->method = METHOD_PERCENTILE;
        point_binning->bin_index = TRUE;
    }
    if (strcmp(method, "skewness") == 0) {
        point_binning->method = METHOD_SKEWNESS;
        point_binning->bin_index = TRUE;
    }
    if (strcmp(method, "trimmean") == 0) {
        if (trim != NULL)
            point_binning->trim = atof(trim) / 100.0;
        else
            G_fatal_error(_("Unable to calculate trimmed mean without the trim option specified!"));
        point_binning->method = METHOD_TRIMMEAN;
        point_binning->bin_index = TRUE;
    }
    if (bin_coordinates) {
        /* x, y */
        point_binning->bin_coordinates = TRUE;
        /* z and n */
        point_binning->bin_sum = TRUE;
        point_binning->bin_n = TRUE;
    }
}


/* check if rows * (cols + 1) go into a size_t */
int check_rows_cols_fit_to_size_t(int rows, int cols)
{
    if (sizeof(size_t) < 8) {
        double dsize = rows * (cols + 1);

        /* TODO: the comparison with double may fail */
        if (dsize != (size_t) rows * (cols + 1))
            return FALSE;
    }
    return TRUE;
}


void point_binning_memory_test(struct PointBinning *point_binning, int rows,
                               int cols, RASTER_MAP_TYPE rtype)
{
    /* allocate memory (test for enough before we start) */
    if (point_binning->bin_n)
        point_binning->n_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(CELL_TYPE));
    if (point_binning->bin_min)
        point_binning->min_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
    if (point_binning->bin_max)
        point_binning->max_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
    if (point_binning->bin_sum)
        point_binning->sum_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
    if (point_binning->bin_sumsq)
        point_binning->sumsq_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
    if (point_binning->bin_index)
        point_binning->index_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(CELL_TYPE));
    if (point_binning->bin_coordinates) {
        point_binning->x_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
        point_binning->y_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
    }
    /* TODO: perhaps none of them needs to be freed */

    /* and then free it again */
    if (point_binning->bin_n)
        G_free(point_binning->n_array);
    if (point_binning->bin_min)
        G_free(point_binning->min_array);
    if (point_binning->bin_max)
        G_free(point_binning->max_array);
    if (point_binning->bin_sum)
        G_free(point_binning->sum_array);
    if (point_binning->bin_sumsq)
        G_free(point_binning->sumsq_array);
    if (point_binning->bin_index)
        G_free(point_binning->index_array);
    if (point_binning->bin_coordinates) {
        G_free(point_binning->x_array);
        G_free(point_binning->y_array);
    }
}


void point_binning_allocate(struct PointBinning *point_binning, int rows,
                            int cols, RASTER_MAP_TYPE rtype)
{
    if (point_binning->bin_n) {
        G_debug(2, "allocating n_array");
        point_binning->n_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(CELL_TYPE));
        blank_array(point_binning->n_array, rows, cols, CELL_TYPE, 0);
    }
    if (point_binning->bin_min) {
        G_debug(2, "allocating min_array");
        point_binning->min_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->min_array, rows, cols, rtype, -1);   /* fill with NULLs */
    }
    if (point_binning->bin_max) {
        G_debug(2, "allocating max_array");
        point_binning->max_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->max_array, rows, cols, rtype, -1);   /* fill with NULLs */
    }
    if (point_binning->bin_sum) {
        G_debug(2, "allocating sum_array");
        point_binning->sum_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->sum_array, rows, cols, rtype, 0);
    }
    if (point_binning->bin_sumsq) {
        G_debug(2, "allocating sumsq_array");
        point_binning->sumsq_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->sumsq_array, rows, cols, rtype, 0);
    }
    if (point_binning->bin_index) {
        G_debug(2, "allocating index_array");
        point_binning->index_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(CELL_TYPE));
        blank_array(point_binning->index_array, rows, cols, CELL_TYPE, -1);     /* fill with NULLs */
    }
    if (point_binning->bin_coordinates) {
        G_debug(2, "allocating x_array and y_array");
        point_binning->x_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->x_array, rows, cols, rtype, 0);
        point_binning->y_array =
            G_calloc((size_t) rows * (cols + 1), Rast_cell_size(rtype));
        blank_array(point_binning->y_array, rows, cols, rtype, 0);
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
    if (point_binning->bin_sum)
        G_free(point_binning->sum_array);
    if (point_binning->bin_sumsq)
        G_free(point_binning->sumsq_array);
    if (point_binning->bin_index) {
        G_free(point_binning->index_array);
        G_free(bin_index_nodes->nodes);
        bin_index_nodes->num_nodes = 0;
        bin_index_nodes->max_nodes = 0;
        bin_index_nodes->nodes = NULL;
    }
    if (point_binning->bin_coordinates) {
        G_free(point_binning->x_array);
        G_free(point_binning->y_array);
    }
}

void write_variance(void *raster_row, void *n_array, void *sum_array,
                    void *sumsq_array, int row, int cols,
                    RASTER_MAP_TYPE rtype, int method)
{
    size_t offset, n_offset;
    int n = 0;
    double variance;
    double sum = 0.;
    double sumsq = 0.;
    int col;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        offset = ((size_t) row * cols + col) * Rast_cell_size(rtype);
        n_offset = ((size_t) row * cols + col) * Rast_cell_size(CELL_TYPE);
        n = Rast_get_c_value(n_array + n_offset, CELL_TYPE);
        sum = Rast_get_d_value(sum_array + offset, rtype);
        sumsq = Rast_get_d_value(sumsq_array + offset, rtype);

        if (n == 0)
            Rast_set_null_value(ptr, 1, rtype);
        else if (n == 1)
            Rast_set_d_value(ptr, 0.0, rtype);
        else {
            variance = (sumsq - sum * sum / n) / n;
            if (variance < GRASS_EPSILON)
                variance = 0.0;

            /* nan test */
            if (variance != variance)
                Rast_set_null_value(ptr, 1, rtype);
            else {

                if (method == METHOD_STDDEV)
                    variance = sqrt(variance);

                else if (method == METHOD_COEFF_VAR)
                    variance = 100 * sqrt(variance) / (sum / n);

                /* nan test */
                if (variance != variance)
                    variance = 0.0;     /* OK for n > 0 ? */

                Rast_set_d_value(ptr, variance, rtype);
            }

        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_median(struct BinIndex *bin_index, void *raster_row,
                  void *index_array, int row, int cols, RASTER_MAP_TYPE rtype)
{
    size_t n_offset;
    int n;
    int j;
    double z;
    int col;
    int node_id, head_id;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        n_offset = ((size_t) row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(index_array + n_offset, CELL_TYPE))      /* no points in cell */
            Rast_set_null_value(ptr, 1, rtype);
        else {                  /* one or more points in cell */

            head_id = Rast_get_c_value(index_array + n_offset, CELL_TYPE);
            node_id = head_id;

            n = 0;

            while (node_id != -1) {     /* count number of points in cell */
                n++;
                node_id = bin_index->nodes[node_id].next;
            }

            if (n == 1)         /* only one point, use that */
                Rast_set_d_value(ptr, bin_index->nodes[head_id].z, rtype);
            else if (n % 2 != 0) {      /* odd number of points: median_i = (n + 1) / 2 */
                n = (n + 1) / 2;
                node_id = head_id;
                for (j = 1; j < n; j++) /* get "median element" */
                    node_id = bin_index->nodes[node_id].next;

                Rast_set_d_value(ptr, bin_index->nodes[node_id].z, rtype);
            }
            else {              /* even number of points: median = (val_below + val_above) / 2 */

                z = (n + 1) / 2.0;
                n = floor(z);
                node_id = head_id;
                for (j = 1; j < n; j++) /* get element "below" */
                    node_id = bin_index->nodes[node_id].next;

                z = (bin_index->nodes[node_id].z +
                     bin_index->nodes[bin_index->nodes[node_id].next].z) / 2;
                Rast_set_d_value(ptr, z, rtype);
            }
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_percentile(struct BinIndex *bin_index, void *raster_row,
                      void *index_array, int row, int cols,
                      RASTER_MAP_TYPE rtype, int pth)
{
    size_t n_offset;
    int n;
    int j;
    double z;
    int col;
    int node_id, head_id;
    int r_low, r_up;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        n_offset = ((size_t) row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(index_array + n_offset, CELL_TYPE))      /* no points in cell */
            Rast_set_null_value(ptr, 1, rtype);
        else {
            head_id = Rast_get_c_value(index_array + n_offset, CELL_TYPE);
            node_id = head_id;
            n = 0;

            while (node_id != -1) {     /* count number of points in cell */
                n++;
                node_id = bin_index->nodes[node_id].next;
            }

            z = (pth * (n + 1)) / 100.0;
            r_low = floor(z);   /* lower rank */
            if (r_low < 1)
                r_low = 1;
            else if (r_low > n)
                r_low = n;

            r_up = ceil(z);     /* upper rank */
            if (r_up > n)
                r_up = n;

            node_id = head_id;
            for (j = 1; j < r_low; j++) /* search lower value */
                node_id = bin_index->nodes[node_id].next;

            z = bin_index->nodes[node_id].z;    /* save lower value */
            node_id = head_id;
            for (j = 1; j < r_up; j++)  /* search upper value */
                node_id = bin_index->nodes[node_id].next;

            z = (z + bin_index->nodes[node_id].z) / 2;
            Rast_set_d_value(ptr, z, rtype);
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_skewness(struct BinIndex *bin_index, void *raster_row,
                    void *index_array, int row, int cols,
                    RASTER_MAP_TYPE rtype)
{
    size_t n_offset;
    int n;
    double z;
    int col;
    int node_id, head_id;
    double variance, mean, skew, sumdev;
    double sum = 0.;
    double sumsq = 0.;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        n_offset = ((size_t) row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(index_array + n_offset, CELL_TYPE))      /* no points in cell */
            Rast_set_null_value(ptr, 1, rtype);
        else {
            head_id = Rast_get_c_value(index_array + n_offset, CELL_TYPE);
            node_id = head_id;

            n = 0;              /* count */
            sum = 0.0;          /* sum */
            sumsq = 0.0;        /* sum of squares */
            sumdev = 0.0;       /* sum of (xi - mean)^3 */
            skew = 0.0;         /* skewness */

            while (node_id != -1) {
                z = bin_index->nodes[node_id].z;
                n++;
                sum += z;
                sumsq += (z * z);
                node_id = bin_index->nodes[node_id].next;
            }

            if (n > 1) {        /* if n == 1, skew is "0.0" */
                mean = sum / n;
                node_id = head_id;
                while (node_id != -1) {
                    z = bin_index->nodes[node_id].z;
                    sumdev += pow((z - mean), 3);
                    node_id = bin_index->nodes[node_id].next;
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
                    void *index_array, int row, int cols,
                    RASTER_MAP_TYPE rtype, double trim)
{
    size_t n_offset;
    int n;
    int j, k;
    int col;
    int node_id, head_id;
    double mean;
    double sum = 0.;
    void *ptr = raster_row;

    for (col = 0; col < cols; col++) {
        n_offset = ((size_t) row * cols + col) * Rast_cell_size(CELL_TYPE);
        if (Rast_is_null_value(index_array + n_offset, CELL_TYPE))      /* no points in cell */
            Rast_set_null_value(ptr, 1, rtype);
        else {
            head_id = Rast_get_c_value(index_array + n_offset, CELL_TYPE);

            node_id = head_id;
            n = 0;
            while (node_id != -1) {     /* count number of points in cell */
                n++;
                node_id = bin_index->nodes[node_id].next;
            }

            if (1 == n)
                mean = bin_index->nodes[head_id].z;
            else {
                k = floor(trim * n + 0.5);      /* number of ranks to discard on each tail */

                if (k > 0 && (n - 2 * k) > 0) { /* enough elements to discard */
                    node_id = head_id;
                    for (j = 0; j < k; j++)     /* move to first rank to consider */
                        node_id = bin_index->nodes[node_id].next;

                    j = k + 1;
                    k = n - k;
                    n = 0;
                    sum = 0.0;

                    while (j <= k) {    /* get values in interval */
                        n++;
                        sum += bin_index->nodes[node_id].z;
                        node_id = bin_index->nodes[node_id].next;
                        j++;
                    }
                }
                else {
                    node_id = head_id;
                    n = 0;
                    sum = 0.0;
                    while (node_id != -1) {
                        n++;
                        sum += bin_index->nodes[node_id].z;
                        node_id = bin_index->nodes[node_id].next;
                    }
                }
                mean = sum / n;
            }
            Rast_set_d_value(ptr, mean, rtype);
        }
        ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
    }
}

void write_values(struct PointBinning *point_binning,
                  struct BinIndex *bin_index_nodes, void *raster_row, int row,
                  int cols, RASTER_MAP_TYPE rtype,
                  struct VectorWriter *vector_writer)
{
    void *ptr = NULL;
    int col;

    switch (point_binning->method) {
    case METHOD_N:             /* n is a straight copy */
        Rast_raster_cpy(raster_row,
                        point_binning->n_array +
                        ((size_t) row * cols * Rast_cell_size(CELL_TYPE)), cols,
                        CELL_TYPE);
        break;

    case METHOD_MIN:
        Rast_raster_cpy(raster_row,
                        point_binning->min_array +
                        ((size_t) row * cols * Rast_cell_size(rtype)), cols, rtype);
        break;

    case METHOD_MAX:
        Rast_raster_cpy(raster_row,
                        point_binning->max_array +
                        ((size_t) row * cols * Rast_cell_size(rtype)), cols, rtype);
        break;

    case METHOD_SUM:
        Rast_raster_cpy(raster_row,
                        point_binning->sum_array +
                        ((size_t) row * cols * Rast_cell_size(rtype)), cols, rtype);
        break;

    case METHOD_RANGE:         /* (max-min) */
        ptr = raster_row;
        for (col = 0; col < cols; col++) {
            size_t offset = ((size_t) row * cols + col) * Rast_cell_size(rtype);
            double min =
                Rast_get_d_value(point_binning->min_array + offset, rtype);
            double max =
                Rast_get_d_value(point_binning->max_array + offset, rtype);
            Rast_set_d_value(ptr, max - min, rtype);
            ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
        }
        break;

    case METHOD_MEAN:          /* (sum / n) */
        ptr = raster_row;
        for (col = 0; col < cols; col++) {
            size_t offset = ((size_t) row * cols + col) * Rast_cell_size(rtype);
            size_t n_offset = ((size_t) row * cols + col) * Rast_cell_size(CELL_TYPE);
            int n = Rast_get_c_value(point_binning->n_array + n_offset,
                                     CELL_TYPE);
            double sum =
                Rast_get_d_value(point_binning->sum_array + offset, rtype);

            if (n == 0)
                Rast_set_null_value(ptr, 1, rtype);
            else
                Rast_set_d_value(ptr, (sum / n), rtype);

            ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
        }
        break;

    case METHOD_STDDEV:        /*  sqrt(variance)        */
    case METHOD_VARIANCE:      /*  (sumsq - sum*sum/n)/n */
    case METHOD_COEFF_VAR:     /*  100 * stdev / mean    */
        write_variance(raster_row, point_binning->n_array,
                       point_binning->sum_array, point_binning->sumsq_array,
                       row, cols, rtype, point_binning->method);
        break;
    case METHOD_MEDIAN:        /* median, if only one point in cell we will use that */
        write_median(bin_index_nodes, raster_row, point_binning->index_array,
                     row, cols, rtype);
        break;
    case METHOD_PERCENTILE:    /* rank = (pth*(n+1))/100; interpolate linearly */
        write_percentile(bin_index_nodes, raster_row,
                         point_binning->index_array, row, cols, rtype,
                         point_binning->pth);
        break;
    case METHOD_SKEWNESS:      /* skewness = sum(xi-mean)^3/(N-1)*s^3 */
        write_skewness(bin_index_nodes, raster_row,
                       point_binning->index_array, row, cols, rtype);
        break;
    case METHOD_TRIMMEAN:
        write_trimmean(bin_index_nodes, raster_row,
                       point_binning->index_array, row, cols, rtype,
                       point_binning->trim);
        break;

    default:
        G_debug(2, "No method selected");
    }
    if (point_binning->bin_coordinates) {
        for (col = 0; col < cols; col++) {
            size_t offset = ((size_t) row * cols + col) * Rast_cell_size(rtype);
            size_t n_offset = ((size_t) row * cols + col) * Rast_cell_size(CELL_TYPE);
            int n = Rast_get_c_value(point_binning->n_array + n_offset,
                                     CELL_TYPE);

            if (n == 0)
                continue;

            double sum_x =
                Rast_get_d_value(point_binning->x_array + offset, rtype);
            double sum_y =
                Rast_get_d_value(point_binning->y_array + offset, rtype);
            /* TODO: we do this also in mean writing */
            double sum_z =
                Rast_get_d_value(point_binning->sum_array + offset, rtype);

            /* We are not writing any categories. They are not needed
             * and potentially it is too much trouble to do it and it is
             * unclear what to write. Not writing them is also a little
             * bit faster. */
            Vect_append_point(vector_writer->points, sum_x, sum_y, sum_z / n);
            Vect_write_line(vector_writer->info, GV_POINT,
                            vector_writer->points, vector_writer->cats);
            Vect_reset_line(vector_writer->points);
            vector_writer->count++;
        }
    }
}

/* TODO: duplication with support.c, refactoring needed */
static void *get_cell_ptr(void *array, int cols, int row, int col,
                          RASTER_MAP_TYPE map_type)
{
    return G_incr_void_ptr(array,
                           ((row * (size_t) cols) +
                            col) * Rast_cell_size(map_type));
}

int update_val(void *array, int cols, int row, int col,
               RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);

    Rast_set_d_value(ptr, value, map_type);
    return 0;
}

int update_moving_mean(void *array, int cols, int row, int col,
                       RASTER_MAP_TYPE rtype, double value, int n)
{
    /* for xy we do this check twice */
    if (n != 0) {
        double m_v;

        row_array_get_value_row_col(array, row, col, cols, rtype, &m_v);
        value = m_v + (value - m_v) / n;
    }
    /* else we just write the initial value */
    return update_val(array, cols, row, col, rtype, value);;
}

void update_value(struct PointBinning *point_binning,
                  struct BinIndex *bin_index_nodes, int cols, int arr_row,
                  int arr_col, RASTER_MAP_TYPE rtype, double x, double y,
                  double z)
{
    if (point_binning->bin_n)
        update_n(point_binning->n_array, cols, arr_row, arr_col);
    if (point_binning->bin_min)
        update_min(point_binning->min_array, cols, arr_row, arr_col, rtype,
                   z);
    if (point_binning->bin_max)
        update_max(point_binning->max_array, cols, arr_row, arr_col, rtype,
                   z);
    if (point_binning->bin_sum)
        update_sum(point_binning->sum_array, cols, arr_row, arr_col, rtype,
                   z);
    if (point_binning->bin_sumsq)
        update_sumsq(point_binning->sumsq_array, cols, arr_row, arr_col,
                     rtype, z);
    if (point_binning->bin_index)
        update_bin_index(bin_index_nodes, point_binning->index_array, cols,
                         arr_row, arr_col, rtype, z);
    if (point_binning->bin_coordinates) {
        /* this assumes that n is already computed for this xyz */
        void *ptr = get_cell_ptr(point_binning->n_array, cols, arr_row,
                                 arr_col, CELL_TYPE);
        int n = Rast_get_c_value(ptr, CELL_TYPE);

        update_moving_mean(point_binning->x_array, cols, arr_row, arr_col,
                           rtype, x, n);
        update_moving_mean(point_binning->y_array, cols, arr_row, arr_col,
                           rtype, y, n);
    }
}
