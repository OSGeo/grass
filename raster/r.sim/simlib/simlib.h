#ifndef __SIMLIB_H__
#define __SIMLIB_H__

/*! \file simlib.h
 * \brief This is the interface for the simlib (SIMWE) library.
 */
#include <stdbool.h>
#include <stdio.h>

#define NUM_THREADS "1"
#if defined(_OPENMP)
#include <omp.h>
#endif

typedef struct {
    int mx, my;                    // Number of columns and rows
    double xmin, xmax, ymin, ymax; // 0, stepx * mx, 0, stepy * my
    double miyy, mixx;             // south * conv, west * conv
    double step, stepx, stepy;     // Size of cell in meters
    double conv;                   // Units to meters factor
    double xp0, yp0;               // stepx / 2, stepy / 2;
} Geometry;

typedef struct {
    double halpha;      // Diffusion increase constant
    double hbeta;       // Weighting factor for water flow velocity vector
    double hhmax;       // Threshold water depth [m]
    double frac;        // Water diffusion constant
    int iterout;        // Time interval for creating output maps [minutes]
    int timesec;        // Time how long the simulation runs [minutes]
    bool ts;            // Time series output
    double mintimestep; // Minimum time step for the simulation [seconds]
} Settings;

typedef struct {
    int iterout;    // Number of iterations for creating output maps
    int miter;      // Total number of iterations
    double si0;     // Mean rainfall excess (or sediment concentration?)
    double sisum;   // Sum of rainfall excess (or sediment concentration?)
    double vmean;   // Mean velocity
    double infmean; // Mean infiltration
    double timec;   // Time coefficient
    double deltap;  // Time step for water
} Setup;

typedef struct {
    int nwalk;             // Number of initial walkers in a single block
    int nwalka;            // Remaining walkers in an iteration
    int nstack;            // Number of output walkers
    struct point3D *stack; // Output 3D walkers
    int maxwa;             // Number of input walkers per block
    double rwalk; // Number of input walkers per block as double precision

} Simulation;

typedef struct {
    double *x;         // x coor for each point
    double *y;         // y coor for each point
    int *cats;         // Category for each point
    int npoints;       // Number of observation points
    int npoints_alloc; // Number of allocated points
    FILE *output;      // Output file descriptor
    int is_open;       // Set to 1 if open, 0 if closed
    char *logfile;     // Log file name
    char *observation; // Observation file name
} ObservationPoints;

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
               const Settings *settings, Simulation *sim,
               ObservationPoints *points);
int output_data(int, double, const Setup *setup, const Geometry *geometry,
                const Settings *settings, const Simulation *sim);
int output_et(const Geometry *geometry);
void free_walkers(Simulation *sim);
void erod(double **, const Setup *setup, const Geometry *geometry);
void create_observation_points(ObservationPoints *points);

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
