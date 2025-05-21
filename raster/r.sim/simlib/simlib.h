#ifndef __SIMLIB_H__
#define __SIMLIB_H__

/*! \file simlib.h
 * \brief This is the interface for the simlib (SIMWE) library.
 */
#include <stdbool.h>
#include <stdio.h>

#define EPS         1.e-7
#define MAXW        7000000
#define UNDEF       -9999

#define NUM_THREADS "1"
#if defined(_OPENMP)
#include <omp.h>
#endif
#ifdef _MSC_VER
#undef min
#undef max
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
    double rwalk;      // Number of input walkers per block as double precision
    struct point3D *w; // Weight of walkers
    struct point2D *vavg; // Average velocity of walkers

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

typedef struct {
    char *rain;       // Rainfall excess raster name (water flow only)
    double rain_val;  // Rainfall excess value (water flow only)
    char *manin;      // Manning's n raster name
    double manin_val; // Manning's n value
    char *infil;      // Infiltration raster name (water flow only)
    double infil_val; // Infiltration value (water flow only)
    char *elevin;     // Elevation raster name
    char *dxin;       // Name of x-derivatives raster map
    char *dyin;       // Name of y-derivatives raster map
    char *traps;      // Traps raster name (water flow only)
    char *wdepth;     // Water depth raster name (sediment only)
    char *detin;      // Detachment coefficient raster name (sediment only)
    char *tranin; // Transport capacity coefficient raster name (sediment only)
    char *tauin;  // Critical shear stress raster name (sediment only)
} Inputs;

typedef struct {
    char *depth;   // Water depth raster name (water flow only)
    char *disch;   // Discharge raster name (water flow only)
    char *err;     // Error raster name (water flow only)
    char *outwalk; // Output walker map name
    char *tc;      // Transport capacity raster name (sediment only)
    char *et;   // Transport limited erosion/deposition map name (sediment only)
    char *conc; // Sediment concentration raster name (sediment only)
    char *flux; // Sediment flux raster name (sediment only)
    char *erdep; // Erosion/deposition raster name (sediment only)
} Outputs;

typedef struct {
    float **zz;      // Elevation [input]
    float **cchez;   // 1/mannings [input]
    double **v1;     // Velocity in x direction [input]
    double **v2;     // Velocity in y direction [input]
    double **slope;  // velocity gradient [input]
    double **gama;   // Walker weights [output]
    double **gammas; // Sum of gamas over blocks [output]
    double **si;     // Rainfall excess for water flow [input]
    double **inf;    // Infiltration rate for water flow  [input]
    double **sigma;  // [internal]
    float **dc;      // Detachment coefficient [input]
    float **tau;     // Critical shear stress [input]
    float **er;      // Erosion [output]
    float **ct;      // Transport capacity coefficient [input]
    float **trap;    // Traps [input]
    float **dif;     // Diffusion coefficient [internal]
} Grids;

struct point2D {
    double x;
    double y;
};
struct point3D {
    double x;
    double y;
    double m;
};

void alloc_grids_water(const Geometry *geometry, const Outputs *outputs,
                       Grids *grids);
void alloc_grids_sediment(const Geometry *geometry, const Outputs *outputs,
                          Grids *grids);
void init_grids_sediment(const Setup *setup, const Geometry *geometry,
                         const Outputs *outputs, Grids *grids);

int input_data(int rows, int cols, Simulation *sim, const Inputs *inputs,
               const Outputs *outputs, Grids *grids);
int grad_check(Setup *setup, const Geometry *geometry, const Settings *settings,
               const Inputs *inputs, const Outputs *outputs, Grids *grids);
void main_loop(const Setup *setup, const Geometry *geometry,
               const Settings *settings, Simulation *sim,
               ObservationPoints *points, const Inputs *inputs,
               const Outputs *outputs, Grids *grids);
int output_data(int, double, const Setup *setup, const Geometry *geometry,
                const Settings *settings, const Simulation *sim,
                const Inputs *inputs, const Outputs *outputs,
                const Grids *grids);
int output_et(const Geometry *geometry, const Outputs *outputs,
              const Grids *grids);
void free_walkers(Simulation *sim, const char *outwalk);
void erod(double **, const Setup *setup, const Geometry *geometry,
          Grids *grids);
void create_observation_points(ObservationPoints *points);

double simwe_rand(void);
double gasdev(void);
void gasdev_for_paralel(double *, double *);
double amax1(double, double);
double amin1(double, double);
int min(int, int);
int max(int, int);

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
