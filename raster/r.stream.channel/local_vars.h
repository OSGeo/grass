#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/raster.h>

#define SQRT2 1.414214
	
typedef struct {
	int stream_num;
	int number_of_cells;
	int order;
	unsigned long int * points;
	float * elevation;
	double * distance;
	unsigned int init_r;
	unsigned int init_c;
} STREAM;	


#ifdef MAIN
#	define GLOBAL
#else
#	define GLOBAL extern
#endif

GLOBAL int nextr[9];
GLOBAL int nextc[9];

GLOBAL int nrows, ncols; 
GLOBAL STREAM* stream_attributes;

GLOBAL struct Cell_head window;
