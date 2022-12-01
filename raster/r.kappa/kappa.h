#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>

struct _gstats_
{
    long *cats;
    long count;
};

struct _layer_
{
    const char *name;
    const char *mapset;
    struct Categories labels;
};

struct _metrics_
{
    long obs;
    long correct;
    long *matrix;
    long *rowsum;
    long *colsum;
    double total_acc;
    double *prod_acc;
    double *user_acc;
    double kappa;
    double kappa_var;
    double *cond_kappa;
};

extern struct Cell_head window;

extern const char *maps[2];
extern const char *output;
extern const char *title;
extern long *matr;
extern long *rlst;
extern int ncat;
extern const char *stats_file;

#define LAYER struct _layer_
extern LAYER *layers;
extern int nlayers;

#define GSTATS struct _gstats_
extern GSTATS *Gstats;
extern size_t nstats;

#define METRICS struct _metrics_
extern METRICS *metrics;
