#include <grass/gis.h>

#include "octree.h"

/* Check whether a node is an internal node (has any children) */
static int has_children(const OctreeNode *node)
{
    for (int i = 0; i < 8; i++) {
        if (node->children[i] != NULL)
            return 1;
    }
    return 0;
}

/* Create a new octree node with the given bounding box.
   Caller must ensure min <= max for each axis. */
OctreeNode *create_octree_node(double min_x, double max_x, double min_y,
                               double max_y, double min_z, double max_z)
{
    if (min_x > max_x || min_y > max_y || min_z > max_z)
        G_fatal_error("create_octree_node: invalid bounds "
                      "(min_x=%f max_x=%f min_y=%f max_y=%f "
                      "min_z=%f max_z=%f)",
                      min_x, max_x, min_y, max_y, min_z, max_z);

    OctreeNode *node = (OctreeNode *)G_malloc(sizeof(OctreeNode));

    for (int i = 0; i < 8; i++)
        node->children[i] = NULL;

    node->points = NULL;
    node->point_count = 0;
    node->point_capacity = 0;
    node->min_x = min_x;
    node->max_x = max_x;
    node->min_y = min_y;
    node->max_y = max_y;
    node->min_z = min_z;
    node->max_z = max_z;
    node->depth = 0;

    return node;
}

/* Determine the child octant index for a given point */
static int get_child_index(const OctreeNode *node, Point3D point)
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

/* Create a child node for the given octant index */
static OctreeNode *create_child_node(OctreeNode *parent, int index)
{
    double mid_x = (parent->min_x + parent->max_x) / 2;
    double mid_y = (parent->min_y + parent->max_y) / 2;
    double mid_z = (parent->min_z + parent->max_z) / 2;

    double new_min_x = (index & 1) ? mid_x : parent->min_x;
    double new_max_x = (index & 1) ? parent->max_x : mid_x;
    double new_min_y = (index & 2) ? mid_y : parent->min_y;
    double new_max_y = (index & 2) ? parent->max_y : mid_y;
    double new_min_z = (index & 4) ? mid_z : parent->min_z;
    double new_max_z = (index & 4) ? parent->max_z : mid_z;

    OctreeNode *child = create_octree_node(new_min_x, new_max_x, new_min_y,
                                           new_max_y, new_min_z, new_max_z);
    child->depth = parent->depth + 1;

    return child;
}

/* Ensure the points array has room for at least the requested capacity */
static void ensure_points_capacity(OctreeNode *node, size_t needed)
{
    if (node->point_capacity >= needed)
        return;

    size_t new_cap =
        needed > MAX_POINTS_PER_NODE ? needed : MAX_POINTS_PER_NODE;
    node->points =
        (Point3D *)G_realloc(node->points, sizeof(Point3D) * new_cap);
    node->point_capacity = new_cap;
}

/* Insert a point into the octree.
   Returns 0 on success, -1 if the point is outside the node bounds. */
int insert_point(OctreeNode *node, Point3D point)
{
    /* Validate that the point is within bounds */
    if (point.x < node->min_x || point.x > node->max_x ||
        point.y < node->min_y || point.y > node->max_y ||
        point.z < node->min_z || point.z > node->max_z) {
        return -1;
    }

    /* Internal node: delegate to the appropriate child */
    if (has_children(node)) {
        int child_index = get_child_index(node, point);

        if (node->children[child_index] == NULL)
            node->children[child_index] = create_child_node(node, child_index);

        return insert_point(node->children[child_index], point);
    }

    /* At max depth, store here regardless of capacity */
    if (node->depth >= OCTREE_MAX_DEPTH) {
        ensure_points_capacity(node, node->point_count + 1);
        node->points[node->point_count++] = point;
        return 0;
    }

    /* Leaf with room: store the point */
    if (node->point_count < MAX_POINTS_PER_NODE) {
        ensure_points_capacity(node, node->point_count + 1);
        node->points[node->point_count++] = point;
        return 0;
    }

    /* Leaf at capacity: subdivide and redistribute existing points */
    Point3D *old_points = node->points;
    size_t old_count = node->point_count;

    node->points = NULL;
    node->point_count = 0;
    node->point_capacity = 0;

    for (size_t i = 0; i < old_count; i++) {
        int child_index = get_child_index(node, old_points[i]);

        if (node->children[child_index] == NULL)
            node->children[child_index] = create_child_node(node, child_index);

        insert_point(node->children[child_index], old_points[i]);
    }
    G_free(old_points);

    /* Insert the new point into the appropriate child */
    int child_index = get_child_index(node, point);

    if (node->children[child_index] == NULL)
        node->children[child_index] = create_child_node(node, child_index);

    return insert_point(node->children[child_index], point);
}

/* Recursively free the octree */
void free_octree(OctreeNode *node)
{
    if (node == NULL)
        return;

    for (int i = 0; i < 8; i++) {
        if (node->children[i] != NULL)
            free_octree(node->children[i]);
    }

    if (node->points != NULL)
        G_free(node->points);
    G_free(node);
}
