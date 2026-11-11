#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gjson.h>
#include <grass/gis.h>
#include <grass/raster.h>

enum OutputFormat { PLAIN, CSV, JSON };

/* Settings shared by all profile points of one run */
struct profile_output {
    int fd;
    RASTER_MAP_TYPE data_type;
    FILE *fp;
    enum OutputFormat format;
    G_JSON_Array *array;
    ColorFormat clr_frmt;
    char *null_string;
    const char *unit;
    double factor;
    int coords;   /* print easting and northing */
    int with_cat; /* print the category of the vector line being profiled */
    int cat;      /* category of the current line, -1 when it has none */
};

/* main.c */
void start_profile(void);
int do_profile(double, double, double, double, double,
               const struct profile_output *);

/* read_rast.c */
int read_rast(double, double, double, const struct profile_output *);

/* input.c */
int input(char *, char *, char *, char *, char *, FILE *);

extern int clr;
extern struct Colors colors;
extern char *fs;
