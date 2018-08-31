
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Definition of a matrix and basic operations with
 *             matrices
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "matrix.h"

int matrix_init(int rows, int cols, MATRIX *res)
{

    int i, j;

    res->rows = rows;
    res->cols = cols;
    res->a = (double **)G_calloc(rows, sizeof(double *));
    if (res->a == NULL)
	return 0;

    for (i = 0; i < rows; i++) {
	res->a[i] = (double *)G_calloc(cols, sizeof(double));
	if (res->a[i] == NULL) {
	    for (j = 0; j < i; j++)
		G_free(res->a[j]);
	    G_free(res->a);
	    return 0;
	}
    }

    return 1;
}

void matrix_free(MATRIX *m)
{
    int i;

    for (i = 0; i < m->rows; i++)
	G_free(m->a[i]);
    G_free(m->a);
    return;
}

int matrix_mult(MATRIX *a, MATRIX *b, MATRIX *res)
{
    if (a->cols != b->rows)
	return 0;

    /*if (!matrix_init(a.rows, b.cols, res))
     * return 0;
     */

    int i, j, k;

    for (i = 0; i < a->rows; i++)
	for (j = 0; j < b->cols; j++) {
	    res->a[i][j] = 0;
	    for (k = 0; k < a->cols; k++)
		res->a[i][j] += a->a[i][k] * b->a[k][j];
	}

    return 1;
}

int matrix_add_identity(double s, MATRIX *m)
{
    if (m->rows != m->cols)
	return 0;
    int i;

    for (i = 0; i < m->rows; i++)
	m->a[i][i] += s;

    return 1;
}

/* three following functions implements elementary row operations on matrices */

/* auxialiry function for matrix_inverse, swaps two rows of given matrix */
void matrix_swap_rows(int x, int y, MATRIX *m)
{
    int i;

    for (i = 0; i < m->cols; i++) {
	double t;

	t = m->a[x][i];
	m->a[x][i] = m->a[y][i];
	m->a[y][i] = t;
    }
    return;
}

/* auxiliary function for matrix_inverse, multiplies row of a matrix by
 * a scalar */
void matrix_row_scalar(int row, double s, MATRIX *m)
{
    int i;

    for (i = 0; i < m->cols; i++)
	m->a[row][i] *= s;
    return;
}

/* auxiliary function for matrix_inverse, adds a multiple of 
 * one row to another.
 * i.e row[ra] = row[ra] + row[rb] * s;
 */
void matrix_row_add_multiple(int ra, int rb, double s, MATRIX *m)
{
    int i;

    for (i = 0; i < m->cols; i++)
	m->a[ra][i] += m->a[rb][i] * s;
    return;
}

/* TODO: don't test directly equality to zero */
int matrix_inverse(MATRIX *a, MATRIX *res, int percents)
{
    int i, j;

    /* not a square matrix */
    if (a->rows != a->cols)
	return 0;

    /* initialize output matrix to the identity matrix */
    if (!matrix_init(a->rows, a->rows, res)) {
	G_fatal_error(_("Out of memory"));
	return 0;
    }
    for (i = 0; i < a->rows; i++) {
	memset(res->a[i], 0, sizeof(double) * a->cols);
	res->a[i][i] = 1;
    }

    /* in order to obtain the inverse of a matrix, we run
     * gauss elimination on the matrix and each time we apply
     * elementary row operation on this matrix, we apply the
     * same operation on the identity matrix. Correctness of
     * this follows from the fact that an invertible matrix
     * is row equivalent to the identity matrix.
     */

    int n = a->rows;

    if (percents)
	G_percent_reset();

    for (i = 0; i < n; i++) {
	int found = 0;

	if (percents)
	    G_percent(i, n, 1);
	for (j = i; j < n; j++) {
	    if (a->a[j][i] != 0) {	/* need to change this row to something */
		found = 1;	/* more sensible */
		matrix_swap_rows(i, j, a);
		matrix_swap_rows(i, j, res);
		break;
	    }
	}
	if (!found)
	    return 0;
	double c = (double)1.0 / a->a[i][i];

	matrix_row_scalar(i, c, a);
	matrix_row_scalar(i, c, res);
	for (j = 0; j < n; j++) {
	    if (i == j)
		continue;
	    double c = -a->a[j][i];

	    if (c == 0.0)
		continue;
	    matrix_row_add_multiple(j, i, c, a);
	    matrix_row_add_multiple(j, i, c, res);
	}
    }

    return 1;
}

void matrix_mult_scalar(double s, MATRIX *m)
{
    int i, j;

    for (i = 0; i < m->rows; i++)
	for (j = 0; j < m->cols; j++)
	    m->a[i][j] *= s;
}

void matrix_add(MATRIX *a, MATRIX *b, MATRIX *res)
{
    int i, j;

    for (i = 0; i < res->rows; i++)
	for (j = 0; j < res->cols; j++)
	    res->a[i][j] = a->a[i][j] + b->a[i][j];
}

void matrix_print(MATRIX *a)
{
    int i, j;

    for (i = 0; i < a->rows; i++) {
	double s = 0;

	for (j = 0; j < a->cols; j++) {
	    printf("%.3lf ", a->a[i][j]);
	    s += a->a[i][j];
	}
	printf("|%.5lf\n", s);
    }
    printf("\n");

}
