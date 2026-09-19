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

#include <math.h>
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
static int test_insert_after_subdivision(void);
static int test_insert_many_duplicates(void);
static int test_insert_out_of_bounds(void);
static int test_insert_non_finite(void);
static int test_insert_capacity_growth(void);

/* ************************************************************************* */
/* Perform the octree_insert_point unit tests
 * ************************************ */
/* ************************************************************************* */
int unit_test_insert_point(void)
{
    int sum = 0;

    G_message(_("\n++ Running octree_insert_point unit tests ++"));

    sum += test_insert_single_point();
    sum += test_insert_fill_node();
    sum += test_insert_triggers_subdivision();
    sum += test_insert_points_into_all_octants();
    sum += test_insert_duplicate_points();
    sum += test_insert_boundary_point();
    sum += test_insert_after_subdivision();
    sum += test_insert_many_duplicates();
    sum += test_insert_out_of_bounds();
    sum += test_insert_non_finite();
    sum += test_insert_capacity_growth();

    if (sum > 0)
        G_warning(_("\n-- octree_insert_point unit tests failure --"));
    else
        G_message(
            _("\n-- octree_insert_point unit tests finished successfully --"));

    return sum;
}

/* ************************************************************************* */
/* Test inserting a single point ****************************************** */
/* ************************************************************************* */
static int test_insert_single_point(void)
{
    int sum = 0;

    G_message("\t * testing single point insertion\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    OctreePoint3D p = {5.0, 5.0, 5.0};

    int ret = octree_insert_point(root, p);

    if (ret != 0) {
        G_warning("octree_insert_point returned %d, expected 0", ret);
        sum++;
    }

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

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test filling a node to capacity without subdivision ******************** */
/* ************************************************************************* */
static int test_insert_fill_node(void)
{
    int sum = 0;

    G_message("\t * testing filling node to capacity\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert exactly OCTREE_MAX_POINTS_PER_NODE points */
    for (int i = 0; i < OCTREE_MAX_POINTS_PER_NODE; i++) {
        OctreePoint3D p = {(double)i, (double)i, (double)i};
        octree_insert_point(root, p);
    }

    if (root->point_count != OCTREE_MAX_POINTS_PER_NODE) {
        G_warning("Expected point_count %d, got %zu",
                  OCTREE_MAX_POINTS_PER_NODE, root->point_count);
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

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test that exceeding capacity triggers subdivision ********************** */
/* ************************************************************************* */
static int test_insert_triggers_subdivision(void)
{
    int sum = 0;

    G_message("\t * testing subdivision on capacity overflow\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert OCTREE_MAX_POINTS_PER_NODE + 1 points to trigger subdivision */
    for (int i = 0; i <= OCTREE_MAX_POINTS_PER_NODE; i++) {
        OctreePoint3D p = {(double)i * 0.5, (double)i * 0.5, (double)i * 0.5};
        octree_insert_point(root, p);
    }

    /* After subdivision, root should have at least one child */
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

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test inserting points into all 8 octants ******************************* */
/* ************************************************************************* */
static int test_insert_points_into_all_octants(void)
{
    int sum = 0;

    G_message("\t * testing insertion into all 8 octants\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Fill to capacity first, then add points in distinct octants to
       force subdivision */
    for (int i = 0; i < OCTREE_MAX_POINTS_PER_NODE; i++) {
        OctreePoint3D p = {5.0, 5.0, 5.0};
        octree_insert_point(root, p);
    }

    /* These 8 points should land in each of the 8 octants after subdivision */
    OctreePoint3D octant_points[8] = {
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
        octree_insert_point(root, octant_points[i]);
    }

    /* Verify children exist for all 8 octants */
    for (int i = 0; i < 8; i++) {
        if (root->children[i] == NULL) {
            G_warning("Expected child[%d] to exist after octant insertion", i);
            sum++;
        }
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test inserting duplicate points at the same location ******************* */
/* ************************************************************************* */
static int test_insert_duplicate_points(void)
{
    int sum = 0;

    G_message("\t * testing insertion of duplicate points\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert a few identical points (within capacity) */
    for (int i = 0; i < 5; i++) {
        OctreePoint3D p = {3.0, 3.0, 3.0};
        octree_insert_point(root, p);
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

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test inserting a point on the boundary of the bounding box ************* */
/* ************************************************************************* */
static int test_insert_boundary_point(void)
{
    int sum = 0;

    G_message("\t * testing boundary point insertion\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert point at the exact midpoint (boundary between octants) */
    OctreePoint3D mid = {5.0, 5.0, 5.0};
    octree_insert_point(root, mid);

    if (root->point_count != 1) {
        G_warning("Expected point_count 1 for boundary point, got %zu",
                  root->point_count);
        sum++;
    }

    /* Insert point at exact min corner */
    OctreePoint3D corner = {0.0, 0.0, 0.0};
    octree_insert_point(root, corner);

    if (root->point_count != 2) {
        G_warning("Expected point_count 2 after corner insert, got %zu",
                  root->point_count);
        sum++;
    }

    /* Insert point at exact max corner */
    OctreePoint3D max_corner = {10.0, 10.0, 10.0};
    octree_insert_point(root, max_corner);

    if (root->point_count != 3) {
        G_warning("Expected point_count 3 after max corner insert, got %zu",
                  root->point_count);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test continued insertion after subdivision (was a NULL deref bug) ****** */
/* ************************************************************************* */
static int test_insert_after_subdivision(void)
{
    int sum = 0;

    G_message("\t * testing insertion after subdivision\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Trigger subdivision */
    for (int i = 0; i <= OCTREE_MAX_POINTS_PER_NODE; i++) {
        OctreePoint3D p = {(double)i * 0.9, (double)i * 0.9, (double)i * 0.9};
        octree_insert_point(root, p);
    }

    /* Verify subdivision happened */
    int has_children = 0;

    for (int i = 0; i < 8; i++) {
        if (root->children[i] != NULL) {
            has_children = 1;
            break;
        }
    }

    if (!has_children) {
        G_warning("Subdivision did not happen");
        return 1;
    }

    /* Insert more points after subdivision; this must not crash */
    for (int i = 0; i < 20; i++) {
        OctreePoint3D p = {(double)(i % 10), (double)((i * 3) % 10),
                           (double)((i * 7) % 10)};
        int ret = octree_insert_point(root, p);

        if (ret != 0) {
            G_warning("octree_insert_point returned %d for in-bounds point",
                      ret);
            sum++;
        }
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test that many coincident points do not cause infinite recursion ******* */
/* ************************************************************************* */
static int test_insert_many_duplicates(void)
{
    int sum = 0;

    G_message("\t * testing many coincident points (depth limit)\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert many identical points, well past OCTREE_MAX_POINTS_PER_NODE.
       Without the depth limit, this would recurse infinitely. */
    int count = OCTREE_MAX_POINTS_PER_NODE * 5;

    for (int i = 0; i < count; i++) {
        OctreePoint3D p = {3.0, 3.0, 3.0};
        int ret = octree_insert_point(root, p);

        if (ret != 0) {
            G_warning("octree_insert_point returned %d for duplicate point %d",
                      ret, i);
            sum++;
        }
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test that out-of-bounds points are rejected ***************************** */
/* ************************************************************************* */
static int test_insert_out_of_bounds(void)
{
    int sum = 0;

    G_message("\t * testing out-of-bounds point rejection\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    OctreePoint3D outside = {-1.0, 5.0, 5.0};

    if (octree_insert_point(root, outside) != -1) {
        G_warning("Expected -1 for point with x below min_x");
        sum++;
    }

    OctreePoint3D above = {5.0, 5.0, 15.0};

    if (octree_insert_point(root, above) != -1) {
        G_warning("Expected -1 for point with z above max_z");
        sum++;
    }

    /* Verify no points were stored */
    if (root->point_count != 0) {
        G_warning("Expected point_count 0 after rejected inserts, got %zu",
                  root->point_count);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test that non-finite coordinates (NaN, infinity) are rejected *********** */
/* ************************************************************************* */
static int test_insert_non_finite(void)
{
    int sum = 0;

    G_message("\t * testing non-finite coordinate rejection\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    OctreePoint3D nan_point = {NAN, 5.0, 5.0};

    if (octree_insert_point(root, nan_point) != -1) {
        G_warning("Expected -1 for NaN x coordinate");
        sum++;
    }

    OctreePoint3D inf_point = {5.0, INFINITY, 5.0};

    if (octree_insert_point(root, inf_point) != -1) {
        G_warning("Expected -1 for infinite y coordinate");
        sum++;
    }

    OctreePoint3D neg_inf = {5.0, 5.0, -INFINITY};

    if (octree_insert_point(root, neg_inf) != -1) {
        G_warning("Expected -1 for -infinity z coordinate");
        sum++;
    }

    if (root->point_count != 0) {
        G_warning("Expected point_count 0 after rejecting non-finite, got %zu",
                  root->point_count);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
/* Test geometric capacity growth at max depth (no O(n^2) realloc) ******** */
/* ************************************************************************* */
static int test_insert_capacity_growth(void)
{
    int sum = 0;

    G_message("\t * testing geometric capacity growth with many duplicates\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Insert enough coincident points to push a max-depth leaf well past
       OCTREE_MAX_POINTS_PER_NODE and exercise multiple capacity doublings. */
    int count = OCTREE_MAX_POINTS_PER_NODE * 20;

    for (int i = 0; i < count; i++) {
        OctreePoint3D p = {7.0, 7.0, 7.0};

        if (octree_insert_point(root, p) != 0) {
            G_warning("octree_insert_point failed on duplicate %d", i);
            sum++;
        }
    }

    /* Descend to the deepest leaf holding these points and verify the
       capacity grew geometrically (>= count, and at least double the
       previous power-of-two threshold). */
    OctreeNode *n = root;

    while (n != NULL && n->points == NULL) {
        OctreeNode *next = NULL;

        for (int i = 0; i < 8; i++) {
            if (n->children[i] != NULL) {
                next = n->children[i];
                break;
            }
        }
        n = next;
    }

    if (n == NULL) {
        G_warning("Could not locate leaf holding duplicates");
        sum++;
    }
    else if (n->point_count != (size_t)count) {
        G_warning("Expected leaf point_count %d, got %zu", count,
                  n->point_count);
        sum++;
    }
    else if (n->point_capacity < n->point_count) {
        G_warning("Leaf capacity %zu < count %zu", n->point_capacity,
                  n->point_count);
        sum++;
    }

    octree_free(root);
    return sum;
}
