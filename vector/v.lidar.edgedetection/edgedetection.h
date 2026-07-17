/**************************************************************
 *
 * MODULE:       v.lidar.edgedetection
 *
 * AUTHOR(S):    Original version in GRASS 5.4 (s.edgedetection):
 *                 Maria Antonia Brovelli, Massimiliano Cannata,
 *                 Ulisse Longoni and Mirko Reguzzoni
 *
 *               Update for GRASS 6.X and improvements:
 *                 Roberto Antolin and Gonzalo Moreno
 *
 * PURPOSE:      Detection of object's edges on a LIDAR data set
 *
 * SPDX-FileCopyrightText: 2006 Politecnico di Milano - Polo Regionale di Como
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 **************************************************************/

/*INCLUDES*/
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/lidar.h>

/*--------------------------------------------------------------------------*/
int edge_detection(struct Cell_head, /**/
                   struct bound_box, /**/
                   double *,         /**/
                   double,           /**/
                   double,           /**/
                   double *,         /**/
                   double, /**/ double, /**/ double, /**/ double /**/);

int Get_Gradient(struct Cell_head, /**/
                 double, /**/ double, /**/ double *, /**/ double *);

void classification(struct Map_info *, /**/
                    struct Cell_head,  /**/
                    struct bound_box,  /**/
                    struct bound_box,  /**/
                    double **,         /**/
                    double *,          /**/
                    double *,          /**/
                    double,            /**/
                    double,            /**/
                    double,            /**/
                    double,            /**/
                    double,            /**/
                    int *, /**/ int, /**/ dbDriver *, /**/ char *,
                    /**/ char *);

int Insert(double, /**/ double, /**/ double, /**/ int, /**/ dbDriver *,
           /**/ char *);

int UpDate(double, /**/ double, /**/ double, /**/ int, /**/ dbDriver *,
           /**/ char *);

int Select(double *, /**/ double *, /**/ double *, /**/ int, /**/ dbDriver *,
           /**/ char *);

int Insert_Interpolation(double, int, dbDriver *, char *);
