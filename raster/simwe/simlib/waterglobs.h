#ifndef __WATERGLOBS_H__
#define __WATERGLOBS_H__

#define EPS     1.e-7
#define MAXW    7000000
#define UNDEF	-9999

#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

/*
GLOBAL FILE *fdelevin, *fddxin, *fddyin, *fdrain, *fdinfil, *fdtraps,
    *fdmanin, *fddepth, *fddisch, *fderr, *fdoutwalk, *fdwalkers;
*/
GLOBAL FILE *fdelevin, *fddxin, *fddyin, *fdrain, *fdinfil, *fdtraps,
    *fdmanin, *fddepth, *fddisch, *fderr;
GLOBAL FILE *fdwdepth, *fddetin, *fdtranin, *fdtauin, *fdtc, *fdet, *fdconc,
    *fdflux, *fderdep;
GLOBAL FILE *fdsfile, *fw;

GLOBAL char *elevin;
GLOBAL char *dxin;
GLOBAL char *dyin;
GLOBAL char *rain;
GLOBAL char *infil;
GLOBAL char *traps;
GLOBAL char *manin;
/* GLOBAL char *sfile; */
GLOBAL char *depth;
GLOBAL char *disch;
GLOBAL char *err;
/* GLOBAL char *outwalk; */
GLOBAL char *mapset;
GLOBAL char *mscale;
GLOBAL char *tserie;

GLOBAL char *wdepth;
GLOBAL char *detin;
GLOBAL char *tranin;
GLOBAL char *tauin;
GLOBAL char *tc;
GLOBAL char *et;
GLOBAL char *conc;
GLOBAL char *flux;
GLOBAL char *erdep;

GLOBAL char *rainval;
GLOBAL char *maninval;
GLOBAL char *infilval;

GLOBAL struct
{
    struct Option *elevin, *dxin, *dyin, *rain, *infil, *traps, *manin,
	*sfile, *depth, *disch, *err, *outwalk, *nwalk, *niter, *outiter,
	*density, *diffc, *hmax, *halpha, *hbeta, *wdepth, *detin, *tranin,
	*tauin, *tc, *et, *conc, *flux, *erdep, *rainval, *maninval,
	*infilval;
} parm;


GLOBAL struct
{
    struct Flag *mscale, *tserie;
} flag;


GLOBAL struct
{
    long int is1, is2;
} seed;


GLOBAL struct Cell_head cellhd;

struct Point
{
    double north, east;
    double z1;
};

/*
GLOBAL struct Point *points;
GLOBAL int npoints;
GLOBAL int npoints_alloc;
*/

GLOBAL int input_data(void);
GLOBAL int seeds(long int, long int);
GLOBAL int seedg(long int, long int);
GLOBAL int grad_check(void);
GLOBAL void erod(double **);
GLOBAL void main_loop(void);
GLOBAL int output_data(int, double);
GLOBAL int output_et(void);
GLOBAL double ulec(void);
GLOBAL double gasdev(void);
GLOBAL double amax1(double, double);
GLOBAL double amin1(double, double);
GLOBAL int min(int, int);
GLOBAL int max(int, int);

GLOBAL double xmin, ymin, xmax, ymax;
GLOBAL double mayy, miyy, maxx, mixx;
GLOBAL int mx, my;
GLOBAL int mx2, my2;

GLOBAL double bxmi, bymi, bxma, byma, bresx, bresy;
GLOBAL int maxwab;
GLOBAL double step, conv;

GLOBAL double frac;
GLOBAL double bxmi, bymi;

GLOBAL float **zz, **cchez;
GLOBAL double **v1, **v2, **slope;
GLOBAL double **gama, **gammas, **si, **inf, **sigma;
GLOBAL float **dc, **tau, **er, **ct, **trap;
GLOBAL float **dif;

/* GLOBAL double vavg[MAXW][2], stack[MAXW][3], w[MAXW][3]; */
GLOBAL double vavg[MAXW][2], w[MAXW][3];
GLOBAL int iflag[MAXW];

GLOBAL double hbeta;
/* GLOBAL int ldemo; */
GLOBAL double hhmax, sisum, vmean;
GLOBAL double infsum, infmean;
GLOBAL int maxw, maxwa, nwalk;
GLOBAL double rwalk, bresx, bresy, xrand, yrand;
GLOBAL double stepx, stepy, xp0, yp0;
GLOBAL double chmean, si0, deltap, deldif, cch, hhc, halpha;
GLOBAL double eps;
/* GLOBAL int maxwab, nstack; */
GLOBAL int maxwab;
GLOBAL int iterout, mx2o, my2o;
GLOBAL int miter, nwalka, lwwfin;
GLOBAL double timec;
GLOBAL int ts, timesec;

GLOBAL double rain_val;
GLOBAL double manin_val;
GLOBAL double infil_val;

GLOBAL struct History history;	/* holds meta-data (title, comments,..) */

#endif /* __WATERGLOBS_H__ */
