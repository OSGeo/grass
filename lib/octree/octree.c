#include <math.h>

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
OctreeNode *octree_create_node(double min_x, double max_x, double min_y,
                               double max_y, double min_z, double max_z)
{
    if (min_x > max_x || min_y > max_y || min_z > max_z)
        G_fatal_error("octree_create_node: invalid bounds "
                      "(min_x=%f max_x=%f min_y=%f max_y=%f "
                      "min_z=%f max_z=%f)",
                      min_x, max_x, min_y, max_y, min_z, max_z);

    OctreeNode *node = (OctreeNode *)G_malloc(sizeof(OctreeNode));

    for (int i = 0; i < 8; i++)
        node->children[i] = NULL;

    node->points = NULL;
    node->point_count = 0;
    node->point_capacity = 0;
    node->subtree_count = 0;
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
static int get_child_index(const OctreeNode *node, OctreePoint3D point)
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

    OctreeNode *child = octree_create_node(new_min_x, new_max_x, new_min_y,
                                           new_max_y, new_min_z, new_max_z);
    child->depth = parent->depth + 1;

    return child;
}

/* Ensure the points array has room for at least the requested capacity.
   Grows geometrically so repeated inserts at a max-depth leaf (e.g., many
   coincident points) stay amortized O(1) instead of O(n) per insert. */
static void ensure_points_capacity(OctreeNode *node, size_t needed)
{
    if (node->point_capacity >= needed)
        return;

    size_t new_cap = node->point_capacity == 0
                         ? (size_t)OCTREE_MAX_POINTS_PER_NODE
                         : node->point_capacity * 2;
    if (new_cap < needed)
        new_cap = needed;

    node->points = (OctreePoint3D *)G_realloc(node->points,
                                              sizeof(OctreePoint3D) * new_cap);
    node->point_capacity = new_cap;
}

/* Insert a point into the octree.
   Returns 0 on success, -1 if the point is outside the node bounds or has
   non-finite coordinates. */
int octree_insert_point(OctreeNode *node, OctreePoint3D point)
{
    /* Reject non-finite coordinates: NaN compares false against any bound,
       which would otherwise silently bypass the bounds check below. */
    if (!isfinite(point.x) || !isfinite(point.y) || !isfinite(point.z))
        return -1;

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

        int rc = octree_insert_point(node->children[child_index], point);

        if (rc == 0)
            node->subtree_count++;
        return rc;
    }

    /* At max depth, store here regardless of capacity */
    if (node->depth >= OCTREE_MAX_DEPTH) {
        ensure_points_capacity(node, node->point_count + 1);
        node->points[node->point_count++] = point;
        node->subtree_count++;
        return 0;
    }

    /* Leaf with room: store the point */
    if (node->point_count < OCTREE_MAX_POINTS_PER_NODE) {
        ensure_points_capacity(node, node->point_count + 1);
        node->points[node->point_count++] = point;
        node->subtree_count++;
        return 0;
    }

    /* Leaf at capacity: subdivide and redistribute existing points.
       subtree_count already accounts for these points and stays unchanged;
       children's subtree_counts grow as each old point is reinserted. */
    OctreePoint3D *old_points = node->points;
    size_t old_count = node->point_count;

    node->points = NULL;
    node->point_count = 0;
    node->point_capacity = 0;

    for (size_t i = 0; i < old_count; i++) {
        int child_index = get_child_index(node, old_points[i]);

        if (node->children[child_index] == NULL)
            node->children[child_index] = create_child_node(node, child_index);

        octree_insert_point(node->children[child_index], old_points[i]);
    }
    G_free(old_points);

    /* Insert the new point into the appropriate child */
    int child_index = get_child_index(node, point);

    if (node->children[child_index] == NULL)
        node->children[child_index] = create_child_node(node, child_index);

    int rc = octree_insert_point(node->children[child_index], point);

    if (rc == 0)
        node->subtree_count++;
    return rc;
}

/* Recursively free the octree */
void octree_free(OctreeNode *node)
{
    if (node == NULL)
        return;

    for (int i = 0; i < 8; i++) {
        if (node->children[i] != NULL)
            octree_free(node->children[i]);
    }

    if (node->points != NULL)
        G_free(node->points);
    G_free(node);
}

/* Whether a node's bounding box intersects the query box (inclusive). */
static int box_intersects(const OctreeNode *node, double min_x, double max_x,
                          double min_y, double max_y, double min_z,
                          double max_z)
{
    return !(node->max_x < min_x || node->min_x > max_x ||
             node->max_y < min_y || node->min_y > max_y ||
             node->max_z < min_z || node->min_z > max_z);
}

/* Whether a point lies inside the inclusive query box. */
static int point_in_box(OctreePoint3D p, double min_x, double max_x,
                        double min_y, double max_y, double min_z, double max_z)
{
    return p.x >= min_x && p.x <= max_x && p.y >= min_y && p.y <= max_y &&
           p.z >= min_z && p.z <= max_z;
}

/* Recursive core of octree_query_box. Returns 1 to propagate early-stop. */
static int query_box_recurse(const OctreeNode *node, double min_x, double max_x,
                             double min_y, double max_y, double min_z,
                             double max_z, OctreePointVisitor visitor,
                             void *user_data, size_t *count)
{
    if (!box_intersects(node, min_x, max_x, min_y, max_y, min_z, max_z))
        return 0;

    for (size_t i = 0; i < node->point_count; i++) {
        if (!point_in_box(node->points[i], min_x, max_x, min_y, max_y, min_z,
                          max_z))
            continue;

        (*count)++;
        if (visitor != NULL && visitor(&node->points[i], user_data) != 0)
            return 1;
    }

    for (int i = 0; i < 8; i++) {
        if (node->children[i] == NULL)
            continue;

        if (query_box_recurse(node->children[i], min_x, max_x, min_y, max_y,
                              min_z, max_z, visitor, user_data, count))
            return 1;
    }
    return 0;
}

size_t octree_query_box(const OctreeNode *node, double min_x, double max_x,
                        double min_y, double max_y, double min_z, double max_z,
                        OctreePointVisitor visitor, void *user_data)
{
    if (node == NULL)
        return 0;

    size_t count = 0;

    query_box_recurse(node, min_x, max_x, min_y, max_y, min_z, max_z, visitor,
                      user_data, &count);
    return count;
}

/* Recursive core of octree_visit_to_depth. Returns 1 to propagate early-stop.
 */
static int visit_to_depth_recurse(const OctreeNode *node, int max_depth,
                                  OctreeNodeVisitor visitor, void *user_data)
{
    int is_leaf = !has_children(node);

    if (is_leaf || node->depth >= max_depth)
        return visitor(node, user_data) != 0;

    for (int i = 0; i < 8; i++) {
        if (node->children[i] == NULL)
            continue;

        if (visit_to_depth_recurse(node->children[i], max_depth, visitor,
                                   user_data))
            return 1;
    }
    return 0;
}

void octree_visit_to_depth(const OctreeNode *node, int max_depth,
                           OctreeNodeVisitor visitor, void *user_data)
{
    if (node == NULL || visitor == NULL)
        return;

    visit_to_depth_recurse(node, max_depth, visitor, user_data);
}

size_t octree_subtree_count(const OctreeNode *node)
{
    return node == NULL ? 0 : node->subtree_count;
}

/* Sum coordinates over an entire subtree for centroid computation. */
static void accumulate_subtree(const OctreeNode *node, double *sx, double *sy,
                               double *sz)
{
    for (size_t i = 0; i < node->point_count; i++) {
        *sx += node->points[i].x;
        *sy += node->points[i].y;
        *sz += node->points[i].z;
    }

    for (int i = 0; i < 8; i++) {
        if (node->children[i] != NULL)
            accumulate_subtree(node->children[i], sx, sy, sz);
    }
}

int octree_subtree_representative(const OctreeNode *node,
                                  OctreePoint3D *out_centroid,
                                  size_t *out_count)
{
    if (out_count != NULL)
        *out_count = 0;

    if (node == NULL)
        return -1;

    size_t count = node->subtree_count;

    if (out_count != NULL)
        *out_count = count;

    if (count == 0)
        return -1;

    if (out_centroid != NULL) {
        double sx = 0.0, sy = 0.0, sz = 0.0;

        accumulate_subtree(node, &sx, &sy, &sz);

        out_centroid->x = sx / (double)count;
        out_centroid->y = sy / (double)count;
        out_centroid->z = sz / (double)count;
    }
    return 0;
}
