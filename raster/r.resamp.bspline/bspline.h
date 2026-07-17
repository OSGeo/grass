/***********************************************************************
 *
 * MODULE:       v.surf.bspline
 *
 * AUTHOR(S):    Roberto Antolin
 *
 * PURPOSE:      Spline Interpolation and cross correlation
 *
 * SPDX-FileCopyrightText: 2006 Politecnico di Milano - Polo Regionale di Como
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 **************************************************************************/

/*INCLUDES*/
#include <grass/lidar.h>
/*STRUCTURES*/
struct Stats {
    double *estima;
    double *error;
    int n_points;
};

struct Param {
    struct Option *in, *in_ext, *out, *out_map, *dbdriver, *dbdatabase, *stepE,
        *stepN, *lambda_f, *type;
    struct Flag *cross_corr;
};

struct RasterPoint {
    int row;
    int col;
};

/* resamp.c */
struct Point *P_Read_Raster_Region_masked(SEGMENT *,          /**/
                                          struct Cell_head *, /**/
                                          struct bound_box,   /**/
                                          struct bound_box,   /**/
                                          int *, /**/ int, /**/ double);
int P_Sparse_Raster_Points(SEGMENT *,          /**/
                           struct Cell_head *, /**/
                           struct Cell_head *, /**/
                           struct bound_box,   /**/
                           struct bound_box,   /**/
                           struct Point *,     /**/
                           double *,           /**/
                           double,             /**/
                           double,             /**/
                           double,             /**/
                           int,                /**/
                           int, /**/ int, /**/ int, /**/ double /**/);
int align_elaboration_box(struct Cell_head *, struct Cell_head *, int);
int align_interp_boxes(struct bound_box *, struct bound_box *,
                       struct Cell_head *, struct bound_box, struct bound_box,
                       int);

/* crosscor.c */
int cross_correlation(SEGMENT *, struct Cell_head *, double, double);
