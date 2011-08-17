
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
#include <grass/raster.h>
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

#define SIGN(x,y) ((y)<0 ? -fabs(x) : fabs(x))
#define SWAP(a,b) {y=(a);(a)=(b);(b)=y;}

#define PGM_MAXMAXVAL 255
#define MAX_MATRIX_SIZE 512

float **matrix(int nr, int nc);
float *vector(int n);

float f1_asm(float **P, int Ng);
float f2_contrast(float **P, int Ng);
float f3_corr(float **P, int Ng);
float f4_var(float **P, int Ng);
float f5_idm(float **P, int Ng);
float f6_savg(float **P, int Ng);
float f7_svar(float **P, int Ng, double S);
float f8_sentropy(float **P, int Ng);
float f9_entropy(float **P, int Ng);
float f10_dvar(float **P, int Ng);
float f11_dentropy(float **P, int Ng);
float f12_icorr(float **P, int Ng);
float f13_icorr(float **P, int Ng);

static float **P_matrix = NULL;
static float **P_matrix0 = NULL;
static float **P_matrix45 = NULL;
static float **P_matrix90 = NULL;
static float **P_matrix135 = NULL;

int tone[PGM_MAXMAXVAL + 1];
static int tones = 0;
static float sentropy = 0.0;
static float *px, *py;
static float Pxpys[2 * PGM_MAXMAXVAL + 2];
static float Pxpyd[2 * PGM_MAXMAXVAL + 2];


void alloc_vars(int size, int dist)
{
    int Ng;

    /* Allocate memory for gray-tone spatial dependence matrix */
    P_matrix0 = matrix(MAX_MATRIX_SIZE + 1, MAX_MATRIX_SIZE + 1);
    P_matrix45 = matrix(MAX_MATRIX_SIZE + 1, MAX_MATRIX_SIZE + 1);
    P_matrix90 = matrix(MAX_MATRIX_SIZE + 1, MAX_MATRIX_SIZE + 1);
    P_matrix135 = matrix(MAX_MATRIX_SIZE + 1, MAX_MATRIX_SIZE + 1);

    if (size * size < 256)
	Ng = size * size;
    else
	Ng = 256;

    px = vector(Ng + 1);
    py = vector(Ng + 1);
}

int set_vars(int **grays, int curr_row, int curr_col,
                int size, int offset, int t_d)
{
    int R0, R45, R90, R135, x, y;
    int row, col, row2, col2, rows, cols;
    int itone, jtone;

    rows = cols = size;

    /* Determine the number of different gray scales (not maxval) */
    for (row = 0; row <= PGM_MAXMAXVAL; row++)
	tone[row] = -1;
    for (row = curr_row - offset; row <= curr_row + offset; row++) {
	for (col = curr_col - offset; col <= curr_col + offset; col++) {
	    if (grays[row][col] < 0) {	/* No data pixel found */
		return 0;
	    }
	    if (grays[row][col] > PGM_MAXMAXVAL)
		G_fatal_error(_("Too many categories (found: %i, max: %i). "
				"Try to rescale or reclassify the map"),
			      grays[row][col], PGM_MAXMAXVAL);
	    tone[grays[row][col]] = grays[row][col];
	}
    }

    /* Collapse array, taking out all zero values */
    tones = 0;
    for (row = 0; row <= PGM_MAXMAXVAL; row++) {
	if (tone[row] != -1)
	    tone[tones++] = tone[row];
    }

    /* Now array contains only the gray levels present (in ascending order) */

    for (row = 0; row < tones; row++)
	for (col = 0; col < tones; col++) {
	    P_matrix0[row][col] = P_matrix45[row][col] = 0;
	    P_matrix90[row][col] = P_matrix135[row][col] = 0;
	}

    /* Find gray-tone spatial dependence matrix */

    for (row = 0; row < rows; row++) {
	row2 = curr_row - offset + row;
	for (col = 0; col < cols; col++) {
	    col2 = curr_col - offset + col;
	    x = 0;
	    while (tone[x] != grays[row2][col2])
		x++;
	    if (col + t_d < cols) {
		y = 0;
		while (tone[y] != grays[row2][col2 + t_d])
		    y++;
		P_matrix0[x][y]++;
		P_matrix0[y][x]++;
	    }
	    if (row + t_d < rows) {
		y = 0;
		while (tone[y] != grays[row2 + t_d][col2])
		    y++;
		P_matrix90[x][y]++;
		P_matrix90[y][x]++;
	    }
	    if (row + t_d < rows && col - t_d >= 0) {
		y = 0;
		while (tone[y] != grays[row2 + t_d][col2 - t_d])
		    y++;
		P_matrix45[x][y]++;
		P_matrix45[y][x]++;
	    }
	    if (row + t_d < rows && col + t_d < cols) {
		y = 0;
		while (tone[y] != grays[row2 + t_d][col2 + t_d])
		    y++;
		P_matrix135[x][y]++;
		P_matrix135[y][x]++;
	    }
	}
    }
    /* Gray-tone spatial dependence matrices are complete */

    /* Find normalizing constants */
    R0 = 2 * rows * (cols - 1);
    R45 = 2 * (rows - 1) * (cols - 1);
    R90 = 2 * (rows - 1) * cols;
    R135 = R45;

    /* Normalize gray-tone spatial dependence matrix */
    for (itone = 0; itone < tones; itone++) {
	for (jtone = 0; jtone < tones; jtone++) {
	    P_matrix0[itone][jtone] /= R0;
	    P_matrix45[itone][jtone] /= R45;
	    P_matrix90[itone][jtone] /= R90;
	    P_matrix135[itone][jtone] /= R135;
	}
    }

    return 1;
}

int set_angle_vars(int angle, int have_px, int have_py, int have_sentr,
                   int have_pxpys, int have_pxpyd)
{
    int i, j, Ng;
    float **P;

    switch (angle) {
	case 0:
	    P_matrix = P_matrix0;
	break;
	case 1:
	    P_matrix = P_matrix45;
	break;
	case 2:
	    P_matrix = P_matrix90;
	break;
	case 3:
	    P_matrix = P_matrix135;
	break;
    }

    if (have_sentr)
	sentropy = f8_sentropy(P_matrix, tones);

    Ng = tones;
    P = P_matrix;

    /*
     * px[i] is the (i-1)th entry in the marginal probability matrix obtained
     * by summing the rows of p[i][j]
     */
    /* Pxpy sum and difference */

    /* reset variabless */
    if (have_px || have_py || have_pxpys || have_pxpyd) {
	for (i = 0; i < Ng; i++) {
	    if (have_px || have_py) {
		px[i] = py[i] = 0;
	    }
	    if (have_pxpys || have_pxpyd) {
		Pxpys[i] = Pxpyd[i] = 0;
	    }
	}
	if (have_pxpys) {
	    for (j = Ng; j < 2 * Ng; j++) {
		Pxpys[j] = 0;
	    }
	}
    }

    if (have_pxpys || have_pxpyd || have_px || have_py) {
	for (i = 0; i < Ng; i++) {
	    for (j = 0; j < Ng; j++) {
		if (have_px || have_py) {
		    px[i] += P[i][j];
		    py[j] += P[i][j];
		}
		if (have_pxpys) {
		    Pxpys[i + j] += P[i][j];
		}
		if (have_pxpyd) {
		    Pxpyd[abs(i - j)] += P[i][j];
		}
	    }
	}
    }

    return 1;
}

float h_measure(int t_m)
{
    switch (t_m) {
	/* Angular Second Moment */
    case 0:
	return (f1_asm(P_matrix, tones));
	break;

    /* Contrast */
    case 1:
	return (f2_contrast(P_matrix, tones));
	break;

    /* Correlation */
    case 2:
	return (f3_corr(P_matrix, tones));
	break;

    /* Variance */
    case 3:
	return (f4_var(P_matrix, tones));
	break;

    /* Inverse Diff Moment */
    case 4:
	return (f5_idm(P_matrix, tones));
	break;

    /* Sum Average */
    case 5:
	return (f6_savg(P_matrix, tones));
	break;

    /* Sum Entropy */
    case 6:
	return (sentropy);
	break;

    /* Sum Variance */
    case 7:
	return (f7_svar(P_matrix, tones, sentropy));
	break;

    /* Entropy */
    case 8:
	return (f9_entropy(P_matrix, tones));
	break;

    /* Difference Variance */
    case 9:
	return (f10_dvar(P_matrix, tones));
	break;

    /* Difference Entropy */
    case 10:
	return (f11_dentropy(P_matrix, tones));
	break;

    /* Measure of Correlation-1 */
    case 11:
	return (f12_icorr(P_matrix, tones));
	break;

    /* Measure of Correlation-2 */
    case 12:
	return (f13_icorr(P_matrix, tones));
	break;
    }

    return 0;
}

void MatrixDealloc(float **A, int N)
{
    /*A is NxN */
    int i;

    for (i = 0; i < N; i++)
	G_free(A[i]);
    G_free(A);
}

/* Angular Second Moment */
/*
 * The angular second-moment feature (ASM) f1 is a measure of homogeneity
 * of the image. In a homogeneous image, there are very few dominant
 * gray-tone transitions. Hence the P matrix for such an image will have
 * fewer entries of large magnitude.
 */
float f1_asm(float **P, int Ng)
{
    int i, j;
    float sum = 0;

    for (i = 0; i < Ng; i++)
	for (j = 0; j < Ng; j++)
	    sum += P[i][j] * P[i][j];

    return sum;
}

/* Contrast */
/*
 * The contrast feature is a difference moment of the P matrix and is a
 * measure of the contrast or the amount of local variations present in an
 * image.
 */
float f2_contrast(float **P, int Ng)
{
    int i, j, n;
    float sum, bigsum = 0;

    for (n = 0; n < Ng; n++) {
	sum = 0;
	for (i = 0; i < Ng; i++) {
	    for (j = 0; j < Ng; j++) {
		if ((i - j) == n || (j - i) == n) {
		    sum += P[i][j];
		}
	    }
	}
	bigsum += n * n * sum;
    }
    return bigsum;
}

/* Correlation */
/*
 * This correlation feature is a measure of gray-tone linear-dependencies
 * in the image.
 */
float f3_corr(float **P, int Ng)
{
    int i, j;
    float sum_sqrx = 0, sum_sqry = 0, tmp = 0;
    float meanx = 0, meany = 0, stddevx, stddevy;


    /* Now calculate the means and standard deviations of px and py */

    /*- fix supplied by J. Michael Christensen, 21 Jun 1991 */

    /*- further modified by James Darrell McCauley, 16 Aug 1991
     *     after realizing that meanx=meany and stddevx=stddevy
     */
    for (i = 0; i < Ng; i++) {
	meanx += px[i] * i;
	sum_sqrx += px[i] * i * i;

	for (j = 0; j < Ng; j++)
	    tmp += i * j * P[i][j];
    }
    meany = meanx;
    sum_sqry = sum_sqrx;
    stddevx = sqrt(sum_sqrx - (meanx * meanx));
    stddevy = stddevx;

    return (tmp - meanx * meany) / (stddevx * stddevy);
}

/* Sum of Squares: Variance */
float f4_var(float **P, int Ng)
{
    int i, j;
    float mean = 0, var = 0;

    /*- Corrected by James Darrell McCauley, 16 Aug 1991
     *  calculates the mean intensity level instead of the mean of
     *  cooccurrence matrix elements
     */
    for (i = 0; i < Ng; i++)
	for (j = 0; j < Ng; j++)
	    mean += i * P[i][j];

    for (i = 0; i < Ng; i++)
	for (j = 0; j < Ng; j++)
	    var += (i + 1 - mean) * (i + 1 - mean) * P[i][j];

    return var;
}

/* Inverse Difference Moment */
float f5_idm(float **P, int Ng)
{
    int i, j;
    float idm = 0;

    for (i = 0; i < Ng; i++)
	for (j = 0; j < Ng; j++)
	    idm += P[i][j] / (1 + (i - j) * (i - j));

    return idm;
}

/* Sum Average */
float f6_savg(float **P, int Ng)
{
    int i;
    float savg = 0;

    for (i = 0; i < 2 * Ng - 1; i++)
	savg += (i + 2) * Pxpys[i];

    return savg;
}

/* Sum Variance */
float f7_svar(float **P, int Ng, double S)
{
    int i;
    float var = 0;

    for (i = 0; i < 2 * Ng - 1; i++)
	var += (i + 2 - S) * (i + 2 - S) * Pxpys[i];

    return var;
}

/* Sum Entropy */
float f8_sentropy(float **P, int Ng)
{
    int i;
    float sentr = 0;

    for (i = 0; i < 2 * Ng - 1; i++)
	sentr -= Pxpys[i] * log10(Pxpys[i] + EPSILON);

    return sentr;
}

/* Entropy */
float f9_entropy(float **P, int Ng)
{
    int i, j;
    float entropy = 0;

    for (i = 0; i < Ng; i++) {
	for (j = 0; j < Ng; j++) {
	    entropy += P[i][j] * log10(P[i][j] + EPSILON);
	}
    }

    return -entropy;
}

/* Difference Variance */
float f10_dvar(float **P, int Ng)
{
    int i, tmp;
    float sum = 0, sum_sqr = 0, var = 0;

    /* Now calculate the variance of Pxpy (Px-y) */
    for (i = 0; i < Ng; i++) {
	sum += Pxpyd[i];
	sum_sqr += Pxpyd[i] * Pxpyd[i];
    }
    tmp = Ng * Ng;
    var = ((tmp * sum_sqr) - (sum * sum)) / (tmp * tmp);

    return var;
}

/* Difference Entropy */
float f11_dentropy(float **P, int Ng)
{
    int i;
    float sum = 0;

    for (i = 0; i < Ng; i++)
	sum += Pxpyd[i] * log10(Pxpyd[i] + EPSILON);

    return -sum;
}

/* Information Measures of Correlation */
float f12_icorr(float **P, int Ng)
{
    int i, j;
    float hx = 0, hy = 0, hxy = 0, hxy1 = 0;

    for (i = 0; i < Ng; i++)
	for (j = 0; j < Ng; j++) {
	    hxy1 -= P[i][j] * log10(px[i] * py[j] + EPSILON);
	    hxy -= P[i][j] * log10(P[i][j] + EPSILON);
	}

    /* Calculate entropies of px and py - is this right? */
    for (i = 0; i < Ng; i++) {
	hx -= px[i] * log10(px[i] + EPSILON);
	hy -= py[i] * log10(py[i] + EPSILON);
    }

    /* fprintf(stderr,"hxy1=%f\thxy=%f\thx=%f\thy=%f\n",hxy1,hxy,hx,hy); */
    return ((hxy - hxy1) / (hx > hy ? hx : hy));
}

/* Information Measures of Correlation */
float f13_icorr(float **P, int Ng)
{
    int i, j;
    float hxy = 0, hxy2 = 0;

    for (i = 0; i < Ng; i++) {
	for (j = 0; j < Ng; j++) {
	    hxy2 -= px[i] * py[j] * log10(px[i] * py[j] + EPSILON);
	    hxy -= P[i][j] * log10(P[i][j] + EPSILON);
	}
    }

    /* fprintf(stderr,"hx=%f\thxy2=%f\n",hx,hxy2); */
    return (sqrt(abs(1 - exp(-2.0 * (hxy2 - hxy)))));
}

float *vector(int n)
{
    float *v;

    v = (float *)G_malloc(n * sizeof(float));
    if (!v)
	G_fatal_error(_("Unable to allocate memory")), exit(EXIT_FAILURE);
    return v;
}

/* Allocates a float matrix with range [nrl..nrh][ncl..nch] */
float **matrix(int nr, int nc)
{
    int i;
    float **m;

    /* allocate pointers to rows */
    m = (float **)G_malloc(nr * sizeof(float *));

    /* allocate rows */
    for (i = 0; i < nr; i++) {
	m[i] = (float *)G_malloc(nc * sizeof(float));
    }
    /* return pointer to array of pointers to rows */
    return m;
}
