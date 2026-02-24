/****************************************************************************
 *
 * MODULE:       test.octree.lib
 *
 * AUTHOR(S):    Corey White
 *
 * PURPOSE:      Unit tests for octree point insertion
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
static int test_insert_single_point(void);
static int test_insert_fill_node(void);
static int test_insert_triggers_subdivision(void);
static int test_insert_points_into_all_octants(void);
static int test_insert_duplicate_points(void);
static int test_insert_boundary_point(void);

/* ************************************************************************* */
/* Perform the insert_point unit tests ************************************ */
/* ************************************************************************* */
int unit_test_insert_point(void)
{
    int sum = 0;

    G_message(_("\n++ Running insert_point unit tests ++"));

    sum += test_insert_single_point();
    sum += test_insert_fill_node();
    sum += test_insert_triggers_subdivision();
    sum += test_insert_points_into_all_octants();
    sum += test_insert_duplicate_points();
    sum += test_insert_boundary_point();

    if (sum > 0)
        G_warning(_("\n-- insert_point unit tests failure --"));
    else
        G_message(_("\n-- insert_point unit tests finished successfully --"));

    return sum;
}

/* ************************************************************************* */
/* Test inserting a single point ****************************************** */
/* ************************************************************************* */
static int test_insert_single_point(void)
{
    int sum = 0;

    G_message("\t * testing single point insertion\n");

    OctreeNode *root = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    Point3D p = {5.0, 5.0, 5.0};

    insert_point(root, p);

    if (root->point_count != 1) {
        G_warning("Expected point_count 1, got %zu", root->point_count);
        sum++;
    }

    if (root->points[0].x != 5.0 || root->points[0].y != 5.0 ||
        root->points[0].z != 5.0) {
        G_warning("Inserted point coordinates do not match");
        sum++;
    }

    /* Verify no children were created */
    for (int i = 0; i < 8; i++) {
        if (root->children[i] != NULL) {
            G_warning("Expected no children after single insert");
            sum++;
            break;
        }
    }

    free_octree(root);
    return sum;
}

/* ************************************************************************* */
/* Test filling a node to capacity without subdivision ******************** */
/* ************************************************************************* */
static int test_insert_fill_node(void)
{
    int sum = 0;

    G_message("\t * testing filling node to capacity\n");

    OctreeNode *root = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert exactly MAX_POINTS_PER_NODE points */
    for (int i = 0; i < MAX_POINTS_PER_NODE; i++) {
        Point3D p = {(double)i, (double)i, (double)i};
        insert_point(root, p);
    }

    if (root->point_count != MAX_POINTS_PER_NODE) {
        G_warning("Expected point_count %d, got %zu", MAX_POINTS_PER_NODE,
                  root->point_count);
        sum++;
    }

    /* Verify no children were created (node is at capacity, not over) */
    for (int i = 0; i < 8; i++) {
        if (root->children[i] != NULL) {
            G_warning("Expected no children when node is at capacity");
            sum++;
            break;
        }
    }

    free_octree(root);
    return sum;
}

/* ************************************************************************* */
/* Test that exceeding capacity triggers subdivision ********************** */
/* ************************************************************************* */
static int test_insert_triggers_subdivision(void)
{
    int sum = 0;

    G_message("\t * testing subdivision on capacity overflow\n");

    OctreeNode *root = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert MAX_POINTS_PER_NODE + 1 points to trigger subdivision */
    for (int i = 0; i <= MAX_POINTS_PER_NODE; i++) {
        Point3D p = {(double)i * 0.5, (double)i * 0.5, (double)i * 0.5};
        insert_point(root, p);
    }

    /* After subdivision, root should have children */
    int has_children = 0;

    for (int i = 0; i < 8; i++) {
        if (root->children[i] != NULL) {
            has_children = 1;
            break;
        }
    }

    if (!has_children) {
        G_warning("Expected children after exceeding capacity");
        sum++;
    }

    /* After subdivision, root's points should be cleared */
    if (root->point_count != 0) {
        G_warning("Expected root point_count 0 after subdivision, got %zu",
                  root->point_count);
        sum++;
    }

    if (root->points != NULL) {
        G_warning("Expected root points to be NULL after subdivision");
        sum++;
    }

    free_octree(root);
    return sum;
}

/* ************************************************************************* */
/* Test inserting points into all 8 octants ******************************* */
/* ************************************************************************* */
static int test_insert_points_into_all_octants(void)
{
    int sum = 0;

    G_message("\t * testing insertion into all 8 octants\n");

    OctreeNode *root = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Fill to capacity first, then add points in distinct octants to
       force subdivision */
    for (int i = 0; i < MAX_POINTS_PER_NODE; i++) {
        Point3D p = {5.0, 5.0, 5.0};
        insert_point(root, p);
    }

    /* These 8 points should land in each of the 8 octants after subdivision */
    Point3D octant_points[8] = {
        {1.0, 1.0, 1.0}, /* octant 0: low x, low y, low z */
        {9.0, 1.0, 1.0}, /* octant 1: high x, low y, low z */
        {1.0, 9.0, 1.0}, /* octant 2: low x, high y, low z */
        {9.0, 9.0, 1.0}, /* octant 3: high x, high y, low z */
        {1.0, 1.0, 9.0}, /* octant 4: low x, low y, high z */
        {9.0, 1.0, 9.0}, /* octant 5: high x, low y, high z */
        {1.0, 9.0, 9.0}, /* octant 6: low x, high y, high z */
        {9.0, 9.0, 9.0}, /* octant 7: high x, high y, high z */
    };

    for (int i = 0; i < 8; i++) {
        insert_point(root, octant_points[i]);
    }

    /* Verify all 8 children exist */
    for (int i = 0; i < 8; i++) {
        if (root->children[i] == NULL) {
            G_warning("Expected child[%d] to exist after octant insertion", i);
            sum++;
        }
    }

    free_octree(root);
    return sum;
}

/* ************************************************************************* */
/* Test inserting duplicate points at the same location ******************* */
/* ************************************************************************* */
static int test_insert_duplicate_points(void)
{
    int sum = 0;

    G_message("\t * testing insertion of duplicate points\n");

    OctreeNode *root = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert a few identical points (within capacity) */
    for (int i = 0; i < 5; i++) {
        Point3D p = {3.0, 3.0, 3.0};
        insert_point(root, p);
    }

    if (root->point_count != 5) {
        G_warning("Expected point_count 5 for duplicates, got %zu",
                  root->point_count);
        sum++;
    }

    /* Verify all stored points have the same coordinates */
    for (int i = 0; i < 5; i++) {
        if (root->points[i].x != 3.0 || root->points[i].y != 3.0 ||
            root->points[i].z != 3.0) {
            G_warning("Duplicate point %d has wrong coordinates", i);
            sum++;
        }
    }

    free_octree(root);
    return sum;
}

/* ************************************************************************* */
/* Test inserting a point on the boundary of the bounding box ************* */
/* ************************************************************************* */
static int test_insert_boundary_point(void)
{
    int sum = 0;

    G_message("\t * testing boundary point insertion\n");

    OctreeNode *root = create_octree_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert point at the exact midpoint (boundary between octants) */
    Point3D mid = {5.0, 5.0, 5.0};
    insert_point(root, mid);

    if (root->point_count != 1) {
        G_warning("Expected point_count 1 for boundary point, got %zu",
                  root->point_count);
        sum++;
    }

    /* Insert point at exact min corner */
    Point3D corner = {0.0, 0.0, 0.0};
    insert_point(root, corner);

    if (root->point_count != 2) {
        G_warning("Expected point_count 2 after corner insert, got %zu",
                  root->point_count);
        sum++;
    }

    /* Insert point at exact max corner */
    Point3D max_corner = {10.0, 10.0, 10.0};
    insert_point(root, max_corner);

    if (root->point_count != 3) {
        G_warning("Expected point_count 3 after max corner insert, got %zu",
                  root->point_count);
        sum++;
    }

    free_octree(root);
    return sum;
}
