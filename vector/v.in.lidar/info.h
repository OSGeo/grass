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

#include <liblas/capi/liblas.h>

#ifndef LIDAR_INFO_H
#define LIDAR_INFO_H

void print_lasinfo(LASHeaderH LAS_header, LASSRSH LAS_srs);

#endif /* LIDAR_INFO_H */
