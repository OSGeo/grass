/****************************************************************************
 *
 * MODULE:       test.octree.lib
 *
 * AUTHOR(S):    Corey White
 *
 * PURPOSE:      Unit tests for octree range query, depth-limited traversal,
 *               and subtree representative accessor
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
static int test_query_empty_tree(void);
static int test_query_null_visitor_counts(void);
static int test_query_disjoint_box(void);
static int test_query_partial_overlap(void);
static int test_query_early_stop(void);
static int test_query_collects_points(void);
static int test_visit_to_depth_root_only(void);
static int test_visit_to_depth_full(void);
static int test_visit_to_depth_cap(void);
static int test_representative_null(void);
static int test_representative_empty(void);
static int test_representative_single(void);
static int test_representative_centroid(void);
static int test_subtree_count_null_and_empty(void);
static int test_subtree_count_matches_inserts(void);
static int test_subtree_count_after_subdivision(void);
static int test_subtree_count_ignores_rejected(void);

/* Visitor state used by several range-query tests. */
typedef struct {
    size_t calls;
    size_t stop_after;
    OctreePoint3D last;
} VisitorState;

static int count_visitor(const OctreePoint3D *p, void *user_data)
{
    VisitorState *s = (VisitorState *)user_data;

    (void)p;
    s->calls++;
    return 0;
}

static int stopping_visitor(const OctreePoint3D *p, void *user_data)
{
    VisitorState *s = (VisitorState *)user_data;

    s->last = *p;
    s->calls++;
    return s->calls >= s->stop_after ? 1 : 0;
}

static int terminal_counter(const OctreeNode *node, void *user_data)
{
    size_t *n = (size_t *)user_data;

    (void)node;
    (*n)++;
    return 0;
}

/* ************************************************************************* */
/* Driver *********************************************************** */
/* ************************************************************************* */
int unit_test_query(void)
{
    int sum = 0;

    G_message(_("\n++ Running octree query unit tests ++"));

    sum += test_query_empty_tree();
    sum += test_query_null_visitor_counts();
    sum += test_query_disjoint_box();
    sum += test_query_partial_overlap();
    sum += test_query_early_stop();
    sum += test_query_collects_points();
    sum += test_visit_to_depth_root_only();
    sum += test_visit_to_depth_full();
    sum += test_visit_to_depth_cap();
    sum += test_representative_null();
    sum += test_representative_empty();
    sum += test_representative_single();
    sum += test_representative_centroid();
    sum += test_subtree_count_null_and_empty();
    sum += test_subtree_count_matches_inserts();
    sum += test_subtree_count_after_subdivision();
    sum += test_subtree_count_ignores_rejected();

    if (sum > 0)
        G_warning(_("\n-- octree query unit tests failure --"));
    else
        G_message(_("\n-- octree query unit tests finished successfully --"));

    return sum;
}

/* ************************************************************************* */
static int test_query_empty_tree(void)
{
    int sum = 0;

    G_message("\t * testing query on empty tree\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    VisitorState s = {0, 0, {0, 0, 0}};
    size_t n = octree_query_box(root, 0, 10, 0, 10, 0, 10, count_visitor, &s);

    if (n != 0 || s.calls != 0) {
        G_warning("Expected 0 matches on empty tree, got n=%zu calls=%zu", n,
                  s.calls);
        sum++;
    }

    if (octree_query_box(NULL, 0, 10, 0, 10, 0, 10, count_visitor, &s) != 0) {
        G_warning("Expected 0 matches for NULL tree");
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_query_null_visitor_counts(void)
{
    int sum = 0;

    G_message("\t * testing query counts with NULL visitor\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    for (int i = 0; i < 7; i++) {
        OctreePoint3D p = {(double)i, (double)i, (double)i};
        octree_insert_point(root, p);
    }

    size_t n = octree_query_box(root, 0, 10, 0, 10, 0, 10, NULL, NULL);

    if (n != 7) {
        G_warning("Expected 7 matches with NULL visitor, got %zu", n);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_query_disjoint_box(void)
{
    int sum = 0;

    G_message("\t * testing query with disjoint box prunes entire tree\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    for (int i = 0; i < 20; i++) {
        OctreePoint3D p = {(double)(i % 10), (double)((i * 3) % 10),
                           (double)((i * 7) % 10)};
        octree_insert_point(root, p);
    }

    VisitorState s = {0, 0, {0, 0, 0}};
    size_t n =
        octree_query_box(root, 100, 200, 100, 200, 100, 200, count_visitor, &s);

    if (n != 0 || s.calls != 0) {
        G_warning("Expected 0 matches for disjoint box, got n=%zu calls=%zu", n,
                  s.calls);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_query_partial_overlap(void)
{
    int sum = 0;

    G_message("\t * testing query with partial overlap\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    OctreePoint3D inside_pts[] = {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}};
    OctreePoint3D outside_pts[] = {{8, 8, 8}, {9, 9, 9}};

    for (size_t i = 0; i < sizeof(inside_pts) / sizeof(inside_pts[0]); i++)
        octree_insert_point(root, inside_pts[i]);
    for (size_t i = 0; i < sizeof(outside_pts) / sizeof(outside_pts[0]); i++)
        octree_insert_point(root, outside_pts[i]);

    VisitorState s = {0, 0, {0, 0, 0}};
    size_t n = octree_query_box(root, 0, 4, 0, 4, 0, 4, count_visitor, &s);

    if (n != 3 || s.calls != 3) {
        G_warning("Expected 3 matches, got n=%zu calls=%zu", n, s.calls);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_query_early_stop(void)
{
    int sum = 0;

    G_message("\t * testing query early-stop on visitor return\n");

    OctreeNode *root = octree_create_node(0.0, 100.0, 0.0, 100.0, 0.0, 100.0);

    /* Insert enough to force subdivision and guarantee multiple leaves. */
    for (int i = 0; i < OCTREE_MAX_POINTS_PER_NODE * 3; i++) {
        OctreePoint3D p = {(double)i, (double)i, (double)i};
        octree_insert_point(root, p);
    }

    VisitorState s = {0, 3, {0, 0, 0}};
    size_t n =
        octree_query_box(root, 0, 100, 0, 100, 0, 100, stopping_visitor, &s);

    if (s.calls != 3) {
        G_warning("Expected visitor stop after 3 calls, got %zu", s.calls);
        sum++;
    }

    /* octree_query_box returns the count of matched points; early-stop
       leaves the count at the stopping call. */
    if (n != 3) {
        G_warning("Expected returned count 3 after stop, got %zu", n);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_query_collects_points(void)
{
    int sum = 0;

    G_message("\t * testing query returns correct points after subdivision\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Force subdivision by exceeding OCTREE_MAX_POINTS_PER_NODE in one octant
       and spreading the rest across others. */
    for (int i = 0; i <= OCTREE_MAX_POINTS_PER_NODE; i++) {
        OctreePoint3D p = {0.5 + i * 0.01, 0.5, 0.5};
        octree_insert_point(root, p);
    }
    OctreePoint3D far_pts[] = {{9, 9, 9}, {8, 1, 1}, {1, 8, 1}};

    for (size_t i = 0; i < sizeof(far_pts) / sizeof(far_pts[0]); i++)
        octree_insert_point(root, far_pts[i]);

    VisitorState s = {0, 0, {0, 0, 0}};
    size_t n = octree_query_box(root, 0, 1, 0, 1, 0, 1, count_visitor, &s);

    if (n != (size_t)(OCTREE_MAX_POINTS_PER_NODE + 1)) {
        G_warning("Expected %d matches in {0..1}^3, got %zu",
                  OCTREE_MAX_POINTS_PER_NODE + 1, n);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_visit_to_depth_root_only(void)
{
    int sum = 0;

    G_message("\t * testing visit_to_depth with max_depth=0\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    for (int i = 0; i < OCTREE_MAX_POINTS_PER_NODE * 2; i++) {
        OctreePoint3D p = {(double)(i % 10), (double)((i * 3) % 10),
                           (double)((i * 7) % 10)};
        octree_insert_point(root, p);
    }

    size_t visits = 0;

    octree_visit_to_depth(root, 0, terminal_counter, &visits);

    if (visits != 1) {
        G_warning("Expected 1 visit at max_depth=0, got %zu", visits);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_visit_to_depth_full(void)
{
    int sum = 0;

    G_message("\t * testing visit_to_depth covers every leaf\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Unsubdivided leaf: single terminal node. */
    OctreePoint3D p = {1, 1, 1};

    octree_insert_point(root, p);

    size_t visits = 0;

    octree_visit_to_depth(root, 100, terminal_counter, &visits);

    if (visits != 1) {
        G_warning("Expected 1 leaf visit on unsubdivided tree, got %zu",
                  visits);
        sum++;
    }

    /* Force subdivision into two distinct octants. */
    octree_free(root);
    root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    for (int i = 0; i < OCTREE_MAX_POINTS_PER_NODE + 1; i++) {
        OctreePoint3D q = {1.0, 1.0, 1.0};
        octree_insert_point(root, q);
    }
    OctreePoint3D far = {9, 9, 9};

    octree_insert_point(root, far);

    visits = 0;
    octree_visit_to_depth(root, 100, terminal_counter, &visits);

    /* The coincident points collapse into one deep leaf; far lands in its
       own child. At minimum we expect 2 terminal nodes. */
    if (visits < 2) {
        G_warning("Expected >= 2 terminal visits after subdivision, got %zu",
                  visits);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_visit_to_depth_cap(void)
{
    int sum = 0;

    G_message("\t * testing visit_to_depth honors depth cap\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Many coincident points force deep subdivision (up to max depth). */
    for (int i = 0; i < OCTREE_MAX_POINTS_PER_NODE * 4; i++) {
        OctreePoint3D p = {5.0, 5.0, 5.0};
        octree_insert_point(root, p);
    }

    size_t visits_shallow = 0;
    size_t visits_deep = 0;

    octree_visit_to_depth(root, 1, terminal_counter, &visits_shallow);
    octree_visit_to_depth(root, 50, terminal_counter, &visits_deep);

    /* Shallow cap should visit no more than deep traversal. */
    if (visits_shallow > visits_deep) {
        G_warning("Shallow visits (%zu) exceed deep visits (%zu)",
                  visits_shallow, visits_deep);
        sum++;
    }
    if (visits_shallow == 0) {
        G_warning("Expected at least one terminal at depth cap 1");
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_representative_null(void)
{
    int sum = 0;

    G_message("\t * testing representative on NULL node\n");

    OctreePoint3D c;
    size_t n = 42;

    if (octree_subtree_representative(NULL, &c, &n) != -1) {
        G_warning("Expected -1 for NULL node");
        sum++;
    }
    if (n != 0) {
        G_warning("Expected out_count=0 for NULL node, got %zu", n);
        sum++;
    }

    return sum;
}

/* ************************************************************************* */
static int test_representative_empty(void)
{
    int sum = 0;

    G_message("\t * testing representative on empty subtree\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    OctreePoint3D c = {1, 2, 3};
    size_t n = 99;

    if (octree_subtree_representative(root, &c, &n) != -1) {
        G_warning("Expected -1 for empty subtree");
        sum++;
    }
    if (n != 0) {
        G_warning("Expected count 0 for empty subtree, got %zu", n);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_representative_single(void)
{
    int sum = 0;

    G_message("\t * testing representative of a single point\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    OctreePoint3D p = {2.5, 3.5, 4.5};

    octree_insert_point(root, p);

    OctreePoint3D c;
    size_t n = 0;

    if (octree_subtree_representative(root, &c, &n) != 0) {
        G_warning("Expected success for single-point subtree");
        sum++;
    }
    if (n != 1 || c.x != p.x || c.y != p.y || c.z != p.z) {
        G_warning("Unexpected representative: n=%zu c=(%f,%f,%f)", n, c.x, c.y,
                  c.z);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_representative_centroid(void)
{
    int sum = 0;

    G_message("\t * testing representative centroid across subdivision\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    /* Spread points so subdivision is triggered; average is (4.5, 4.5, 4.5) */
    OctreePoint3D pts[] = {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}, {4, 4, 4},
                           {5, 5, 5}, {6, 6, 6}, {7, 7, 7}, {8, 8, 8},
                           {9, 9, 9}, {1, 9, 1}, {9, 1, 9}, {1, 1, 9}};
    size_t total = sizeof(pts) / sizeof(pts[0]);
    double sx = 0, sy = 0, sz = 0;

    for (size_t i = 0; i < total; i++) {
        octree_insert_point(root, pts[i]);
        sx += pts[i].x;
        sy += pts[i].y;
        sz += pts[i].z;
    }

    OctreePoint3D c;
    size_t n = 0;

    octree_subtree_representative(root, &c, &n);

    if (n != total) {
        G_warning("Expected count %zu, got %zu", total, n);
        sum++;
    }

    double tol = 1e-9;

    if (fabs(c.x - sx / total) > tol || fabs(c.y - sy / total) > tol ||
        fabs(c.z - sz / total) > tol) {
        G_warning("Centroid mismatch: got (%f,%f,%f) expected (%f,%f,%f)", c.x,
                  c.y, c.z, sx / total, sy / total, sz / total);
        sum++;
    }

    /* Passing NULL for both out parameters should still succeed (return
       only reports 0/-1); just verify no crash. */
    if (octree_subtree_representative(root, NULL, NULL) != 0) {
        G_warning("Expected success when out parameters are NULL");
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_subtree_count_null_and_empty(void)
{
    int sum = 0;

    G_message("\t * testing subtree_count on NULL and empty trees\n");

    if (octree_subtree_count(NULL) != 0) {
        G_warning("Expected 0 count for NULL node");
        sum++;
    }

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    if (octree_subtree_count(root) != 0) {
        G_warning("Expected 0 count for empty tree");
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_subtree_count_matches_inserts(void)
{
    int sum = 0;

    G_message("\t * testing subtree_count matches insert count\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    size_t n = 7;

    for (size_t i = 0; i < n; i++) {
        OctreePoint3D p = {(double)i, (double)i, (double)i};
        octree_insert_point(root, p);
    }

    if (octree_subtree_count(root) != n) {
        G_warning("Expected root subtree_count %zu, got %zu", n,
                  octree_subtree_count(root));
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_subtree_count_after_subdivision(void)
{
    int sum = 0;

    G_message("\t * testing subtree_count after subdivision sums children\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    size_t n = OCTREE_MAX_POINTS_PER_NODE * 4 + 3;

    for (size_t i = 0; i < n; i++) {
        OctreePoint3D p = {(double)(i % 10), (double)((i * 3) % 10),
                           (double)((i * 7) % 10)};
        octree_insert_point(root, p);
    }

    if (octree_subtree_count(root) != n) {
        G_warning("Root subtree_count %zu != inserted %zu",
                  octree_subtree_count(root), n);
        sum++;
    }

    /* Root must be internal after this many inserts; verify each child's
       count sums to the root's. */
    size_t child_total = 0;

    for (int i = 0; i < 8; i++) {
        if (root->children[i] != NULL)
            child_total += octree_subtree_count(root->children[i]);
    }

    if (child_total != n) {
        G_warning("Sum of child counts %zu != root count %zu", child_total, n);
        sum++;
    }

    octree_free(root);
    return sum;
}

/* ************************************************************************* */
static int test_subtree_count_ignores_rejected(void)
{
    int sum = 0;

    G_message("\t * testing subtree_count ignores rejected inserts\n");

    OctreeNode *root = octree_create_node(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);

    OctreePoint3D good = {1, 1, 1};
    OctreePoint3D out_of_bounds = {100, 100, 100};
    OctreePoint3D nan_point = {NAN, 1, 1};

    octree_insert_point(root, good);
    octree_insert_point(root, out_of_bounds);
    octree_insert_point(root, nan_point);

    if (octree_subtree_count(root) != 1) {
        G_warning("Expected count 1 after 2 rejected inserts, got %zu",
                  octree_subtree_count(root));
        sum++;
    }

    octree_free(root);
    return sum;
}
