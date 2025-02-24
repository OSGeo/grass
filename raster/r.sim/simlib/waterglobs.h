#ifndef __WATERGLOBS_H__
#define __WATERGLOBS_H__

#define EPS   1.e-7
#define MAXW  7000000
#define UNDEF -9999

#include <grass/raster.h>
#ifdef _MSC_VER
#undef min
#undef max
#endif

extern char *depth;
extern char *disch;
extern char *err;
extern char *outwalk;

extern char *tc;
extern char *et;
extern char *conc;
extern char *flux;
extern char *erdep;

struct point2D {
    double x;
    double y;
};
struct point3D {
    double x;
    double y;
    double m;
};

extern double simwe_rand(void);
extern double gasdev(void);
extern void gasdev_for_paralel(double *, double *);
extern double amax1(double, double);
extern double amin1(double, double);
extern int min(int, int);
extern int max(int, int);

extern float **zz, **cchez;
extern double **v1, **v2, **slope;
extern double **gama, **gammas, **si, **inf, **sigma;
extern float **dc, **tau, **er, **ct, **trap;
extern float **dif;

extern struct point3D *w;
extern struct point2D *vavg;

extern struct History history; /* holds meta-data (title, comments,..) */

#endif /* __WATERGLOBS_H__ */
