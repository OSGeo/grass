/*
 * lidar-related projection functions
 *
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (refactoring and various additions)
 *
 * Copyright 2011-2016 by Vaclav Petras, and the GRASS Development Team
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef __PROJECTION_H__
#define __PROJECTION_H__

#include <grass/gis.h>

void projection_mismatch_report(struct Cell_head cellhd,
                                struct Cell_head loc_wind,
                                struct Key_Value *loc_proj_info,
                                struct Key_Value *loc_proj_units,
                                struct Key_Value *proj_info,
                                struct Key_Value *proj_units, int err);
void projection_check_wkt(struct Cell_head cellhd, struct Cell_head loc_wind,
                          const char *projstr, int override, int verbose);

#endif /* __PROJECTION_H__ */
