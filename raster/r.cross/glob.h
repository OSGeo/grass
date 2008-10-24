
/****************************************************************************
 *
 * MODULE:       r.cross
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Creates a cross product of the category values from
 *               multiple raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef __R_CROSS_GLOB_H__
#define __R_CROSS_GLOB_H__

#include <grass/gis.h>

#define NFILES 30		/* maximum number of layers */
#define SHIFT 6			/* 2^SHIFT cats per node */
#define INCR 16

#define FOUND 0
#define LEFT  1
#define RIGHT 2

extern int nfiles;
extern int nrows;
extern int ncols;
extern int NCATS;
extern char *names[NFILES];
extern struct Categories labels[NFILES];

typedef struct
{
    CELL *cat;
    CELL *result;
    int left;
    int right;
} NODE;

extern NODE *tree;		/* tree of values */
extern int tlen;		/* allocate tree size */
extern int N;			/* number of actual nodes in tree */

typedef struct
{
    CELL *cat;
    CELL result;
} RECLASS;

extern RECLASS *reclass;
extern CELL *table;

#endif /* __R_CROSS_GLOB_H__ */
