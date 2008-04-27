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

#define NFILES 10  /* maximum number of layers */
#define SHIFT 6 /* 2^SHIFT cats per node */
#define INCR 16

#define FOUND 0
#define LEFT  1
#define RIGHT 2

#ifndef GLOBAL
# define GLOBAL extern
# define INIT(x)
#else
# define INIT(x) =(x)
#endif

GLOBAL int nfiles;
GLOBAL int nrows;
GLOBAL int ncols;
GLOBAL int NCATS INIT(1<<SHIFT);
GLOBAL char *names[NFILES];
GLOBAL struct Categories labels[NFILES];

typedef struct
{
    CELL *cat;
    CELL *result;
    int left;
    int right;
} NODE;

GLOBAL NODE *tree; /* tree of values */
GLOBAL int tlen;   /* allocate tree size */
GLOBAL int N;      /* number of actual nodes in tree */

typedef struct
{
    CELL *cat;
    CELL result;
} RECLASS;

GLOBAL RECLASS *reclass INIT(NULL);
GLOBAL CELL *table INIT(NULL);

#endif /* __R_CROSS_GLOB_H__ */
