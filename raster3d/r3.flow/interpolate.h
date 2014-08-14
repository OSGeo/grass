#ifndef INTERPOLATE_H
#define INTERPOLATE_H

#include <grass/raster3d.h>

#include "r3flow_structs.h"

int interpolate_velocity(RASTER3D_Region * region, RASTER3D_Map ** map,
			 const double north, const double east,
			 const double top, double *vel_x, double *vel_y,
			 double *vel_z);
int get_gradient(RASTER3D_Region * region,
		 struct Gradient_info *gradient_info, const double north,
		 const double east, const double top, double *vel_x, double *vel_y,
		 double *vel_z);
#endif // INTERPOLATE_H
