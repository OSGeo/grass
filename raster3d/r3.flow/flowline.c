/*!
   \file flowline.c

   \brief Generates flowlines as vector lines

   (C) 2014 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2).  Read the file COPYING that comes with GRASS
   for details.

   \author Anna Petrasova
 */
#include <grass/raster3d.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>

#include "r3flow_structs.h"
#include "integrate.h"
#include "flowline.h"
#include "voxel_traversal.h"

static void write_segment(struct Map_info *flowline_vec,
			  struct line_pnts *points, struct line_cats *cats,
			  const double *point, int *cat)
{
    Vect_append_point(points, point[0], point[1], point[2]);

    Vect_cat_set(cats, 1, *cat);
    (*cat)++;
    Vect_write_line(flowline_vec, GV_LINE, points, cats);
    Vect_reset_line(points);
    Vect_reset_cats(cats);
    Vect_append_point(points, point[0], point[1], point[2]);
}

static void write_segment_db(struct field_info *finfo, dbDriver * driver,
			     dbString * sql, const double velocity,
			     double scalar_value, double sampled_map_value,
			     int write_scalar, int use_sampled_map,
			     const int cat)
{
    char buf[200];

    sprintf(buf, "insert into %s values (%d, %e", finfo->table, cat, velocity);
    db_set_string(sql, buf);
    if (write_scalar) {
	sprintf(buf, ", %e", scalar_value);
	db_append_string(sql, buf);
    }
    if (use_sampled_map) {
        sprintf(buf, ", %e", sampled_map_value);
        db_append_string(sql, buf);
    }
    db_append_string(sql, ")");


    if (db_execute_immediate(driver, sql) != DB_OK) {
	G_fatal_error(_("Unable to insert new record: '%s'"),
		      db_get_string(sql));
    }
}

static double get_map_value(RASTER3D_Region * region, RASTER3D_Map *map,
			    double north, double east, double top)
{
    int col, row, depth;
    double val;

    Rast3d_location2coord(region, north, east, top, &col, &row, &depth);
    Rast3d_get_value(map, col, row, depth, &val,
		     DCELL_TYPE);

    return val;
}

/*!
   \brief Computes flowline by integrating velocity field.

   \param region pointer to current 3D region
   \param seed starting seed (point)
   \param velocity_field pointer to array of 3 3D raster maps
   \param integration pointer to integration struct
   \param flowline_vec pointer to Map_info struct of flowline vector
   \param cats pointer to line_cats struct of flowline vector
   \param points pointer to line_pnts struct of flowline vector
   \param[in,out] cat starting category of the newly created flow line
   \param if_table TRUE if attribute table should be created and filled
 */
void compute_flowline(RASTER3D_Region * region, const struct Seed *seed,
		      struct Gradient_info *gradient_info,
		      RASTER3D_Map * flowacc, RASTER3D_Map * sampled_map,
		      struct Integration *integration,
		      struct Map_info *flowline_vec, struct line_cats *cats,
		      struct line_pnts *points, int *cat, int if_table,
		      struct field_info *finfo, dbDriver * driver)
{
    int i, j, count;
    double delta_t;
    double velocity_norm;
    double point[3], new_point[3];
    double vel_x, vel_y, vel_z;
    double min_step, max_step;
    int col, row, depth;
    int last_col, last_row, last_depth;
    int coor_diff;
    DCELL scalar_value;
    DCELL sampled_map_value;
    FCELL value;
    int *trav_coords;
    int size, trav_count;
    dbString sql;
    double velocity;

    point[0] = seed->x;
    point[1] = seed->y;
    point[2] = seed->z;

    last_col = last_row = last_depth = -1;

    size = 5;
    value = 0;
    trav_coords = G_malloc(3 * size * sizeof(int));

    if (seed->flowline) {
	/* append first point */
	Vect_append_point(points, seed->x, seed->y, seed->z);
	db_init_string(&sql);
    }
    count = 1;
    while (count <= integration->limit) {
	if (get_velocity(region, gradient_info, point[0], point[1], point[2],
			 &vel_x, &vel_y, &vel_z) < 0)
	    break;		/* outside region */
	velocity_norm = norm(vel_x, vel_y, vel_z);

	if (velocity_norm <= VELOCITY_EPSILON)
	    break;		/* zero velocity means end of propagation */

	/* convert to time */
	delta_t = get_time_step(integration->unit, integration->step,
				velocity_norm, integration->cell_size);

	/* integrate */
	min_step = get_time_step("cell", integration->min_step, velocity_norm,
				 integration->cell_size);
	max_step = get_time_step("cell", integration->max_step, velocity_norm,
				 integration->cell_size);
	delta_t *= (integration->actual_direction == FLOWDIR_UP ? 1 : -1);
	if (rk45_integrate_next
	    (region, gradient_info, point, new_point,
	     &delta_t, &velocity, min_step, max_step,
	     integration->max_error) < 0)
	    break;

	if (seed->flowline) {
	    if (if_table) {
		write_segment(flowline_vec, points, cats, new_point, cat);
		if (gradient_info->compute_gradient)
		    scalar_value = get_map_value(region, gradient_info->scalar_map,
						 point[1], point[0], point[2]);
		if (sampled_map)
		    sampled_map_value = get_map_value(region, sampled_map,
						      point[1], point[0], point[2]);
		write_segment_db(finfo, driver, &sql, velocity, scalar_value,
				 sampled_map_value, gradient_info->compute_gradient,
				 sampled_map ? 1 : 0, *cat - 1);
	    }
	    else
		Vect_append_point(points, point[0], point[1], point[2]);
	}
	if (seed->flowaccum) {
	    Rast3d_location2coord(region, new_point[1], new_point[0],
				  new_point[2], &col, &row, &depth);
	    if (!(last_col == col && last_row == row && last_depth == depth)) {
		value = Rast3d_get_float(flowacc, col, row, depth);
		Rast3d_put_float(flowacc, col, row, depth, value + 1);
		if (last_col >= 0) {
		    coor_diff = (abs(last_col - col) + abs(last_row - row) +
				 abs(last_depth - depth));
		    /* if not run for the 1. time and previous and next point coordinates
		       differ by more than 1 voxel coordinate */
		    if (coor_diff > 1) {
			traverse(region, point, new_point, &trav_coords, &size,
				 &trav_count);
			for (j = 0; j < trav_count; j++) {
			    value =
				Rast3d_get_float(flowacc,
						 trav_coords[3 * j + 0],
						 trav_coords[3 * j + 1],
						 trav_coords[3 * j + 2]);
			    Rast3d_put_float(flowacc, trav_coords[3 * j + 0],
					     trav_coords[3 * j + 1],
					     trav_coords[3 * j + 2],
					     value + 1);
			}
		    }
		}
		last_col = col;
		last_row = row;
		last_depth = depth;
	    }
	}
	for (i = 0; i < 3; i++) {
	    point[i] = new_point[i];
	}
	count++;

    }
    if (seed->flowline) {
	if (points->n_points > 1) {
	    Vect_cat_set(cats, 1, *cat);
	    (*cat)++;
	    Vect_write_line(flowline_vec, GV_LINE, points, cats);
	    G_debug(1, "Flowline ended after %d steps", count - 1);
	}
	Vect_reset_line(points);
	Vect_reset_cats(cats);
	db_free_string(&sql);
    }
    G_free(trav_coords);
}
