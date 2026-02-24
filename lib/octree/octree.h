#ifndef OCTREE_H
#define OCTREE_H

#include <stdlib.h>

// Define the maximum number of points per octree node before splitting
#define MAX_POINTS_PER_NODE 10

// Point structure
typedef struct {
    double x, y, z;
} Point3D;

// Octree node structure
typedef struct OctreeNode {
    struct OctreeNode *children[8]; // 8 children for octants
    Point3D *points;                // Array of points in this node
    size_t point_count;             // Number of points in this node
    double min_x, max_x, min_y, max_y, min_z, max_z; // Bounding box
} OctreeNode;

// Function prototypes
OctreeNode *create_octree_node(double min_x, double max_x, double min_y,
                               double max_y, double min_z, double max_z);
void insert_point(OctreeNode *node, Point3D point);
void free_octree(OctreeNode *node);

#endif // OCTREE_H
