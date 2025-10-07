#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gjson.h>
#include <grass/gis.h>
#include <grass/raster.h>

enum OutputFormat { PLAIN, CSV, JSON };

/* main.c */
int do_profile(double, double, double, double, int, double, int, int, FILE *,
               char *, const char *, double, enum OutputFormat, G_JSON_Array *,
               ColorFormat);

/* read_rast.c */
int read_rast(double, double, double, int, int, RASTER_MAP_TYPE, FILE *, char *,
              enum OutputFormat, G_JSON_Array *, ColorFormat);

/* input.c */
int input(char *, char *, char *, char *, char *, FILE *);

extern int clr;
extern struct Colors colors;
extern char *fs;
