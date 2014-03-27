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

typedef enum {o_streams, o_dirs, o_elev, o_accum, input_size } inputs;
/* eny new oredering systems must be also declared here before orders_size*/
typedef enum {o_strahler, o_horton, o_shreve, o_hack, o_topo, orders_size } orders;

typedef struct {
	char* name;
	int required;
	char* description;
} IO;

typedef struct {
    int stream;	/* topology */
    int next_stream;
    int trib_num;
    int trib[5];
    int cells_num;
    int init; /* index to recalculate into r,c*/
    int outlet; /* index to recalculate into r,c */
    double length;   /* geometry */ 
    double accum_length;
    double distance; /* distance to outlet */
    double stright; /* use next to calculate sinusoid */
    double accum;
    double init_elev;
    double outlet_elev; /* use next to calculate drop and gradient */
} STREAM;

GLOBAL char** output_map_names;
GLOBAL int ** all_orders;

GLOBAL int nextr[9];
GLOBAL int nextc[9];
GLOBAL int nrows, ncols;
GLOBAL int use_vector, use_accum;


/* stream topo */
GLOBAL int init_num, outlet_num;
GLOBAL STREAM* stream_attributes;
GLOBAL unsigned int* init_streams;
GLOBAL unsigned int* outlet_streams;
GLOBAL unsigned long int* init_cells;

/* output vector */
GLOBAL struct Map_info Out; 
