#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/raster.h>

#define SQRT2 1.414214
	
typedef struct {
	int r, c;
	int is_outlet;
	} POINT;
	
typedef struct { 
	int index;
	int is_outlet;
	int r, c; /* outlet */
	float elev_diff;
	float elev_spring, elev_outlet;
	float slope; /* cumulative */
	float gradient;
	float length; /* cumulative */
	int order;
	double basin_area; /* basin order */
	int cell_num;
	} STREAM;	
	
typedef struct {
	int order;
	int stream_num;
	double sum_length;
	double avg_length;
	double std_length;
	float avg_slope;
	float std_slope;
	float avg_gradient;
	float std_gradient;
	double sum_area;
	double avg_area;
	double std_area;
	float avg_elev_diff;
	float std_elev_diff;
	float bifur_ratio;
	float std_bifur_ratio;
	float reg_bifur_ratio;
	float length_ratio;
	float std_length_ratio;
	float reg_length_ratio;
	float area_ratio;
	float std_area_ratio;
	float reg_area_ratio;
	float slope_ratio;
	float std_slope_ratio;
	float reg_slope_ratio;
	float gradient_ratio;
	float std_gradient_ratio;
	float reg_gradient_ratio;
	float stream_frequency;
	float drainage_density;
} STATS;

#ifdef MAIN
#	define GLOBAL
#else
#	define GLOBAL extern
#endif

GLOBAL int nextr[9];
GLOBAL int nextc[9];

GLOBAL double total_basins;
GLOBAL int nrows, ncols; 

GLOBAL POINT *fifo_points;
GLOBAL int fifo_max;
	
GLOBAL int outlets_num; /* number outlets: index for stream statistics*/
GLOBAL STREAM *stat_streams;
GLOBAL STATS *ord_stats;
GLOBAL STATS stats_total;
