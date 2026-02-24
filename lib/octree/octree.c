#include "octree.h"
#include <stdio.h>

// Create a new octree node
OctreeNode *create_octree_node(double min_x, double max_x, double min_y,
                               double max_y, double min_z, double max_z)
{
    OctreeNode *node = (OctreeNode *)malloc(sizeof(OctreeNode));
    for (int i = 0; i < 8; i++) {
        node->children[i] = NULL;
    }
    node->points = (Point3D *)malloc(sizeof(Point3D) * MAX_POINTS_PER_NODE);
    node->point_count = 0;
    node->min_x = min_x;
    node->max_x = max_x;
    node->min_y = min_y;
    node->max_y = max_y;
    node->min_z = min_z;
    node->max_z = max_z;
    return node;
}

// Determine the child index for a given point
static int get_child_index(OctreeNode *node, Point3D point)
{
    int index = 0;
    if (point.x > (node->min_x + node->max_x) / 2)
        index |= 1;
    if (point.y > (node->min_y + node->max_y) / 2)
        index |= 2;
    if (point.z > (node->min_z + node->max_z) / 2)
        index |= 4;
    return index;
}

// Subdivide a node into 8 children
static void subdivide_node(OctreeNode *node)
{
    double mid_x = (node->min_x + node->max_x) / 2;
    double mid_y = (node->min_y + node->max_y) / 2;
    double mid_z = (node->min_z + node->max_z) / 2;

    for (int i = 0; i < 8; i++) {
        double new_min_x = (i & 1) ? mid_x : node->min_x;
        double new_max_x = (i & 1) ? node->max_x : mid_x;
        double new_min_y = (i & 2) ? mid_y : node->min_y;
        double new_max_y = (i & 2) ? node->max_y : mid_y;
        double new_min_z = (i & 4) ? mid_z : node->min_z;
        double new_max_z = (i & 4) ? node->max_z : mid_z;

        node->children[i] = create_octree_node(new_min_x, new_max_x, new_min_y,
                                               new_max_y, new_min_z, new_max_z);
    }
}

// Insert a point into the octree
void insert_point(OctreeNode *node, Point3D point)
{
    if (node->point_count < MAX_POINTS_PER_NODE) {
        node->points[node->point_count++] = point;
    }
    else {
        // If we exceed the point capacity, we need to subdivide and
        // redistribute points
        if (node->children[0] == NULL) {
            subdivide_node(node);
        }

        // Re-insert existing points into the appropriate children
        for (size_t i = 0; i < node->point_count; i++) {
            int child_index = get_child_index(node, node->points[i]);
            insert_point(node->children[child_index], node->points[i]);
        }

        // Now insert the new point
        int new_child_index = get_child_index(node, point);
        insert_point(node->children[new_child_index], point);

        // Clear the current node's points (they've moved to children)
        free(node->points);
        node->points = NULL;
        node->point_count = 0;
    }
}

// Recursively free the octree
void free_octree(OctreeNode *node)
{
    if (node == NULL)
        return;

    for (int i = 0; i < 8; i++) {
        if (node->children[i] != NULL) {
            free_octree(node->children[i]);
        }
    }

    free(node->points);
    free(node);
}
