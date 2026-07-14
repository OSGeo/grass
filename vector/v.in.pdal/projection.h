/****************************************************************************
 *
 * MODULE:       v.in.lidar
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      projection related functions
 * SPDX-FileCopyrightText: 2015 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
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

void projection_check_wkt(struct Cell_head cellhd, struct Cell_head loc_wind,
                          const char *projstr, int override, int return_value,
                          int verbose);

int is_wkt_projection_same_as_loc(const char *wkt);
void wkt_projection_mismatch_report(const char *wkt);
char *location_projection_as_wkt(int prettify);

#endif /* PROJECTION_CHECKS_H */
