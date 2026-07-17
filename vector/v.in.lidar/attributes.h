/****************************************************************************
 *
 * MODULE:       v.in.lidar
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      projection related functions
 * SPDX-FileCopyrightText: 2015 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#ifndef GRASS_LIDAR_TO_ATTRIBUTES_H
#define GRASS_LIDAR_TO_ATTRIBUTES_H

struct field_info;
struct dbDriver; /* TODO: is this correct forward declaration? */

void create_table_for_lidar(struct Map_info *vector_map, const char *name,
                            int layer, dbDriver **db_driver,
                            struct field_info **finfo, int have_time,
                            int have_color);

void las_point_to_attributes(struct field_info *Fi, dbDriver *driver, int cat,
                             LASPointH LAS_point, double x, double y, double z,
                             int have_time, int have_color);

#endif /* GRASS_LIDAR_TO_ATTRIBUTES_H */
