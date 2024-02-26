/*
 * r.smooth
 *
 *   Copyright 2024 by Maris Nartiss, and The GRASS Development Team
 *   Author: Maris Nartiss
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

typedef struct {
    int nrows;
    int ncols;
    double **cells;
} all_cells;

/* anisotropic_diffusion.c */
double diffuse(int, float, float, double, double, double, double, double,
               double, double, double, double);
