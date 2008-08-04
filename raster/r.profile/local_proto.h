#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/* main.c */
int do_profile(double, double, double, double, char *, int, double, int, int,
	       FILE *, char *);

/* read_rast.c */
int read_rast(double, double, double, int, int, RASTER_MAP_TYPE, FILE *,
	      char *);

/* input.c */
int input(char *, char *, char *, char *, char *);

#ifdef MAIN
int clr;
struct Colors colors;
#else
extern int clr;
extern struct Colors colors;
#endif
