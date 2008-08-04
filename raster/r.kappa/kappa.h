#ifndef GLOBAL
# define GLOBAL extern
# define INIT(x)
#else
# define INIT(x)=x
#endif

#include <grass/gis.h>
#include <math.h>

GLOBAL struct Cell_head window;

GLOBAL char *maps[2];
GLOBAL char *output;
GLOBAL char *title;
GLOBAL long *matr;
GLOBAL long *rlst;
GLOBAL int ncat;
GLOBAL char *stats_file;

#define LAYER struct _layer_
GLOBAL LAYER
{
    char *name;
    char *mapset;
    struct Categories labels;
} *layers INIT(NULL);
GLOBAL int nlayers INIT(0);

#define GSTATS struct _gstats_
GLOBAL GSTATS
{
    long *cats;
    long count;
} *Gstats INIT(NULL);
GLOBAL size_t nstats INIT(0);
