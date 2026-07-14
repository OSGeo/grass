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

#include <liblas/capi/liblas.h>

#ifndef LIDAR_INFO_H
#define LIDAR_INFO_H

void print_lasinfo(LASHeaderH LAS_header, LASSRSH LAS_srs);

#endif /* LIDAR_INFO_H */
