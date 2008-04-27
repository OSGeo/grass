/*
 *  Calculates univariate statistics from the non-null cells
 *
 *   Copyright (C) 2004-2007 by the GRASS Development Team
 *   Author(s): Soeren Gebbert
 *              Based on r.univar from Hamish Bowman, University of Otago, New Zealand
 *              and Martin Landa
 *
 *      This program is free software under the GNU General Public
 *      License (>=v2). Read the file COPYING that comes with GRASS
 *      for details.
 *
 */

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/G3d.h>
#include <grass/glocale.h>

/*- Parameters and global variables -----------------------------------------*/
typedef struct
{
    double sum;
    double sumsq;
    double min;
    double max;
    unsigned int n_perc;
    int *perc;
    double sum_abs;
    int n;
    int size;
    DCELL *dcell_array;
    FCELL *fcell_array;
    CELL *cell_array;
    int map_type;
} univar_stat;

/* command line options are the same for raster and raster3d maps */
typedef struct
{
    struct Option *inputfile, *percentile;
    struct Flag *shell_style, *extended;
} param_type;

#ifdef MAIN
param_type param;
#else
extern param_type param;
#endif

/* fn prototypes */
void heapsort_double(double *data, int n);
void heapsort_float(float *data, int n);
void heapsort_int(int *data, int n);
int print_stats(univar_stat * stats);
univar_stat *create_univar_stat_struct(int map_type, int size, int n_perc);
void free_univar_stat_struct(univar_stat * stats);

#endif
