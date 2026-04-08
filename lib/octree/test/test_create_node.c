/****************************************************************************
 *
 * MODULE:       test.octree.lib
 *
 * AUTHOR(S):    Corey White
 *
 * PURPOSE:      Unit tests for octree node creation
 *
 * COPYRIGHT:    (C) 2026 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "test_octree.h"

/* prototypes */
static int test_create_node_basic(void);
static int test_create_node_bounds(void);
static int test_create_node_children_null(void);
static int test_create_node_zero_bounds(void);
static int test_create_node_negative_bounds(void);
static int test_create_node_lazy_allocation(void);

/* ************************************************************************* */
/* Perform the create_octree_node unit tests ****************************** */
/* ************************************************************************* */
int unit_test_create_node(void)
{
    int sum = 0;

    G_message(_("\n++ Running create_octree_node unit tests ++"));

    sum += test_create_node_basic();
    sum += test_create_node_bounds();
    sum += test_create_node_children_null();
    sum += test_create_node_zero_bounds();
    sum += test_create_node_negative_bounds();
    sum += test_create_node_lazy_allocation();

    if (sum > 0)
        G_warning(_("\n-- create_octree_node unit tests failure --"));
    else
        G_message(_("\n-- create_octree_node unit tests finished "
                    "successfully --"));

    return sum;
}

/* ************************************************************************* */
/* Test basic node creation *********************************************** */
/* ************************************************************************* */
static int test_create_node_basic(void)
{
    int sum = 0;

    G_message("\t * testing basic node creation\n");

    OctreeNode *node = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    if (node == NULL) {
        G_warning("create_octree_node returned NULL");
        return 1;
    }

    if (node->point_count != 0) {
        G_warning("Expected point_count 0, got %zu", node->point_count);
        sum++;
    }

    if (node->depth != 0) {
        G_warning("Expected depth 0, got %d", node->depth);
        sum++;
    }

    free_octree(node);
    return sum;
}

/* ************************************************************************* */
/* Test that bounding box values are set correctly ************************ */
/* ************************************************************************* */
static int test_create_node_bounds(void)
{
    int sum = 0;

    G_message("\t * testing node bounding box values\n");

    double min_x = -5.0, max_x = 15.0;
    double min_y = -10.0, max_y = 20.0;
    double min_z = 0.0, max_z = 100.0;

    OctreeNode *node =
        create_octree_node(min_x, max_x, min_y, max_y, min_z, max_z);

    if (node->min_x != min_x) {
        G_warning("Expected min_x %f, got %f", min_x, node->min_x);
        sum++;
    }
    if (node->max_x != max_x) {
        G_warning("Expected max_x %f, got %f", max_x, node->max_x);
        sum++;
    }
    if (node->min_y != min_y) {
        G_warning("Expected min_y %f, got %f", min_y, node->min_y);
        sum++;
    }
    if (node->max_y != max_y) {
        G_warning("Expected max_y %f, got %f", max_y, node->max_y);
        sum++;
    }
    if (node->min_z != min_z) {
        G_warning("Expected min_z %f, got %f", min_z, node->min_z);
        sum++;
    }
    if (node->max_z != max_z) {
        G_warning("Expected max_z %f, got %f", max_z, node->max_z);
        sum++;
    }

    free_octree(node);
    return sum;
}

/* ************************************************************************* */
/* Test that all children are initialized to NULL ************************* */
/* ************************************************************************* */
static int test_create_node_children_null(void)
{
    int sum = 0;

    G_message("\t * testing that children are initialized to NULL\n");

    OctreeNode *node = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    for (int i = 0; i < 8; i++) {
        if (node->children[i] != NULL) {
            G_warning("Expected child[%d] to be NULL", i);
            sum++;
        }
    }

    free_octree(node);
    return sum;
}

/* ************************************************************************* */
/* Test node creation with zero-sized bounds ****************************** */
/* ************************************************************************* */
static int test_create_node_zero_bounds(void)
{
    int sum = 0;

    G_message("\t * testing node creation with zero-sized bounds\n");

    OctreeNode *node = create_octree_node(5.0, 5.0, 5.0, 5.0, 5.0, 5.0);

    if (node == NULL) {
        G_warning("create_octree_node returned NULL for zero-sized bounds");
        return 1;
    }

    if (node->min_x != node->max_x || node->min_y != node->max_y ||
        node->min_z != node->max_z) {
        G_warning("Zero-sized bounds not preserved");
        sum++;
    }

    free_octree(node);
    return sum;
}

/* ************************************************************************* */
/* Test node creation with negative coordinate bounds ********************* */
/* ************************************************************************* */
static int test_create_node_negative_bounds(void)
{
    int sum = 0;

    G_message("\t * testing node creation with negative bounds\n");

    OctreeNode *node =
        create_octree_node(-100.0, -50.0, -200.0, -100.0, -50.0, -10.0);

    if (node == NULL) {
        G_warning("create_octree_node returned NULL for negative bounds");
        return 1;
    }

    if (node->min_x != -100.0 || node->max_x != -50.0) {
        G_warning("Negative x bounds not preserved");
        sum++;
    }
    if (node->min_y != -200.0 || node->max_y != -100.0) {
        G_warning("Negative y bounds not preserved");
        sum++;
    }
    if (node->min_z != -50.0 || node->max_z != -10.0) {
        G_warning("Negative z bounds not preserved");
        sum++;
    }

    free_octree(node);
    return sum;
}

/* ************************************************************************* */
/* Test that points array is lazily allocated (NULL until first insert) *** */
/* ************************************************************************* */
static int test_create_node_lazy_allocation(void)
{
    int sum = 0;

    G_message("\t * testing lazy allocation of points array\n");

    OctreeNode *node = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    if (node->points != NULL) {
        G_warning("Expected NULL points array before first insert");
        sum++;
    }

    if (node->point_capacity != 0) {
        G_warning("Expected point_capacity 0, got %zu", node->point_capacity);
        sum++;
    }

    /* After one insert, points should be allocated */
    Point3D p = {5.0, 5.0, 5.0};
    insert_point(node, p);

    if (node->points == NULL) {
        G_warning("Expected non-NULL points array after insert");
        sum++;
    }

    if (node->point_capacity < 1) {
        G_warning("Expected point_capacity >= 1, got %zu",
                  node->point_capacity);
        sum++;
    }

    free_octree(node);
    return sum;
}
