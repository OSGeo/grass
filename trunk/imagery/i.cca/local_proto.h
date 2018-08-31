#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/raster.h>

/* matrix.c */
int product(double*, double, double**, int);
int setdiag(double*, int, double**);
int getsqrt(double**, int, double**, double**);
int solveq(double**, int, double**, double**);
int print_matrix(double **matrix, int bands);

/* stats.c */
int within(int, int, double*, double***, double**, int);
int between(int, int, double*, double**, double**, int);

/* transform.c */
int transform(int*, int*, int, int, double**, int, CELL*,
	      CELL*);

#endif /* __LOCAL_PROTO_H__ */
