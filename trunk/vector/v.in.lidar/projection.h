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


#ifndef PROJECTION_CHECKS_H
#define PROJECTION_CHECKS_H

void projection_mismatch_report(struct Cell_head cellhd,
                                struct Cell_head loc_wind,
                                struct Key_Value *loc_proj_info,
                                struct Key_Value *loc_proj_units,
                                struct Key_Value *proj_info,
                                struct Key_Value *proj_units, int err);

void projection_check_wkt(struct Cell_head cellhd,
                          struct Cell_head loc_wind,
                          const char *projstr, int override, int verbose);

#endif /* PROJECTION_CHECKS_H */
