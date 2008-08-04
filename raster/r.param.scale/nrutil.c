#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "nrutil.h"


#define NR_END 1
#define FREE_ARG char*

float sqrarg;
double dsqrarg;
double dmaxarg1, dmaxarg2;
double dminarg1, dminarg2;
float maxarg1, maxarg2;
float minarg1, minarg2;
long lmaxarg1, lmaxarg2;
long lminarg1, lminarg2;
int imaxarg1, imaxarg2;
int iminarg1, iminarg2;


/* allocate a float vector with subscript range v[nl..nh] */
float *vector(int nl, int nh)
{
    float *v;

    v = (float *)G_malloc(((nh - nl + 1 + NR_END) * sizeof(float)));

    return v - nl + NR_END;
}


/* allocate an int vector with subscript range v[nl..nh] */
int *ivector(int nl, int nh)
{
    int *v;

    v = (int *)G_malloc(((nh - nl + 1 + NR_END) * sizeof(int)));

    return v - nl + NR_END;
}


/* allocate an unsigned char vector with subscript range v[nl..nh] */
unsigned char *cvector(int nl, int nh)
{
    unsigned char *v;

    v = (unsigned char *)
	G_malloc(((nh - nl + 1 + NR_END) * sizeof(unsigned char)));

    return v - nl + NR_END;
}


/* allocate an unsigned long vector with subscript range v[nl..nh] */
unsigned long *lvector(int nl, int nh)
{
    unsigned long *v;

    v = (unsigned long *)G_malloc(((nh - nl + 1 + NR_END) * sizeof(long)));

    return v - nl + NR_END;
}


/* allocate a double vector with subscript range v[nl..nh] */
double *dvector(int nl, int nh)
{
    double *v;

    v = (double *)G_malloc(((nh - nl + 1 + NR_END) * sizeof(double)));

    return v - nl + NR_END;
}


/* allocate a float matrix with subscript range m[nrl..nrh][ncl..nch] */
float **matrix(int nrl, int nrh, int ncl, int nch)
{
    int i, nrow = nrh - nrl + 1, ncol = nch - ncl + 1;
    float **m;

    /* allocate pointers to rows */
    m = (float **)G_malloc(((nrow + NR_END) * sizeof(float *)));
    m += NR_END;
    m -= nrl;

    /* allocate rows and set pointers to them */
    m[nrl] = (float *)G_malloc(((nrow * ncol + NR_END) * sizeof(float)));
    m[nrl] += NR_END;
    m[nrl] -= ncl;

    for (i = nrl + 1; i <= nrh; i++)
	m[i] = m[i - 1] + ncol;

    /* return pointer to array of pointers to rows */
    return m;
}


/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
double **dmatrix(int nrl, int nrh, int ncl, int nch)
{
    int i, nrow = nrh - nrl + 1, ncol = nch - ncl + 1;
    double **m;

    /* allocate pointers to rows */
    m = (double **)G_malloc(((nrow + NR_END) * sizeof(double *)));
    m += NR_END;
    m -= nrl;

    /* allocate rows and set pointers to them */
    m[nrl] = (double *)G_malloc(((nrow * ncol + NR_END) * sizeof(double)));
    m[nrl] += NR_END;
    m[nrl] -= ncl;

    for (i = nrl + 1; i <= nrh; i++)
	m[i] = m[i - 1] + ncol;

    /* return pointer to array of pointers to rows */
    return m;
}


/* allocate a int matrix with subscript range m[nrl..nrh][ncl..nch] */
int **imatrix(int nrl, int nrh, int ncl, int nch)
{
    int i, nrow = nrh - nrl + 1, ncol = nch - ncl + 1;
    int **m;

    /* allocate pointers to rows */
    m = (int **)G_malloc(((nrow + NR_END) * sizeof(int *)));
    m += NR_END;
    m -= nrl;


    /* allocate rows and set pointers to them */
    m[nrl] = (int *)G_malloc(((nrow * ncol + NR_END) * sizeof(int)));
    m[nrl] += NR_END;
    m[nrl] -= ncl;

    for (i = nrl + 1; i <= nrh; i++)
	m[i] = m[i - 1] + ncol;

    /* return pointer to array of pointers to rows */
    return m;
}


/* point a submatrix [newrl..][newcl..] to a[oldrl..oldrh][oldcl..oldch] */
float **submatrix(float **a, int oldrl, int oldrh, int oldcl, int oldch,
		  int newrl, int newcl)
{
    int i, j, nrow = oldrh - oldrl + 1, ncol = oldcl - newcl;
    float **m;

    /* allocate array of pointers to rows */
    m = (float **)G_malloc(((nrow + NR_END) * sizeof(float *)));
    m += NR_END;
    m -= newrl;

    /* set pointers to rows */
    for (i = oldrl, j = newrl; i <= oldrh; i++, j++)
	m[j] = a[i] + ncol;

    /* return pointer to array of pointers to rows */
    return m;
}


/* allocate a float matrix m[nrl..nrh][ncl..nch] that points to the matrix
   declared in the standard C manner as a[nrow][ncol], where nrow=nrh-nrl+1
   and ncol=nch-ncl+1. The routine should be called with the address
   &a[0][0] as the first argument. */
float **convert_matrix(float *a, int nrl, int nrh, int ncl, int nch)
{
    int i, j, nrow = nrh - nrl + 1, ncol = nch - ncl + 1;
    float **m;

    /* allocate pointers to rows */
    m = (float **)G_malloc(((nrow + NR_END) * sizeof(float *)));
    m += NR_END;
    m -= nrl;

    /* set pointers to rows */
    m[nrl] = a - ncl;
    for (i = 1, j = nrl + 1; i < nrow; i++, j++)
	m[j] = m[j - 1] + ncol;

    /* return pointer to array of pointers to rows */
    return m;
}


/* allocate a float 3tensor with range t[nrl..nrh][ncl..nch][ndl..ndh] */
float ***f3tensor(int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
    int i, j, nrow = nrh - nrl + 1, ncol = nch - ncl + 1, ndep =
	ndh - ndl + 1;
    float ***t;

    /* allocate pointers to pointers to rows */
    t = (float ***)G_malloc(((nrow + NR_END) * sizeof(float **)));
    t += NR_END;
    t -= nrl;

    /* allocate pointers to rows and set pointers to them */
    t[nrl] = (float **)G_malloc(((nrow * ncol + NR_END) * sizeof(float *)));
    t[nrl] += NR_END;
    t[nrl] -= ncl;

    /* allocate rows and set pointers to them */
    t[nrl][ncl] =
	(float *)G_malloc(((nrow * ncol * ndep + NR_END) * sizeof(float)));
    t[nrl][ncl] += NR_END;
    t[nrl][ncl] -= ndl;

    for (j = ncl + 1; j <= nch; j++)
	t[nrl][j] = t[nrl][j - 1] + ndep;
    for (i = nrl + 1; i <= nrh; i++) {
	t[i] = t[i - 1] + ncol;
	t[i][ncl] = t[i - 1][ncl] + ncol * ndep;

	for (j = ncl + 1; j <= nch; j++)
	    t[i][j] = t[i][j - 1] + ndep;
    }

    /* return pointer to array of pointers to rows */
    return t;
}


/* free a float vector allocated with vector() */
void free_vector(float *v, int nl, int nh)
{
    G_free((FREE_ARG) (v + nl - NR_END));
}


/* free an int vector allocated with ivector() */
void free_ivector(int *v, int nl, int nh)
{
    G_free((FREE_ARG) (v + nl - NR_END));
}


/* free an unsigned char vector allocated with cvector() */
void free_cvector(unsigned char *v, int nl, int nh)
{
    G_free((FREE_ARG) (v + nl - NR_END));
}


/* free an unsigned long vector allocated with lvector() */
void free_lvector(unsigned long *v, int nl, int nh)
{
    G_free((FREE_ARG) (v + nl - NR_END));
}


/* free a double vector allocated with dvector() */
void free_dvector(double *v, int nl, int nh)
{
    G_free((FREE_ARG) (v + nl - NR_END));
}


/* free a float matrix allocated by matrix() */
void free_matrix(float **m, int nrl, int nrh, int ncl, int nch)
{
    G_free((FREE_ARG) (m[nrl] + ncl - NR_END));
    G_free((FREE_ARG) (m + nrl - NR_END));
}


/* free a double matrix allocated by dmatrix() */
void free_dmatrix(double **m, int nrl, int nrh, int ncl, int nch)
{
    G_free((FREE_ARG) (m[nrl] + ncl - NR_END));
    G_free((FREE_ARG) (m + nrl - NR_END));
}


/* free an int matrix allocated by imatrix() */
void free_imatrix(int **m, int nrl, int nrh, int ncl, int nch)
{
    G_free((FREE_ARG) (m[nrl] + ncl - NR_END));
    G_free((FREE_ARG) (m + nrl - NR_END));
}


/* free a submatrix allocated by submatrix() */
void free_submatrix(float **b, int nrl, int nrh, int ncl, int nch)
{
    G_free((FREE_ARG) (b + nrl - NR_END));
}


/* free a matrix allocated by convert_matrix() */
void free_convert_matrix(float **b, int nrl, int nrh, int ncl, int nch)
{
    G_free((FREE_ARG) (b + nrl - NR_END));
}


/* free a float f3tensor allocated by f3tensor() */
void free_f3tensor(float ***t, int nrl, int nrh, int ncl, int nch,
		   int ndl, int ndh)
{
    G_free((FREE_ARG) (t[nrl][ncl] + ndl - NR_END));
    G_free((FREE_ARG) (t[nrl] + ncl - NR_END));
    G_free((FREE_ARG) (t + nrl - NR_END));
}
