#define EPS     1.e-7
#define MAXW    7000000
#define UNDEF	-9999

#ifdef MAIN
FILE *fdelevin, *fddxin, *fddyin, *fdrain, *fdinfil, *fdtraps, *fdmanin, *fddepth, *fddisch, *fderr, *fdoutwalk,*fdwalkers;
FILE *fdwdepth,*fddetin,*fdtranin,*fdtauin, *fdtc, *fdet, *fdconc, *fdflux,*fderdep;
FILE *fdsfile,*fw;

char *elevin;
char *dxin;
char *dyin;
char *rain;
char *infil;
char *traps;
char *manin;
char *sfile;
char *depth;
char *disch;
char *err;
char *outwalk;
char *mapset;

/* Flag*/
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

/*New Input val*/
char *rainval;
char *maninval;
char *infilval;

  struct
  {
    struct Option *elevin,*dxin,*dyin,*rain,*infil,*traps,*manin,*sfile,*depth,*disch,*err,
*outwalk,*nwalk,*niter,*outiter,*density,*diffc,*hmax,*halpha,*hbeta,*wdepth,
*detin,*tranin,*tauin,*tc,*et,*conc,*flux,*erdep,*rainval,*maninval,*infilval;
  } parm;

  struct
  {
    struct Flag *tserie;
  } flag;


struct {
    long int is1, is2;
} seed;

struct Cell_head cellhd;

struct Point
{
    double north, east;
    double z1;
};
struct Point *points;
int npoints;
int npoints_alloc;


int input_data(void);
int seeds(long int,long int);
int seedg(long int,long int);
int grad_check(void);
void erod(double **);
void main_loop(void);
int output_data(int,double);
int output_et(void);
double ulec(void);
double gasdev(void);
double amax1(double,double);
double amin1(double,double);
int min(int,int);
int max(int,int);

double xmin, ymin, xmax, ymax;
double mayy, miyy, maxx, mixx;
int mx, my;
int mx2, my2;

/*double bxmi,bymi,bxma,byma,bresx,bresy;*/
int maxwab;
double step,conv;

double frac;
double bxmi, bymi;

float **zz, **cchez;
double **v1, **v2, **slope;
double **gama, **gammas,**si,**inf,**sigma;
float **dc,**tau,**er, **ct, **trap;
float  **dif;

double vavg[MAXW][2], stack[MAXW][3],w[MAXW][3];
int iflag[MAXW];

double hbeta;
int ldemo;
double hhmax, sisum,vmean;
double infsum,infmean;
int maxw, maxwa, nwalk;
double rwalk, bresx, bresy, xrand, yrand;
double stepx, stepy, xp0, yp0;
double chmean, si0, deltap, deldif, cch, hhc,halpha;
double eps;
int maxwab, nstack;
int iterout, mx2o, my2o;
int miter,nwalka,lwwfin;
double timec;
int ts, timesec;

double rain_val;
double manin_val;
double infil_val;

#else
extern FILE *fdelevin, *fddxin, *fddyin, *fdrain, *fdinfil, *fdtraps, *fdmanin, *fddepth, *fddisch, *fderr, *fdoutwalk,*fdwalkers;
extern FILE *fdwdepth,*fddetin,*fdtranin,*fdtauin, *fdtc, *fdet, *fdconc, *fdflux,*fderdep;
extern FILE *fdsfile,*fw;

extern char *elevin;
extern char *dxin;
extern char *dyin;
extern char *rain;
extern char *infil;
extern char *traps;
extern char *manin;
extern char *sfile;
extern char *depth;
extern char *disch;
extern char *err;
extern char *outwalk;
extern char *mapset;
extern char *tserie;

extern char *wdepth;
extern char *detin;
extern char *tranin;
extern char *tauin;
extern char *tc;
extern char *et;
extern char *conc;
extern char *flux;
extern char *erdep;

extern char *rainval;
extern char *maninval;
extern char *infilval;

extern struct
  {
    struct Option *elevin,*dxin,*dyin,*rain,*infil,*traps,*manin,*sfile,*depth,*disch,*err,
*outwalk,*nwalk,*niter,*outiter,*density,*diffc,*hmax,*halpha,*hbeta,*wdepth,
*detin,*tranin,*tauin,*tc,*et,*conc,*flux,*erdep,*rainval,*maninval,*infilval;
  } parm;

extern struct
  {
    struct Flag *tserie;
  } flag;


extern struct {
    long int is1, is2;
} seed;


extern struct Cell_head cellhd;

extern struct Point
{
    double north, east;
    double z1;
};
extern struct Point *points;
extern int npoints;
extern int npoints_alloc;


extern int input_data(void);
extern int seeds(long int,long int);
extern int seedg(long int,long int);
extern int grad_check(void);
extern void erod(double **);
extern void main_loop(void);
extern int output_data(int,double);
extern int output_et(void);
extern double ulec(void);
extern double gasdev(void);
extern double amax1(double,double);
extern double amin1(double,double);
extern int min(int,int);
extern int max(int,int);

extern double xmin, ymin, xmax, ymax;
extern double mayy, miyy, maxx, mixx;
extern int mx, my;
extern int mx2, my2;

/*extern double bxmi,bymi,bxma,byma,bresx,bresy;*/
extern int maxwab;
extern double step,conv;

extern double frac;
extern double bxmi, bymi;

extern float **zz, **cchez;
extern double **v1, **v2, **slope;
extern double **gama, **gammas,**si,**inf,**sigma;
extern float **dc,**tau,**er, **ct, **trap;
extern float  **dif;

extern double vavg[MAXW][2], stack[MAXW][3],w[MAXW][3];
extern int iflag[MAXW];

extern double hbeta;
extern int ldemo;
extern double hhmax, sisum,vmean;
extern double infsum,infmean;
extern int maxw, maxwa, nwalk;
extern double rwalk, bresx, bresy, xrand, yrand;
extern double stepx, stepy, xp0, yp0;
extern double chmean, si0, deltap, deldif, cch, hhc,halpha;
extern double eps;
extern int maxwab, nstack;
extern int iterout, mx2o, my2o;
extern int miter,nwalka,lwwfin;
extern double timec;
extern int ts, timesec;

extern double rain_val;
extern double manin_val;
extern double infil_val;
#endif
