#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#define SQRT2 1.414214
	
typedef struct {
	int stream;
	int next_stream;
	int number_of_cells;
	int order;
	unsigned long int * points;
	float * elevation;
	double * distance;
	unsigned long int init;
	unsigned long int outlet; /* outlet is cell from next stream */
	int last_cell_dir; /* to add outlet to vector */
	float direction;
	float length;
	float stright;
	float drop;
	float tangent;
	float continuation;
	int number_of_sectors;
	int* sector_breakpoints; /* index of breakpoints in *points vector */
	int* sector_cats;
	float* sector_directions;
	float* sector_strights;
	double* sector_lengths;
	float* sector_drops; /* gradient calculated at the end */
} STREAM;	

typedef struct {
	float long_dir_diff;
	float short_dir_diff;
	int long_break;
	int decision;
} DIRCELLS;

#ifdef MAIN
#	define GLOBAL
#else
#	define GLOBAL extern
#endif

#ifdef MAIN
#	define GLOBAL
#else
#	define GLOBAL extern
#endif

#ifndef PI
 #define PI (4*atan(1))
#endif

#define DEG2RAD(d) ((d)*PI/180)
#define RAD2DEG(r) ((r)*180/PI)

GLOBAL int nextr[9];
GLOBAL int nextc[9];

GLOBAL int nrows, ncols; 
GLOBAL STREAM* stream_attributes;

GLOBAL struct Cell_head window;
