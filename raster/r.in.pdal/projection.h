<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
/****************************************************************************
 *
 * MODULE:       r.in.pdal
 *               adapted from v.in.lidar
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      projection related functions
 * COPYRIGHT:    (C) 2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
#ifndef PROJECTION_CHECKS_H
#define PROJECTION_CHECKS_H

void projection_mismatch_report(struct Cell_head cellhd,
                                struct Cell_head loc_wind,
                                struct Key_Value *loc_proj_info,
                                struct Key_Value *loc_proj_units,
                                struct Key_Value *proj_info,
                                struct Key_Value *proj_units, int err);

<<<<<<< HEAD
void projection_check_wkt(struct Cell_head cellhd, struct Cell_head loc_wind,
                          const char *projstr, int override, int return_value,
                          int verbose);
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
void projection_check_wkt(struct Cell_head cellhd, struct Cell_head loc_wind,
                          const char *projstr, int override, int return_value,
                          int verbose);
=======
void projection_check_wkt(struct Cell_head cellhd,
                          struct Cell_head loc_wind,
                          const char *projstr, int override,
                          int return_value, int verbose);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
void projection_check_wkt(struct Cell_head cellhd, struct Cell_head loc_wind,
                          const char *projstr, int override, int return_value,
                          int verbose);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void projection_check_wkt(struct Cell_head cellhd, struct Cell_head loc_wind,
                          const char *projstr, int override, int return_value,
                          int verbose);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main

int is_wkt_projection_same_as_loc(const char *wkt);
void wkt_projection_mismatch_report(const char *wkt);
char *location_projection_as_wkt(int prettify);

#endif /* PROJECTION_CHECKS_H */
