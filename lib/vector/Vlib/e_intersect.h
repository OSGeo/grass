#ifndef GRASS_E_INTERSECT_H
#define GRASS_E_INTERSECT_H

#define FZERO(X, TOL) (fabs(X)<TOL)
#define FEQUAL(X, Y, TOL) (fabs(X-Y)<TOL)

/*int segment_intersection_2d_e(double ax1, double ay1, double ax2, double ay2, double bx1, double by1, double bx2, double by2,
    double *x1, double *y1, double *x2, double *y2);
int segment_intersection_2d_test(double ax1, double ay1, double ax2, double ay2, double bx1, double by1, double bx2, double by2,
    double *x1, double *y1, double *x2, double *y2);*/

int segment_intersection_2d_tol(double ax1, double ay1, double ax2, double ay2, double bx1, double by1, double bx2, double by2,
    double *x1, double *y1, double *x2, double *y2, double tol);

int segment_intersection_2d(double ax1, double ay1, double ax2, double ay2, double bx1, double by1, double bx2, double by2,
    double *x1, double *y1, double *x2, double *y2);

    
int almost_equal(double a, double b, int bits);

#endif
