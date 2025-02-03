#ifndef __SIMLIB_H__
#define __SIMLIB_H__

/*! \file simlib.h
 * \brief This is the interface for the simlib (SIMWE) library.
 */
#include <stdbool.h>

#define NUM_THREADS "1"
#if defined(_OPENMP)
#include <omp.h>
#endif

typedef struct {
    int mx, my;
    double xmin, xmax, ymin, ymax;
    double miyy, mixx;
    double step, stepx, stepy;
    double conv;
    double xp0, yp0;
} Geometry;

typedef struct {
    double halpha;
    double hbeta;
    double hhmax;
    double frac; /* Water diffusion constant */
    int iterout; /* Time interval for creating output maps [minutes] */
    int timesec; /* Time how long the simulation runs [minutes] */
    bool ts;     /* Time series output */
    double mintimestep;
} Settings;

typedef struct {
    int iterout;    /* Number of iterations for creating output maps */
    int miter;      /* Total number of iterations */
    double si0;     /* Mean rainfall excess (or sediment concentration?) */
    double sisum;   /* Sum of rainfall excess (or sediment concentration?) */
    double vmean;   /* Mean velocity */
    double infmean; /* Mean infiltration */
    double timec;
    double deltap; /* Time step for water */
} Setup;

typedef struct {
    int nwalka;            /* Remaining walkers */
    int nstack;            /* Number of output walkers */
    struct point3D *stack; /* Output 3D walkers */
    int maxwa;
    double rwalk;
    int nwalk;
} Simulation;

struct WaterParams {

    double rain_val;
    double manin_val;
    double infil_val;

    char *elevin;
    char *dxin;
    char *dyin;
    char *rain;
    char *infil;
    char *traps;
    char *manin;
    char *depth;
    char *disch;
    char *err;
    char *outwalk;
    char *observation;
    char *logfile;
    char *mapset;
    char *tserie;

    char *wdepth;
    char *detin;
    char *tranin;
    char *tauin;
    char *tc;
    char *et;
    char *conc;
    char *flux;
    char *erdep;

    char *rainval;
    char *maninval;
    char *infilval;
};

void WaterParams_init(struct WaterParams *wp);
void init_library_globals(struct WaterParams *wp);
void alloc_grids_water(const Geometry *geometry);
void alloc_grids_sediment(const Geometry *geometry);
void init_grids_sediment(const Setup *setup, const Geometry *geometry);

int input_data(int rows, int cols, Simulation *sim);
int grad_check(Setup *setup, const Geometry *geometry,
               const Settings *settings);
void main_loop(const Setup *setup, const Geometry *geometry,
               const Settings *settings, Simulation *sim);
int output_data(int, double, const Setup *setup, const Geometry *geometry,
                const Settings *settings, const Simulation *sim);
int output_et(const Geometry *geometry);
void free_walkers(Simulation *sim);
void erod(double **, const Setup *setup, const Geometry *geometry);

struct options {
    struct Option *elevin, *dxin, *dyin, *rain, *infil, *traps, *manin,
        *observation, *depth, *disch, *err, *outwalk, *nwalk, *niter,
        *mintimestep, *outiter, *density, *diffc, *hmax, *halpha, *hbeta,
        *wdepth, *detin, *tranin, *tauin, *tc, *et, *conc, *flux, *erdep,
        *rainval, *maninval, *infilval, *logfile, *seed, *threads;
};

struct flags {
    struct Flag *tserie, *generateSeed;
};

#endif /* __SIMLIB_H__ */
