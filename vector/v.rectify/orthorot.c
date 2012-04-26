
/***********************************************************************

   orthorot.c

   written by: Markus Metz

   loosely based on crs3d.c

   2D/3D Transformation with orthogonal rotation

************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include "crs.h"

#define MSUCCESS     1		/* SUCCESS */
#define MNPTERR      0		/* NOT ENOUGH POINTS */
#define MUNSOLVABLE -1		/* NOT SOLVABLE */
#define MMEMERR     -2		/* NOT ENOUGH MEMORY */
#define MPARMERR    -3		/* PARAMETER ERROR */
#define MINTERR     -4		/* INTERNAL ERROR */

/***********************************************************************

  FUNCTION PROTOTYPES FOR STATIC (INTERNAL) FUNCTIONS

************************************************************************/

static int calccoef(struct Control_Points_3D *, double *, int);
static int calcscale(struct Control_Points_3D *, double *);

void transpose_matrix(int, int, double **, double **);
void matmult(int, int, int, double **, double **, double **);
void copy_matrix(int, int, double **, double **);
void init_matrix(int, int, double **);
void scale_matrix(int, int, double, double **, double **);
void matrix_multiply(int, int, double **, double *, double *);
void subtract_matrix(int, int, double **, double **, double **);
double trace(int, int, double **);

/***********************************************************************

  TRANSFORM A SINGLE COORDINATE PAIR.

************************************************************************/

int CRS_georef_or(double e1,	/* EASTING TO BE TRANSFORMED */
	          double n1,	/* NORTHING TO BE TRANSFORMED */
	          double z1,	/* HEIGHT TO BE TRANSFORMED */
	          double *e,	/* EASTING, TRANSFORMED */
	          double *n,	/* NORTHING, TRANSFORMED */
	          double *z,	/* HEIGHT, TRANSFORMED */
	          double OR[]	/* TRANSFORMATION COEFFICIENTS */
    )
{
    *e = OR[9]  + OR[12] * (OR[0] * e1 + OR[1] * n1 + OR[2] * z1);
    *n = OR[10] + OR[13] * (OR[3] * e1 + OR[4] * n1 + OR[5] * z1);
    *z = OR[11] + OR[14] * (OR[6] * e1 + OR[7] * n1 + OR[8] * z1);

    return MSUCCESS;
}

/***********************************************************************

  COMPUTE THE FORWARD AND BACKWARD GEOREFFERENCING COEFFICIENTS
  BASED ON A SET OF CONTROL POINTS
   
  ORTHOGONAL TRANSFORMATION (ORTHOGONAL ROTATION MATRIX)
   
  Rotation matrix
  OR[0] OR[1] OR[2]
  OR[3] OR[4] OR[5]
  OR[6] OR[7] OR[8]

  OR[9]  x shift
  OR[10] y shift
  OR[11] z shift

  OR[12] x or global scale
  OR[13] y scale
  OR[14] z scale

************************************************************************/

int CRS_compute_georef_equations_or(struct Control_Points_3D *cp,
                                    double OR12[], double OR21[])
{
    double *tempptr, *OR;
    int status, i, numactive;
    struct Control_Points_3D cpc,     /* center points */
			     cpr;     /* reduced to center */

    cpc.count = 1;
    cpc.e1 = (double *)G_calloc(cpc.count, sizeof(double));
    cpc.e2 = (double *)G_calloc(cpc.count, sizeof(double));
    cpc.n1 = (double *)G_calloc(cpc.count, sizeof(double));
    cpc.n2 = (double *)G_calloc(cpc.count, sizeof(double));
    cpc.z1 = (double *)G_calloc(cpc.count, sizeof(double));
    cpc.z2 = (double *)G_calloc(cpc.count, sizeof(double));
    cpc.status = (int *)G_calloc(cpc.count, sizeof(int));

    cpc.e1[0] = 0.0;
    cpc.e2[0] = 0.0;
    cpc.n1[0] = 0.0;
    cpc.n2[0] = 0.0;
    cpc.z1[0] = 0.0;
    cpc.z2[0] = 0.0;
    cpc.status[0] = 1;

    /* center points */
    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0) {
	    numactive++;
	    cpc.e1[0] += cp->e1[i];
	    cpc.e2[0] += cp->e2[i];
	    cpc.n1[0] += cp->n1[i];
	    cpc.n2[0] += cp->n2[i];
	    cpc.z1[0] += cp->z1[i];
	    cpc.z2[0] += cp->z2[i];
	}
    }

    /* this version of 3D transformation needs 3 control points */
    if (numactive < 3)
	return MNPTERR;

    cpc.e1[0] /= numactive;
    cpc.e2[0] /= numactive;
    cpc.n1[0] /= numactive;
    cpc.n2[0] /= numactive;
    cpc.z1[0] /= numactive;
    cpc.z2[0] /= numactive;

    /* shift to center points */
    cpr.count = numactive;
    cpr.e1 = (double *)G_calloc(cpr.count, sizeof(double));
    cpr.e2 = (double *)G_calloc(cpr.count, sizeof(double));
    cpr.n1 = (double *)G_calloc(cpr.count, sizeof(double));
    cpr.n2 = (double *)G_calloc(cpr.count, sizeof(double));
    cpr.z1 = (double *)G_calloc(cpr.count, sizeof(double));
    cpr.z2 = (double *)G_calloc(cpr.count, sizeof(double));
    cpr.status = (int *)G_calloc(cpr.count, sizeof(int));

    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0) {
	    cpr.e1[numactive] = cp->e1[i] - cpc.e1[0];
	    cpr.e2[numactive] = cp->e2[i] - cpc.e2[0];
	    cpr.n1[numactive] = cp->n1[i] - cpc.n1[0];
	    cpr.n2[numactive] = cp->n2[i] - cpc.n2[0];
	    cpr.z1[numactive] = cp->z1[i] - cpc.z1[0];
	    cpr.z2[numactive] = cp->z2[i] - cpc.z2[0];
	    cpr.status[numactive] = 1;
	    numactive++;
	}
    }
    
    /* CALCULATE THE FORWARD TRANSFORMATION COEFFICIENTS */

    OR = OR12;
    status = calccoef(&cpr, OR, 3);

    calcscale(&cpr, OR);

    /* CALCULATE FORWARD SHIFTS */

    OR[9] = OR[10] = OR[11] = 0.0;
    
    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0) {
	    OR[9]  += cp->e2[i] - OR[12] *
	              (OR[0] * cp->e1[i] +
		       OR[1] * cp->n1[i] + 
		       OR[2] * cp->z1[i]);
	    OR[10] += cp->n2[i] - OR[13] *
	              (OR[3] * cp->e1[i] +
		       OR[4] * cp->n1[i] +
		       OR[5] * cp->z1[i]);
	    OR[11] += cp->z2[i] - OR[14] *
	              (OR[6] * cp->e1[i] +
		       OR[7] * cp->n1[i] +
		       OR[8] * cp->z1[i]);
	    
	    numactive++;
	}
    }
    OR[9] /= numactive;
    OR[10] /= numactive;
    OR[11] /= numactive;
    
    /* SWITCH THE 1 AND 2 EASTING, NORTHING, AND HEIGHT ARRAYS */

    tempptr = cpr.e1;
    cpr.e1 = cpr.e2;
    cpr.e2 = tempptr;
    tempptr = cpr.n1;
    cpr.n1 = cpr.n2;
    cpr.n2 = tempptr;
    tempptr = cpr.z1;
    cpr.z1 = cpr.z2;
    cpr.z2 = tempptr;

    /* CALCULATE THE BACKWARD TRANSFORMATION COEFFICIENTS */

    OR = OR21;
    status = calccoef(&cpr, OR, 3);

    if (status != MSUCCESS)
	return status;

    calcscale(&cpr, OR);

    /* SWITCH THE 1 AND 2 EASTING, NORTHING, AND HEIGHT ARRAYS BACK */

    tempptr = cpr.e1;
    cpr.e1 = cpr.e2;
    cpr.e2 = tempptr;
    tempptr = cpr.n1;
    cpr.n1 = cpr.n2;
    cpr.n2 = tempptr;
    tempptr = cpr.z1;
    cpr.z1 = cpr.z2;
    cpr.z2 = tempptr;

    /* CALCULATE BACKWARD SHIFTS */

    OR[9] = OR[10] = OR[11] = 0.0;
    
    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0) {
	    OR[9]  += cp->e1[i] - OR[12] *
	              (OR[0] * cp->e2[i] +
		       OR[1] * cp->n2[i] +
		       OR[2] * cp->z2[i]);
	    OR[10] += cp->n1[i] - OR[13] *
	              (OR[3] * cp->e2[i] +
		       OR[4] * cp->n2[i] +
		       OR[5] * cp->z2[i]);
	    OR[11] += cp->z1[i] - OR[14] *
	              (OR[6] * cp->e2[i] +
		       OR[7] * cp->n2[i] +
		       OR[8] * cp->z2[i]);
	    numactive++;
	}
    }
    OR[9] /= numactive;
    OR[10] /= numactive;
    OR[11] /= numactive;
    
    OR = OR12;
    G_debug(1, "********************************");
    G_debug(1, "Forward transformation:");
    G_debug(1, "Orthogonal rotation matrix:");
    G_debug(1, "%.4f %.4f %.4f", OR[0], OR[1], OR[2]);
    G_debug(1, "%.4f %.4f %.4f", OR[3], OR[4], OR[5]);
    G_debug(1, "%.4f %.4f %.4f", OR[6], OR[7], OR[8]);
    G_debug(1, "x, y, z shift: %.4f %.4f %.4f", OR[9], OR[10], OR[11]);
    G_debug(1, "x, y, z scale: %.4f %.4f %.4f", OR[12], OR[13], OR[14]);

    OR = OR21;
    G_debug(1, "********************************");
    G_debug(1, "Backward transformation:");
    G_debug(1, "Orthogonal rotation matrix:");
    G_debug(1, "%.4f %.4f %.4f", OR[0], OR[1], OR[2]);
    G_debug(1, "%.4f %.4f %.4f", OR[3], OR[4], OR[5]);
    G_debug(1, "%.4f %.4f %.4f", OR[6], OR[7], OR[8]);
    G_debug(1, "x, y, z shift: %.4f %.4f %.4f", OR[9], OR[10], OR[11]);
    G_debug(1, "x, y, z scale: %.4f %.4f %.4f", OR[12], OR[13], OR[14]);

    return status;
}

/***********************************************************************

  COMPUTE THE GEOREFFERENCING COEFFICIENTS
  BASED ON A SET OF CONTROL POINTS

************************************************************************/

static int calccoef(struct Control_Points_3D *cp, double OR[], int ndims)
{
    double **src_mat = NULL;
    double **src_mat_T = NULL;
    double **dest_mat = NULL;
    double **dest_mat_T = NULL;
    double **src_dest_mat = NULL;
    double *S_vec = NULL;
    double **R_mat = NULL;
    double **R_mat_T = NULL;
    double **mat_mn1 = NULL;
    double **mat_mn2 = NULL;
    double **mat_nm1 = NULL;
    double **mat_nm2 = NULL;
    double **mat_nn1 = NULL;
    double **E_mat = NULL;
    double **P_mat = NULL;
    double **Q_mat = NULL;
    double *D_vec = NULL;
    double *one_vec = NULL;
    double trace1 = 0.0;
    double trace2 = 0.0;
    int numactive;		/* NUMBER OF ACTIVE CONTROL POINTS */
    int m, n, i, j;
    int status;

    /* CALCULATE THE NUMBER OF VALID CONTROL POINTS */

    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0)
	    numactive++;
    }
    m = numactive;
    n = ndims;

    src_mat = G_alloc_matrix(m, n);
    dest_mat = G_alloc_matrix(m, n);

    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0) {
	    src_mat[numactive][0] = cp->e1[i];
	    src_mat[numactive][1] = cp->n1[i];
	    src_mat[numactive][2] = cp->z1[i];

	    dest_mat[numactive][0] = cp->e2[i];
	    dest_mat[numactive][1] = cp->n2[i];
	    dest_mat[numactive][2] = cp->z2[i];

	    numactive++;
	}
    }

    D_vec = G_alloc_vector(ndims);

    src_mat_T = G_alloc_matrix(n, m);
    dest_mat_T = G_alloc_matrix(n, m);
    src_dest_mat = G_alloc_matrix(n, n);
    R_mat = G_alloc_matrix(n, n);
    R_mat_T = G_alloc_matrix(n, n);

    mat_mn1 = G_alloc_matrix(m, n);
    mat_mn2 = G_alloc_matrix(m, n);
    mat_nm1 = G_alloc_matrix(n, m);
    mat_nm2 = G_alloc_matrix(n, m);
    mat_nn1 = G_alloc_matrix(n, n);

    E_mat = G_alloc_matrix(m, m);
    P_mat = G_alloc_matrix(ndims, ndims);
    Q_mat = G_alloc_matrix(ndims, ndims);

    transpose_matrix(m, n, dest_mat, dest_mat_T);

    for (i = 0; i < m; i++) {
	for (j = 0; j < m; j++) {
	    if (i != j) {
		E_mat[i][j] = -1.0 / (double)m;
	    }
	    else{
		E_mat[i][j] = 1.0 - 1.0 / (double)m;
	    }
	}
    }

    matmult(n, m, m, dest_mat_T, E_mat, mat_nm1);
    matmult(n, m, n, mat_nm1, src_mat, src_dest_mat);
    copy_matrix(n, n, src_dest_mat, P_mat);
    copy_matrix(n, n, src_dest_mat, mat_nn1);

    status = G_math_svduv(D_vec, mat_nn1, P_mat, n, Q_mat, n);

    if (status == 0)
	status = MSUCCESS;

    transpose_matrix(n, n, P_mat, mat_nn1);

    /* rotation matrix */
    matmult(n, n, n, Q_mat, mat_nn1, R_mat_T);
    transpose_matrix(n, n, R_mat_T, R_mat);

    /* scale */
    matmult(n, n, n, src_dest_mat, R_mat_T, mat_nn1);
    trace1 = trace(n, n, mat_nn1);

    transpose_matrix(m, n, src_mat, src_mat_T);
    matmult(n, m, m, src_mat_T, E_mat, mat_nm1);
    matmult(n, m, n, mat_nm1, src_mat, mat_nn1);
    trace2 = trace(n, n, mat_nn1);

    OR[14] = trace1 / trace2;

    /* shifts */
    matmult(m, n, n, src_mat, R_mat_T, mat_mn1);
    scale_matrix(m, n, OR[14], mat_mn1, mat_mn2);
    subtract_matrix(m, n, dest_mat, mat_mn2, mat_mn1);
    scale_matrix(m, n, 1.0 / m, mat_mn1, mat_mn2);
    transpose_matrix(m, n, mat_mn2, mat_nm1);

    S_vec = G_alloc_vector(n);
    one_vec = G_alloc_vector(m);

    for (i = 0; i < m; i++){
	one_vec[i] = 1.0;
    }

    matrix_multiply(n, m, mat_nm1, one_vec, S_vec);

    /* matrix to vector */
    for (i = 0; i < ndims; i++) {
	for (j = 0; j < ndims; j++) {
	    OR[i * ndims + j] = R_mat[i][j];
	}
    }
    
    G_free_matrix(src_mat);
    G_free_matrix(src_mat_T);
    G_free_matrix(dest_mat);
    G_free_matrix(dest_mat_T);
    G_free_matrix(src_dest_mat);
    G_free_vector(D_vec);
    G_free_matrix(E_mat);
    G_free_matrix(P_mat);
    G_free_matrix(Q_mat);
    G_free_matrix(R_mat);
    G_free_matrix(R_mat_T);
    G_free_matrix(mat_mn1);
    G_free_matrix(mat_mn2);
    G_free_matrix(mat_nm1);
    G_free_matrix(mat_nm2);
    G_free_matrix(mat_nn1);
    G_free_vector(S_vec);
    G_free_vector(one_vec);

    return status;
}

/***********************************************************************

  CALCULATE SCALE

************************************************************************/
static int calcscale(struct Control_Points_3D *cp, double OR[])
{
    double sumX, sumY, sumsqX, sumsqY, sumXY;
    int numactive;		/* NUMBER OF ACTIVE CONTROL POINTS */
    int i;

    /* CALCULATE SCALE */
    sumX = sumY = sumsqX = sumsqY = sumXY = 0.0;
    
    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0) {
	    double c1, c2;
	    
	    c1 = OR[0] * cp->e1[i] + OR[1] * cp->n1[i] + OR[2] * cp->z1[i];
	    c2 = cp->e2[i];

	    sumX += c1;
	    sumY += c2;
	    sumsqX += c1 * c1;
	    sumsqY += c2 * c2;
	    sumXY += c1 * c2;

	    c1 = OR[3] * cp->e1[i] + OR[4] * cp->n1[i] + OR[5] * cp->z1[i];
	    c2 = cp->n2[i];

	    sumX += c1;
	    sumY += c2;
	    sumsqX += c1 * c1;
	    sumsqY += c2 * c2;
	    sumXY += c1 * c2;

	    c1 = OR[6] * cp->e1[i] + OR[7] * cp->n1[i] + OR[8] * cp->z1[i];
	    c2 = cp->z2[i];

	    sumX += c1;
	    sumY += c2;
	    sumsqX += c1 * c1;
	    sumsqY += c2 * c2;
	    sumXY += c1 * c2;
	    
	    numactive++;
	}
    }

    OR[12] = (sumXY - sumX * sumY / numactive) /
            (sumsqX - sumX * sumX / numactive);
	    
    if (fabs(OR[12] - OR[14]) > 10 * GRASS_EPSILON) {
	G_debug(1, "Scale mismatch: %.4f %.4f", OR[12], OR[14]);
	OR[12] = OR[14];
    }

    OR[13] = OR[14] = OR[12];

    return MSUCCESS;
}

void transpose_matrix(int m, int n, double **src_matrix, double **dest_matrix)
{
    int i, j;

    for(i = 0; i < m; i++) {
	for(j = 0; j < n; j++)  {
	    dest_matrix[j][i] = src_matrix[i][j];
	}
    }
}

void matmult(int m, int n, int p, double **mat1, double **mat2, double **mat3)
{
    int i, j, k;
    double sum;

    /* input   mat1: m x n */
    /* input   mat2: n x p */
    /* output  mat3: m x p */
    for (i = 0; i < m; i++) {
	for (j = 0; j < p; j++) {
	    sum = 0.0;
	    for (k = 0; k < n; k++) {
		sum += mat1[i][k] * mat2[k][j];
	    }
	    mat3[i][j] = sum;
	}
    }
}

void copy_matrix(int n, int m, double **src_matrix, double **dest_matrix)
{
    int i, j;

    for (i = 0; i < n; i++) {
	for (j = 0; j < m; j++) {
	    dest_matrix[i][j] = src_matrix[i][j];
	}
    }
}

double trace(int n, int m, double **matrix)
{
    int i;
    double t = 0.0;

    for (i = 0; i < n; i++) {
	t += matrix[i][i];
    }

    return t;
}

void init_matrix(int n, int m, double **matrix)
{
    int i, j;

    for (i = 0; i < n; i++) {
	for (j = 0; j < m; j++) {
	    matrix[i][j] = 0.0;
	}
    }
}

void scale_matrix(int n, int m, double scal, double **src_matrix,
                  double **dest_matrix)
{
    int i, j;

    for (i = 0; i < n; i++) {
	for (j = 0; j < m; j++) {
	    dest_matrix[i][j] = scal * src_matrix[i][j];
	}
    }
}

void subtract_matrix(int n, int m, double **mat1, double **mat2,
                     double **mat3)
{
    int i, j;

    for (i = 0; i < n; i++) {
	for (j = 0; j < m; j++) {
	    mat3[i][j] = mat1[i][j] - mat2[i][j];
	}
    }
}

void matrix_multiply(int n, int m, double **mat, double *iv,
                     double *ov)
{
    int i, j;

    for (i = 0; i < n; i++) {
	ov[i] = 0.0;
	for(j = 0; j < m; j++) {
	    ov[i] += mat[i][j] * iv[j];
	}
    }
}
