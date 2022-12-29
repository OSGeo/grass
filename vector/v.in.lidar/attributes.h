
/****************************************************************************
 *
 * MODULE:       v.in.lidar
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      projection related functions
 * COPYRIGHT:    (C) 2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/


#ifndef GRASS_LIDAR_TO_ATTRIBUTES_H
#define GRASS_LIDAR_TO_ATTRIBUTES_H

struct field_info;
struct dbDriver;                /* TODO: is this correct forward declaration? */

void create_table_for_lidar(struct Map_info *vector_map, const char *name,
                            int layer, dbDriver ** db_driver,
                            struct field_info **finfo, int have_time,
                            int have_color);

void las_point_to_attributes(struct field_info *Fi, dbDriver * driver,
                             int cat, LASPointH LAS_point, double x,
                             double y, double z, int have_time,
                             int have_color);

#endif /* GRASS_LIDAR_TO_ATTRIBUTES_H */
