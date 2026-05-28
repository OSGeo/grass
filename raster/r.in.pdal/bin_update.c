/*
 * r.in.pdal Functions performing value updates on each incoming point
 *            during point binning process
 *   Copyright 2006 by M. Hamish Bowman, and the GRASS Development Team
 *   Author: M. Hamish Bowman, University of Otago, Dunedin, New Zealand
 *     Maris Nartiss code refactoring for r.in.pdal
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>

#include "point_binning.h"
#include "bin_update.h"

static int new_node(struct BinIndex *bin_index, size_t size)
{
    int n = bin_index->num_nodes++;

    if (bin_index->num_nodes >= bin_index->max_nodes) {
        bin_index->max_nodes += SIZE_INCREMENT;
        bin_index->nodes =
            G_realloc(bin_index->nodes, (size_t)bin_index->max_nodes * size);
    }

    return n;
}

void update_val(void *array, int cols, int row, int col,
                RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);

    Rast_set_d_value(ptr, value, map_type);
    return;
}

void update_n(void *array, int cols, int row, int col)
{
    void *ptr = get_cell_ptr(array, cols, row, col, CELL_TYPE);
    CELL old_n;

    old_n = Rast_get_c_value(ptr, CELL_TYPE);
    Rast_set_c_value(ptr, (1 + old_n), CELL_TYPE);

    return;
}

void update_min(void *array, int cols, int row, int col,
                RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    if (Rast_is_null_value(ptr, map_type))
        Rast_set_d_value(ptr, (DCELL)value, map_type);
    else {
        old_val = Rast_get_d_value(ptr, map_type);
        if (value < old_val)
            Rast_set_d_value(ptr, (DCELL)value, map_type);
    }
    return;
}

void update_max(void *array, int cols, int row, int col,
                RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    if (Rast_is_null_value(ptr, map_type))
        Rast_set_d_value(ptr, (DCELL)value, map_type);
    else {
        old_val = Rast_get_d_value(ptr, map_type);
        if (value > old_val)
            Rast_set_d_value(ptr, (DCELL)value, map_type);
    }

    return;
}

/* Implements improved Kahan–Babuska algorithm by Neumaier, A. 1974 */
void update_sum(void *sum_array, void *c_array, int cols, int row, int col,
                RASTER_MAP_TYPE map_type, double value)
{
    DCELL old_sum, old_c, tmp;
    void *s_ptr = get_cell_ptr(sum_array, cols, row, col, map_type);
    void *c_ptr = get_cell_ptr(c_array, cols, row, col, map_type);

    old_sum = Rast_get_d_value(s_ptr, map_type);
    old_c = Rast_get_d_value(c_ptr, map_type);
    tmp = old_sum + value;

    if (fabs(old_sum) >= fabs(value))
        Rast_set_d_value(c_ptr, old_c + (old_sum - tmp) + value, map_type);
    else
        Rast_set_d_value(c_ptr, old_c + (value - tmp) + old_sum, map_type);

    Rast_set_d_value(s_ptr, tmp, map_type);

    return;
}

/* Implements Welford algorithm */
void update_m2(void *n_array, void *mean_array, void *m2_array, int cols,
               int row, int col, RASTER_MAP_TYPE map_type, double value)
{
    int n;
    double m2, mean, d1, d2;
    void *n_ptr = get_cell_ptr(n_array, cols, row, col, CELL_TYPE);
    void *mean_ptr = get_cell_ptr(mean_array, cols, row, col, map_type);
    void *m2_ptr = get_cell_ptr(m2_array, cols, row, col, map_type);

    n = Rast_get_c_value(n_ptr, CELL_TYPE);
    mean = Rast_get_d_value(mean_ptr, map_type);
    m2 = Rast_get_d_value(m2_ptr, map_type);

    n++;
    Rast_set_c_value(n_ptr, n, CELL_TYPE);

    if (n == 1) {
        Rast_set_d_value(mean_ptr, value, map_type);
        return;
    }

    d1 = value - mean;
    mean = mean + (d1 / n);
    d2 = value - mean;
    m2 = m2 + (d1 * d2);

    Rast_set_d_value(mean_ptr, mean, map_type);
    Rast_set_d_value(m2_ptr, m2, map_type);

    return;
}

void update_moving_mean(void *array, int cols, int row, int col,
                        RASTER_MAP_TYPE rtype, double value, int n)
{
    /* for xy we do this check twice */
    if (n != 0) {
        double m_v;

        row_array_get_value_row_col(array, row, col, cols, rtype, &m_v);
        value = m_v + (value - m_v) / n;
    }
    /* else we just write the initial value */
    update_val(array, cols, row, col, rtype, value);
    return;
}

/* add node to sorted, single linked list
 * returns id if head has to be saved to index array, otherwise -1 */
int add_z_node(struct BinIndex *bin_index, int head, double z)
{
    int node_id, last_id, newnode_id, head_id;

    head_id = head;
    node_id = head_id;
    last_id = head_id;

    while (node_id != -1 &&
           ((struct z_node *)bin_index->nodes)[node_id].z < z) {
        last_id = node_id;
        node_id = ((struct z_node *)bin_index->nodes)[node_id].next;
    }

    /* end of list, simply append */
    if (node_id == -1) {
        newnode_id = new_node(bin_index, sizeof(struct z_node));
        ((struct z_node *)bin_index->nodes)[newnode_id].next = -1;
        ((struct z_node *)bin_index->nodes)[newnode_id].z = z;
        ((struct z_node *)bin_index->nodes)[last_id].next = newnode_id;
        return -1;
    }
    else if (node_id == head_id) { /* pole position, insert as head */
        newnode_id = new_node(bin_index, sizeof(struct z_node));
        ((struct z_node *)bin_index->nodes)[newnode_id].next = head_id;
        head_id = newnode_id;
        ((struct z_node *)bin_index->nodes)[newnode_id].z = z;
        return (head_id);
    }
    else { /* somewhere in the middle, insert */
        newnode_id = new_node(bin_index, sizeof(struct z_node));
        ((struct z_node *)bin_index->nodes)[newnode_id].z = z;
        ((struct z_node *)bin_index->nodes)[newnode_id].next = node_id;
        ((struct z_node *)bin_index->nodes)[last_id].next = newnode_id;
        return -1;
    }
}

void add_cnt_node(struct BinIndex *bin_index, int head, int value)
{
    int node_id, newnode_id, head_id, next;

    head_id = head;
    node_id = head_id;
    next = node_id;

    while (next != -1) {
        node_id = next;
        if (((struct cnt_node *)bin_index->nodes)[node_id].value == value) {
            ((struct cnt_node *)bin_index->nodes)[node_id].count++;
            return;
        }
        next = ((struct cnt_node *)bin_index->nodes)[node_id].next;
    }

    newnode_id = new_node(bin_index, sizeof(struct cnt_node));
    ((struct cnt_node *)bin_index->nodes)[newnode_id].next = -1;
    ((struct cnt_node *)bin_index->nodes)[newnode_id].value = value;
    ((struct cnt_node *)bin_index->nodes)[newnode_id].count = 1;
    ((struct cnt_node *)bin_index->nodes)[node_id].next = newnode_id;
    return;
}

/* Unlike the other functions, this one is not using map_type (RASTER_MAP_TYPE)
 * because the values (z) are always doubles and the index is integer. */
void update_bin_z_index(struct BinIndex *bin_index, void *index_array, int cols,
                        int row, int col, double value)
{
    int head_id;
    void *ptr = index_array;

    ptr = G_incr_void_ptr(ptr, (((size_t)row * cols) + col) *
                                   Rast_cell_size(CELL_TYPE));

    /* first node */
    if (Rast_is_null_value(ptr, CELL_TYPE)) {
        head_id = new_node(bin_index, sizeof(struct z_node));
        ((struct z_node *)bin_index->nodes)[head_id].next = -1;
        ((struct z_node *)bin_index->nodes)[head_id].z = value;
        /* store index to head */
        Rast_set_c_value(ptr, head_id, CELL_TYPE);
    }
    /* head is already there */
    else {

        /* get index to head */
        head_id = Rast_get_c_value(ptr, CELL_TYPE);
        head_id = add_z_node(bin_index, head_id, value);
        /* if id valid, store index to head */
        if (head_id != -1)
            Rast_set_c_value(ptr, head_id, CELL_TYPE);
    }

    return;
}

void update_bin_cnt_index(struct BinIndex *bin_index, void *index_array,
                          int cols, int row, int col, int value)
{
    int head_id;
    void *ptr = index_array;

    ptr = G_incr_void_ptr(ptr, (((size_t)row * cols) + col) *
                                   Rast_cell_size(CELL_TYPE));

    /* first node */
    if (Rast_is_null_value(ptr, CELL_TYPE)) {
        head_id = new_node(bin_index, sizeof(struct cnt_node));
        ((struct cnt_node *)bin_index->nodes)[head_id].next = -1;
        ((struct cnt_node *)bin_index->nodes)[head_id].value = value;
        ((struct cnt_node *)bin_index->nodes)[head_id].count = 1;
        /* store index to head */
        Rast_set_c_value(ptr, head_id, CELL_TYPE);
    }
    /* head is already there */
    else {
        head_id = Rast_get_c_value(ptr, CELL_TYPE);
        add_cnt_node(bin_index, head_id, value);
    }

    return;
}

/* Co-moment value update */
void update_com_node(struct com_node *cn, int item, double x, double y)
{
    double dx;

    dx = x - cn->meanx[item];
    cn->meanx[item] = cn->meanx[item] + (dx / cn->n);
    cn->meany[item] = cn->meany[item] + (y - cn->meany[item]) / cn->n;
    cn->comoment[item] = cn->comoment[item] + dx * (y - cn->meany[item]);
}

void update_bin_com_index(struct BinIndex *bin_index, void *index_array,
                          int cols, int row, int col, double x, double y,
                          double z)
{
    int node_id;
    void *ptr = index_array;

    ptr = G_incr_void_ptr(ptr, (((size_t)row * cols) + col) *
                                   Rast_cell_size(CELL_TYPE));

    if (Rast_is_null_value(ptr, CELL_TYPE)) {
        node_id = new_node(bin_index, sizeof(struct com_node));

        ((struct com_node *)bin_index->nodes)[node_id].n = 0;
        ((struct com_node *)bin_index->nodes)[node_id].meanx =
            (double *)G_calloc(6, sizeof(double));
        ((struct com_node *)bin_index->nodes)[node_id].meany =
            (double *)G_calloc(6, sizeof(double));
        ((struct com_node *)bin_index->nodes)[node_id].comoment =
            (double *)G_calloc(6, sizeof(double));

        /* store index to node */
        Rast_set_c_value(ptr, node_id, CELL_TYPE);
    }
    /* node is already there */
    else {
        node_id = Rast_get_c_value(ptr, CELL_TYPE);
    }

    /* update values */
    ((struct com_node *)bin_index->nodes)[node_id].n++;
    update_com_node(&(((struct com_node *)bin_index->nodes)[node_id]), 0, x, x);
    update_com_node(&(((struct com_node *)bin_index->nodes)[node_id]), 1, x, y);
    update_com_node(&(((struct com_node *)bin_index->nodes)[node_id]), 2, x, z);
    update_com_node(&(((struct com_node *)bin_index->nodes)[node_id]), 3, y, y);
    update_com_node(&(((struct com_node *)bin_index->nodes)[node_id]), 4, x, z);
    update_com_node(&(((struct com_node *)bin_index->nodes)[node_id]), 5, z, z);

    return;
}

/* 0 on NULL, 1 on success */
int row_array_get_value_row_col(void *array, int arr_row, int arr_col, int cols,
                                RASTER_MAP_TYPE rtype, double *value)
{
    void *ptr = array;

    ptr = G_incr_void_ptr(ptr, (((size_t)arr_row * cols) + arr_col) *
                                   Rast_cell_size(rtype));
    if (Rast_is_null_value(ptr, rtype))
        return 0;
    if (rtype == DCELL_TYPE)
        *value = (double)*(DCELL *)ptr;
    else if (rtype == FCELL_TYPE)
        *value = (double)*(FCELL *)ptr;
    else
        *value = (double)*(CELL *)ptr;
    return 1;
}
