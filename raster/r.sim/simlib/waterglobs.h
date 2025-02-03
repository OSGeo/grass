#ifndef __WATERGLOBS_H__
#define __WATERGLOBS_H__

#define EPS   1.e-7
#define MAXW  7000000
#define UNDEF -9999

#include <grass/raster.h>

extern char *elevin;
extern char *dxin;
extern char *dyin;
extern char *rain;
extern char *infil;
extern char *traps;
extern char *manin;
extern char *depth;
extern char *disch;
extern char *err;
extern char *outwalk;
extern char *observation;
extern char *logfile;
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

struct _points {
    double *x;         /* x coor for each point */
    double *y;         /* y coor for each point */
    int *cats;         /* Category for each point */
    int npoints;       /* Number of observation points */
    int npoints_alloc; /* Number of allocated points */
    FILE *output;      /* Output file descriptor */
    int is_open;       /* Set to 1 if open, 0 if closed */
};

struct point2D {
    double x;
    double y;
};
struct point3D {
    double x;
    double y;
    double m;
};

extern struct _points points;
extern double simwe_rand(void);
extern double gasdev(void);
extern void gasdev_for_paralel(double *, double *);
extern double amax1(double, double);
extern double amin1(double, double);
extern int min(int, int);
extern int max(int, int);
extern void create_observation_points(void);

extern float **zz, **cchez;
extern double **v1, **v2, **slope;
extern double **gama, **gammas, **si, **inf, **sigma;
extern float **dc, **tau, **er, **ct, **trap;
extern float **dif;

extern struct point3D *w;
extern struct point2D *vavg;

extern double rain_val;
extern double manin_val;
extern double infil_val;

extern struct History history; /* holds meta-data (title, comments,..) */

#endif /* __WATERGLOBS_H__ */
