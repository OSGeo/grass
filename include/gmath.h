
/******************************************************************************
 * gmath.h
 * Top level header file for gmath units

 * @Copyright David D.Gray <ddgray@armadce.demon.co.uk>
 * 27th. Sep. 2000
 * Last updated: 2007-08-26
 *

 * This file is part of GRASS GIS. It is free software. You can 
 * redistribute it and/or modify it under the terms of 
 * the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 ******************************************************************************/

#ifndef GMATH_H_
#define GMATH_H_

#include <grass/config.h>
#if defined(HAVE_LIBLAPACK) && defined(HAVE_LIBBLAS) && defined(HAVE_G2C_H)
 /* only include if available */
#include <grass/la.h>
#endif
#include <stddef.h>

/* fft.c */
int fft(int, double *[2], int, int, int);
int fft2(int, double (*)[2], int, int, int);

/* gauss.c */
double G_math_rand_gauss(int, double);

/* max_pow2.c */
long G_math_max_pow2(long);
long G_math_min_pow2(long);

/* rand1.c */
float G_math_rand(int);

/* del2g.c */
int del2g(double *[2], int, double);

/* findzc.c */
int G_math_findzc(double[], int, double[], double, int);

/* getg.c */
int getg(double, double *[2], int);

/* eigen.c */
int eigen(double **, double **, double *, int);
int egvorder2(double *, double **, long);
int transpose2(double **, long);

/* jacobi.c */
#define MX 9
int jacobi(double[MX][MX], long, double[MX], double[MX][MX]);
int egvorder(double[MX], double[MX][MX], long);
int transpose(double[MX][MX], long);

/* mult.c */
int mult(double *v1[2], int size1, double *v2[2], int size2, double *v3[2],
	 int size3);

/* dalloc.c */
double *G_alloc_vector(size_t);
double **G_alloc_matrix(int, int);
float *G_alloc_fvector(size_t);
float **G_alloc_fmatrix(int, int);
void G_free_vector(double *);
void G_free_matrix(double **);
void G_free_fvector(float *);
void G_free_fmatrix(float **);

/* eigen_tools.c */
int G_tqli(double[], double[], int, double **);
void G_tred2(double **, int, double[], double[]);

/* ialloc.c */
int *G_alloc_ivector(size_t);
int **G_alloc_imatrix(int, int);
void G_free_ivector(int *);
void G_free_imatrix(int **);

/* lu.c */
int G_ludcmp(double **, int, int *, double *);
void G_lubksb(double **, int, int *, double[]);

/* svd.c */
int G_svdcmp(double **, int, int, double *, double **);
int G_svbksb(double **, double[], double **, int, int, double[], double[]);
int G_svelim(double *, int);

#endif /* GMATH_H_ */
