/****************************************************************************
 *
 * MODULE:       r.texture
 * AUTHOR(S):    Carmine Basco - basco@unisannio.it
 *               with hints from:
 *                         prof. Giulio Antoniol - antoniol@ieee.org
 *                         prof. Michele Ceccarelli - ceccarelli@unisannio.it

 * PURPOSE:      Intended to explain GRASS raster programming.
 *               Create map raster with textural features.
 *
 * COPYRIGHT:    (C) 2002 by University of Sannio (BN) - Italy
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted. This
 * software is provided "as is" without express or implied warranty.
 *
 *****************************************************************************/

#define PGM_MAXMAXVAL 255

typedef struct matvec {
    float **P_matrix;
    float **P_matrix0;
    float **P_matrix45;
    float **P_matrix90;
    float **P_matrix135;

    float *px, *py;
    float *Pxpys, *Pxpyd;

    int *tone;
    int Ng;
} matvec;

int bsearch_gray(int *array, int n, int val);

float **matrix(int nr, int nc);
float *vector(int n);

void alloc_vars(int, struct matvec *);
void dealloc_vars(struct matvec *);
int set_vars(struct matvec *, int **grays, int curr_row, int curr_col, int size,
             int offset, int t_d, int with_nulls);
int set_angle_vars(struct matvec *mv, int angle, int have_px, int have_py,
                   int have_pxpys, int have_pxpyd);
