#ifndef OCTREE_H
#define OCTREE_H

#include <stdlib.h>

#include <grass/gis.h>

/* Maximum number of points per octree node before splitting */
#define OCTREE_MAX_POINTS_PER_NODE 10

/* Maximum tree depth to prevent infinite recursion with coincident points */
#define OCTREE_MAX_DEPTH           21

/* Point structure */
typedef struct {
    double x, y, z;
} OctreePoint3D;

/* Octree node structure */
typedef struct OctreeNode {
    struct OctreeNode *children[8]; /* 8 children for octants */
    OctreePoint3D *points;          /* Array of points in this node */
    size_t point_count;             /* Number of points in this node */
    size_t point_capacity;          /* Allocated capacity for points */
    size_t subtree_count;           /* Total points in this node's subtree */
    double min_x, max_x, min_y, max_y, min_z, max_z; /* Bounding box */
    int depth; /* Depth of this node in the tree */
} OctreeNode;

/* Visitor invoked per matching point in a range query.
   Return 0 to continue iteration, non-zero to stop early.
   The point pointer aliases internal storage and must not be retained past
   the call or across tree modifications. */
typedef int (*OctreePointVisitor)(const OctreePoint3D *point, void *user_data);

/* Visitor invoked per terminal node in a depth-limited traversal.
   A terminal node is either a true leaf (no children) or a node at the
   requested max_depth. Return 0 to continue, non-zero to stop early. */
typedef int (*OctreeNodeVisitor)(const OctreeNode *node, void *user_data);

/* Function prototypes */
OctreeNode *octree_create_node(double min_x, double max_x, double min_y,
                               double max_y, double min_z, double max_z);
int octree_insert_point(OctreeNode *node, OctreePoint3D point);
void octree_free(OctreeNode *node);

/* Visit every point inside the inclusive axis-aligned query box.
   Subtrees whose bounding box does not intersect the query are pruned.
   visitor may be NULL to count matches without a callback.
   Returns the number of matching points visited. */
size_t octree_query_box(const OctreeNode *node, double min_x, double max_x,
                        double min_y, double max_y, double min_z, double max_z,
                        OctreePointVisitor visitor, void *user_data);

/* Traverse the tree, invoking visitor once per terminal node. A node is
   treated as terminal when it has no children or when its depth reaches
   max_depth. Intended for level-of-detail rendering: pick max_depth based
   on zoom and emit one primitive per visited node. */
void octree_visit_to_depth(const OctreeNode *node, int max_depth,
                           OctreeNodeVisitor visitor, void *user_data);

/* Return the cached total number of points in this node's subtree.
   Returns 0 for a NULL node. O(1). */
size_t octree_subtree_count(const OctreeNode *node);

/* Compute the centroid and total point count of a subtree.
   out_centroid and out_count may each be NULL if not needed.
   Returns 0 on success, -1 if the subtree contains no points.
   The count is O(1) (cached); computing the centroid is O(subtree size). */
int octree_subtree_representative(const OctreeNode *node,
                                  OctreePoint3D *out_centroid,
                                  size_t *out_count);

#endif /* OCTREE_H */
