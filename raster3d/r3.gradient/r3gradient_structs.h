#ifndef R3GRADIENT_STRUCTS_H
#define R3GRADIENT_STRUCTS_H

#include <grass/raster3d.h>

struct Gradient_block {
    RASTER3D_Array_double input;
    RASTER3D_Array_double dx;
    RASTER3D_Array_double dy;
    RASTER3D_Array_double dz;
};

#endif // R3GRADIENT_STRUCTS_H
