#include <grass/raster.h>

/* alloc.c */
double *vector(int, int);
double **matrix(int, int, int, int);
int free_vector(double *, int, int);
int free_matrix(double **, int, int, int, int);

/* can_invert.c */
int can_invert(double **, int);

/* copy.c */
int copy_covariances(double **, double **, int);

/* eigen.c */
int eigen(double **, double *, int);
int tqli(double[], double[], int, double **);
int tred2(double **, int, double[], double[]);

/* lookup_class.c */
int lookup_class(CELL *, int, CELL *, int, CELL *);
