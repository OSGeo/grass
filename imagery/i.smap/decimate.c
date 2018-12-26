#include <grass/gis.h>
#include "bouman.h"
#include "region.h"


static void decimate(LIKELIHOOD ***, struct Region *, int, LIKELIHOOD ***,
		     double);
static void up_ll(LIKELIHOOD *, int, double, LIKELIHOOD *);


void make_pyramid(LIKELIHOOD **** ll_pym,	/* log likelihood pyramid, ll_pym[scale][i][j][class] */
		  struct Region *region,	/* specifies image subregion */
		  int M,	/* number of classes */
		  double *alpha	/* decimation parameters */
    )
{
    int D;
    int wd, ht;
    struct Region region_buff;

    /* save region structure */
    copy_reg(region, &region_buff);

    D = 0;
    reg_to_wdht(region, &wd, &ht);
    while ((wd > 2) && (ht > 2)) {
	G_debug(1, "D = %d  alpha = %f; 1-alpha = %f", D, alpha[D],
		1 - alpha[D]);
	decimate(ll_pym[D], region, M, ll_pym[D + 1], alpha[D]);
	dec_reg(region);
	reg_to_wdht(region, &wd, &ht);
	D++;
    }

    /* restore region structure */
    copy_reg(&region_buff, region);
}

static void decimate(
			/* decimate statistics ll1 to form ll2 */
			LIKELIHOOD *** ll1,	/* log likelihood ll1[i][j][class], at fine resolution */
			struct Region *region1,	/* specifies image subregion */
			int M,	/* number of classes */
			LIKELIHOOD *** ll2,	/* loglikelihood ll1[i][j][class], at coarse resolution */
			double alpha	/* transition parameter */
    )
{
    struct Region *region2, reg_spc;	/* coarse resolution region */
    int wflag, hflag;		/* flags indicate odd number of pixels */
    int i, j, m;
    LIKELIHOOD *node;		/* coarse resolution point */
    LIKELIHOOD *pt1, *pt2, *pt3, *pt4;	/* fine resolution neighbors */

    region2 = &reg_spc;

    copy_reg(region1, region2);
    dec_reg(region2);

    wflag = region1->xmax & 1;
    hflag = region1->ymax & 1;

    for (i = region2->ymin; i < region2->ymax; i++)
	for (j = region2->xmin; j < region2->xmax; j++) {
	    pt1 = ll1[2 * i][2 * j];
	    pt2 = ll1[2 * i][2 * j + 1];
	    pt3 = ll1[2 * i + 1][2 * j];
	    pt4 = ll1[2 * i + 1][2 * j + 1];

	    node = ll2[i][j];
	    for (m = 0; m < M; m++)
		node[m] = 0.0;

	    up_ll(pt1, M, alpha, node);
	    up_ll(pt2, M, alpha, node);
	    up_ll(pt3, M, alpha, node);
	    up_ll(pt4, M, alpha, node);
	}

    if (wflag) {
	for (i = region2->ymin; i < region2->ymax; i++) {
	    node = ll2[i][region2->xmax - 1];
	    for (m = 0; m < M; m++)
		node[m] = 0.0;

	    pt1 = ll1[2 * i][region1->xmax - 1];
	    pt2 = ll1[2 * i + 1][region1->xmax - 1];
	    up_ll(pt1, M, alpha, node);
	    up_ll(pt2, M, alpha, node);
	}
    }

    if (hflag) {
	for (j = region2->xmin; j < region2->xmax; j++) {
	    node = ll2[region2->ymax - 1][j];
	    for (m = 0; m < M; m++)
		node[m] = 0.0;

	    pt1 = ll1[region1->ymax - 1][2 * j];
	    pt2 = ll1[region1->ymax - 1][2 * j + 1];
	    up_ll(pt1, M, alpha, node);
	    up_ll(pt2, M, alpha, node);
	}
    }

    if (hflag && wflag) {
	node = ll2[region2->ymax - 1][region2->xmax - 1];
	for (m = 0; m < M; m++)
	    node[m] = 0.0;

	pt1 = ll1[region1->ymax - 1][region1->xmax - 1];
	up_ll(pt1, M, alpha, node);
    }
}


static void up_ll(LIKELIHOOD * pt1,	/* array of log likelihood values, pt1[class] */
		  int M, double alpha, LIKELIHOOD * pt2	/* array of log likelihood values, pt2[class] */
    )
{
    static int m;
    static double sum, max, cprob[256];

    if (alpha != 1.0) {
	max = pt1[0];
	for (m = 1; m < M; m++)
	    if (pt1[m] > max)
		max = pt1[m];

	sum = 0;
	for (m = 0; m < M; m++) {
	    cprob[m] = exp(pt1[m] - max);
	    sum += cprob[m];
	}

	sum = ((1 - alpha) * sum) / M;

	for (m = 0; m < M; m++)
	    pt2[m] += log(sum + alpha * cprob[m]) + max;
    }
    else {
	for (m = 0; m < M; m++)
	    pt2[m] += pt1[m];
    }
}

char ***get_pyramid(int w0, int h0, size_t size)
{
    char ***pym;
    int D, wn, hn;

    D = levels(w0, h0);
    pym = (char ***)G_malloc((D + 1) * sizeof(char *));

    D = 0;
    wn = w0;
    hn = h0;
    pym[D] = (char **)get_img(wn, hn, size);
    while ((wn > 2) && (hn > 2)) {
	D++;
	wn = wn / 2;
	hn = hn / 2;
	pym[D] = (char **)get_img(wn, hn, size);
    }

    return (pym);
}


void free_pyramid(char *pym, int wd, int ht)
{
    unsigned char ***pt;
    int i, D;

    pt = (unsigned char ***)pym;
    D = levels(wd, ht);
    for (i = 0; i <= D; i++)
	free_img(pt[i]);
    G_free(pym);
}

char ****get_cubic_pyramid(int w0, int h0, int M, size_t size)
{
    char ****pym;
    int D, wn, hn;

    D = levels(w0, h0);
    pym = (char ****)G_malloc((D + 1) * sizeof(char *));

    D = 0;
    wn = w0;
    hn = h0;
    pym[D] = (char ***)multialloc(size, 3, hn, wn, M);
    while ((wn > 2) && (hn > 2)) {
	D++;
	wn = wn / 2;
	hn = hn / 2;
	pym[D] = (char ***)multialloc(size, 3, hn, wn, M);
    }

    return (pym);
}


void free_cubic_pyramid(char *pym, int wd, int ht, int M)
{
    int i, D;
    char **pt;

    pt = (char **)pym;
    D = levels(wd, ht);
    for (i = 0; i <= D; i++)
	multifree(pt[i], M);
    G_free(pym);
}

int levels(int wd, int ht)
{
    int D;

    D = 0;
    while ((wd > 2) && (ht > 2)) {
	D++;
	wd = wd / 2;
	ht = ht / 2;
    }
    return (D);
}
