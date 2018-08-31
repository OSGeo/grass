/*!
 * \file qtree.c
 *
 * \author
 * H. Mitasova, I. Kosinovsky, D. Gerdes, Fall 1993,
 * University of Illinois and
 * US Army Construction Engineering Research Lab
 *
 * \author H. Mitasova (University of Illinois),
 * \author I. Kosinovsky, (USA-CERL)
 * \author D.Gerdes (USA-CERL)
 *
 * \author modified by H. Mitasova, November 1996 (include variable smoothing)
 *
 * \copyright
 * (C) 1993-1996 by Helena Mitasova and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the
 * GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS for details.
 */


#ifndef DATAQUAD_H

#define DATAQUAD_H

#define NW   1
#define NE   2
#define SW   3
#define SE   4


/*!
 * Point structure to keep coordinates
 *
 * It also contains smoothing for the given point.
 */
struct triple
{
    double x;
    double y;
    double z;
    double sm;  /*!< variable smoothing */
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
