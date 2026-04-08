#ifndef OCTREE_H
#define OCTREE_H

#include <stdlib.h>

#include <grass/gis.h>

/* Maximum number of points per octree node before splitting */
#define MAX_POINTS_PER_NODE 10

/* Maximum tree depth to prevent infinite recursion with coincident points */
#define OCTREE_MAX_DEPTH    21

/* Point structure */
typedef struct {
    double x, y, z;
} Point3D;

/* Octree node structure */
typedef struct OctreeNode {
    struct OctreeNode *children[8]; /* 8 children for octants */
    Point3D *points;                /* Array of points in this node */
    size_t point_count;             /* Number of points in this node */
    size_t point_capacity;          /* Allocated capacity for points */
    double min_x, max_x, min_y, max_y, min_z, max_z; /* Bounding box */
    int depth; /* Depth of this node in the tree */
} OctreeNode;

/* Function prototypes */
OctreeNode *create_octree_node(double min_x, double max_x, double min_y,
                               double max_y, double min_z, double max_z);
int insert_point(OctreeNode *node, Point3D point);
void free_octree(OctreeNode *node);

#endif /* OCTREE_H */
