#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "growing.h"

double **P, **cvxHull, **punti_bordo;

/*----------------------------------------------------------------------------------------------------*/
void regGrow8(struct Cell_head Elaboration, struct element_grow **mat,
	      double **punti, int *lung, int r, int c, int v, double Th_j,
	      int maxP)
{
    extern int count_obj;

    mat[r][c].clas = v;
    mat[r][c].obj = count_obj;

    punti[*lung][0] = c;
    punti[*lung][1] = r;
    punti[*lung][2] = mat[r][c].interp;

    assert((*lung)++ < maxP - 1);	/* Condition to finish regGrow8 */

    if (r - 1 >= 0) {
	if ((mat[r - 1][c].clas > Th_j) && (mat[r - 1][c].clas < v) &&
	    (mat[r - 1][c].fi != 0))
	    regGrow8(Elaboration, mat, punti, lung, r - 1, c, v, Th_j, maxP);
    }

    if (c - 1 >= 0) {
	if ((mat[r][c - 1].clas > Th_j) && (mat[r][c - 1].clas < v) &&
	    (mat[r][c - 1].fi != 0))
	    regGrow8(Elaboration, mat, punti, lung, r, c - 1, v, Th_j, maxP);
    }

    if (c + 1 < Elaboration.cols) {
	if ((mat[r][c + 1].clas > Th_j) && (mat[r][c + 1].clas < v) &&
	    (mat[r][c + 1].fi != 0))
	    regGrow8(Elaboration, mat, punti, lung, r, c + 1, v, Th_j, maxP);
    }

    if (r + 1 < Elaboration.rows) {
	if ((mat[r + 1][c].clas > Th_j) && (mat[r + 1][c].clas < v) &&
	    (mat[r + 1][c].fi != 0))
	    regGrow8(Elaboration, mat, punti, lung, r + 1, c, v, Th_j, maxP);
    }

    if ((r - 1 >= 0) && (c - 1 >= 0)) {
	if ((mat[r - 1][c - 1].clas > Th_j) && (mat[r - 1][c - 1].clas < v) &&
	    (mat[r - 1][c - 1].fi != 0))
	    regGrow8(Elaboration, mat, punti, lung, r - 1, c - 1, v, Th_j,
		     maxP);
    }

    if ((r - 1 >= 0) && (c + 1 < Elaboration.cols)) {
	if ((mat[r - 1][c + 1].clas > Th_j) && (mat[r - 1][c + 1].clas < v) &&
	    (mat[r - 1][c + 1].fi != 0))
	    regGrow8(Elaboration, mat, punti, lung, r - 1, c + 1, v, Th_j,
		     maxP);
    }

    if ((r + 1 < Elaboration.rows) && (c - 1 >= 0)) {
	if ((mat[r + 1][c - 1].clas > Th_j) && (mat[r + 1][c - 1].clas < v) &&
	    (mat[r + 1][c - 1].fi != 0))
	    regGrow8(Elaboration, mat, punti, lung, r + 1, c - 1, v, Th_j,
		     maxP);
    }

    if ((r + 1 < Elaboration.rows) && (c + 1 < Elaboration.cols)) {
	if ((mat[r + 1][c + 1].clas > Th_j) && (mat[r + 1][c + 1].clas < v) &&
	    (mat[r + 1][c + 1].fi != 0))
	    regGrow8(Elaboration, mat, punti, lung, r + 1, c + 1, v, Th_j,
		     maxP);
    }
}

int ccw(double **P, int i, int j, int k)
/* true if points i, j, k counterclockwise */
{
    double a, b, c, d;

    /* It compares coord differences */
    a = P[i][0] - P[j][0];
    b = P[i][1] - P[j][1];
    c = P[k][0] - P[j][0];
    d = P[k][1] - P[j][1];
    return a * d - b * c <= 0;
}


int cmpl(const void *a, const void *b)
{
    double v;

    CMPM(0, a, b);
    CMPM(1, b, a);
    return 0;
}

int cmph(const void *a, const void *b)
{
    return cmpl(b, a);
}

int make_chain(double **V, int n, int (*cmp) (const void *, const void *))
{
    int i, j, s = 1;
    double *t;

    qsort(V, (size_t) n, sizeof(double *), cmp);	/* It sorts the V's index */

    /* */
    for (i = 2; i < n; i++) {
	for (j = s; j >= 1 && ccw(V, i, j, j - 1); j--) ;
	s = j + 1;
	t = V[s];
	V[s] = V[i];
	V[i] = t;
    }
    return s;
}

int ch2d(double **P, int n)
{
    int u = make_chain(P, n, cmpl);	/* make lower hull */

    if (!n)
	return 0;
    P[n] = P[0];

    return u + make_chain(P + u, n - u + 1, cmph);	/* make upper hull */
}

void print_hull(double **P, double **pti, int m, double **h)
{
    int i;

    for (i = 0; i < m; i++) {
	h[i][0] = pti[(P[i] - pti[0]) / 2][0];
	h[i][1] = pti[(P[i] - pti[0]) / 2][1];
	h[i][2] = pti[(P[i] - pti[0]) / 2][2];
    }
}

int checkHull(int cR, int cC, double **oldHull, int lungOld)
{
    double **newP;
    double **newPoint;
    int count, lungHullNew;

    newP = Pvector(0, lungOld + 1);
    newPoint = G_alloc_matrix(lungOld + 1, 2);

    for (count = 0; count < lungOld; count++) {
	newPoint[count][0] = oldHull[count][0];
	newPoint[count][1] = oldHull[count][1];
	newP[count] = newPoint[count];
    }

    newPoint[lungOld][0] = cC;
    newPoint[lungOld][1] = cR;

    newP[lungOld] = newPoint[lungOld];

    lungHullNew = ch2d(newP, lungOld + 1);

    if (lungOld != lungHullNew) {
	G_free_matrix(newPoint);
	free_Pvector(newP, 0, lungOld + 1);
	return 0;
    }
    else {
	for (count = 0; count < lungOld; count++) {
	    if ((oldHull[count][0] != newP[count][0]) ||
		(oldHull[count][1] != newP[count][1])) {
		G_free_matrix(newPoint);
		free_Pvector(newP, 0, lungOld + 1);
		return 0;
	    }
	}
    }
    G_free_matrix(newPoint);
    free_Pvector(newP, 0, lungOld + 1);
    return 1;
}

/*---------------------------------------------------------------------------------------------------*/
double pianOriz(double **punti, int obsNum, double *minNS, double *minEW,
		double *maxNS, double *maxEW, struct element_grow **mat,
		int CBordo)
{
    int c1;
    double minBordo, medioBordo;	/*, minBordo1; */

    /*Calcola coordinate min e max della zona e media delle righe e delle colonne */
    *minNS = punti[0][1];
    *maxNS = punti[0][1];
    *minEW = punti[0][0];
    *maxEW = punti[0][0];

    medioBordo = 0;

    minBordo = punti[0][2];
    /*minBordo1 = punti[0][2]; */

    for (c1 = 0; c1 < obsNum; c1++) {
	if (punti[c1][0] > *maxEW)
	    *maxEW = punti[c1][0];

	if (punti[c1][0] < *minEW)
	    *minEW = punti[c1][0];

	if (punti[c1][1] > *maxNS)
	    *maxNS = punti[c1][1];

	if (punti[c1][1] < *minNS)
	    *minNS = punti[c1][1];
	/*
	   if ((punti[c1][2] < minBordo1) && (mat[(int)(punti[c1][1])][(int)(punti[c1][0])].clas >= 1)
	   && (mat[(int)(punti[c1][1])][(int)(punti[c1][0])].clas < CBordo)) {
	   minBordo1 = punti[c1][2];
	   }
	 */
	if (punti[c1][2] < minBordo)
	    minBordo = punti[c1][2];

	medioBordo += punti[c1][2];
    }
    medioBordo /= obsNum;
    return medioBordo;
}

/*----------------------------------------------------------------------------------------------*/
double **Pvector(long nl, long nh)
{
    double **v;

    v = (double **)calloc((size_t) (nh - nl + 1 + NR_END), sizeof(double *));
    if (!v)
	nrerror("allocation failure in dvector()");

    return v - nl + NR_END;
}

struct element_grow **P_alloc_element(int rows, int cols)
{
    struct element_grow **m;
    int i;

    m = (struct element_grow **)G_calloc((rows + 1),
					 sizeof(struct element_grow *));
    m[0] =
	(struct element_grow *)G_calloc(rows * cols,
					sizeof(struct element_grow));

    for (i = 1; i <= rows; i++)
	m[i] = m[i - 1] + cols;

    return m;
}

void nrerror(char error_text[])
/* standard error handler */
{
    G_debug(1, "run-time error...");
    G_debug(1, "%s", error_text);
    G_fatal_error(_("...now exiting to system..."));
    exit(EXIT_FAILURE);
}

struct element_grow **structMatrix(long nrl, long nrh, long ncl, long nch)
{
    long i, nrow = nrh - nrl + 1, ncol = nch - ncl + 1;
    struct element_grow **m;

    /* allocate pointers to rows */
    m = (struct element_grow **)calloc((size_t) (nrow + NR_END),
				       sizeof(struct element_grow *));
    if (!m)
	nrerror("allocation failure 1 in matrix()");

    m += NR_END;
    m -= nrl;

    /* allocate rows and set pointers to them */
    m[nrl] =
	(struct element_grow *)calloc((size_t) (nrow * ncol + NR_END),
				      sizeof(struct element_grow));
    if (!m[nrl])
	nrerror("allocation failure 2 in matrix()");

    m[nrl] += NR_END;
    m[nrl] -= ncl;

    for (i = nrl + 1; i <= nrh; i++)
	m[i] = m[i - 1] + ncol;

    /* return pointer to array of pointers to rows */
    return m;
}

void free_Pvector(double **v, long nl, long nh)
{

    free((FREE_ARG) (v + nl - NR_END));
}

void free_structmatrix(struct element_grow **m, long nrl, long nrh, long ncl,
		       long nch)
{
    free((FREE_ARG) (m[nrl] + ncl - NR_END));
    free((FREE_ARG) (m + nrl - NR_END));
}
