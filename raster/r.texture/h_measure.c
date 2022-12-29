
/****************************************************************************
 *
 * MODULE:       r.texture
 * AUTHOR(S):    Carmine Basco - basco@unisannio.it
 *               with hints from: 
 * 			prof. Giulio Antoniol - antoniol@ieee.org
 * 			prof. Michele Ceccarelli - ceccarelli@unisannio.it
 *               Markus Metz (optimization and bug fixes)
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

#define PGM_MAXMAXVAL 255
#define MAX_MATRIX_SIZE 512

float **matrix(int nr, int nc);
float *vector(int n);

float f1_asm(void);
float f2_contrast(void);
float f3_corr(void);
float f4_var(void);
float f5_idm(void);
float f6_savg(void);
float f7_svar(void);
float f8_sentropy(void);
float f9_entropy(void);
float f10_dvar(void);
float f11_dentropy(void);
float f12_icorr(void);
float f13_icorr(void);

static float **P_matrix = NULL;
static float **P_matrix0 = NULL;
static float **P_matrix45 = NULL;
static float **P_matrix90 = NULL;
static float **P_matrix135 = NULL;

int tone[PGM_MAXMAXVAL + 1];
static int Ng = 0;
static float *px, *py;
static float Pxpys[2 * PGM_MAXMAXVAL + 2];
static float Pxpyd[2 * PGM_MAXMAXVAL + 2];


void alloc_vars(int size)
{
    int msize2;

    /* Allocate memory for gray-tone spatial dependence matrix */
    P_matrix0 = matrix(MAX_MATRIX_SIZE + 1, MAX_MATRIX_SIZE + 1);
    P_matrix45 = matrix(MAX_MATRIX_SIZE + 1, MAX_MATRIX_SIZE + 1);
    P_matrix90 = matrix(MAX_MATRIX_SIZE + 1, MAX_MATRIX_SIZE + 1);
    P_matrix135 = matrix(MAX_MATRIX_SIZE + 1, MAX_MATRIX_SIZE + 1);

    if (size * size < 256)
	msize2 = size * size;
    else
	msize2 = 256;

    px = vector(msize2 + 1);
    py = vector(msize2 + 1);
}

static int bsearch_gray(int *array, int n, int val)
{
    int lo, hi, mid;

    lo = 0;
    hi = n - 1;

    while (lo <= hi) {
	mid = (lo + hi) >> 1;
	
	if (array[mid] == val)
	    return mid;

	if (array[mid] > val)
	    hi = mid - 1;
	else
	    lo = mid + 1;
    }

    return -1;
}

int set_vars(int **grays, int curr_row, int curr_col,
                int size, int offset, int t_d, int with_nulls)
{
    int R0, R45, R90, R135, x, y;
    int row, col, row2, col2, rows, cols;
    int rowmin, rowmax, colmin, colmax, wrows, wcols, rowd, cold;
    int itone, jtone;
    int cnt;

    rows = cols = size;
    wrows = Rast_window_rows();
    wcols = Rast_window_cols();

    /* Determine the number of different gray scales (not maxval) */
    for (row = 0; row <= PGM_MAXMAXVAL; row++)
	tone[row] = -1;
    cnt = 0;
    rowmin = curr_row - offset;
    if (rowmin < 0)
	rowmin = 0;
    rowmax = curr_row + offset;
    if (rowmax > wrows - 1)
	rowmax = wrows - 1;
    colmin = curr_col - offset;
    if (colmin < 0)
	colmin = 0;
    colmax = curr_col + offset;
    if (colmax > wcols - 1)
	colmax = wcols - 1;
    for (row = rowmin; row <= rowmax; row++) {
	for (col = colmin; col <= colmax; col++) {
	    if (grays[row][col] < 0) {	/* No data pixel found */
		continue;
	    }
	    if (grays[row][col] > PGM_MAXMAXVAL)
		G_fatal_error(_("Too many categories (found: %i, max: %i). "
				"Try to rescale or reclassify the map"),
			      grays[row][col], PGM_MAXMAXVAL);
	    tone[grays[row][col]] = grays[row][col];
	    cnt++;
	}
    }
    /* what is the minimum number of pixels 
     * to get reasonable texture measurements ? 
     * at the very least, any of R0, R45, R90, R135 must be > 1 */
    if (cnt < size * size / 4 || (!with_nulls && cnt < size * size))
	return 0;

    /* Collapse array, taking out all zero values */
    Ng = 0;
    for (row = 0; row <= PGM_MAXMAXVAL; row++) {
	if (tone[row] != -1)
	    tone[Ng++] = tone[row];
    }

    /* Now array contains only the gray levels present (in ascending order) */

    for (row = 0; row < Ng; row++)
	for (col = 0; col < Ng; col++) {
	    P_matrix0[row][col] = P_matrix45[row][col] = 0;
	    P_matrix90[row][col] = P_matrix135[row][col] = 0;
	}

    /* Find normalizing constants */
    /* not correct in case of NULL cells: */
    /*
    R0 = 2 * rows * (cols - t_d);
    R45 = 2 * (rows - t_d) * (cols - t_d);
    R90 = 2 * (rows - t_d) * cols;
    R135 = R45;
    */

    /* count actual cooccurrences for each angle */
    R0 = R45 = R90 = R135 = 0;

    /* Find gray-tone spatial dependence matrix */
    for (row = 0; row < rows; row++) {
	row2 = curr_row - offset + row;
	if (row2 < 0 || row2 >= wrows)
	    continue;
	for (col = 0; col < cols; col++) {
	    col2 = curr_col - offset + col;
	    if (col2 < 0 || col2 >= wcols)
		continue;
	    if (grays[row2][col2] < 0)
		continue;
	    x = bsearch_gray(tone, Ng, grays[row2][col2]);
	    rowd = row2;
	    cold = col2 + t_d;
	    if (col + t_d < cols && cold < wcols &&
	        grays[rowd][cold] >= 0) {
		y = bsearch_gray(tone, Ng, grays[rowd][cold]);
		P_matrix0[x][y]++;
		P_matrix0[y][x]++;
		R0 += 2;
	    }
	    rowd = row2 + t_d;
	    cold = col2;
	    if (row + t_d < rows && rowd < wrows &&
	        grays[rowd][cold] >= 0) {
		y = bsearch_gray(tone, Ng, grays[rowd][cold]);
		P_matrix90[x][y]++;
		P_matrix90[y][x]++;
		R90 += 2;
	    }
	    rowd = row2 + t_d;
	    cold = col2 - t_d;
	    if (row + t_d < rows && rowd < wrows &&
	        col - t_d >= 0 && cold >= 0 &&
	        grays[rowd][cold] >= 0) {
		y = bsearch_gray(tone, Ng, grays[rowd][cold]);
		P_matrix45[x][y]++;
		P_matrix45[y][x]++;
		R45 += 2;
	    }
	    rowd = row2 + t_d;
	    cold = col2 + t_d;
	    if (row + t_d < rows && rowd < wrows &&
	        col + t_d < cols && cold < wcols &&
	        grays[rowd][cold] >= 0) {
		y = bsearch_gray(tone, Ng, grays[rowd][cold]);
		P_matrix135[x][y]++;
		P_matrix135[y][x]++;
		R135 += 2;
	    }
	}
    }
    /* Gray-tone spatial dependence matrices are complete */

    /* Normalize gray-tone spatial dependence matrix */
    for (itone = 0; itone < Ng; itone++) {
	for (jtone = 0; jtone < Ng; jtone++) {
	    P_matrix0[itone][jtone] /= R0;
	    P_matrix45[itone][jtone] /= R45;
	    P_matrix90[itone][jtone] /= R90;
	    P_matrix135[itone][jtone] /= R135;
	}
    }

    return 1;
}

int set_angle_vars(int angle, int have_px, int have_py,
                   int have_pxpys, int have_pxpyd)
{
    int i, j;
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
    case 1:
	return (f1_asm());
	break;

    /* Contrast */
    case 2:
	return (f2_contrast());
	break;

    /* Correlation */
    case 3:
	return (f3_corr());
	break;

    /* Variance */
    case 4:
	return (f4_var());
	break;

    /* Inverse Diff Moment */
    case 5:
	return (f5_idm());
	break;

    /* Sum Average */
    case 6:
	return (f6_savg());
	break;

    /* Sum Variance */
    case 7:
	return (f7_svar());
	break;

    /* Sum Entropy */
    case 8:
	return (f8_sentropy());
	break;

    /* Entropy */
    case 9:
	return (f9_entropy());
	break;

    /* Difference Variance */
    case 10:
	return (f10_dvar());
	break;

    /* Difference Entropy */
    case 11:
	return (f11_dentropy());
	break;

    /* Measure of Correlation-1 */
    case 12:
	return (f12_icorr());
	break;

    /* Measure of Correlation-2 */
    case 13:
	return (f13_icorr());
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
float f1_asm(void)
{
    int i, j;
    float sum = 0;
    float **P = P_matrix;

    /*
    for (i = 0; i < Ng; i++)
	for (j = 0; j < Ng; j++)
	    sum += P[i][j] * P[i][j];
    */

    for (i = 0; i < Ng; i++) {
	sum += P[i][i] * P[i][i];
	for (j = 0; j < i; j++)
	    sum += 2 * P[i][j] * P[i][j];
    }

    return sum;
}

/* Contrast */
/*
 * The contrast feature is a difference moment of the P matrix and is a
 * measure of the contrast or the amount of local variations present in an
 * image.
 */
float f2_contrast(void)
{
    int i, j /*, n */;
    float /* sum,*/ bigsum = 0;
    float **P = P_matrix;

    /* the three-loop version does not work 
     * when gray tones that do not occur in the current window 
     * have been removed in tone and P* */
    /*
    for (n = 0; n < Ng; n++) {
	sum = 0;
	for (i = 0; i < Ng; i++) {
	    for (j = 0; j < Ng; j++) {
		if ((i - j) == n ||
		    (j - i) == n) {
		    sum += P[i][j];
		}
	    }
	}
	bigsum += n * n * sum;
    }
    */

    /* two-loop version */
    for (i = 0; i < Ng; i++) {
	for (j = 0; j < i; j++) {
	    bigsum += 2 * P[i][j] * (tone[i] - tone[j]) * (tone[i] - tone[j]);
	}
    }

    return bigsum;
}

/* Correlation */
/*
 * This correlation feature is a measure of gray-tone linear-dependencies
 * in the image.
 */
float f3_corr(void)
{
    int i, j;
    float sum_sqr = 0, tmp = 0;
    float mean = 0, stddev;
    float **P = P_matrix;

    /* Now calculate the means and standard deviations of px and py */

    /*- fix supplied by J. Michael Christensen, 21 Jun 1991 */

    /*- further modified by James Darrell McCauley, 16 Aug 1991
     *     after realizing that meanx=meany and stddevx=stddevy
     */
    for (i = 0; i < Ng; i++) {
	mean += px[i] * tone[i];
	sum_sqr += px[i] * tone[i] * tone[i];

	for (j = 0; j < Ng; j++)
	    tmp += tone[i] * tone[j] * P[i][j];
    }
    stddev = sqrt(sum_sqr - (mean * mean));
    
    if (stddev == 0)	/* stddev < GRASS_EPSILON or similar ? */
	return 0;

    return (tmp - mean * mean) / (stddev * stddev);
}

/* Sum of Squares: Variance */
float f4_var(void)
{
    int i, j;
    float mean = 0, var = 0;
    float **P = P_matrix;

    /*- Corrected by James Darrell McCauley, 16 Aug 1991
     *  calculates the mean intensity level instead of the mean of
     *  cooccurrence matrix elements
     */
    for (i = 0; i < Ng; i++)
	for (j = 0; j < Ng; j++)
	    mean += tone[i] * P[i][j];

    for (i = 0; i < Ng; i++)
	for (j = 0; j < Ng; j++)
	    var += (tone[i] - mean) * (tone[i] - mean) * P[i][j];

    return var;
}

/* Inverse Difference Moment */
float f5_idm(void)
{
    int i, j;
    float idm = 0;
    float **P = P_matrix;

    /*
    for (i = 0; i < Ng; i++)
	for (j = 0; j < Ng; j++)
	    idm += P[i][j] / (1 + (tone[i] - tone[j]) * (tone[i] - tone[j]));
    */

    for (i = 0; i < Ng; i++) {
	idm += P[i][i];
	for (j = 0; j < i; j++)
	    idm += 2 * P[i][j] / (1 + (tone[i] - tone[j]) * (tone[i] - tone[j]));
    }

    return idm;
}

/* Sum Average */
float f6_savg(void)
{
    int i, j, k;
    float savg = 0;
    float *P = Pxpys;

    /*
    for (i = 0; i < 2 * Ng - 1; i++)
	savg += (i + 2) * Pxpys[i];
    */

    for (i = 0; i < Ng; i++) {
	for (j = 0; j < Ng; j++) {
	    k = i + j;
	    savg += (tone[i] + tone[j]) * P[k];
	}
    }

    return savg;
}

/* Sum Variance */
float f7_svar(void)
{
    int i, j, k;
    float var = 0;
    float *P = Pxpys;
    float savg = f6_savg();
    float tmp;

    /*
    for (i = 0; i < 2 * Ng - 1; i++)
	var += (i + 2 - savg) * (i + 2 - savg) * Pxpys[i];
    */

    for (i = 0; i < Ng; i++) {
	for (j = 0; j < Ng; j++) {
	    k = i + j;
	    tmp = tone[i] + tone[j] - savg;
	    var += tmp * tmp * P[k];
	}
    }

    return var;
}

/* Sum Entropy */
float f8_sentropy(void)
{
    int i;
    float sentr = 0;
    float *P = Pxpys;

    for (i = 0; i < 2 * Ng - 1; i++) {
	if (P[i] > 0)
	    sentr -= P[i] * log2(P[i]);
    }

    return sentr;
}

/* Entropy */
float f9_entropy(void)
{
    int i, j;
    float entropy = 0;
    float **P = P_matrix;

    /*
    for (i = 0; i < Ng; i++) {
	for (j = 0; j < Ng; j++) {
	    if (P[i][j] > 0)
		entropy += P[i][j] * log2(P[i][j]);
	}
    }
    */

    for (i = 0; i < Ng; i++) {
	if (P[i][i] > 0)
	    entropy += P[i][i] * log2(P[i][i]);
	for (j = 0; j < i; j++) {
	    if (P[i][j] > 0)
		entropy += 2 * P[i][j] * log2(P[i][j]);
	}
    }

    return -entropy;
}

/* Difference Variance */
float f10_dvar(void)
{
    int i, tmp;
    float sum = 0, sum_sqr = 0, var = 0;
    float *P = Pxpyd;

    /* Now calculate the variance of Pxpy (Px-y) */
    for (i = 0; i < Ng; i++) {
	sum += P[i];
	sum_sqr += P[i] * P[i];
    }
    /* tmp = Ng * Ng; */
    if (Ng > 1) {
	tmp = (tone[Ng - 1] - tone[0]) * (tone[Ng - 1] - tone[0]);
	var = ((tmp * sum_sqr) - (sum * sum)) / (tmp * tmp);
    }

    return var;
}

/* Difference Entropy */
float f11_dentropy(void)
{
    int i;
    float sum = 0;
    float *P = Pxpyd;

    for (i = 0; i < Ng; i++) {
	if (P[i] > 0)
	    sum += P[i] * log2(P[i]);
    }

    return -sum;
}

/* Information Measures of Correlation */
float f12_icorr(void)
{
    int i, j;
    float hx = 0, hy = 0, hxy = 0, hxy1 = 0;
    float **P = P_matrix;

    for (i = 0; i < Ng; i++) {
	for (j = 0; j < Ng; j++) {
	    if (px[i] * py[j] > 0)
		hxy1 -= P[i][j] * log2(px[i] * py[j]);
	    if (P[i][j] > 0)
		hxy -= P[i][j] * log2(P[i][j]);
	}

    /* Calculate entropies of px and py - is this right? */
	if (px[i] > 0)
	    hx -= px[i] * log2(px[i]);
	if (py[i] > 0)
	    hy -= py[i] * log2(py[i]);
    }

    /* fprintf(stderr,"hxy1=%f\thxy=%f\thx=%f\thy=%f\n",hxy1,hxy,hx,hy); */
    if (hx == 0 && hy == 0)
	return 0;

    return ((hxy - hxy1) / (hx > hy ? hx : hy));
}

/* Information Measures of Correlation */
float f13_icorr(void)
{
    int i, j;
    float hxy = 0, hxy2 = 0;
    float **P = P_matrix;

    for (i = 0; i < Ng; i++) {
	for (j = 0; j < Ng; j++) {
	    if (px[i] * py[j] > 0)
		hxy2 -= px[i] * py[j] * log2(px[i] * py[j]);
	    if (P[i][j] > 0)
		hxy -= P[i][j] * log2(P[i][j]);
	}
    }

    /* fprintf(stderr,"hx=%f\thxy2=%f\n",hx,hxy2); */
    return (sqrt(fabs(1 - exp(-2.0 * (hxy2 - hxy)))));
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
