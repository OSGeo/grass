
#include <grass/gis.h>
#include <math.h>

struct _gstats_
{
    long *cats;
    long count;
};

struct _layer_
{
    char *name;
    char *mapset;
    struct Categories labels;
};

extern struct Cell_head window;

extern char *maps[2];
extern char *output;
extern char *title;
extern long *matr;
extern long *rlst;
extern int ncat;
extern char *stats_file;

#define LAYER struct _layer_
extern LAYER *layers;
extern int nlayers;

#define GSTATS struct _gstats_
extern GSTATS *Gstats;
extern size_t nstats;
