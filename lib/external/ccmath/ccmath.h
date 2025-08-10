/*  ccmath.h    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 *
 * Modified by Soeren gebbert 2009/01/08
 * Removed al unused functions in GRASS. Only the linear algebra
 * functions are used.
 * ------------------------------------------------------------------------
 */
/*
   CCM

   Numerical Analysis Toolkit Header File
   ELF Shared Library Version
 */
/* Required for Shared Library */
#ifndef _CCMATH_H_
#define _CCMATH_H_
#define XMATH 1

/* Define File Pointers and Standard Library */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Definitions of Types */

#ifndef NULL
#define NULL ((void *)0
#endif

/* Complex Types */
#ifndef CPX
#ifndef _MSC_VER
struct complex {
    double re, im;
};
typedef struct complex Cpx;
#else
/* _MSVC has complex struct and cannot be used */
struct gcomplex {
    double re, im;
};
typedef struct gcomplex Cpx;
#endif /* _MSC_VER */

#define CPX 1
#endif

/*   Linear Algebra     */

/* Real Linear Systems */

int minv(double *a, int n);

int psinv(double *v, int n);

int ruinv(double *a, int n);

int solv(double *a, double *b, int n);

int solvps(double *s, double *x, int n);

int solvru(double *a, double *b, int n);

void solvtd(double *a, double *b, double *c, double *x, int m);

void eigen(double *a, double *eval, int n);

void eigval(double *a, double *eval, int n);

double evmax(double *a, double *u, int n);

int svdval(double *d, double *a, int m, int n);

int sv2val(double *d, double *a, int m, int n);

int svduv(double *d, double *a, double *u, int m, double *v, int n);

int sv2uv(double *d, double *a, double *u, int m, double *v, int n);

int svdu1v(double *d, double *a, int m, double *v, int n);

int sv2u1v(double *d, double *a, int m, double *v, int n);

void mmul(double *mat, double *a, double *b, int n);

void rmmult(double *mat, double *a, double *b, int m, int k, int n);

void vmul(double *vp, double *mat, double *v, int n);

double vnrm(double *u, double *v, int n);

void matprt(double *a, int n, int m, char *fmt);

void fmatprt(FILE *fp, double *a, int n, int m, char *fmt);

void trnm(double *a, int n);

void mattr(double *a, double *b, int m, int n);

void otrma(double *at, double *u, double *a, int n);

void otrsm(double *st, double *u, double *s0, int n);

void mcopy(double *a, double *b, int m);

void ortho(double *evc, int n);

void smgen(double *a, double *eval, double *evec, int n);

/* utility routines for real symmertic eigensystems */

void house(double *a, double *d, double *ud, int n);

void housev(double *a, double *d, double *ud, int n);

int qreval(double *eval, double *ud, int n);

int qrevec(double *eval, double *evec, double *dp, int n);

/* utility routines for singular value decomposition */

int qrbdi(double *d, double *e, int n);

int qrbdv(double *d, double *e, double *u, int m, double *v, int n);

int qrbdu1(double *d, double *e, double *u, int m, double *v, int n);

void ldumat(double *a, double *u, int m, int n);

void ldvmat(double *a, double *v, int n);

void atou1(double *a, int m, int n);

void atovm(double *v, int n);

/* Complex Matrix Algebra */

int cminv(Cpx *a, int n);

int csolv(Cpx *a, Cpx *b, int n);

void heigvec(Cpx *a, double *eval, int n);

void heigval(Cpx *a, double *eval, int n);

double hevmax(Cpx *a, Cpx *u, int n);

void cmmul(Cpx *c, Cpx *a, Cpx *b, int n);

void cmmult(Cpx *c, Cpx *a, Cpx *b, int m, int k, int n);

void cvmul(Cpx *vp, Cpx *mat, Cpx *v, int n);

Cpx cvnrm(Cpx *u, Cpx *v, int n);

void cmprt(Cpx *a, int n, int m, char *fmt);

void trncm(Cpx *a, int n);

void hconj(Cpx *u, int n);

void cmattr(Cpx *a, Cpx *b, int m, int n);

void utrncm(Cpx *at, Cpx *u, Cpx *a, int n);

void utrnhm(Cpx *ht, Cpx *u, Cpx *h0, int n);

void cmcpy(Cpx *a, Cpx *b, int n);

void unitary(Cpx *u, int n);

void hmgen(Cpx *h, double *eval, Cpx *u, int n);

int csolv(Cpx *a, Cpx *b, int n);

/* utility routines for hermitian eigen problems */

void chouse(Cpx *a, double *d, double *ud, int n);

void chousv(Cpx *a, double *d, double *ud, int n);

void qrecvc(double *eval, Cpx *evec, double *ud, int n);
#endif
