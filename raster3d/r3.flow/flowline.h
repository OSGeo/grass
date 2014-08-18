#ifndef FLOWLINE_H
#define FLOWLINE_H

#include <grass/raster3d.h>
#include <grass/vector.h>

#include "r3flow_structs.h"

static const double VELOCITY_EPSILON = 1e-8;

void compute_flowline(RASTER3D_Region * region, const struct Seed *seed,
		      struct Gradient_info *gradient_info,
		      RASTER3D_Map * flowacc, RASTER3D_Map *sampled_map,
		      struct Integration *integration,
		      struct Map_info *flowline_vec, struct line_cats *cats,
		      struct line_pnts *points, int *cat, int if_table,
		      struct field_info *finfo, dbDriver *driver);
#endif // FLOWLINE_H
