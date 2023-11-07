#ifndef __USER_H__
#define __USER_H__

#include <grass/vector.h>
#include "points.h"

int translate_oct(struct octtree *, double, double, double, double);
int interp_call(struct octtree *, struct octtree *);
int INPUT(struct Map_info *, char *, char *, char *);
int OUTGR(void);
int min1(int, int);
int max1(int, int);
double amax1(double, double);
double amin1(double, double);
double erfr(double);
double crs(double);
void crs_full(double, double, double *, double *, double *, double *);
int COGRR1(double, double, double, int, int, int, int, struct quadruple *,
           struct point_3d);
int POINT(int, struct quadruple *, struct point_3d);
int LINEQS(int, int, int, int *, double *);
void clean(void);
int point_save(double, double, double, double);

#endif
