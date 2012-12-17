#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "bouman.h"
#include "region.h"

#define EM_PRECISION 1e-4
#define ML_PRECISION 1e-6

static void seq_MAP_routine(unsigned char ***, struct Region *,
			    LIKELIHOOD ****, int, double *, float **);
static double alpha_dec_max(double ***);
static void print_N(double ***);
static void print_alpha(double *);
static void interp(unsigned char **, struct Region *, unsigned char **,
		   LIKELIHOOD ***, int, double *, int, double ***, int, float **);
void MLE(unsigned char **, LIKELIHOOD ***, struct Region *, int, float **);
static int up_char(int, int, struct Region *, unsigned char **,
		   unsigned char **);


void seq_MAP(unsigned char ***sf_pym,	/* pyramid of segmentations */
	     struct Region *region,	/* specifies image subregion */
	     LIKELIHOOD **** ll_pym,	/* pyramid of class statistics */
	     int M,		        /* number of classes */
	     double *alpha_dec,	        /* decimation parameters returned by seq_MAP */
	     float **goodness          /* goodness of fit */
    )
{
    int repeat;

    /* Repeat segmentation to get values for alpha_dec */
    for (repeat = 0; repeat < 2; repeat++) {
	/* Construct image log likelihood pyramid */
	make_pyramid(ll_pym, region, M, alpha_dec);
	G_debug(1, "Pyramid constructed");

	/* Perform sequential MAP segmentation using EM algorithm */
	seq_MAP_routine(sf_pym, region, ll_pym, M, alpha_dec, goodness);
    }
}

static void seq_MAP_routine(unsigned char ***sf_pym,	/* pyramid of segmentations */
			    struct Region *region,	/* specifies image subregion */
			    LIKELIHOOD **** ll_pym,	/* pyramid of class statistics */
			    int M,	                /* number of classes */
			    double *alpha_dec,	        /* decimation parameters returned by seq_MAP */
			    float **goodness            /* goodness of fit */
    )
{
    int j, k;			/* loop index */
    int wd, ht;			/* width and height at each resolution */
    int *period;		/* sampling period at each resolution */
    int D;			/* number of resolutions -1 */
    double ***N;		/* transition probability statistics; N[2][3][2] */
    double alpha[3];		/* transition probability parameters */
    double tmp[3];		/* temporary transition probability parameters */
    double diff1;		/* change in parameter estimates */
    double diff2;		/* change in log likelihood */
    struct Region *regionary;	/* array of region stuctures */

    /* determine number of resolutions */
    D = levels_reg(region);

    /* allocate memory */
    if ((N = (double ***)multialloc(sizeof(double), 3, 2, 3, 2)) == NULL)
	G_fatal_error(_("Unable to allocate memory"));

    regionary = (struct Region *)G_malloc((D + 1) * sizeof(struct Region));
    period = (int *)G_malloc(D * sizeof(int));

    /* Compute the image region at each resolution.       */
    k = 0;
    copy_reg(region, &(regionary[k]));
    reg_to_wdht(&(regionary[k]), &wd, &ht);
    while ((wd > 2) && (ht > 2)) {
	copy_reg(&(regionary[k]), &(regionary[k + 1]));
	dec_reg(&(regionary[k + 1]));
	reg_to_wdht(&(regionary[k + 1]), &wd, &ht);
	k++;
    }

    /* Compute sampling period for EM algorithm at each resolution. */
    for (k = 0; k < D; k++) {
	period[k] = (int)pow(2.0, (D - k - 2) / 2.0);
	if (period[k] < 1)
	    period[k] = 1;
    }

    /* Compute Maximum Likelihood estimate at coarsest resolution */
    MLE(sf_pym[D], ll_pym[D], &(regionary[D]), M, NULL);

    /* Initialize the transition parameters */
    alpha[0] = 0.5 * (3.0 / 7.0);
    alpha[1] = 0.5 * (2.0 / 7.0);
    alpha[2] = 0.0;

    /* Interpolate the classification at each resolution */
    for (D--; D >= 0; D--) {
	G_debug(1, "Resolution = %d; period = %d", D, period[D]);

	for (j = 0; j < 3; j++)
	    alpha[j] *= (1 - EM_PRECISION * 10);
	print_alpha(alpha);
	/* Apply EM algorithm to estimate alpha. Continue for *
	 * fixed number of iterations or until convergence.   */
	do {
	    interp(sf_pym[D], &(regionary[D]), sf_pym[D + 1], ll_pym[D], M,
		   alpha, period[D], N, 1, NULL);
	    print_N(N);
	    G_debug(4, "log likelihood = %f", log_like(N, alpha, M));
	    for (j = 0; j < 3; j++)
		tmp[j] = alpha[j];

	    alpha_max(N, alpha, M, ML_PRECISION);
	    print_alpha(alpha);
	    G_debug(4, "log likelihood = %f", log_like(N, alpha, M));

	    for (diff1 = j = 0; j < 3; j++)
		diff1 += fabs(tmp[j] - alpha[j]);
	    diff2 = log_like(N, alpha, M) - log_like(N, tmp, M);
	} while ((diff1 > EM_PRECISION) && (diff2 > 0));
	/* get goodness of fit for D == 0 */
	if (D == 0)
	    interp(sf_pym[D], &(regionary[D]), sf_pym[D + 1], ll_pym[D], M, alpha,
		   1, N, 0, goodness);
	else
	    interp(sf_pym[D], &(regionary[D]), sf_pym[D + 1], ll_pym[D], M, alpha,
		   1, N, 0, NULL);
	alpha_dec[D] = alpha_dec_max(N);

	print_N(N);
	alpha_max(N, alpha, M, ML_PRECISION);
	print_alpha(alpha);
    }

    /* free up N */
    G_free((char *)regionary);
    G_free((char *)period);
    multifree((char *)N, 3);
}

static double alpha_dec_max(double ***N)
{
    int i, j, k;
    double N_marg[2];		/* Marginal transition rate */
    double N_sum;		/* total number of transitions counted */

    for (k = 0; k < 2; k++) {
	N_marg[k] = 0;
	for (i = 0; i < 3; i++)
	    for (j = 0; j < 2; j++) {
		N_marg[k] += N[k][i][j];
	    }
    }

    N_sum = N_marg[0] + N_marg[1];

    if (N_sum == 0)
	return (0.0);

    return (N_marg[1] / N_sum);
}

static void print_N(
		       /* prints out class transition statistics */
		       double ***N)
{
    int n0, n1, n2;

    G_debug(2, "Class transition statistics");
    for (n0 = 0; n0 < 2; n0++) {
	for (n1 = 0; n1 < 3; n1++) {
	    for (n2 = 0; n2 < 2; n2++)
		G_debug(3, "   %f", N[n0][n1][n2]);
	}
    }
}


static void print_alpha(
			   /* prints out transition parameters. */
			   double *alpha)
{
    G_debug(2, "Transition probabilities: %f %f %f; %f",
	    alpha[0], alpha[1], alpha[2],
	    1.0 - alpha[0] - 2 * alpha[1] - alpha[2]);
}


static void interp(
		      /* Estimates finer resolution segmentation from coarser resolution
		         segmentation and texture statistics. */
		      unsigned char **sf1,	/* finer resolution segmentation */
		      struct Region *region,	/* image region */
		      unsigned char **sf2,	/* coarser resolution segmentation */
		      LIKELIHOOD *** ll,	/* Log likelihood ll[i][j][class] */
		      int M,	/* number of classes */
		      double *alpha,	/* transition probability parameters; alpha[3] */
		      int period,	/* sampling period of interpolation */
		      double ***N,	/* transition probability statistics; N[2][3][2] */
		      int statflag,	/* compute transition statistics if == 1 */
		      float **goodness  /* cost of best class */
    )
{
    int i, j;			/* pixel index */
    int m;			/* class index */
    int bflag;			/* boundary flag */
    int nn0, nn1, nn2;		/* transition counts */
    int *n0, *n1, *n2;		/* transition counts for each possible pixel class */
    unsigned char *nbr[8];	/* pointers to neighbors at courser resolution */
    double cost, mincost;	/* cost of class selection; minimum cost */
    int best = 0;		/* class of minimum cost selection */
    double Constant, tmp;
    double *pdf;		/* propability density function of class selections */
    double Z;			/* normalizing costant for pdf */
    double alpha0, alpha1, alpha2;	/* transition probabilities */
    double log_tbl[2][3][2];	/* log of transition probability */

    /* allocate memory for pdf */
    pdf = (double *)G_malloc(M * sizeof(double));
    n0 = (int *)G_malloc(M * sizeof(int));
    n1 = (int *)G_malloc(M * sizeof(int));
    n2 = (int *)G_malloc(M * sizeof(int));

    /* set constants */
    alpha0 = alpha[0];
    alpha1 = alpha[1];
    alpha2 = alpha[2];
    Constant = (1 - alpha0 - 2 * alpha1 - alpha2) / M;
    if (Constant < 0)
	G_fatal_error(_("Invalid parameter values"));

    /* precompute logs and zero static vector */
    for (nn0 = 0; nn0 < 2; nn0++)
	for (nn1 = 0; nn1 < 3; nn1++)
	    for (nn2 = 0; nn2 < 2; nn2++) {
		tmp = (alpha0 * nn0 + alpha1 * nn1 + alpha2 * nn2) + Constant;
		if (tmp == 0)
		    log_tbl[nn0][nn1][nn2] = HUGE_VAL;
		else
		    log_tbl[nn0][nn1][nn2] = -log(tmp);
		if (statflag)
		    N[nn0][nn1][nn2] = 0;
	    }

    /* classify points and compute expectation of N */
    for (i = region->ymin; i < region->ymax; i += period)
	for (j = region->xmin; j < region->xmax; j += period) {
	    /* compute minimum cost class */
	    mincost = HUGE_VAL;
	    bflag = up_char(i, j, region, sf2, nbr);
	    for (m = 0; m < M; m++) {
		nn0 = n0[m] = (m == (*nbr[0]));
		nn1 = n1[m] = (m == (*nbr[1])) + (m == (*nbr[2]));
		nn2 = n2[m] = (m == (*nbr[3]));

		pdf[m] = cost = log_tbl[nn0][nn1][nn2] - ll[i][j][m];
		if (cost < mincost) {
		    mincost = cost;
		    best = m;
		}
	    }
	    sf1[i][j] = best;
	    /* save cost as best fit indicator */
	    if (goodness)
		goodness[i][j] = mincost;

	    /* if not on boundary, compute expectation of N */
	    if ((!bflag) && (statflag)) {
		Z = 0.0;
		for (m = 0; m < M; m++) {
		    if (pdf[m] == HUGE_VAL)
			pdf[m] = 0;
		    else
			pdf[m] = exp(mincost - pdf[m]);
		    Z += pdf[m];
		}
		for (m = 0; m < M; m++)
		    N[n0[m]][n1[m]][n2[m]] += pdf[m] / Z;
	    }
	}

    G_free((char *)pdf);
    G_free((char *)n0);
    G_free((char *)n1);
    G_free((char *)n2);
}

void MLE(			/* computes maximum likelihood classification */
	    unsigned char **sf,	/* segmentation classes */
	    LIKELIHOOD *** ll,	/* texture statistics */
	    struct Region *region,	/* image region */
	    int M,		/* number of classes */
	    float **goodness    /* goodness of fit */
    )
{
    int i, j, m, best;
    double max;

    for (i = region->ymin; i < region->ymax; i++)
	for (j = region->xmin; j < region->xmax; j++) {
	    max = ll[i][j][0];
	    best = 0;
	    for (m = 1; m < M; m++) {
		if (max < ll[i][j][m]) {
		    max = ll[i][j][m];
		    best = m;
		}
	    }
	    sf[i][j] = best;
	    if (goodness)
		goodness[i][j] = max;
	}
}


static int up_char(
		      /* Computes list of pointers to nieghbors at next coarser resolution. *
		       * Returns flag when on boundary.                                     */
		      int i, int j,	/* fine resolution pixel location */
		      struct Region *region,	/* fine resolution image region */
		      unsigned char **img,	/* course resolution image */
		      unsigned char **pt	/* list of pointers */
    )
{
    static int xmax, ymax;
    static int bflag;		/* =1 when on boundary */
    static int i2, j2;		/* base indices at coarser level */
    static int di, dj;		/* displacements at coarser level */

    /* create new xmax and ymax */
    xmax = region->xmax;
    ymax = region->ymax;

    /* check for images of odd length */
    if (xmax & 1) {
	xmax--;
	if (j == xmax)
	    j--;
    }
    if (ymax & 1) {
	ymax--;
	if (i == ymax)
	    i--;
    }

    /* compute indices */
    di = ((i & 1) << 1) - 1;
    dj = ((j & 1) << 1) - 1;
    i2 = i >> 1;
    j2 = j >> 1;

    /* enforce an absorptive boundary */
    bflag = 0;
    if ((i == region->ymin) && region->free.top) {
	di = 0;
	bflag = 1;
    }
    if ((i == ymax - 1) && region->free.bottom) {
	di = 0;
	bflag = 1;
    }
    /* mod shapiro */
    /*  if((j==region->ymax)&&region->free.left)    { dj=0; bflag=1; } */
    if ((j == region->xmin) && region->free.left) {
	dj = 0;
	bflag = 1;
    }
    /* end mod shapiro */
    if ((j == xmax - 1) && region->free.right) {
	dj = 0;
	bflag = 1;
    }

    /* compute pointers */
    pt[0] = img[i2] + j2;
    pt[1] = img[i2] + j2 + dj;
    pt[2] = img[i2 + di] + j2;
    pt[3] = img[i2 + di] + j2 + dj;

    return (bflag);
}
