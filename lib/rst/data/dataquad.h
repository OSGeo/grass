
/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1992
 * University of Illinois
 * US Army Construction Engineering Research Lab  
 * Copyright 1992, H. Mitasova (University of Illinois),
 * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)   
 *
 * Modified by H.Mitasova November 1996 to include variable smoothing
 */


#ifndef DATAQUAD_H

#define DATAQUAD_H

#define NW   1
#define NE   2
#define SW   3
#define SE   4

struct triple
{
    double x;
    double y;
    double z;
    double sm;			/* structure extended to incl. variable smoothing */
};

struct quaddata
{
    double x_orig;
    double y_orig;
    double xmax;
    double ymax;
    int n_rows;
    int n_cols;
    int n_points;
    struct triple *points;
};

struct triple *quad_point_new(double, double, double, double);
struct quaddata *quad_data_new(double, double, double, double, int, int, int,
			       int);
int quad_compare(struct triple *, struct quaddata *);
int quad_add_data(struct triple *, struct quaddata *, double);
int quad_intersect(struct quaddata *, struct quaddata *);
int quad_division_check(struct quaddata *, int);
struct quaddata **quad_divide_data(struct quaddata *, int, double);
int quad_get_points(struct quaddata *, struct quaddata *, int);

#endif
