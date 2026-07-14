/****************************************************************************
 *
 * MODULE:       r.cross
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Creates a cross product of the category values from
 *               multiple raster map layers.
 *
 * SPDX-FileCopyrightText: 2006 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 ***************************************************************************/

#ifndef __R_CROSS_GLOB_H__
#define __R_CROSS_GLOB_H__

#include <grass/gis.h>
#include <grass/raster.h>

#define NFILES 30 /* maximum number of layers */

extern int nfiles;
extern int nrows;
extern int ncols;
extern const char *names[NFILES];
extern struct Categories labels[NFILES];

typedef struct {
    CELL *cat;
    CELL result;
} RECLASS;

extern RECLASS *reclass;
extern CELL *table;

#endif /* __R_CROSS_GLOB_H__ */
