#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/raster.h>

#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif


#ifndef PI2			/* PI/2 */
#define PI2 (2*atan(1))
#endif

#ifndef PI4			/* PI/4 */
#define PI4 (atan(1))
#endif

#ifndef PI
#define PI (4*atan(1))
#endif

#ifndef M2PI			/* 2*PI */
#define M2PI (8*atan(1))
#endif

#ifndef GRADE
#define GRADE (atan(1)/45)
#endif


#ifndef PI2PERCENT
#define PI2PERCENT (50/atan(1))
#endif

#ifndef UNKNOWN
#define UNKNOWN -1
#endif


#define DEGREE2RAD(a) ((a)/(180/PI))
#define RAD2DEGREE(a) ((a)*(180/PI))

#undef MIN
#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef char *STRING;

typedef struct
{
    char elevname[150];
    RASTER_MAP_TYPE raster_type;
    FCELL **elev;
    int fd;			/* file descriptor */
} MAPS;

typedef struct
{				/* struct is used both for interface and output */
    char *name;
    int required;
    char *description;
    char *gui;
    RASTER_MAP_TYPE out_data_type;
    int fd;
    void *buffer;
} IO;

typedef struct
{
    char name[100];
    int fd;
    CELL *forms_buffer;
} MULTI;

typedef struct
{
    int num_positives;
    int num_negatives;
    unsigned char positives;
    unsigned char negatives;
    int pattern[8];
    float elevation[8];
    double distance[8];
    double x[8], y[8];		/* cartesian coordinates of geomorphon */
} PATTERN;

typedef enum
{
    ZERO,			/* zero cats do not accept zero category */
    FL,				/* flat */
    PK,				/* peak, summit */
    RI,				/* ridge */
    SH,				/* shoulder */
    CV,				/* convex slope */
    SL,				/* slope */
    CN,				/* concave slope */
    FS,				/* footslope */
    VL,				/* valley */
    PT,				/* pit, depression */
    __,				/* error */
    CNT				/* counter */
} FORMS;

typedef struct
{
    int cat;
    int r;
    int g;
    int b;
    char *label;
} CATCOLORS;

typedef struct
{
    double cat;
    int r;
    int g;
    int b;
    char *label;
} FCOLORS;

/* variables */
GLOBAL MAPS elevation;
GLOBAL int nrows, ncols, row_radius_size, row_buffer_size;
GLOBAL int search_cells, skip_cells, step_cells, start_cells;
GLOBAL int num_of_steps;
GLOBAL double search_distance, skip_distance, flat_distance;
GLOBAL double start_distance, step_distance;	/* multiresolution mode */
GLOBAL double flat_threshold, flat_threshold_height;
GLOBAL double H, V;
GLOBAL struct Cell_head window;
GLOBAL int cell_step;

/* main */
GLOBAL unsigned int global_ternary_codes[6562];

/* memory */
int open_map(MAPS * rast);
int create_maps(void);
int shift_buffers(int row);
int get_cell(int col, float *buf_row, void *buf, RASTER_MAP_TYPE raster_type);
int free_map(FCELL ** map, int n);
int write_form_cat_colors(char *raster, CATCOLORS * ccolors);
int write_contrast_colors(char *);

/* geom */
int calc_pattern(PATTERN * pattern, int row, int cur_row, int col);
unsigned int ternary_rotate(unsigned int value);
int determine_form(int num_plus, int num_minus);
int determine_binary(int *pattern, int sign);
int determine_ternary(int *pattern);
int rotate(unsigned char binary);
float intensity(float *elevation, int pattern_size);
float exposition(float *elevation);
float range(float *elevation);
float variance(float *elevation, int n);
int shape(PATTERN * pattern, int pattern_size, float *azimuth,
	  float *elongation, float *width);
float extends(PATTERN * pattern, int pattern_size);
int radial2cartesian(PATTERN *);

/* multires */
int reset_multi_patterns(void);
int calc_multi_patterns(int row, int cur_row, int col);
int update_pattern(int k, int i,
		   double zenith_height, double zenith_distance,
		   double zenith_angle, double nadir_height,
		   double nadir_distance, double nadir_angle);

#endif
