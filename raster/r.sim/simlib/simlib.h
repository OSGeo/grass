#ifndef __SIMLIB_H__
#define __SIMLIB_H__

/*! \file simlib.h
 * \brief This is the interface for the simlib (SIMWE) library.
 */


#define NUM_THREADS "1"
#if defined(_OPENMP)
#include <omp.h>
#endif

struct WaterParams
{
    double xmin, ymin, xmax, ymax;
    double mayy, miyy, maxx, mixx;
    int mx, my;
    int mx2, my2;

    double bxmi, bymi, bxma, byma, bresx, bresy;
    int maxwab;
    double step, conv;

    double frac;

    double hbeta;
    double hhmax, sisum, vmean;
    double infsum, infmean;
    int maxw, maxwa, nwalk;
    double rwalk, xrand, yrand;
    double stepx, stepy, xp0, yp0;
    double chmean, si0, deltap, deldif, cch, hhc, halpha;
    double eps;
    int nstack; 
    int iterout, mx2o, my2o;
    int miter, nwalka;
    double timec;
    int ts, timesec;

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
    char *mscale;
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
void alloc_grids_water();
void alloc_grids_sediment();
void init_grids_sediment();

int input_data(void);
int grad_check(void);
void main_loop(void);
int output_data(int, double);

struct options
{
    struct Option *elevin, *dxin, *dyin, *rain, *infil, *traps, *manin,
	*observation, *depth, *disch, *err, *outwalk, *nwalk, *niter, *outiter,
	*density, *diffc, *hmax, *halpha, *hbeta, *wdepth, *detin, *tranin,
	*tauin, *tc, *et, *conc, *flux, *erdep, *rainval, *maninval,
	*infilval, *logfile, *seed, *threads;
};

struct flags
{
    struct Flag *mscale, *tserie, *generateSeed;
};

#endif /* __SIMLIB_H__ */
