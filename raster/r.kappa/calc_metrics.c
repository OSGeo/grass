#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"

static int longcomp(const void *aa, const void *bb);
static int collapse(long *l, int n);


void calc_metrics(void)
{
    int i, j, k;
    long *clst;
    int ncat1, ncat2;
    int cndx;
    double *pi, *pj, *pii;
    double p0 = 0, pC = 0;
    double inter1 = 0, inter2 = 0;
    int a_i, b_i;

    /* get the cat lists */
    rlst = (long *)G_calloc(nstats * 2, sizeof(long));
    clst = (long *)G_calloc(nstats, sizeof(long));
    for (i = 0; i < nstats; i++) {
        rlst[i] = Gstats[i].cats[0];
        clst[i] = Gstats[i].cats[1];
    }

    /* sort the cat lists */
    qsort(rlst, nstats, sizeof(long), longcomp);
    qsort(clst, nstats, sizeof(long), longcomp);

    /* remove repeated cats */
    ncat1 = collapse(rlst, nstats);
    ncat2 = collapse(clst, nstats);

    /* copy clst to the end of rlst, remove repeated cats, and free unused memory */
    for (i = 0; i < ncat2; i++)
        rlst[ncat1 + i] = clst[i];
    qsort(rlst, ncat1 + ncat2, sizeof(long), longcomp);
    ncat = collapse(rlst, ncat1 + ncat2);
    rlst = (long *)G_realloc(rlst, ncat * sizeof(long));
    G_free(clst);

    /* fill matrix with observed counts */
    metrics = (METRICS *) G_malloc(sizeof(METRICS));
    metrics->matrix = (long *)G_malloc((size_t)ncat * ncat * sizeof(long));
    for (i = 0; i < ncat * ncat; i++)
        metrics->matrix[i] = 0;
    for (i = 0; i < nstats; i++) {
        for (j = 0; j < ncat; j++)
            if (rlst[j] == Gstats[i].cats[0])
                break;
        for (k = 0; k < ncat; k++)
            if (rlst[k] == Gstats[i].cats[1])
                break;
        /* matrix: reference in columns, classification in rows */
        metrics->matrix[j * ncat + k] = Gstats[i].count;
    }

    /* Calculate marginals */
    metrics->obs = 0;
    metrics->correct = 0;
    metrics->colsum = (long *)G_malloc(ncat * sizeof(long));
    metrics->rowsum = (long *)G_malloc(ncat * sizeof(long));
    for (cndx = 0; cndx < ncat; cndx++) {
        long t_col = 0;
        long t_row = 0;
        long x = cndx;

        for (k = 0; k < ncat; k++) {
            t_col += metrics->matrix[x];
            x += ncat;
            t_row += metrics->matrix[cndx * ncat + k];
        }
        metrics->obs += t_row;
        metrics->colsum[cndx] = t_col;
        metrics->rowsum[cndx] = t_row;
    }
    if (metrics->obs == 0) {
        metrics->total_acc = 0;
        metrics->kappa = -999;
        metrics->kappa_var = -999;
        return;
    }

    /* Calculate kappa values */
    pi = (double *)G_calloc(ncat, sizeof(double));
    pj = (double *)G_calloc(ncat, sizeof(double));
    pii = (double *)G_calloc(ncat, sizeof(double));
    metrics->cond_kappa = (double *)G_calloc(ncat, sizeof(double));
    metrics->user_acc = (double *)G_calloc(ncat, sizeof(double));
    metrics->prod_acc = (double *)G_calloc(ncat, sizeof(double));

    for (i = 0; i < ncat; i++) {
        for (j = 0; j < nstats; j++) {
            if (Gstats[j].cats[0] == rlst[i]) {
                pi[i] += Gstats[j].count;
            }

            if (Gstats[j].cats[1] == rlst[i]) {
                pj[i] += Gstats[j].count;
            }

            if ((Gstats[j].cats[0] == Gstats[j].cats[1]) &&
                (Gstats[j].cats[0] == rlst[i])) {
                pii[i] += Gstats[j].count;
            }
        }
        metrics->correct += pii[i];
    }

    metrics->total_acc = 100. * metrics->correct / metrics->obs;

    /* turn observations into probabilities */
    for (i = 0; i < ncat; i++) {
        pi[i] = pi[i] / metrics->obs;
        pj[i] = pj[i] / metrics->obs;
        pii[i] = pii[i] / metrics->obs;
        if (pi[i] == 0)
            metrics->user_acc[i] = -999;
        else
            metrics->user_acc[i] = 100 * (pii[i] / pi[i]);
        if (pj[i] == 0)
            metrics->prod_acc[i] = -999;
        else
            metrics->prod_acc[i] = 100 * (pii[i] / pj[i]);
        /* theta 1 */
        p0 += pii[i];
        /* theta 2 */
        pC += pi[i] * pj[i];
    }
    if (pC != 1)
        metrics->kappa = (p0 - pC) / (1 - pC);
    else
        metrics->kappa = -999;

    /* conditional kappa */
    for (i = 0; i < ncat; i++) {
        if (pi[i] == 0 || (pi[i] == 1 && pj[i] == 1))
            metrics->cond_kappa[i] = -999;
        else
            metrics->cond_kappa[i] =
                (pii[i] - pi[i] * pj[i]) / (pi[i] - pi[i] * pj[i]);
        inter1 += pii[i] * pow(((1 - pC) - (1 - p0) * (pi[i] + pj[i])), 2.);
    }

    /* kappa variance */
    for (j = 0; j < nstats; j++) {
        if (Gstats[j].cats[0] != Gstats[j].cats[1]) {
            for (i = 0; i < ncat; i++) {
                if (Gstats[j].cats[0] == rlst[i])
                    a_i = i;
                if (Gstats[j].cats[1] == rlst[i])
                    b_i = i;
            }
            inter2 +=
                Gstats[j].count * pow((pi[a_i] + pj[b_i]), 2.) / metrics->obs;
        }
    }
    metrics->kappa_var = (inter1 + pow((1 - p0), 2.) * inter2 -
                          pow((p0 * pC - 2 * pC + p0), 2.)) / pow((1 - pC),
                                                                  4.) /
        metrics->obs;

    G_free(pi);
    G_free(pj);
    G_free(pii);
};


/* remove repeated values */
static int collapse(long *l, int n)
{
    long *c;
    int m;

    c = l;
    m = 1;
    while (n-- > 0) {
        if (*c != *l) {
            c++;
            *c = *l;
            m++;
        }
        l++;
    }

    return m;
}


static int longcomp(const void *aa, const void *bb)
{
    const long *a = aa;
    const long *b = bb;

    if (*a < *b)
        return -1;

    return (*a > *b);
}
