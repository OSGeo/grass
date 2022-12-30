#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>

struct _gstats_ {
    long *cats;
    long count;
};

struct _layer_ {
    const char *name;
    const char *mapset;
    struct Categories labels;
};

struct _metrics_ {
    long observations;
    long correct;
    long *matrix;
    long *row_sum;
    long *col_sum;
    double overall_accuracy;
    double *producers_accuracy;
    double *users_accuracy;
    double kappa;
    double kappa_variance;
    double *conditional_kappa;
<<<<<<< HEAD
<<<<<<< HEAD
    double mcc;
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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

static const double na_value = -999.0;
