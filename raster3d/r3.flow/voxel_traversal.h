#ifndef VOXEL_TRAVERSAL_H
#define VOXEL_TRAVERSAL_H

#include <grass/raster3d.h>

void traverse(RASTER3D_Region *region, double *start, double *end,
              int **coordinates, int *size, int *coor_count);

#endif // VOXEL_TRAVERSAL_H
