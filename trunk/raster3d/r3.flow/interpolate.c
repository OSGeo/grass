/*!
   \file interpolate.c

   \brief Trilinear interpolation

   (C) 2014 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2).  Read the file COPYING that comes with GRASS
   for details.

   \author Anna Petrasova
 */

#include <grass/gis.h>
#include <grass/raster3d.h>

#include "r3flow_structs.h"
#include "interpolate.h"

/*!
   \brief Finds 8 nearest voxels from a point.

   \param region pointer to current 3D region
   \param north,east,top geographic coordinates
   \param[out] x,y,z pointer to indices of neighbouring voxels
 */
static void find_nearest_voxels(RASTER3D_Region * region,
				const double north, const double east,
				const double top, int *x, int *y, int *z)
{
    double n_minus, e_minus, t_minus;
    double n_plus, e_plus, t_plus;

    n_minus = north - region->ns_res / 2;
    n_plus = north + region->ns_res / 2;
    e_minus = east - region->ew_res / 2;
    e_plus = east + region->ew_res / 2;
    t_minus = top - region->tb_res / 2;
    t_plus = top + region->tb_res / 2;

    Rast3d_location2coord(region, n_minus, e_minus, t_minus, x++, y++, z++);
    Rast3d_location2coord(region, n_minus, e_plus, t_minus, x++, y++, z++);
    Rast3d_location2coord(region, n_plus, e_minus, t_minus, x++, y++, z++);
    Rast3d_location2coord(region, n_plus, e_plus, t_minus, x++, y++, z++);
    Rast3d_location2coord(region, n_minus, e_minus, t_plus, x++, y++, z++);
    Rast3d_location2coord(region, n_minus, e_plus, t_plus, x++, y++, z++);
    Rast3d_location2coord(region, n_plus, e_minus, t_plus, x++, y++, z++);
    Rast3d_location2coord(region, n_plus, e_plus, t_plus, x, y, z);
}

/*!
   \brief Trilinear interpolation

   Computation is based on the sum of values of nearest voxels
   weighted by the relative distance of a point
   to the center of the nearest voxels.

   \param array_values pointer to values of 8 (neigboring) voxels
   \param x,y,z relative coordinates (0 - 1)
   \param[out] interpolated pointer to the array (of size 3) of interpolated values
 */
static void trilinear_interpolation(const double *array_values,
				    const double x, const double y,
				    const double z, double *interpolated)
{
    int i, j;
    double rx, ry, rz, value;
    double weights[8];

    rx = 1 - x;
    ry = 1 - y;
    rz = 1 - z;
    weights[0] = rx * ry * rz;
    weights[1] = x * ry * rz;
    weights[2] = rx * y * rz;
    weights[3] = x * y * rz;
    weights[4] = rx * ry * z;
    weights[5] = x * ry * z;
    weights[6] = rx * y * z;
    weights[7] = x * y * z;


    /* weighted sum of surrounding values */
    for (i = 0; i < 3; i++) {
	value = 0;
	for (j = 0; j < 8; j++) {
	    value += weights[j] * array_values[i * 8 + j];
	}
	interpolated[i] = value;
    }
}

/*!
   \brief Converts geographic to relative coordinates

   Converts geographic to relative coordinates
   which are needed for trilinear interpolation.


   \param region pointer to current 3D region
   \param north,east,top geographic coordinates
   \param[out] x,y,z pointer to relative coordinates (0 - 1)
 */
static void get_relative_coords_for_interp(RASTER3D_Region * region,
					   const double north,
					   const double east,
					   const double top, double *x,
					   double *y, double *z)
{
    int col, row, depth;
    double temp;

    Rast3d_location2coord(region, north, east, top, &col, &row, &depth);

    /* x */
    temp = east - region->west - col * region->ew_res;
    *x = (temp > region->ew_res / 2 ?
	  temp - region->ew_res / 2 : temp + region->ew_res / 2)
	/ region->ew_res;
    /* y */
    temp = north - region->south - (region->rows - row - 1) * region->ns_res;
    *y = (temp > region->ns_res / 2 ?
	  temp - region->ns_res / 2 : temp + region->ns_res / 2)
	/ region->ns_res;
    /* z */
    temp = top - region->bottom - depth * region->tb_res;
    *z = (temp > region->tb_res / 2 ?
	  temp - region->tb_res / 2 : temp + region->tb_res / 2)
	/ region->tb_res;
}

/*!
   \brief Interpolates velocity at a given point.

   \param region pointer to current 3D region
   \param map pointer to array of 3 3D raster maps (velocity field)
   \param north,east,top geographic coordinates
   \param[out] vel_x,vel_y,vel_z interpolated velocity

   \return 0 success
   \return -1 out of region
 */
int interpolate_velocity(RASTER3D_Region * region, RASTER3D_Map ** map,
			 const double north, const double east,
			 const double top, double *vel_x, double *vel_y,
			 double *vel_z)
{
    int i, j;
    double values[24];		/* 3 x 8, 3 dimensions, 8 neighbors */
    double value;
    double interpolated[3];
    int x[8], y[8], z[8];
    double rel_x, rel_y, rel_z;
    int type;

    /* check if we are out of region, any array should work */
    if (!Rast3d_is_valid_location(region, north, east, top))
	return -1;

    find_nearest_voxels(region, north, east, top, x, y, z);
    /* get values of the nearest cells */
    for (i = 0; i < 3; i++) {
	type = Rast3d_tile_type_map(map[i]);
	for (j = 0; j < 8; j++) {

	    Rast3d_get_value_region(map[i], x[j], y[j], z[j], &value, type);
	    if (Rast_is_null_value(&value, type))
		values[i * 8 + j] = 0;
	    else
		values[i * 8 + j] = value;
	}
    }

    /* compute weights */
    get_relative_coords_for_interp(region, north, east, top,
				   &rel_x, &rel_y, &rel_z);

    trilinear_interpolation(values, rel_x, rel_y, rel_z, interpolated);
    *vel_x = interpolated[0];
    *vel_y = interpolated[1];
    *vel_z = interpolated[2];

    return 0;
}
/*!
   \brief Computes gradient for a point.

   \param region pointer to current 3D region
   \param gradient_info struct which remembers values
          related to gradient computation to avoid computation every time
   \param north,east,top geographic coordinates
   \param[out] vel_x,vel_y,vel_z interpolated gradient components

   \return 0 success
   \return -1 out of region
 */
int get_gradient(RASTER3D_Region * region,
		 struct Gradient_info *gradient_info, const double north,
		 const double east, const double top, double *vel_x, double *vel_y,
		 double *vel_z)
{

    int i, d, r, c, count;
    int near_x[8], near_y[8], near_z[8];
    int minx, maxx, miny, maxy, minz, maxz;
    int xshift, yshift, zshift;
    double rel_x, rel_y, rel_z;
    double scalar_map_array[64];
    double grad_x_map_array[64], grad_y_map_array[64], grad_z_map_array[64];
    RASTER3D_Array_double array;
    RASTER3D_Array_double grad_x, grad_y, grad_z;
    RASTER3D_Array_double *grad_xyz[3];
    double step[3];
    double interpolated[3];

    step[0] = region->ew_res;
    step[1] = region->ns_res;
    step[2] = region->tb_res;

    array.array = scalar_map_array;
    array.sx = array.sy = array.sz = 4;
    grad_x.array = grad_x_map_array;
    grad_y.array = grad_y_map_array;
    grad_z.array = grad_z_map_array;
    grad_x.sx = grad_x.sy = grad_x.sz = 4;
    grad_y.sx = grad_y.sy = grad_y.sz = 4;
    grad_z.sx = grad_z.sy = grad_z.sz = 4;

    find_nearest_voxels(region, north, east, top, near_x, near_y, near_z);
    minx = near_x[0];
    maxx = near_x[7];
    miny = near_y[7];
    maxy = near_y[0];
    minz = near_z[0];
    maxz = near_z[7];

    /* position of x, y, z neighboring voxels */
    if (!gradient_info->initialized ||
	(gradient_info->neighbors_pos[0] != minx ||
	 gradient_info->neighbors_pos[1] != miny ||
	 gradient_info->neighbors_pos[2] != minz)) {

	gradient_info->neighbors_pos[0] = minx;
	gradient_info->neighbors_pos[1] = miny;
	gradient_info->neighbors_pos[2] = minz;
	gradient_info->initialized = TRUE;

	/* just to be sure, we check that at least one voxel is inside */
	if (maxx < 0 || minx >= region->cols ||
	    maxy < 0 || miny >= region->rows ||
	    maxz < 0 || minz >= region->depths)
	    return -1;

	/* these if's are here to handle edge cases
	   min is changed to represent the min coords of the 4x4x4 array
	   from which the gradient will be computed
	   shift is relative position of the neighbors within this 4x4x4 array */
	if (minx == 0 || minx == -1) {
	    xshift = minx;
	    minx = 0;
	}
	else if (maxx >= region->cols - 1) {
	    minx = maxx < region->cols ? maxx - 3 : maxx - 4;
	    xshift = maxx < region->cols ? 2 : 3;
	}
	else {
	    minx -= 1;
	    xshift = 1;
	}

	if (miny == 0 || miny == -1) {
	    yshift = miny;
	    miny = 0;
	}
	else if (maxy >= region->rows - 1) {
	    miny = maxy < region->rows ? maxy - 3 : maxy - 4;
	    yshift = maxy < region->rows ? 2 : 3;
	}
	else {
	    miny -= 1;
	    yshift = 1;
	}

	if (minz == 0 || minz == -1) {
	    zshift = minz;
	    minz = 0;
	}
	else if (maxz >= region->depths - 1) {
	    minz = maxz < region->depths ? maxz - 3 : maxz - 4;
	    zshift = maxz < region->depths ? 2 : 3;
	}
	else {
	    minz -= 1;
	    zshift = 1;
	}

	/* get the 4x4x4 block of the array */
	Rast3d_get_block(gradient_info->scalar_map, minx, miny, minz,
			 4, 4, 4, array.array, DCELL_TYPE);
	Rast3d_gradient_double(&array, step, &grad_x, &grad_y, &grad_z);
	grad_xyz[0] = &grad_x;
	grad_xyz[1] = &grad_y;
	grad_xyz[2] = &grad_z;
	/* go through x, y, z and all 8 neighbors and store their value
	   if the voxel is outside, add 0 (weight) */
	for (i = 0; i < 3; i++) {
	    count = 0;
	    for (d = 0; d < 2; d++)
		for (r = 1; r > -1; r--)
		    for (c = 0; c < 2; c++) {
			if (d + zshift < 0 || d + zshift > 3 ||
			    r + yshift < 0 || r + yshift > 3 ||
			    c + xshift < 0 || c + xshift > 3)
			    gradient_info->neighbors_values[i * 8 + count] =
				0;
			else
			    gradient_info->neighbors_values[i * 8 + count] =
				RASTER3D_ARRAY_ACCESS(grad_xyz[i], c + xshift,
						      r + yshift, d + zshift);
			count++;
		    }
	}
    }
    get_relative_coords_for_interp(region, north, east, top, &rel_x, &rel_y, &rel_z);
    trilinear_interpolation(gradient_info->neighbors_values,
			    rel_x, rel_y, rel_z, interpolated);

    *vel_x = interpolated[0];
    *vel_y = interpolated[1];
    *vel_z = interpolated[2];

    return 0;



}
