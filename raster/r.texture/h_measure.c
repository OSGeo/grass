
/****************************************************************************
 *
 * MODULE:       r.texture
 * AUTHOR(S):    Carmine Basco - basco@unisannio.it
 *               with hints from: 
 * 			prof. Giulio Antoniol - antoniol@ieee.org
 * 			prof. Michele Ceccarelli - ceccarelli@unisannio.it
 *
 * PURPOSE:      Create map raster with textural features.
 *
 * COPYRIGHT:    (C) 2003 by University of Sannio (BN) - Italy 
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted. This 
 * software is provided "as is" without express or implied warranty.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#define RADIX 2.0
#define EPSILON 0.000000001

#define BL  "Direction             "
#define F1  "Angular Second Moment "
#define F2  "Contrast              "
#define F3  "Correlation           "
#define F4  "Variance              "
#define F5  "Inverse Diff Moment   "
#define F6  "Sum Average           "
#define F7  "Sum Variance          "
#define F8  "Sum Entropy           "
#define F9  "Entropy               "
#define F10 "Difference Variance   "
#define F11 "Difference Entropy    "
#define F12 "Measure of Correlation-1 "
#define F13 "Measure of Correlation-2 "
#define F14 "Max Correlation Coeff "

#define SIGN(x,y) ((y)<0 ? -fabs(x) : fabs(x))
#define SWAP(a,b) {y=(a);(a)=(b);(b)=y;}

#define PGM_MAXMAXVAL 255

void results(), hessenberg(), mkbalanced(), reduction(), simplesrt();
float f1_asm(), f2_contrast(), f3_corr(), f4_var(), f5_idm(),
f6_savg(), f7_svar(), f8_sentropy(), f9_entropy(), f10_dvar(),
f11_dentropy(), f12_icorr(), f13_icorr(), f14_maxcorr(), *vector(),
**matrix();


static float **P_matrix0 = NULL;
static float **P_matrix45 = NULL;
static float **P_matrix90 = NULL;
static float **P_matrix135 = NULL;

int tone[PGM_MAXMAXVAL + 1];

float h_measure(int **grays, int rows, int cols, int t_m, int t_d)
{
    int R0, R45, R90, angle, d, x, y;
    int row, col;
    int itone, jtone, tones;
    static int memory_all = 0;
    float sentropy[4];

    d = t_d;

    /* Determine the number of different gray scales (not maxval) */
    for (row = PGM_MAXMAXVAL; row >= 0; --row)
	tone[row] = -1;
    for (row = rows - 1; row >= 0; --row)
	for (col = 0; col < cols; ++col) {
	    if (grays[row][col] < 0)	/* No data pixel found */
		G_fatal_error(_("Negative or no data pixel found. "
				"This module is not yet able to process no data holes in a map, "
				"please fill with r.fillnulls or other algorithms"));
	    if (grays[row][col] > PGM_MAXMAXVAL)
		G_fatal_error(_("Too many categories (found: %i, max: %i). "
				"Try to rescale or reclassify the map"),
			      grays[row][col], PGM_MAXMAXVAL);
	    tone[grays[row][col]] = grays[row][col];
	}
    for (row = PGM_MAXMAXVAL, tones = 0; row >= 0; --row)
	if (tone[row] != -1)
	    tones++;

    /* Collapse array, taking out all zero values */
    for (row = 0, itone = 0; row <= PGM_MAXMAXVAL; row++)
	if (tone[row] != -1)
	    tone[itone++] = tone[row];
    /* Now array contains only the gray levels present (in ascending order) */

    /* Allocate memory for gray-tone spatial dependence matrix */

    if (!memory_all) {
	P_matrix0 = matrix(0, 512, 0, 512);
	P_matrix45 = matrix(0, 512, 0, 512);
	P_matrix90 = matrix(0, 512, 0, 512);
	P_matrix135 = matrix(0, 512, 0, 512);
	memory_all = 1;
    }
    for (row = 0; row < tones; ++row)
	for (col = 0; col < tones; ++col) {
	    P_matrix0[row][col] = P_matrix45[row][col] = 0;
	    P_matrix90[row][col] = P_matrix135[row][col] = 0;
	}

    /* Find gray-tone spatial dependence matrix */

    for (row = 0; row < rows; ++row)
	for (col = 0; col < cols; ++col)
	    for (x = 0, angle = 0; angle <= 135; angle += 45) {
		while (tone[x] != grays[row][col])
		    x++;
		if (angle == 0 && col + d < cols) {
		    y = 0;
		    while (tone[y] != grays[row][col + d])
			y++;
		    P_matrix0[x][y]++;
		    P_matrix0[y][x]++;
		}
		if (angle == 90 && row + d < rows) {
		    y = 0;
		    while (tone[y] != grays[row + d][col])
			y++;
		    P_matrix90[x][y]++;
		    P_matrix90[y][x]++;
		}
		if (angle == 45 && row + d < rows && col - d >= 0) {
		    y = 0;
		    while (tone[y] != grays[row + d][col - d])
			y++;
		    P_matrix45[x][y]++;
		    P_matrix45[y][x]++;
		}
		if (angle == 135 && row + d < rows && col + d < cols) {
		    y = 0;
		    while (tone[y] != grays[row + d][col + d])
			y++;
		    P_matrix135[x][y]++;
		    P_matrix135[y][x]++;
		}
	    }
    /* Gray-tone spatial dependence matrices are complete */

    /* Find normalizing constants */
    R0 = 2 * rows * (cols - 1);
    R45 = 2 * (rows - 1) * (cols - 1);
    R90 = 2 * (rows - 1) * cols;

    /* Normalize gray-tone spatial dependence matrix */
    for (itone = 0; itone < tones; ++itone)
	for (jtone = 0; jtone < tones; ++jtone) {
	    P_matrix0[itone][jtone] /= R0;
	    P_matrix45[itone][jtone] /= R45;
	    P_matrix90[itone][jtone] /= R90;
	    P_matrix135[itone][jtone] /= R45;
	}

    switch (t_m) {
    case 0:
	return (f1_asm(P_matrix0, tones));
	break;
    case 1:
	return (f1_asm(P_matrix45, tones));
	break;
    case 2:
	return (f1_asm(P_matrix90, tones));
	break;
    case 3:
	return (f1_asm(P_matrix135, tones));
	break;

    case 4:
	return (f2_contrast(P_matrix0, tones));
	break;
    case 5:
	return (f2_contrast(P_matrix45, tones));
	break;
    case 6:
	return (f2_contrast(P_matrix90, tones));
	break;
    case 7:
	return (f2_contrast(P_matrix135, tones));
	break;

    case 8:
	return (f3_corr(P_matrix0, tones));
	break;
    case 9:
	return (f3_corr(P_matrix45, tones));
	break;
    case 10:
	return (f3_corr(P_matrix90, tones));
	break;
    case 11:
	return (f3_corr(P_matrix135, tones));
	break;

    case 12:
	return (f4_var(P_matrix0, tones));
	break;
    case 13:
	return (f4_var(P_matrix45, tones));
	break;
    case 14:
	return (f4_var(P_matrix90, tones));
	break;
    case 15:
	return (f4_var(P_matrix135, tones));
	break;

    case 16:
	return (f5_idm(P_matrix0, tones));
	break;
    case 17:
	return (f5_idm(P_matrix45, tones));
	break;
    case 18:
	return (f5_idm(P_matrix90, tones));
	break;
    case 19:
	return (f5_idm(P_matrix135, tones));
	break;

    case 20:
	return (f6_savg(P_matrix0, tones));
	break;
    case 21:
	return (f6_savg(P_matrix45, tones));
	break;
    case 22:
	return (f6_savg(P_matrix90, tones));
	break;
    case 23:
	return (f6_savg(P_matrix135, tones));
	break;

    case 24:
	sentropy[0] = f8_sentropy(P_matrix0, tones);
	return (sentropy[0]);
	break;
    case 25:
	sentropy[1] = f8_sentropy(P_matrix45, tones);
	return (sentropy[1]);
	break;
    case 26:
	sentropy[2] = f8_sentropy(P_matrix90, tones);
	return (sentropy[2]);
	break;
    case 27:
	sentropy[3] = f8_sentropy(P_matrix135, tones);
	return (sentropy[3]);
	break;
    case 28:
	return (f7_svar(P_matrix0, tones, sentropy[0]));
	break;
    case 29:
	return (f7_svar(P_matrix45, tones, sentropy[1]));
	break;
    case 30:
	return (f7_svar(P_matrix90, tones, sentropy[2]));
	break;
    case 31:
	return (f7_svar(P_matrix135, tones, sentropy[3]));
	break;

    case 32:
	return (f9_entropy(P_matrix0, tones));
	break;
    case 33:
	return (f9_entropy(P_matrix45, tones));
	break;
    case 34:
	return (f9_entropy(P_matrix90, tones));
	break;
    case 35:
	return (f9_entropy(P_matrix135, tones));
	break;

    case 36:
	return (f10_dvar(P_matrix0, tones));
	break;
    case 37:
	return (f10_dvar(P_matrix45, tones));
	break;
    case 38:
	return (f10_dvar(P_matrix90, tones));
	break;
    case 39:
	return (f10_dvar(P_matrix135, tones));
	break;

    case 40:
	return (f11_dentropy(P_matrix0, tones));
	break;
    case 41:
	return (f11_dentropy(P_matrix45, tones));
	break;
    case 42:
	return (f11_dentropy(P_matrix90, tones));
	break;
    case 43:
	return (f11_dentropy(P_matrix135, tones));
	break;

    case 44:
	return (f12_icorr(P_matrix0, tones));
	break;
    case 45:
	return (f12_icorr(P_matrix45, tones));
	break;
    case 46:
	return (f12_icorr(P_matrix90, tones));
	break;
    case 47:
	return (f12_icorr(P_matrix135, tones));
	break;

    case 48:
	return (f13_icorr(P_matrix0, tones));
	break;
    case 49:
	return (f13_icorr(P_matrix45, tones));
	break;
    case 50:
	return (f13_icorr(P_matrix90, tones));
	break;
    case 51:
	return (f13_icorr(P_matrix135, tones));
	break;

    case 52:
	return (f14_maxcorr(P_matrix0, tones));
	break;
    case 53:
	return (f14_maxcorr(P_matrix45, tones));
	break;
    case 54:
	return (f14_maxcorr(P_matrix90, tones));
	break;
    case 55:
	return (f14_maxcorr(P_matrix135, tones));
	break;
    }
    exit(0);
}

void MatrixDealloc(float **A, int N)
{
    /*A is NxN */
    int i;

    for (i = 0; i < N; ++i)
	G_free(A[i]);
    G_free(A);
}
float f1_asm(float **P, int Ng)

/* Angular Second Moment */
{
    int i, j;
    float sum = 0;

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    sum += P[i][j] * P[i][j];

    return sum;

    /*
     * The angular second-moment feature (ASM) f1 is a measure of homogeneity
     * of the image. In a homogeneous image, there are very few dominant
     * gray-tone transitions. Hence the P matrix for such an image will have
     * fewer entries of large magnitude.
     */
}


float f2_contrast(float **P, int Ng)

/* Contrast */
{
    int i, j, n;
    float sum = 0, bigsum = 0;

    for (n = 0; n < Ng; ++n) {
	for (i = 0; i < Ng; ++i)
	    for (j = 0; j < Ng; ++j)
		if ((i - j) == n || (j - i) == n)
		    sum += P[i][j];
	bigsum += n * n * sum;

	sum = 0;
    }
    return bigsum;

    /*
     * The contrast feature is a difference moment of the P matrix and is a
     * measure of the contrast or the amount of local variations present in an
     * image.
     */
}

float f3_corr(float **P, int Ng)

/* Correlation */
{
    int i, j;
    float sum_sqrx = 0, sum_sqry = 0, tmp, *px;
    float meanx = 0, meany = 0, stddevx, stddevy;

    px = vector(0, Ng);
    for (i = 0; i < Ng; ++i)
	px[i] = 0;

    /*
     * px[i] is the (i-1)th entry in the marginal probability matrix obtained
     * by summing the rows of p[i][j]
     */
    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    px[i] += P[i][j];


    /* Now calculate the means and standard deviations of px and py */

    /*- fix supplied by J. Michael Christensen, 21 Jun 1991 */

    /*- further modified by James Darrell McCauley, 16 Aug 1991
     *     after realizing that meanx=meany and stddevx=stddevy
     */
    for (i = 0; i < Ng; ++i) {
	meanx += px[i] * i;
	sum_sqrx += px[i] * i * i;
    }
    meany = meanx;
    sum_sqry = sum_sqrx;
    stddevx = sqrt(sum_sqrx - (meanx * meanx));
    stddevy = stddevx;

    /* Finally, the correlation ... */
    for (tmp = 0, i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    tmp += i * j * P[i][j];

    G_free(px);

    return (tmp - meanx * meany) / (stddevx * stddevy);
    /*
     * This correlation feature is a measure of gray-tone linear-dependencies
     * in the image.
     */
}


float f4_var(float **P, int Ng)

/* Sum of Squares: Variance */
{
    int i, j;
    float mean = 0, var = 0;

    /*- Corrected by James Darrell McCauley, 16 Aug 1991
     *  calculates the mean intensity level instead of the mean of
     *  cooccurrence matrix elements
     */
    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    mean += i * P[i][j];

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    var += (i + 1 - mean) * (i + 1 - mean) * P[i][j];

    return var;
}

float f5_idm(float **P, int Ng)

/* Inverse Difference Moment */
{
    int i, j;
    float idm = 0;

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    idm += P[i][j] / (1 + (i - j) * (i - j));

    return idm;
}

float Pxpy[2 * PGM_MAXMAXVAL];

float f6_savg(float **P, int Ng)

/* Sum Average */
{
    int i, j;
    extern float Pxpy[2 * PGM_MAXMAXVAL];
    float savg = 0;

    for (i = 0; i <= 2 * Ng; ++i)
	Pxpy[i] = 0;

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    Pxpy[i + j + 2] += P[i][j];
    for (i = 2; i <= 2 * Ng; ++i)
	savg += i * Pxpy[i];

    return savg;
}


float f7_svar(float **P, int Ng, double S)

/* Sum Variance */
{
    int i, j;
    extern float Pxpy[2 * PGM_MAXMAXVAL];
    float var = 0;

    for (i = 0; i <= 2 * Ng; ++i)
	Pxpy[i] = 0;

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    Pxpy[i + j + 2] += P[i][j];

    for (i = 2; i <= 2 * Ng; ++i)
	var += (i - S) * (i - S) * Pxpy[i];

    return var;
}

float f8_sentropy(float **P, int Ng)

/* Sum Entropy */
{
    int i, j;
    extern float Pxpy[2 * PGM_MAXMAXVAL];
    float sentropy = 0;

    for (i = 0; i <= 2 * Ng; ++i)
	Pxpy[i] = 0;

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    Pxpy[i + j + 2] += P[i][j];

    for (i = 2; i <= 2 * Ng; ++i)
	sentropy -= Pxpy[i] * log10(Pxpy[i] + EPSILON);

    return sentropy;
}


float f9_entropy(float **P, int Ng)

/* Entropy */
{
    int i, j;
    float entropy = 0;

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    entropy += P[i][j] * log10(P[i][j] + EPSILON);

    return -entropy;
}


float f10_dvar(float **P, int Ng)

/* Difference Variance */
{
    int i, j, tmp;
    extern float Pxpy[2 * PGM_MAXMAXVAL];
    float sum = 0, sum_sqr = 0, var = 0;

    for (i = 0; i <= 2 * Ng; ++i)
	Pxpy[i] = 0;

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    Pxpy[abs(i - j)] += P[i][j];

    /* Now calculate the variance of Pxpy (Px-y) */
    for (i = 0; i < Ng; ++i) {
	sum += Pxpy[i];
	sum_sqr += Pxpy[i] * Pxpy[i];
    }
    tmp = Ng * Ng;
    var = ((tmp * sum_sqr) - (sum * sum)) / (tmp * tmp);

    return var;
}

float f11_dentropy(float **P, int Ng)

/* Difference Entropy */
{
    int i, j;
    extern float Pxpy[2 * PGM_MAXMAXVAL];
    float sum = 0;

    for (i = 0; i <= 2 * Ng; ++i)
	Pxpy[i] = 0;

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j)
	    Pxpy[abs(i - j)] += P[i][j];

    for (i = 0; i < Ng; ++i)
	sum += Pxpy[i] * log10(Pxpy[i] + EPSILON);

    return -sum;
}

float f12_icorr(float **P, int Ng)

/* Information Measures of Correlation */
{
    int i, j;
    float *px, *py;
    float hx = 0, hy = 0, hxy = 0, hxy1 = 0, hxy2 = 0;

    px = vector(0, Ng);
    py = vector(0, Ng);

    /*
     * px[i] is the (i-1)th entry in the marginal probability matrix obtained
     * by summing the rows of p[i][j]
     */
    for (i = 0; i < Ng; ++i) {
	for (j = 0; j < Ng; ++j) {
	    px[i] += P[i][j];
	    py[j] += P[i][j];
	}
    }

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j) {
	    hxy1 -= P[i][j] * log10(px[i] * py[j] + EPSILON);
	    hxy2 -= px[i] * py[j] * log10(px[i] * py[j] + EPSILON);
	    hxy -= P[i][j] * log10(P[i][j] + EPSILON);
	}

    /* Calculate entropies of px and py - is this right? */
    for (i = 0; i < Ng; ++i) {
	hx -= px[i] * log10(px[i] + EPSILON);
	hy -= py[i] * log10(py[i] + EPSILON);
    }
    G_free(px);
    G_free(py);
    /* fprintf(stderr,"hxy1=%f\thxy=%f\thx=%f\thy=%f\n",hxy1,hxy,hx,hy); */
    return ((hxy - hxy1) / (hx > hy ? hx : hy));
}

float f13_icorr(float **P, int Ng)

/* Information Measures of Correlation */
{
    int i, j;
    float *px, *py;
    float hx = 0, hy = 0, hxy = 0, hxy1 = 0, hxy2 = 0;

    px = vector(0, Ng);
    py = vector(0, Ng);

    /*
     * px[i] is the (i-1)th entry in the marginal probability matrix obtained
     * by summing the rows of p[i][j]
     */
    for (i = 0; i < Ng; ++i) {
	for (j = 0; j < Ng; ++j) {
	    px[i] += P[i][j];
	    py[j] += P[i][j];
	}
    }

    for (i = 0; i < Ng; ++i)
	for (j = 0; j < Ng; ++j) {
	    hxy1 -= P[i][j] * log10(px[i] * py[j] + EPSILON);
	    hxy2 -= px[i] * py[j] * log10(px[i] * py[j] + EPSILON);
	    hxy -= P[i][j] * log10(P[i][j] + EPSILON);
	}

    /* Calculate entropies of px and py */
    for (i = 0; i < Ng; ++i) {
	hx -= px[i] * log10(px[i] + EPSILON);
	hy -= py[i] * log10(py[i] + EPSILON);
    }
    G_free(px);
    G_free(py);
    /* fprintf(stderr,"hx=%f\thxy2=%f\n",hx,hxy2); */
    return (sqrt(abs(1 - exp(-2.0 * (hxy2 - hxy)))));
}

float f14_maxcorr(float **P, int Ng)

/* Returns the Maximal Correlation Coefficient */
{
    int i, j, k;
    float *px, *py, **Q;
    float *x, *iy, tmp;

    px = vector(0, Ng);
    py = vector(0, Ng);
    Q = matrix(1, Ng + 1, 1, Ng + 1);
    x = vector(1, Ng);
    iy = vector(1, Ng);

    /*
     * px[i] is the (i-1)th entry in the marginal probability matrix obtained
     * by summing the rows of p[i][j]
     */
    for (i = 0; i < Ng; ++i) {
	for (j = 0; j < Ng; ++j) {
	    px[i] += P[i][j];
	    py[j] += P[i][j];
	}
    }

    /* Find the Q matrix */
    for (i = 0; i < Ng; ++i) {
	for (j = 0; j < Ng; ++j) {
	    Q[i + 1][j + 1] = 0;
	    for (k = 0; k < Ng; ++k)
		Q[i + 1][j + 1] += P[i][k] * P[j][k] / px[i] / py[k];
	}
    }

    /* Balance the matrix */
    mkbalanced(Q, Ng);
    /* Reduction to Hessenberg Form */
    reduction(Q, Ng);
    /* Finding eigenvalue for nonsymetric matrix using QR algorithm */
    hessenberg(Q, Ng, x, iy);
    /* simplesrt(Ng,x); */
    /* Returns the sqrt of the second largest eigenvalue of Q */
    for (i = 2, tmp = x[1]; i <= Ng; ++i)
	tmp = (tmp > x[i]) ? tmp : x[i];

    MatrixDealloc(Q, Ng);
    G_free(px);
    G_free(py);
    G_free(x);
    G_free(iy);
    return sqrt(x[Ng - 1]);
}

float *vector(int nl, int nh)
{
    float *v;

    v = (float *)G_malloc((unsigned)(nh - nl + 1) * sizeof(float));
    if (!v)
	G_fatal_error(_("Unable to allocate memory")), exit(EXIT_FAILURE);
    return v;
}


float **matrix(int nrl, int nrh, int ncl, int nch)

/* Allocates a float matrix with range [nrl..nrh][ncl..nch] */
{
    int i;
    float **m;

    /* allocate pointers to rows */
    m = (float **)G_malloc((unsigned)(nrh - nrl + 1) * sizeof(float *));

    /* allocate rows */
    for (i = 0; i < (nrh - nrl + 1); i++) {
	m[i] = (float *)G_malloc((unsigned)(nch - ncl + 1) * sizeof(float));
    }
    /* return pointer to array of pointers to rows */
    return m;
}

void simplesrt(int n, float arr[])
{
    int i, j;
    float a;

    for (j = 2; j <= n; j++) {
	a = arr[j];
	i = j - 1;
	while (i > 0 && arr[i] > a) {
	    arr[i + 1] = arr[i];
	    i--;
	}
	arr[i + 1] = a;
    }
}

void mkbalanced(float **a, int n)
{
    int last, j, i;
    float s, r, g, f, c, sqrdx;

    sqrdx = RADIX * RADIX;
    last = 0;
    while (last == 0) {
	last = 1;
	for (i = 1; i <= n; i++) {
	    r = c = 0.0;
	    for (j = 1; j <= n; j++)
		if (j != i) {
		    c += fabs(a[j][i]);
		    r += fabs(a[i][j]);
		}
	    if (c && r) {
		g = r / RADIX;
		f = 1.0;
		s = c + r;
		while (c < g) {
		    f *= RADIX;
		    c *= sqrdx;
		}
		g = r * RADIX;
		while (c > g) {
		    f /= RADIX;
		    c /= sqrdx;
		}
		if ((c + r) / f < 0.95 * s) {
		    last = 0;
		    g = 1.0 / f;
		    for (j = 1; j <= n; j++)
			a[i][j] *= g;
		    for (j = 1; j <= n; j++)
			a[j][i] *= f;
		}
	    }
	}
    }
}


void reduction(float **a, int n)
{
    int m, j, i;
    float y, x;

    for (m = 2; m < n; m++) {
	x = 0.0;
	i = m;
	for (j = m; j <= n; j++) {
	    if (fabs(a[j][m - 1]) > fabs(x)) {
		x = a[j][m - 1];
		i = j;
	    }
	}
	if (i != m) {
	    for (j = m - 1; j <= n; j++)
		SWAP(a[i][j], a[m][j])
		    for (j = 1; j <= n; j++)
		    SWAP(a[j][i], a[j][m])
	}
	if (x) {
	    for (i = m + 1; i <= n; i++) {
		if ((y = a[i][m - 1])) {
		    y /= x;
		    a[i][m - 1] = y;
		    for (j = m; j <= n; j++)
			a[i][j] -= y * a[m][j];
		    for (j = 1; j <= n; j++)
			a[j][m] += y * a[j][i];
		}
	    }
	}
    }
}

void hessenberg(float **a, int n, float wr[], float wi[])
{
    int nn, m, l, k, j, its, i, mmin;
    float z, y, x, w, v, u, t, s, r, q, p, anorm;

    anorm = fabs(a[1][1]);
    for (i = 2; i <= n; i++)
	for (j = (i - 1); j <= n; j++)
	    anorm += fabs(a[i][j]);
    nn = n;
    t = 0.0;
    while (nn >= 1) {
	its = 0;
	do {
	    for (l = nn; l >= 2; l--) {
		s = fabs(a[l - 1][l - 1]) + fabs(a[l][l]);
		if (s == 0.0)
		    s = anorm;
		if ((float)(fabs(a[l][l - 1]) + s) == s)
		    break;
	    }
	    x = a[nn][nn];
	    if (l == nn) {
		wr[nn] = x + t;
		wi[nn--] = 0.0;
	    }
	    else {
		y = a[nn - 1][nn - 1];
		w = a[nn][nn - 1] * a[nn - 1][nn];
		if (l == (nn - 1)) {
		    p = 0.5 * (y - x);
		    q = p * p + w;
		    z = sqrt(fabs(q));
		    x += t;
		    if (q >= 0.0) {
			z = p + SIGN(z, p);
			wr[nn - 1] = wr[nn] = x + z;
			if (z)
			    wr[nn] = x - w / z;
			wi[nn - 1] = wi[nn] = 0.0;
		    }
		    else {
			wr[nn - 1] = wr[nn] = x + p;
			wi[nn - 1] = -(wi[nn] = z);
		    }
		    nn -= 2;
		}
		else {
		    if (its == 30)
			G_fatal_error(_("Too many iterations to required to find %s - giving up"),
				      F14), exit(1);
		    if (its == 10 || its == 20) {
			t += x;
			for (i = 1; i <= nn; i++)
			    a[i][i] -= x;
			s = fabs(a[nn][nn - 1]) + fabs(a[nn - 1][nn - 2]);
			y = x = 0.75 * s;
			w = -0.4375 * s * s;
		    }
		    ++its;
		    for (m = (nn - 2); m >= l; m--) {
			z = a[m][m];
			r = x - z;
			s = y - z;
			p = (r * s - w) / a[m + 1][m] + a[m][m + 1];
			q = a[m + 1][m + 1] - z - r - s;
			r = a[m + 2][m + 1];
			s = fabs(p) + fabs(q) + fabs(r);
			p /= s;
			q /= s;
			r /= s;
			if (m == l)
			    break;
			u = fabs(a[m][m - 1]) * (fabs(q) + fabs(r));
			v = fabs(p) * (fabs(a[m - 1][m - 1]) + fabs(z) +
				       fabs(a[m + 1][m + 1]));
			if ((float)(u + v) == v)
			    break;
		    }
		    for (i = m + 2; i <= nn; i++) {
			a[i][i - 2] = 0.0;
			if (i != (m + 2))
			    a[i][i - 3] = 0.0;
		    }
		    for (k = m; k <= nn - 1; k++) {
			if (k != m) {
			    p = a[k][k - 1];
			    q = a[k + 1][k - 1];
			    r = 0.0;
			    if (k != (nn - 1))
				r = a[k + 2][k - 1];
			    if ((x = fabs(p) + fabs(q) + fabs(r))) {
				p /= x;
				q /= x;
				r /= x;
			    }
			}
			if ((s = SIGN(sqrt(p * p + q * q + r * r), p))) {
			    if (k == m) {
				if (l != m)
				    a[k][k - 1] = -a[k][k - 1];
			    }
			    else
				a[k][k - 1] = -s * x;
			    p += s;
			    x = p / s;
			    y = q / s;
			    z = r / s;
			    q /= p;
			    r /= p;
			    for (j = k; j <= nn; j++) {
				p = a[k][j] + q * a[k + 1][j];
				if (k != (nn - 1)) {
				    p += r * a[k + 2][j];
				    a[k + 2][j] -= p * z;
				}
				a[k + 1][j] -= p * y;
				a[k][j] -= p * x;
			    }
			    mmin = nn < k + 3 ? nn : k + 3;
			    for (i = l; i <= mmin; i++) {
				p = x * a[i][k] + y * a[i][k + 1];
				if (k != (nn - 1)) {
				    p += z * a[i][k + 2];
				    a[i][k + 2] -= p * r;
				}
				a[i][k + 1] -= p * q;
				a[i][k] -= p;
			    }
			}
		    }
		}
	    }
	} while (l < nn - 1);
    }
}
