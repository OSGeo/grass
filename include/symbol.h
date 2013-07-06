#ifndef GRASS_SYMBOL_H
#define GRASS_SYMBOL_H

/* definitions and structures */

/* Warning : structure is not exactly the same as format. */

#define S_NONE    0		/* no object (used for reading) */

/* elements */
#define S_LINE    1		/* line */
#define S_ARC     2		/* arc */

/* parts */
#define S_STRING  1
#define S_POLYGON 2

#define S_COL_DEFAULT 1		/* default color */
#define S_COL_NONE    2		/* no color */
#define S_COL_DEFINED 3		/* color defined in symbol file */

typedef struct
{
    int color;			/* reset default */
    int r, g, b;
    double fr, fg, fb;
} SYMBCOLOR;

/* symbol element: line or arc */
typedef struct
{
    int type;			/* S_LINE or S_ARC */
    union
    {
	struct
	{
	    int count, alloc;
	    double *x, *y;
	} line;
	struct
	{
	    int clock;		/* 1 clockwise, 0 counter clockwise */
	    double x, y, r, a1, a2;
	} arc;
    } coor;
} SYMBEL;

/* string of elements */
typedef struct
{
    int count, alloc;		/* number of elements */
    SYMBEL **elem;		/* array of elements */
    int scount, salloc;		/* number of points in stroked version */
    double *sx, *sy;		/* coordinates in stroked version */
} SYMBCHAIN;

/* part */
typedef struct
{
    int type;			/* S_STRING or S_POLYGON */
    SYMBCOLOR color;
    SYMBCOLOR fcolor;
    int count, alloc;		/* number of rings */
    SYMBCHAIN **chain;		/* array strings */
} SYMBPART;

typedef struct
{
    double scale;		/* to get symbol of size 1, each vertex must be multiplied by this scale */
    int count, alloc;		/* numer of parts */
    SYMBPART **part;		/* objects ( parts ) */
} SYMBOL;

#include <grass/defs/symbol.h>

#endif
