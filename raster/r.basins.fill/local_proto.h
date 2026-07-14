/****************************************************************************
 *
 * MODULE:       r.basins.fill
 *
 * AUTHOR(S):    Dale White - Dept. of Geography, Pennsylvania State U.
 *               Larry Band - Dept. of Geography, University of Toronto
 *
 * PURPOSE:      Generates a raster map layer showing watershed subbasins.
 *
 * SPDX-FileCopyrightText: 2005 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 ****************************************************************************/

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

/* read_map.c */
CELL *read_map(const char *, int, int, int);

#endif /* __LOCAL_PROTO_H__ */
