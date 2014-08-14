#ifndef R3FLOW_STRUCTS_H
#define R3FLOW_STRUCTS_H

#include <grass/raster3d.h>

struct Seed
{
    double x;
    double y;
    double z;
    int flowline;
    int flowaccum;
};

enum flowdir {FLOWDIR_UP, FLOWDIR_DOWN, FLOWDIR_BOTH};

struct Integration
{
    enum flowdir direction_type;
    enum flowdir actual_direction;
    char *unit;
    double step;
    double cell_size;
    int limit;
    double max_error;
    double max_step;
    double min_step;
};

struct Gradient_info
{
    int compute_gradient;
    RASTER3D_Map *velocity_maps[3];
    RASTER3D_Map *scalar_map;
    double neighbors_values[24];
    int neighbors_pos[3];
    int initialized;
};

#endif // R3FLOW_STRUCTS_H
