/****************************************************************************
 *
 * MODULE:       test.octree.lib
 *
 * AUTHOR(S):    Corey White
 *
 * PURPOSE:      Unit tests for octree memory deallocation
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
static int test_free_null_node(void);
static int test_free_empty_tree(void);
static int test_free_tree_with_points(void);
static int test_free_subdivided_tree(void);

/* ************************************************************************* */
/* Perform the free_octree unit tests ************************************* */
/* ************************************************************************* */
int unit_test_free_octree(void)
{
    int sum = 0;

    G_message(_("\n++ Running free_octree unit tests ++"));

    sum += test_free_null_node();
    sum += test_free_empty_tree();
    sum += test_free_tree_with_points();
    sum += test_free_subdivided_tree();

    if (sum > 0)
        G_warning(_("\n-- free_octree unit tests failure --"));
    else
        G_message(_("\n-- free_octree unit tests finished successfully --"));

    return sum;
}

/* ************************************************************************* */
/* Test that free_octree handles NULL gracefully ************************** */
/* ************************************************************************* */
static int test_free_null_node(void)
{
    G_message("\t * testing free_octree with NULL\n");

    /* Should not crash */
    free_octree(NULL);

    return 0;
}

/* ************************************************************************* */
/* Test freeing an empty tree (no points, no children) ******************** */
/* ************************************************************************* */
static int test_free_empty_tree(void)
{
    G_message("\t * testing free of empty tree\n");

    OctreeNode *root = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Should not crash; just exercises the deallocation path */
    free_octree(root);

    return 0;
}

/* ************************************************************************* */
/* Test freeing a tree that contains points (leaf node) ******************* */
/* ************************************************************************* */
static int test_free_tree_with_points(void)
{
    G_message("\t * testing free of tree with points\n");

    OctreeNode *root = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    for (int i = 0; i < 5; i++) {
        Point3D p = {(double)i, (double)i, (double)i};
        insert_point(root, p);
    }

    /* Should not crash; frees the points array and the node */
    free_octree(root);

    return 0;
}

/* ************************************************************************* */
/* Test freeing a subdivided tree (multiple levels) *********************** */
/* ************************************************************************* */
static int test_free_subdivided_tree(void)
{
    G_message("\t * testing free of subdivided tree\n");

    OctreeNode *root = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert enough diverse points to trigger subdivision and deeper levels */
    for (int i = 0; i <= MAX_POINTS_PER_NODE * 3; i++) {
        Point3D p = {(double)(i % 10), (double)((i * 3) % 10),
                     (double)((i * 7) % 10)};
        insert_point(root, p);
    }

    /* Should not crash; recursively frees all children and their points */
    free_octree(root);

    return 0;
}
