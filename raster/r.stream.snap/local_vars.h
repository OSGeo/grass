#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#ifdef MAIN
#  define GLOBAL
#else
#  define GLOBAL extern
#endif

#define SQR(x) ((x) * (x))

typedef struct
{
    int r, c;
    int di, dj;			/* shift */
    int cat;
    double accum;
    int stream;
    int status;			/* 1=skipped,2=unresolved,3=snapped,4=correct */
} OUTLET;

GLOBAL int nextr[9];
GLOBAL int nextc[9];

GLOBAL OUTLET *points;
GLOBAL int nrows, ncols;
GLOBAL float **distance_mask;
