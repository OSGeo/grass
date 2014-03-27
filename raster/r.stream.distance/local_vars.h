#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/raster.h>

#ifdef MAIN
#  define GLOBAL
#else
#  define GLOBAL extern
#endif

#define SQRT2 1.414214
#define UPSTREAM 0
#define DOWNSTREAM 1

typedef struct {
	int r,c;
} OUTLET;

typedef struct {
	int r,c;
  float cur_dist;
  float target_elev;
} POINT;

GLOBAL int nextr[9];
GLOBAL int nextc[9];

GLOBAL OUTLET* outlets;
GLOBAL int nrows, ncols;
GLOBAL int fifo_max;
GLOBAL POINT* fifo_points;
GLOBAL int accum;
