#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "matvec.h"

int bsearch_gray(int *array, int n, int val)
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

void MatrixDealloc(float **A, int N)
{
    /*A is NxN */
    int i;

    for (i = 0; i < N; i++)
        G_free(A[i]);
    G_free(A);
}

void alloc_vars(int size, struct matvec *mv)
{
    int msize2 = (size * size < 256) ? size * size : 256;
    size = PGM_MAXMAXVAL;

    /* Allocate memory for gray-tone spatial dependence matrix */
    mv->P_matrix0 = matrix(size + 1, size + 1);
    mv->P_matrix45 = matrix(size + 1, size + 1);
    mv->P_matrix90 = matrix(size + 1, size + 1);
    mv->P_matrix135 = matrix(size + 1, size + 1);

    mv->px = vector(msize2 + 1);
    mv->py = vector(msize2 + 1);
    mv->Pxpys = vector(2 * msize2 + 2);
    mv->Pxpyd = vector(2 * msize2 + 2);

    mv->tone = (int *)G_malloc((PGM_MAXMAXVAL + 1) * sizeof(int));
    mv->Ng = 0;
}

void dealloc_vars(struct matvec *mv)
{
    /* Deallocate memory for gray-tone spatial dependence matrix */
    MatrixDealloc(mv->P_matrix0, PGM_MAXMAXVAL + 1);
    MatrixDealloc(mv->P_matrix45, PGM_MAXMAXVAL + 1);
    MatrixDealloc(mv->P_matrix90, PGM_MAXMAXVAL + 1);
    MatrixDealloc(mv->P_matrix135, PGM_MAXMAXVAL + 1);

    G_free(mv->px);
    G_free(mv->py);
    G_free(mv->Pxpys);
    G_free(mv->Pxpyd);

    G_free(mv->tone);
}

int set_vars(struct matvec *mv, int **grays, int curr_row, int curr_col,
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
        mv->tone[row] = -1;
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
            if (grays[row][col] < 0) { /* No data pixel found */
                continue;
            }
            if (grays[row][col] > PGM_MAXMAXVAL)
                G_fatal_error(_("Too many categories (found: %i, max: %i). "
                                "Try to rescale or reclassify the map"),
                              grays[row][col], PGM_MAXMAXVAL);
            mv->tone[grays[row][col]] = grays[row][col];
            cnt++;
        }
    }
    /* what is the minimum number of pixels
     * to get reasonable texture measurements ?
     * at the very least, any of R0, R45, R90, R135 must be > 1 */
    if (cnt < size * size / 4 || (!with_nulls && cnt < size * size))
        return 0;

    /* Collapse array, taking out all zero values */
    mv->Ng = 0;
    for (row = 0; row <= PGM_MAXMAXVAL; row++) {
        if (mv->tone[row] != -1)
            mv->tone[mv->Ng++] = mv->tone[row];
    }

    /* Now array contains only the gray levels present (in ascending order) */

    for (row = 0; row < mv->Ng; row++)
        for (col = 0; col < mv->Ng; col++) {
            mv->P_matrix0[row][col] = mv->P_matrix45[row][col] = 0;
            mv->P_matrix90[row][col] = mv->P_matrix135[row][col] = 0;
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
            x = bsearch_gray(mv->tone, mv->Ng, grays[row2][col2]);
            rowd = row2;
            cold = col2 + t_d;
            if (col + t_d < cols && cold < wcols && grays[rowd][cold] >= 0) {
                y = bsearch_gray(mv->tone, mv->Ng, grays[rowd][cold]);
                mv->P_matrix0[x][y]++;
                mv->P_matrix0[y][x]++;
                R0 += 2;
            }
            rowd = row2 + t_d;
            cold = col2;
            if (row + t_d < rows && rowd < wrows && grays[rowd][cold] >= 0) {
                y = bsearch_gray(mv->tone, mv->Ng, grays[rowd][cold]);
                mv->P_matrix90[x][y]++;
                mv->P_matrix90[y][x]++;
                R90 += 2;
            }
            rowd = row2 + t_d;
            cold = col2 - t_d;
            if (row + t_d < rows && rowd < wrows && col - t_d >= 0 &&
                cold >= 0 && grays[rowd][cold] >= 0) {
                y = bsearch_gray(mv->tone, mv->Ng, grays[rowd][cold]);
                mv->P_matrix45[x][y]++;
                mv->P_matrix45[y][x]++;
                R45 += 2;
            }
            rowd = row2 + t_d;
            cold = col2 + t_d;
            if (row + t_d < rows && rowd < wrows && col + t_d < cols &&
                cold < wcols && grays[rowd][cold] >= 0) {
                y = bsearch_gray(mv->tone, mv->Ng, grays[rowd][cold]);
                mv->P_matrix135[x][y]++;
                mv->P_matrix135[y][x]++;
                R135 += 2;
            }
        }
    }
    /* Gray-tone spatial dependence matrices are complete */

    /* Normalize gray-tone spatial dependence matrix */
    for (itone = 0; itone < mv->Ng; itone++) {
        for (jtone = 0; jtone < mv->Ng; jtone++) {
            mv->P_matrix0[itone][jtone] /= R0;
            mv->P_matrix45[itone][jtone] /= R45;
            mv->P_matrix90[itone][jtone] /= R90;
            mv->P_matrix135[itone][jtone] /= R135;
        }
    }

    return 1;
}

int set_angle_vars(struct matvec *mv, int angle, int have_px, int have_py,
                   int have_pxpys, int have_pxpyd)
{
    int i, j;
    float **P;

    switch (angle) {
    case 0:
        mv->P_matrix = mv->P_matrix0;
        break;
    case 1:
        mv->P_matrix = mv->P_matrix45;
        break;
    case 2:
        mv->P_matrix = mv->P_matrix90;
        break;
    case 3:
        mv->P_matrix = mv->P_matrix135;
        break;
    }

    P = mv->P_matrix;

    /*
     * px[i] is the (i-1)th entry in the marginal probability matrix obtained
     * by summing the rows of p[i][j]
     */
    /* Pxpy sum and difference */

    /* reset variabless */
    if (have_px || have_py || have_pxpys || have_pxpyd) {
        for (i = 0; i < mv->Ng; i++) {
            if (have_px || have_py) {
                mv->px[i] = mv->py[i] = 0;
            }
            if (have_pxpys || have_pxpyd) {
                mv->Pxpys[i] = mv->Pxpyd[i] = 0;
            }
        }
        if (have_pxpys) {
            for (j = mv->Ng; j < 2 * mv->Ng; j++) {
                mv->Pxpys[j] = 0;
            }
        }
    }

    if (have_pxpys || have_pxpyd || have_px || have_py) {
        for (i = 0; i < mv->Ng; i++) {
            for (j = 0; j < mv->Ng; j++) {
                if (have_px || have_py) {
                    mv->px[i] += P[i][j];
                    mv->py[j] += P[i][j];
                }
                if (have_pxpys) {
                    mv->Pxpys[i + j] += P[i][j];
                }
                if (have_pxpyd) {
                    mv->Pxpyd[abs(i - j)] += P[i][j];
                }
            }
        }
    }

    return 1;
}
