#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/raster.h>

#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif


#ifndef PI2                     /* PI/2 */
#define PI2 (2*atan(1))
#endif

#ifndef PI
#define PI (4*atan(1))
#endif

#define DEGREE2RAD(a) ((a)/(180/PI))
#define RAD2DEGREE(a) ((a)*(180/PI))

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/* Number of cardinal directions. */
#define NUM_DIRS 8

typedef struct
{
    char elevname[150];
    RASTER_MAP_TYPE raster_type;
    FCELL **elev;
    int fd;                     /* file descriptor */
} MAPS;

typedef struct
{
    int num_positives;
    int num_negatives;
    unsigned char positives;
    unsigned char negatives;
    int pattern[NUM_DIRS];
    float elevation[NUM_DIRS];
    double distance[NUM_DIRS];
    double x[NUM_DIRS], y[NUM_DIRS];    /* cartesian coordinates of geomorphon */
    double e[NUM_DIRS], n[NUM_DIRS];    /* projection-specific coordinates */
} PATTERN;

typedef enum
{
    ZERO,                       /* zero cats do not accept zero category */
    FL,                         /* flat */
    PK,                         /* peak (summit) */
    RI,                         /* ridge */
    SH,                         /* shoulder */
    SP,                         /* spur (convex slope) */
    SL,                         /* slope */
    HL,                         /* hollow (concave slope) */
    FS,                         /* footslope */
    VL,                         /* valley */
    PT,                         /* pit (depression) */
    __,                         /* error (impossible) */
    CNT                         /* counter */
} FORMS;

/* main */
GLOBAL MAPS elevation;
GLOBAL int ncols, row_radius_size, row_buffer_size;
GLOBAL int skip_cells;
GLOBAL double search_distance, flat_distance;
GLOBAL double flat_threshold, flat_threshold_height;
GLOBAL struct Cell_head window;
GLOBAL int cell_step;
/* Zenith/nadir comparison modes. */
GLOBAL enum
{
    ANGLEV1,
    ANGLEV2,
    ANGLEV2_DISTANCE
} compmode;

/* memory */
int open_map(MAPS * rast);
int shift_buffers(int row);
int free_map(FCELL ** map, int n);
int write_form_cat_colors(char *raster);
int write_contrast_colors(char *);

/* pattern */
int calc_pattern(PATTERN * pattern, int row, int cur_row, int col, const int);
extern const char *dirname[];

/* geom */
void generate_ternary_codes();
unsigned int ternary_rotate(unsigned int value);
FORMS determine_form(int num_plus, int num_minus);
int form_deviation(const unsigned, const unsigned);
int determine_binary(int *pattern, int sign);
int determine_ternary(int *pattern);
int preliminary_ternary(const int *);
int rotate(unsigned char binary);
float intensity(float *elevation, int pattern_size);
float exposition(float *elevation);
float range(float *elevation);
float variance(float *elevation, int n);
int shape(PATTERN * pattern, int pattern_size, float *azimuth,
          float *elongation, float *width);
float extends(PATTERN * pattern);
double octa_perimeter(const PATTERN *);
double octa_area(const PATTERN *);
double mesh_perimeter(const PATTERN *);
double mesh_area(const PATTERN *);
int radial2cartesian(PATTERN *);

/* profile */
void prof_int(const char *, const int);
void prof_bln(const char *, const int);
void prof_dbl(const char *, const double);
void prof_mtr(const char *, const double);
void prof_str(const char *, const char *);
void prof_utc(const char *, const time_t);
void prof_sso(const char *);
void prof_eso();
void prof_pattern(const double, const PATTERN *);
void prof_map_info();
unsigned prof_write(FILE *, const char *);
#endif
